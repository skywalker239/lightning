// vim: set tabstop=4 expandtab:
#pragma once

#include <pd/lightning/acceptor_instance_store.H>
#include <pd/lightning/snapshot_var.H>
#include <pd/base/thr.H>
#include <vector>

namespace pd {

//! An in-memory cache of committed Paxos instances.
//  Organized as a large ring buffer which is not
//  allowed to overflow past the last snapshot revision.
class value_cache_t : public acceptor_instance_store_t {
    value_cache_t(size_t cache_size,
                  snapshot_var_t snapshot_var);

    //! Returns the instance iid if it is in the cache.
    virtual ref_t<acceptor_instance_t> lookup(instance_id_t iid);

    //! Returns false if storing iid would overflow the ring buffer
    //  past the current snapshot.
    virtual bool store(instance_id_t iid,
                       ref_t<acceptor_instance_t> instance);

    //! Does nothing.
    virtual void updated(instance_id_t iid);
private:
    value_cache_t(const value_cache_t&) = delete;
    value_cache_t& operator=(const value_cache_t&) = delete;

    const size_t cache_size_;
    std::vector<ref_t<acceptor_instance_t> > cache_;
    instance_id_t begin_;

    snapshot_var_t snapshot_;

    thr::spinlock_t lock_;
};

}  // namespace pd
