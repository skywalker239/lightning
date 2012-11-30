// Copyright (C) 2012, Alexander Kharitonov <alexander.kharitonov@gmail.com>
// Copyright (C) 2012, YANDEX LLC.
// This code may be distributed under the terms of the GNU GPL v3.
// See ‘http://www.gnu.org/licenses/gpl.html’.
// vim: set tabstop=4 expandtab:
#pragma once

#include <pd/lightning/acceptor_instance.H>
#include <pd/lightning/defs.H>

namespace pd {

//! An interface to a bunch of Paxos instances,
//  such as a pending pool or a value cache.
class acceptor_instance_store_t {
public:
    virtual ~acceptor_instance_store_t() {}

    //! Look up the instance iid.
    //  Returns NULL on failure.
    virtual ref_t<acceptor_instance_t> lookup(instance_id_t iid) = 0;

    //! Store instance at iid.
    //  Returns false on failure.
    virtual bool store(instance_id_t iid,
                       ref_t<acceptor_instance_t> instance) = 0;

    //! Notify the store that instance iid has been updated.
    //  Used e.g. to move committed instances from the pending pool
    //  to the value cache.
    virtual void updated(instance_id_t iid) = 0;
};

}  // namespace pd
