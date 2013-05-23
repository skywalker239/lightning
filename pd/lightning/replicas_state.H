#pragma once

#include <pd/lightning/defs.H>

namespace pd { namespace lightning {

class replicas_state_t {
public:
    void latest_entry_updated(replica_id_t replica, term_t term, iid_t last_iid);

    iid_t get_latest_entry(replica_id_t replica);

    iid_t get_latest_commitable();

    iid_t wait_latest_commitable_change(iid_t old_iid);
};

}} // namespace pd::lightning
