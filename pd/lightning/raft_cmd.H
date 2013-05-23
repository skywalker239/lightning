#pragma once

#include <pd/lightning/pi_ext.H>
#include <pd/lightning/defs.H>

namespace pd { namespace lightning { namespace raft {

// WARNING enums send over network, do not change existing values
enum class type_t {
    APPEND = 0,
    VOTE = 1
};

inline term_t term(const ref_t<pi_ext_t>& cmd) {
    return cmd->pi().s_ind(0).s_int();
}

inline type_t type(const ref_t<pi_ext_t>& cmd) {
    return static_cast<type_t>(cmd->pi().s_ind(1).s_int());
}

namespace append {
    inline replica_id_t leader_id(const ref_t<pi_ext_t>& cmd) {
        return cmd->pi().s_ind(2).s_int();
    }

    inline term_t prev_iid(const ref_t<pi_ext_t>& cmd) {
        return cmd->pi().s_ind(3).s_int();
    }

    inline term_t prev_term(const ref_t<pi_ext_t>& cmd) {
        return cmd->pi().s_ind(4).s_int();
    }

    inline iid_t commit_iid(const ref_t<pi_ext_t>& cmd) {
        return cmd->pi().s_ind(5).s_int();
    }

    inline size_t n_entries(const ref_t<pi_ext_t>& cmd) {
        return cmd->pi().s_ind(6).__array()._count();
    }

    ref_t<pi_ext_t> get_entry(const ref_t<pi_ext_t>& cmd, size_t n);

    ref_t<pi_ext_t> build(term_t term,
                          iid_t prev_iid,
                          term_t prev_term,
                          ref_t<pi_ext_t> entry,
                          iid_t commit_iid,
                          replica_id_t leader_id);

    ref_t<pi_ext_t> build_response(term_t term,
                                   bool is_append_succeeded,
                                   iid_t latest_iid,
                                   term_t latest_term);
} // namespace append

namespace vote {
    inline replica_id_t candidate_id(const ref_t<pi_ext_t>& cmd) {
        return cmd->pi().s_ind(2).s_int();
    }

    inline iid_t last_iid(const ref_t<pi_ext_t>& cmd) {
        return cmd->pi().s_ind(3).s_int();
    }

    inline term_t last_term(const ref_t<pi_ext_t>& cmd) {
        return cmd->pi().s_ind(4).s_int();
    }

    ref_t<pi_ext_t> build_response(term_t term, bool is_vote_granted);
} // namespace vote

}}} // namespace pd::lightning::raft
