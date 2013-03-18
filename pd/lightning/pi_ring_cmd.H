#pragma once
// Copyright (C) 2012, Korotkiy Fedor <prime@yandex-team.ru>
// Copyright (C) 2012, YANDEX LLC.
// This code may be distributed under the terms of the GNU GPL v3.
// See ‘http://www.gnu.org/licenses/gpl.html’.
// vim: set tabstop=4 expandtab:
#include <vector>
#include <utility>

#include <pd/lightning/pi_ext.H>
#include <pd/lightning/defs.H>
#include <pd/lightning/value.H>

namespace pd {

namespace cmd {

/**
 * pi_ring_cmd.H defines functions and datatypes for pibf's creation,
 * validation and accessing.
 *
 * NOTE: pibf fields *must* be accessed with functions defined in this
 * header(ring_cmd_type(), request_id() etc...).
 *
 * pibf's have following structure:
 *
 *   ring_cmd ::= [cmd_type [request_id ring_id dst_host_id] body]
 *
 * where body depends on cmd_type.
 *
 * BATCH:
 *   body ::= [start_iid end_iid ballot_id failed_instances_array]
 *   failed_instances_array ::= [failed_instance...]
 *   failed_instance ::= [iid highest_promise instance_status]
 *
 * PROMISE:
 *   body ::= [iid instance_status fail?]
 *   fail ::= highest_promise highest_proposed last_proposal?
 *
 * VOTE:
 *   body ::= [iid ballot_id value_id]
 */

namespace ring {
    // NOTE: enum send over network, do not change existing values
    enum class type_t {
        BATCH = 1,
        PROMISE = 2,
        VOTE = 3
    };

    struct header_t {
        request_id_t request_id;
        ring_id_t ring_id;
        host_id_t dst_host_id;
    };

    //! Check pibf blob structure.
    bool is_valid(const ref_t<pi_ext_t>& ring_cmd);

    inline type_t type(const ref_t<pi_ext_t>& cmd) {
        return static_cast<type_t>(cmd->pi().s_ind(0).s_int());
    }

    inline request_id_t request_id(const ref_t<pi_ext_t>& cmd) {
        return cmd->pi().s_ind(1).s_ind(0).s_int();

    }

    inline ring_id_t ring_id(const ref_t<pi_ext_t>& cmd) {
        return cmd->pi().s_ind(1).s_ind(1).s_int();
    }

    inline host_id_t dst_host_id(const ref_t<pi_ext_t>& cmd) {
        return cmd->pi().s_ind(1).s_ind(2).s_int();
    }
} // namespace ring

namespace batch {
    struct fail_t {
        instance_id_t iid;
        ballot_id_t highest_promised;
    };

    struct body_t {
        instance_id_t start_iid;
        instance_id_t end_iid;
        ballot_id_t ballot_id;
        const std::vector<fail_t>& fails;
    };

    ref_t<pi_ext_t> build(const ring::header_t& header,
                          const body_t& body);

    std::vector<fail_t> fails_pi_to_vector(const pi_t::array_t& fails);
    std::vector<fail_t> merge_fails(const std::vector<fail_t>& local,
                                    const std::vector<fail_t>& received);

    inline instance_id_t start_iid(const ref_t<pi_ext_t>& ring_cmd) {
        return ring_cmd->pi().s_ind(2).s_ind(0).s_int();
    }

    inline instance_id_t end_iid(const ref_t<pi_ext_t>& ring_cmd) {
        return ring_cmd->pi().s_ind(2).s_ind(1).s_int();
    }

    inline ballot_id_t ballot_id(const ref_t<pi_ext_t>& ring_cmd) {
        return ring_cmd->pi().s_ind(2).s_ind(2).s_int();
    }

    inline const pi_t::array_t& fails(const ref_t<pi_ext_t>& ring_cmd) {
        return ring_cmd->pi().s_ind(2).s_ind(3).__array();
    }

    inline instance_id_t fail_iid(const pi_t& fail) {
        return fail.s_ind(0).s_int();
    }

    inline ballot_id_t fail_highest_promise(const pi_t& fail) {
        return fail.s_ind(1).s_int();
    }

    bool is_body_valid(const ref_t<pi_ext_t>& ring_cmd);
} // namespace batch

namespace promise {
    // NOTE: enum send over network, do not change existing values
    enum class status_t {
        SUCCESS = 1,
        // either 1) instance is open, but ballot_id is too low
        //        2) value != NULL
        //        3) iid is to high
        FAILED = 2,
    };

    struct fail_t {
        ballot_id_t highest_promised;
        ballot_id_t highest_proposed;
        value_t* last_proposal;
    };

    struct body_t {
        instance_id_t iid;
        ballot_id_t ballot_id;
        status_t status;
        fail_t* fail;
    };

    ref_t<pi_ext_t> build(const ring::header_t& header,
                          const body_t& body);

    inline instance_id_t iid(const ref_t<pi_ext_t>& cmd) {
        return cmd->pi().s_ind(2).s_ind(0).s_int();
    }

    inline ballot_id_t ballot_id(const ref_t<pi_ext_t>& cmd) {
        return cmd->pi().s_ind(2).s_ind(1).s_int();
    }

    inline status_t status(const ref_t<pi_ext_t>& cmd) {
        return static_cast<status_t>(cmd->pi().s_ind(2).s_ind(2).s_int());
    }

    ballot_id_t highest_promised(const ref_t<pi_ext_t>& cmd);
    ballot_id_t highest_proposed(const ref_t<pi_ext_t>& cmd);
    value_t& last_proposal(const ref_t<pi_ext_t>& cmd);
} // namespace promise

namespace vote {
    struct body_t {
        instance_id_t iid;
        ballot_id_t ballot_id;
        value_id_t value_id;
    };

    ref_t<pi_ext_t> build(const ring::header_t& header,
                          const body_t& body);

    inline instance_id_t iid(const ref_t<pi_ext_t>& cmd) {
        return cmd->pi().s_ind(2).s_ind(0).s_int();
    }

    inline ballot_id_t ballot_id(const ref_t<pi_ext_t>& cmd) {
        return cmd->pi().s_ind(2).s_ind(1).s_int();
    }

    inline value_id_t value_id(const ref_t<pi_ext_t>& cmd) {
        return cmd->pi().s_ind(2).s_ind(2).s_int();
    }

    bool is_body_valid(const ref_t<pi_ext_t>& cmd);
}

} // namespace cmd

} // namespace pd
