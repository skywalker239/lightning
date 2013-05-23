#include <phantom/io_raft_consensus/io_raft_multicaster.H>

#include <pd/lightning/raft_cmd.H>

#include <phantom/pd.H>
#include <phantom/io_raft_consensus/io_raft_state.H>
#include <phantom/io_raft_consensus/io_log_replica.H>

namespace phantom {

using namespace pd::lightning;

void io_raft_multicaster_t::run() {
    while(true) {
        term_t current_term = raft_state_->wait_state(io_raft_state_t::replica_state_t::LEADER);

        for(iid_t multicasted_iid = log_replica_->last_commited() + 1;; ++multicasted_iid) {
            log_replica_->wait_append(multicasted_iid, NULL);

            ref_t<pi_ext_t> entry;
            term_t prev_term;
            auto err = log_replica_->get_entry(multicasted_iid, &entry, &prev_term);
            if(err != io_follower_log_replica_t::err_t::OK) {
                break;
            }

            ref_t<pi_ext_t> append_cmd = raft::append::build(
                current_term,
                multicasted_iid - 1, // underflow ?
                prev_term,
                entry,
                log_replica_->last_commited(),
                raft_state_->replica_id()
            );

            // NOTE:
            if(!raft_state_->is_in_state(io_raft_state_t::replica_state_t::LEADER, current_term)) {
                break;
            }

            // multicast cmd
        }
    }
}

} // namespace phantom
