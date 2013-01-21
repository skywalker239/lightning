// Copyright (C) 2012, Korotkiy Fedor <prime@yandex-team.ru>
// Copyright (C) 2012, YANDEX LLC.
// This code may be distributed under the terms of the GNU GPL v3.
// See ‘http://www.gnu.org/licenses/gpl.html’.
// vim: set tabstop=4 expandtab:
#include <phantom/ring_handler/ring_handler_proto.H>

#include <pd/base/log.H>
#include <pd/lightning/pi_ext.H>
#include <pd/lightning/pi_ring_cmd.H>

#include <phantom/module.H>
#include <phantom/ring_handler/ring_handler.H>

namespace phantom {

MODULE(ring_handler);

void ring_handler_proto_t::config_t::check(const in_t::ptr_t& p) const {
    if(this_host_id == kInvalidHostId) {
        config::error(p, "ring_handler_proto_t.this_host_id must be set");
    }

    if(!phase1_batch_handler) {
        config::error(p, "ring_handler_proto_t.phase1_batch_handler required");
    }

    if(!phase1_handler) {
        config::error(p, "ring_handler_proto_t.phase1_handler required");
    }

    if(!phase2_handler) {
        config::error(p, "ring_handler_proto_t.phase2_handler required");
    }
}

void ring_handler_proto_t::stat(out_t& /*out*/, bool /*clear*/) {
//    TODO(prime@): write stat
}

ring_handler_proto_t::ring_handler_proto_t(const string_t&,
                                           const config_t& config)
    : this_host_id_(config.this_host_id),
      phase1_batch_handler_(config.phase1_batch_handler),
      phase1_handler_(config.phase1_handler),
      phase2_handler_(config.phase2_handler),
      current_ring_(kInvalidRingId) {}


bool ring_handler_proto_t::request_proc(in_t::ptr_t& in_ptr,
                                        out_t&,
                                        const netaddr_t&,
                                        const netaddr_t&) {
    ref_t<pi_ext_t> ring_cmd;

    try {
        ring_cmd = pi_ext_t::parse(in_ptr, &pi_t::parse_app);
    } catch(exception_t& ex) {
        ex.log();
        return false;
    }

    if(!is_ring_cmd_valid(ring_cmd)) {
        log_error("invalid ring cmd schema");
        return false;
    }

    if(ring_id(ring_cmd) != current_ring_ ||
       dst_host_id(ring_cmd) != this_host_id_) {
        log_debug("wrong dst in ring cmd { ring_id = %d, dst_host_id = %d }",
                  ring_id(ring_cmd),
                  dst_host_id(ring_cmd));
        // When ring changes some commands may be sent to wrong host.
        // If we close connection here it may cause lot of unnecessary
        // reconnections.
        return true;
    }

    switch(ring_cmd_type(ring_cmd)) {
      case PHASE1_BATCH:
        phase1_batch_handler_->handle_cmd(ring_cmd);
        break;
      case PHASE1:
        phase1_handler_->handle_cmd(ring_cmd);
        break;
      case PHASE2:
        phase2_handler_->handle_cmd(ring_cmd);
        break;
      default:
        log_error("unknown ring cmd type");
        return false;
    }

    return true;
}

void ring_handler_proto_t::ring_changed(ring_id_t new_ring) {
    current_ring_ = new_ring;
}

namespace ring_handler_proto {
config_binding_sname(ring_handler_proto_t);
config_binding_type(ring_handler_proto_t, ring_handler_t);
config_binding_value(ring_handler_proto_t, this_host_id);
config_binding_value(ring_handler_proto_t, phase1_batch_handler);
config_binding_value(ring_handler_proto_t, phase1_handler);
config_binding_value(ring_handler_proto_t, phase2_handler);

config_binding_ctor(io_stream::proto_t, ring_handler_proto_t);
}

} // namespace phantom