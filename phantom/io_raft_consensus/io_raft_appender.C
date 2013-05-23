#include <phantom/io_raft_consensus/io_raft_appender.H>

#include <pd/lightning/raft_cmd.H>

#include <phantom/io_raft_consensus/io_raft_state.H>

using namespace pd::lightning;

namespace phantom {

ref_t<pi_ext_t> io_raft_appender_t::handle_rpc(ref_t<pi_ext_t> cmd) {
    append_result_t result = append(cmd);

    return raft::append::build_response(
        raft_state_->current_term(),
        result.success,
        result.latest_iid,
        result.latest_term
    );
}

void io_raft_appender_t::handle_multicast(ref_t<pi_ext_t> cmd) {
    append(cmd);
}

io_raft_appender_t::append_result_t io_raft_appender_t::append(ref_t<pi_ext_t> cmd) {
    append_result_t result;

    term_t cmd_term = raft::term(cmd);

    term_t prev_term = raft::append::prev_term(cmd);
    iid_t prev_iid = raft::append::prev_iid(cmd);

    for(size_t entry_num = 0; entry_num < raft::append::n_entries(cmd); ++entry_num) {
        ref_t<pi_ext_t> entry = raft::append::get_entry(cmd, entry_num);

        if(!raft_state_->append(cmd_term, prev_iid, prev_term, entry)) {
            break;
        }

        prev_term = cmd_term;
        ++prev_iid;
    }

    result.success = raft_state_->append(cmd_term, prev_iid, prev_term, ref_t<pi_ext_t>(NULL));

    if(result.success) {
        raft_state_->commit(cmd_term, raft::append::commit_iid(cmd));
    }

    return result;
}

} // namespace phantom
