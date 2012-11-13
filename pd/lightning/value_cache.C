// vim: set tabstop=4 expandtab:
#include <pd/lightning/value_cache.H>
#include <pd/base/assert.H>

namespace pd {

value_cache_t::value_cache_t(size_t cache_size,
                             snapshot_var_t snapshot_var)
    : cache_size_(cache_size),
      cache_(cache_size_),
      begin_(0),
      snapshot_(snapshot_var)
{}

ref_t<acceptor_instance_t> value_cache_t::lookup(instance_id_t iid) {
    thr::spinlock_guard_t guard(lock_);

    if(iid < begin_) {
        return NULL;
    }

    const size_t array_index = iid % cache_size_;
    ref_t<acceptor_instance_t>& instance = cache_[array_index];
    if(!instance || instance->instance_id() != iid) {
        return NULL;
    }
    return instance;
}

bool value_cache_t::store(instance_id_t iid,
                          ref_t<acceptor_instance_t> instance)
{
    assert(iid = instance->instance_id());
    assert(instance->committed());
    snapshot_.update();

    const instance_id_t snapshot_version = snapshot_.snapshot_version();
    const size_t array_index = iid % cache_size_;

    thr::spinlock_guard_t guard(lock_);
    if(iid < begin_) {
        return false;
    } else if(iid >= begin_ + cache_size_) {
        instance_id_t new_begin = iid + 1 - cache_size_;
        if(new_begin > snapshot_version) {
            return false;
        }
        begin_ = new_begin;
    }

    cache_[array_index] = instance;
    return true;
}

void value_cache_t::updated(instance_id_t)
{}

}  // namespace pd
