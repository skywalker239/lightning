#include <phantom/io_raft_consensus/io_raft_leader.H>

#include <pd/lightning/raft_cmd.H>

#include <phantom/io_raft_consensus/io_raft_state.H>
#include <phantom/io_raft_consensus/io_replica_set.H>

using namespace pd::lightning;

namespace phantom {

void io_raft_leader_t::run() {
    while(true) {
        term_t current_term = raft_state_->wait_state(io_raft_state_t::replica_state_t::CANDIDATE);

        ref_t<election_t> election(new election_t(raft_state_->get_cluster_conf()));

        if(run_election(election, current_term)) {
            raft_state_->won_election(current_term);

            iid_t safe_commit_iid = INVALID_IID;
            while(raft_state_->is_in_state(io_raft_state_t::replica_state_t::LEADER, current_term)) {
                safe_commit_iid = replica_set_->wait_safe_commit_iid(current_term, safe_commit_iid);

                // cluster_conf in io_replica_set_t is updated during
                // this call. Required for cluster configuration
                // change to be atomic.
                raft_state_->commit(current_term, safe_commit_iid);
            }
        }
    }
}

void io_raft_leader_t::handle_client_rpc(ref_t<pi_ext_t> cmd) {
    raft_state_->leader_append(cmd);
}

} // namespace phantom
