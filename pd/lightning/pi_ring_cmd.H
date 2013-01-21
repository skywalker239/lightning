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

// TODO(prime@): ask mamchits@ is there a better way
enum ring_cmd_type_t {
    PHASE1_BATCH = pi_t::_private + 1,
    PHASE1,
    PHASE2
};

enum failed_instance_status_t {
    LOW_BALLOT_ID = pi_t::_private + 1,
    RESERVED,
    IID_TO_LOW
};

//! Check pibf blob structure.
bool is_ring_cmd_valid(const ref_t<pi_ext_t>& ring_cmd);

struct ring_cmd_header_t {
    request_id_t request_id;
    ring_id_t ring_id;
    host_id_t dst_host_id;
};

struct failed_instance_t {
    instance_id_t iid;
    ballot_id_t highest_promise;
    failed_instance_status_t status;
};

struct ring_batch_cmd_body_t {
    instance_id_t start_instance_id;
    instance_id_t end_instance_id;
    ballot_id_t ballot_id;
    const std::vector<failed_instance_t>& failed_instances;
};

ref_t<pi_ext_t> build_ring_batch_cmd(
    const ring_cmd_header_t& header,
    const ring_batch_cmd_body_t& body);

std::vector<failed_instance_t> failed_instances_pi_to_vector(
        const pi_t::array_t& failed_instances);

std::vector<failed_instance_t> merge_failed_instances(
        const std::vector<failed_instance_t>& local,
        const std::vector<failed_instance_t>& received);

inline ring_cmd_type_t ring_cmd_type(const ref_t<pi_ext_t>& ring_cmd) {
    return ring_cmd->pi().s_ind(0).__<ring_cmd_type_t>();
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

inline instance_id_t start_instance_id(const ref_t<pi_ext_t>& ring_cmd) {
    return ring_cmd->pi().s_ind(2).s_ind(0).s_int();
}

inline instance_id_t end_instance_id(const ref_t<pi_ext_t>& ring_cmd) {
    return ring_cmd->pi().s_ind(2).s_ind(1).s_int();
}

inline ballot_id_t ballot_id(const ref_t<pi_ext_t>& ring_cmd) {
    return ring_cmd->pi().s_ind(2).s_ind(2).s_int();
}

inline const pi_t::array_t& failed_instances(const ref_t<pi_ext_t>& ring_cmd) {
    return ring_cmd->pi().s_ind(2).s_ind(3).__array();
}

inline instance_id_t failed_instance_iid(const pi_t& failed_instance) {
    return failed_instance.s_ind(0).s_int();
}

inline ballot_id_t failed_instance_highest_promise(
        const pi_t& failed_instance) {
    return failed_instance.s_ind(1).s_int();
}

inline failed_instance_status_t failed_instance_status(
        const pi_t& failed_instance) {
    return failed_instance.s_ind(2).__<failed_instance_status_t>();
}

} // namespace pd
