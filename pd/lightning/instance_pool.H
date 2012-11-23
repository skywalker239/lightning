#pragma once

#include <vector>
#include <queue>
#include <algorithm>

#include "pd/lightning/defs.H"
#include "pd/bq/bq_cond.H"

namespace pd {

//! Maintains two pools of instance id's: one for open
//  (ready for Paxos Phase 2) and one for reserved
//  (for which full Paxos Phase 1 needs to be performed).
class instance_pool_t {
public:
    //! open_instances_limit is the maximum number of open instances
    //  that we keep without blocking adding new ones.
    instance_pool_t(size_t open_instances_limit,
                    size_t reserved_instances_limit) : 
        open_instances_limit_(open_instances_limit),
        reserved_instances_limit_(reserved_instances_limit),
        open_instances_(),
        reserved_instances_()
    {}

    //! Adds a new open instance id.
    //  Blocks if the open instance pool is full OR the
    //  reserved instance pool is full.
    void push_open_instance(instance_id_t iid);

    //! Adds a new reserved instance id.
    //  Blocks if the reserved instance pool is full.
    void push_reserved_instance(instance_id_t iid);

    //! Returns the minimal open instance id from the pool.
    //  Blocks if the open pool is empty.
    instance_id_t pop_open_instance();

    //! Returns the minimal reserved instance id from the pool.
    //  Blocks if the reserved pool is empty.
    instance_id_t pop_reserved_instance();

private:
    size_t open_instances_limit_;
    size_t reserved_instances_limit_;

    typedef std::priority_queue<instance_id_t, 
                                std::vector<instance_id_t>,
                                std::greater<size_t> > instance_heap;
    instance_heap open_instances_;
    instance_heap reserved_instances_;

    bq_cond_t open_instances_cond_;
    bq_cond_t reserved_instances_cond_;
};

}  // namespace pd
