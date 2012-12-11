// Copyright (C) 2012, Korotkiy Fedor <prime@yandex-team.ru>
// Copyright (C) 2012, YANDEX LLC.
// This code may be distributed under the terms of the GNU GPL v3.
// See ‘http://www.gnu.org/licenses/gpl.html’.
// vim: set tabstop=4 expandtab:
#include <pd/base/log.H>
#include <pd/lightning/pi_ext.H>
#include <pd/lightning/pi_struct.H>
#include <pd/lightning/defs.H>

#include <phantom/pd.H>
#include <phantom/io_stream/proto.H>
#include <phantom/io_transport_config/io_transport_config.H>

#pragma GCC visibility push(default)

namespace phantom {

/**
 * Receives pibf blobs from network and forwards them to
 * batch phase 1 executor, phase 1 executor or phase 2 executor
 * depending on type of blob.
 */
class ring_handler_proto_t : public io_stream::proto_t {
public:
    struct config_t : public io_t::config_t {
//        config::objptr_t<io_batch_phase1_executor_t> batch_phase1_executor;
//        config::objptr_t<io_phase1_executor_t> phase1_executor;
//        config::objptr_t<io_phase2_executor_t> phase2_executor;

        void check(const in_t::ptr_t& p) const;
    };

    virtual bool request_proc(
        in_t::ptr_t& ptr,
        out_t& /* out */,
        const netaddr_t& /* local_addr */,
        const netaddr_t& /* remote_addr */);

    virtual void stat(out_t& out, bool clear);
private:
//    io_batch_phase1_executor& batch_phase1_executor_;
//    io_phase1_executor& phase1_executor_;
//    io_phase2_executor& phase2_executor_;

    bool parse_cmd_type(
        const ref_t<pi_ext_t>& ring_cmd,
        ring_cmd_type_t* type);
};

bool ring_handler_proto_t::request_proc(in_t::ptr_t& in_ptr,
                                        out_t&,
                                        const netaddr_t&,
                                        const netaddr_t&) {
    ref_t<pi_ext_t> ring_cmd;

    try {
        ring_cmd->parse(in_ptr, &pi_t::parse_app);
    } catch(exception_t& ex) {
        ex.log();
        return false;
    }

    ring_cmd_type_t cmd_type;
    if(!parse_cmd_type(ring_cmd, &cmd_type)) {
        return false;
    }

    switch(cmd_type) {
      case PHASE1_BATCH:
        
      default:
        log_error("unknown ring cmd type");
    }

    return true;
}

bool ring_handler_proto_t::parse_cmd_type(
        const ref_t<pi_ext_t>& ring_cmd,
        ring_cmd_type_t* cmd_type) {
    if(ring_cmd->root().value.type() != pi_t::_map) {
        log_error("ring cmd is not map");
        return false;
    }

    const pi_t::map_t& ring_cmd_map = ring_cmd->root().value.__map();

    if(!get_enum_field(ring_cmd_map, RING_CMD_TYPE_FIELD, cmd_type)) {
        log_error("cmd_type field is required");
        return false;
    }

    return true;
}

#pragma GCC visibility pop

} // namespace phantom
