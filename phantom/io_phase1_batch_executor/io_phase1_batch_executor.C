// Copyright (C) 2012, Alexey Pervushin <billyevans@yandex-team.ru>
// Copyright (C) 2012, YANDEX LLC.
// This code may be distributed under the terms of the GNU GPL v3.
// See ‘http://www.gnu.org/licenses/gpl.html’.
// vim: set tabstop=4 expandtab:
#include <phantom/io_phase1_batch_executor/io_phase1_batch_executor.H>
#include <pd/lightning/pi_struct.H>

namespace phantom {

void io_phase1_batch_executor::init() {

}

void io_phase1_batch_executor::run() {

}

void io_phase1_batch_executor::fini() {

}

bool io_phase1_batch_executor::handle_cmd(
        const ref_t<pi_ext_t>& ring_cmd_raw) {
    batch_ring_cmd_t ring_cmd;

    if(!parse_cmd(ring_cmd_raw, &ring_cmd)) {
        return false;
    }

    return true;
}

bool io_phase1_batch_executor::parse_cmd(
        const ref_t<pi_ext_t>& ring_cmd_raw,
        batch_ring_cmd_t* ring_cmd) {
    ring_cmd->ring_cmd_raw = ring_cmd_raw;

    if(ring_cmd_raw->root().value.type() != pi_t::_map) {
        log_error("ring cmd is not map");
        return false;
    }

    const pi_t::map_t& ring_cmd_map = ring_cmd_raw->root().value.__map();

    if(!get_int_field(ring_cmd_map,
                      RING_ID_FIELD,
                      &(ring_cmd->ring_id)) ||

       !get_int_field(ring_cmd_map,
                      RING_SENDER_HOST_ID_FIELD,
                      &(ring_cmd->ring_id)) ||

       !get_int_field(ring_cmd_map,
                      START_INSTANCE_ID_FIELD,
                      &(ring_cmd->start_instance_id)) ||

       !get_int_field(ring_cmd_map,
                      END_INSTANCE_ID_FIELD,
                      &(ring_cmd->end_instance_id)) ||

       !get_int_field(ring_cmd_map,
                      REQUEST_ID_FIELD,
                      &(ring_cmd->request_id)) ||

       !get_int_field(ring_cmd_map,
                      BALLOT_ID_FIELD,
                      &(ring_cmd->ballot_id))) {
        log_error("one of int fields is missing");
        return false;
    }

    pi_t const* reserved_instances = ring_cmd_map.lookup(
        pi_t(pi_t::_enum, RESERVED_INSTANCES_FIELD));

    if(!reserved_instances ||
       reserved_instances->type() != pi_t::_array) {
        log_error("reserved instances array is missing");
        return false;
    }

    ring_cmd->reserved_instances = &reserved_instances->__array();

    return true;
}

} // namespace phantom
