// Copyright (C) 2012, Alexander Kharitonov <alexander.kharitonov@gmail.com>
// Copyright (C) 2012, YANDEX LLC.
// This code may be distributed under the terms of the GNU GPL v3.
// See ‘http://www.gnu.org/licenses/gpl.html’.
// vim: set tabstop=4 expandtab:
#pragma once

#include <pd/lightning/acceptor_instance_store.H>
#include <pd/base/thr.H>
#include <vector>

namespace pd {

//! Manages pending Paxos instances.
//
//  The pending instances are arranged in a small fixed-size ring buffer.
//  Instances are only evicted from the ring buffer if they are committed.
//  Committed instances are transferred to another acceptor_instance_store_t
//  (a value cache on an acceptor or a value heap on a learner)
class pending_pool_t : public acceptor_instance_store_t {
public:
    using acceptor_instance_store_t::err_t;

    pending_pool_t(size_t pool_size,
                   acceptor_instance_store_t& committed_store);

    //! If instance iid is not in buffer and it is possible to insert it
    //  without overwriting an uncommitted instance, lookup will insert a
    //  new instance.
    //  If it is not possible to insert a new instance, returns NULL.
    virtual ref_t<acceptor_instance_t> lookup(instance_id_t iid, err_t* err);

    //! Always returns false (all manipulations must be done with
    //  mutating the reference that lookup() returns).
    virtual bool store(instance_id_t iid,
                       ref_t<acceptor_instance_t> instance);

    //! If instance id iid is not in buffer, does nothing.
    //  Otherwise if iid is committed, flushes it to
    //  committed_store.
    virtual void updated(instance_id_t iid);
private:
    pending_pool_t(const pending_pool_t&) = delete;
    pending_pool_t& operator=(const pending_pool_t&) = delete;

    //! Tries to accommodate iid by expanding the ring buffer
    //  without erasing uncommitted instances.
    //  Returns true on success, false on failure.
    bool try_expand(instance_id_t iid);

    //! Called when lookup succeeds.
    //  If the ring buffer entry corresponding to iid is
    //  uninitialized or contains an older instance,
    //  properly initializes it before returning the instance.
    ref_t<acceptor_instance_t> init_and_fetch_instance(instance_id_t iid);

    const size_t pool_size_;
    //! The pool contains the range [begin_, begin_ + pool_size_)
    //  (perhaps with holes)
    std::vector<ref_t<acceptor_instance_t> > pool_;
    instance_id_t begin_;

    acceptor_instance_store_t& committed_store_;

    thr::spinlock_t lock_;
};

}  // namespace pd
