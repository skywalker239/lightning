// vim: set tabstop=4 expandtab:
#include <pd/lightning/pending_pool.H>
#include <pd/base/exception.H>

namespace pd {

pending_pool_t::pending_pool_t(size_t pool_size,
                               acceptor_instance_store_t& committed_store)
    : pool_size_(pool_size),
      pool_(pool_size_),
      begin_(0),
      committed_store_(committed_store)
{}

ref_t<acceptor_instance_t> pending_pool_t::lookup(instance_id_t iid) {
    thr::spinlock_guard_t guard(lock_);

    if(iid < begin_) {
        return committed_store_.lookup(iid);
    } else if(iid >= begin_ + pool_size_) {
        if(!try_expand(iid)) {
            return NULL;
        }
    }
    return init_and_fetch_instance(iid);
}

bool pending_pool_t::store(instance_id_t iid,
                           ref_t<acceptor_instance_t>)
{
    throw exception_log_t(log::error, "pending_pool.store(%ld)", iid);
}

void pending_pool_t::updated(instance_id_t iid)
{
    thr::spinlock_guard_t guard(lock_);
    const size_t array_index = iid % pool_size_;
    ref_t<acceptor_instance_t> instance = pool_[array_index];
    if(!instance || instance->instance_id() != iid) {
        return;
    }

    if(instance->committed()) {
        //! Tentatively flush instance to committed_store.
        //  We do not care if it succeeds here because
        //  it will be enforced later in try_expand
        //  when this instance goes out of our scope.
        committed_store_.store(iid, instance);
    }
}

ref_t<acceptor_instance_t> pending_pool_t::init_and_fetch_instance(instance_id_t iid) {
    const size_t array_index = iid % pool_size_;
    ref_t<acceptor_instance_t>& instance = pool_[array_index];
    if(!instance || instance->instance_id() != iid) {
        instance = new acceptor_instance_t(iid);
    }
    return instance;
}

bool pending_pool_t::try_expand(instance_id_t iid) {
    instance_id_t new_begin = iid + 1 - pool_size_;
    for(instance_id_t i = begin_; i < new_begin; ++i) {
        const size_t array_index = i % pool_size_;
        ref_t<acceptor_instance_t>& instance = pool_[array_index];
        if(!instance ||
           instance->instance_id() != i ||
           !instance->committed())
        {
            return false;
        }
        //! We check the possibility of finally flushing the committed instance here.
        if(!committed_store_.store(i, instance)) {
            return false;
        }

        begin_ = i + 1;
    }
    return true;
}


}  // namespace pd
