// Copyright (C) 2012, Alexander Kharitonov <alexander.kharitonov@gmail.com>
// Copyright (C) 2012, YANDEX LLC.
// This code may be distributed under the terms of the GNU GPL v3.
// See ‘http://www.gnu.org/licenses/gpl.html’.
// vim: set tabstop=4 expandtab:
#pragma once

#include <pd/lightning/defs.H>

namespace pd {

//! Maintains two pools of instance id's: one for open
//  (ready for Paxos Phase 2) and one for reserved
//  (for which full Paxos Phase 1 needs to be performed).
class instance_pool_t {
public:
    //! open_instances_limit is the maximum number of open instances
    //  that we keep without blocking adding new ones.
    instance_pool_t(size_t open_instances_limit,
                    size_t reserved_instances_limit);

    //! Adds a new open instance id.
    //  Blocks if the open instance pool is full OR the
    //  reserved instance pool is full.
    void push_open_instance(instance_id_t iid);

    //! Adds a new reserved instance id.
    //  Blocks if the reserved instance pool is full.
    void push_reserved_instance(instance_id_t iid);

    //! Returns the minimal open instance id from the pool.
    //  Blocks if the open pool is empty.
    void pop_open_instance();

    //! Returns the minimal reserved instance id from the pool.
    //  Blocks if the reserved pool is empty.
    void pop_reserved_instance();
};

}  // namespace pd
