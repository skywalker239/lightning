#pragma once
// Copyright (C) 2012, Korotkiy Fedor <prime@yandex-team.ru>
// Copyright (C) 2012, YANDEX LLC.
// This code may be distributed under the terms of the GNU GPL v3.
// See ‘http://www.gnu.org/licenses/gpl.html’.
// vim: set tabstop=4 expandtab:
#include <atomic>

#include <pd/base/thr.H>
#include <pd/base/config.H>
#include <pd/lightning/defs.H>

#include <phantom/pd.H>
#include <phantom/io_stream/proto.H>
#include <phantom/io_phase1_batch_executor/io_phase1_batch_executor.H>

#include <phantom/ring_handler/ring_handler.H>

#pragma GCC visibility push(default)

namespace phantom {

/**
 * Receives pibf commands, checks pibfs structure.
 *
 * If everything is ok, forwards commands to phase 1 batch executor,
 * phase 1 executor or phase 2 executor depending on type of command.
 *
 * Connection is closed if pibf blob is corrupted.
 */
class ring_handler_proto_t : public io_stream::proto_t {
public:
    typedef ring_handler_proto::ring_handler_t ring_handler_t;

    struct config_t {
        config_binding_type_ref(ring_handler_t);

        config::objptr_t<ring_handler_t> phase1_batch_handler;
        config::objptr_t<ring_handler_t> phase1_handler;
        config::objptr_t<ring_handler_t> phase2_handler;

        void check(const in_t::ptr_t& p) const;
    };

    ring_handler_proto_t(const string_t& /* name */,
                         const config_t& config);

    virtual bool request_proc(
        in_t::ptr_t& ptr,
        out_t& /* out */,
        const netaddr_t& /* local_addr */,
        const netaddr_t& /* remote_addr */);

    virtual void stat(out_t& out, bool clear);
private:
    ring_handler_t* phase1_batch_handler_;
    ring_handler_t* phase1_handler_;
    ring_handler_t* phase2_handler_;
};

#pragma GCC visibility pop

} // namespace phantom
