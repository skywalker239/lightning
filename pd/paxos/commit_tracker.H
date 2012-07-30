#pragma once

#include <pd/base/thr.H>
#include <stdint.h>
#include <set>
#include <vector>

namespace pd {

class commit_tracker_t {
public:
    commit_tracker_t();

    void reset(uint64_t start_instance_id);

    bool add_committed_instance(uint64_t instance_id,
                                std::vector<uint64_t>* instances_to_recover);

    uint64_t first_unknown_instance() const;
private:
    // The not yet committed instance ids are
    // not_committed_instance_ids_ and
    // [after_last_committed_instance_id_, +\infty).
    uint64_t after_last_committed_instance_id_;
    std::set<uint64_t> not_committed_instance_ids_;

    mutable thr::spinlock_t lock_;
};

}  // namespace pd
