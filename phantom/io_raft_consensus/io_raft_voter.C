#include <phantom/io_raft_consensus/io_raft_voter.H>

#include <pd/lightning/raft_cmd.H>

#include <phantom/io_raft_consensus/io_raft_state.H>

using namespace pd::lightning;

namespace phantom {

ref_t<pi_ext_t> io_raft_voter_t::handle_rpc(ref_t<pi_ext_t> cmd) {
    bool is_vote_granted = raft_state_->vote(raft::term(cmd),
                                             raft::vote::candidate_id(cmd),
                                             raft::vote::last_iid(cmd),
                                             raft::vote::last_term(cmd));

    return raft::vote::build_response(raft_state_->current_term(), is_vote_granted);
}

} // namespace phantom
