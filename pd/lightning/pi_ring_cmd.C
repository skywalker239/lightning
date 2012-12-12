// Copyright (C) 2012, Korotkiy Fedor <prime@yandex-team.ru>
// Copyright (C) 2012, YANDEX LLC.
// This code may be distributed under the terms of the GNU GPL v3.
// See ‘http://www.gnu.org/licenses/gpl.html’.
// vim: set tabstop=4 expandtab:
#include <pd/lightning/pi_ring_cmd.H>

namespace pd {

bool is_ring_cmd_header_valid(const ref_t<pi_ext_t>& ring_cmd) {
    const pi_t& header = ring_cmd->pi().s_ind(1);

    if (header.type() != pi_t::_array ||
        header.__array()._count() != 3) {
        return false;
    }

    return true;
}

bool is_ring_cmd_batch_valid(const ref_t<pi_ext_t>& ring_cmd) {
    const pi_t& batch_data = ring_cmd->pi().s_ind(2);

    if(batch_data.type() != pi_t::_array ||
       batch_data.__array()._count() != 4) {
        return false;
    }

    if(batch_data.s_ind(4).type() != pi_t::_array) {
        return false;
    }

    // TODO(prime@) add fields sanity check
    return true;
}

bool is_ring_cmd_valid(const ref_t<pi_ext_t>& ring_cmd) {
    if(ring_cmd->pi().type() != pi_t::_array) {
        return false;
    }

    if(!is_ring_cmd_header_valid(ring_cmd)) {
        return false;
    }

    switch(ring_cmd->pi().s_enum()) {
      case PHASE1_BATCH:
        if(is_ring_cmd_batch_valid(ring_cmd)) {
            return true;
        } else {
            return false;
        }
      case PHASE1:
        // TODO(prime@) write validation code
        return false;
      case PHASE2:
        // TODO(prime@) write validation code
        return false;
      default:
        return false;
    }
}


}
