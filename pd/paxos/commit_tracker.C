// vim: set tabstop=4 expandtab:
#include "commit_tracker.H"

namespace pd {

using std::set;
using std::vector;

commit_tracker_t::commit_tracker_t()
    : after_last_committed_instance_id_(0)
{}

void commit_tracker_t::reset(uint64_t start_instance_id) {
    thr::spinlock_guard_t guard(lock_);

    set<uint64_t> filtered_instance_ids;
    for(auto i = not_committed_instance_ids_.begin();
        i != not_committed_instance_ids_.end();
        ++i)
    {
        if(*i >= start_instance_id) {
            filtered_instance_ids.insert(*i);
        }
    }

    not_committed_instance_ids_.swap(filtered_instance_ids);

    if(after_last_committed_instance_id_ < start_instance_id) {
        after_last_committed_instance_id_ = start_instance_id;
    }
}

bool commit_tracker_t::add_committed_instance(
    uint64_t instance_id,
    vector<uint64_t>* instances_to_recover)
{
    thr::spinlock_guard_t guard(lock_);

    if(instance_id > after_last_committed_instance_id_) {
        for(uint64_t iid = after_last_committed_instance_id_;
            iid < instance_id;
            ++iid)
        {
            not_committed_instance_ids_.insert(iid);
            instances_to_recover->push_back(iid);
        }

        after_last_committed_instance_id_ = instance_id + 1;
        return true;
    } else {
        return (not_committed_instance_ids_.erase(instance_id) == 1);
    }
}

uint64_t commit_tracker_t::first_unknown_instance() const {
    thr::spinlock_guard_t guard(lock_);

    return (not_committed_instance_ids_.empty()) ?
               after_last_committed_instance_id_ :
               *not_committed_instance_ids_.begin();
}

}  // namespace pd
