// Copyright (C) 2012, Korotkiy Fedor <prime@yandex-team.ru>
// Copyright (C) 2012, YANDEX LLC.
// This code may be distributed under the terms of the GNU GPL v3.
// See ‘http://www.gnu.org/licenses/gpl.html’.
// vim: set tabstop=4 expandtab:
#include <pd/base/log.H>
#include <pd/lightning/pi_ext.H>
#include <pd/lightning/pi_ring_cmd.H>
#include <pd/lightning/defs.H>

#include <phantom/pd.H>
#include <phantom/io_stream/proto.H>
#include <phantom/io_phase1_batch_executor/io_phase1_batch_executor.H>
#include <phantom/io_transport_config/io_transport_config.H>

#pragma GCC visibility push(default)

namespace phantom {

/**
 * Receives pibf blobs from network and forwards them to
 * phase 1 batch executor, phase 1 executor or phase 2 executor
 * depending on type of blob.
 */
class ring_handler_proto_t : public io_stream::proto_t {
public:
    struct config_t : public io_t::config_t {
        config::objptr_t<io_phase1_batch_executor_t> phase1_batch_executor;
//        config::objptr_t<io_phase1_executor_t> phase1_executor;
//        config::objptr_t<io_phase2_executor_t> phase2_executor;

        void check(const in_t::ptr_t& p) const;
    };

    ring_handler_proto_t(const string_t& name,
                         const config_t& config);

    virtual bool request_proc(
        in_t::ptr_t& ptr,
        out_t& /* out */,
        const netaddr_t& /* local_addr */,
        const netaddr_t& /* remote_addr */);

    virtual void stat(out_t& out, bool clear);
private:
    io_phase1_batch_executor_t& phase1_batch_executor_;
//    io_phase1_executor& phase1_executor_;
//    io_phase2_executor& phase2_executor_;

    bool parse_cmd_type(
        const ref_t<pi_ext_t>& ring_cmd,
        ring_cmd_type_t* type);
};

ring_handler_proto_t::ring_handler_proto_t(const string_t&,
                                           const config_t& config)
    : phase1_batch_executor_(*config.phase1_batch_executor) {}


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

    if(!is_ring_cmd_valid(ring_cmd)) {
        return false;
    }

    switch(ring_cmd_type(ring_cmd)) {
      case PHASE1_BATCH:
//        phase1_batch_executor_.handle_cmd(ring_cmd);
      default:
        log_error("unknown ring cmd type");
    }

    return true;
}

#pragma GCC visibility pop

} // namespace phantom
