#pragma once
// Copyright (C) 2012, Korotkiy Fedor <prime@yandex-team.ru>
// Copyright (C) 2012, YANDEX LLC.
// This code may be distributed under the terms of the GNU GPL v3.
// See ‘http://www.gnu.org/licenses/gpl.html’.
// vim: set tabstop=4 expandtab:
#include <pd/lightning/pi_ext.H>
#include <pd/lightning/defs.H>

namespace pd {

//! Check pibf blob structure. Doesn't check field values.
bool is_ring_cmd_valid(const ref_t<pi_ext_t>& ring_cmd);

inline ring_cmd_type_t ring_cmd_type(const ref_t<pi_ext_t>& ring_cmd) {
    return static_cast<ring_cmd_type_t>(ring_cmd->pi().s_ind(0).s_enum());
}

inline request_id_t request_id(const ref_t<pi_ext_t>& ring_cmd) {
    return ring_cmd->pi().s_ind(1).s_ind(0).s_int();
}

inline ring_id_t ring_id(const ref_t<pi_ext_t>& ring_cmd) {
    return ring_cmd->pi().s_ind(1).s_ind(1).s_int();
}

inline host_id_t host_id(const ref_t<pi_ext_t>& ring_cmd) {
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

inline const pi_t::array_t& reserved_instances(const ref_t<pi_ext_t>& ring_cmd) {
    return ring_cmd->pi().s_ind(2).s_ind(3).__array();
}

} // namespace pd
