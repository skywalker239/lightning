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

namespace pd {

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
 * PHASE1_BATCH:
 *   body ::= [start_iid end_iid ballot_id failed_instances_array]
 *   failed_instances_array ::= [failed_instance...]
 *   failed_instance ::= [iid highest_promise instance_status]
 *
 * PHASE1:
 *   body ::= [iid instance_status highest_promise? value? value_id?]
 *
 * PHASE2:
 *   body ::=
 */

// NOTE: enum send over network, do not change
enum class ring_cmd_type_t {
    PHASE1_BATCH = 1,
    PHASE1 = 2,
    PHASE2 = 3
};

// NOTE: enum send over network, do not change
enum class instance_status_t {
    // instance is open, but ballot_id is to low
    LOW_BALLOT_ID = 1,
    // instance value != NULL
    RESERVED = 2,
    OPEN = 3,
    // acceptor would never participate in this instance because it is
    // too old(forgotten or dead)
    IID_TOO_LOW = 4,
    IID_TOO_HIGH = 5
};

//! Check pibf blob structure.
bool is_ring_cmd_valid(const ref_t<pi_ext_t>& ring_cmd);

struct ring_cmd_header_t {
    request_id_t request_id;
    ring_id_t ring_id;
    host_id_t dst_host_id;
};

struct batch_fail_t {
    instance_id_t iid;
    ballot_id_t highest_promised;
    instance_status_t status;
};

struct ring_batch_cmd_body_t {
    instance_id_t start_iid;
    instance_id_t end_iid;
    ballot_id_t ballot_id;
    const std::vector<batch_fail_t>& fails;
};

ref_t<pi_ext_t> build_ring_batch_cmd(
    const ring_cmd_header_t& header,
    const ring_batch_cmd_body_t& body);

std::vector<batch_fail_t> fails_pi_to_vector(
        const pi_t::array_t& fails);

std::vector<batch_fail_t> merge_fails(
        const std::vector<batch_fail_t>& local,
        const std::vector<batch_fail_t>& received);

inline ring_cmd_type_t ring_cmd_type(const ref_t<pi_ext_t>& ring_cmd) {
    return static_cast<ring_cmd_type_t>(ring_cmd->pi().s_ind(0).s_int());
}

inline request_id_t request_id(const ref_t<pi_ext_t>& ring_cmd) {
    return ring_cmd->pi().s_ind(1).s_ind(0).s_int();
}

inline ring_id_t ring_id(const ref_t<pi_ext_t>& ring_cmd) {
    return ring_cmd->pi().s_ind(1).s_ind(1).s_int();
}

inline host_id_t dst_host_id(const ref_t<pi_ext_t>& ring_cmd) {
    return ring_cmd->pi().s_ind(1).s_ind(2).s_int();
}

inline instance_id_t batch_start_iid(const ref_t<pi_ext_t>& ring_cmd) {
    return ring_cmd->pi().s_ind(2).s_ind(0).s_int();
}

inline instance_id_t batch_end_iid(const ref_t<pi_ext_t>& ring_cmd) {
    return ring_cmd->pi().s_ind(2).s_ind(1).s_int();
}

inline ballot_id_t batch_ballot_id(const ref_t<pi_ext_t>& ring_cmd) {
    return ring_cmd->pi().s_ind(2).s_ind(2).s_int();
}

inline const pi_t::array_t& batch_fails(const ref_t<pi_ext_t>& ring_cmd) {
    return ring_cmd->pi().s_ind(2).s_ind(3).__array();
}

inline instance_id_t fail_iid(const pi_t& fail) {
    return fail.s_ind(0).s_int();
}

inline ballot_id_t fail_highest_promise(const pi_t& fail) {
    return fail.s_ind(1).s_int();
}

inline instance_status_t fail_status(const pi_t& fail) {
    return static_cast<instance_status_t>(fail.s_ind(2).s_int());
}

} // namespace pd
