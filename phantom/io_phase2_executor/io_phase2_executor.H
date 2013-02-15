#pragma once
// Copyright (C) 2012, Fedor Korotkiy <prime@yandex-team.ru>
// Copyright (C) 2012, YANDEX LLC.
// This code may be distributed under the terms of the GNU GPL v3.
// See ‘http://www.gnu.org/licenses/gpl.html’.
// vim: set tabstop=4 expandtab:
#include <pd/base/config.H>
#include <pd/base/netaddr_ipv4.H>
#include <pd/lightning/defs.H>

#include <phantom/pd.H>
#include <phantom/io_blob_sender/io_blob_sender.H>
#include <phantom/io_blob_receiver/handler.H>
#include <phantom/io_paxos_executor/io_paxos_executor.H>

#pragma GCC visibility push(default)
namespace phantom {

class io_phase2_executor_t : public io_paxos_executor_t,
                             public io_blob_receiver::handler_t {
public:
    struct config_t : public io_t::config_t {
        config::objptr_t<io_blob_sender_t> blob_sender;
    };

    io_phase2_executor_t(const string_t& name,
                         const config_t& config);

    // from io_blob_receiver::handler_t
    virtual void handle(ref_t<pi_ext_t> udp_cmd,
                        const netaddr_t& /* remote_addr */);

private:
    io_blob_sender_t* blob_sender_;
    io_guid_t* udp_guid_generator_;

    // from io_paxos_executor_t
    virtual void run_proposer();
    virtual void accept_ring_cmd(const ref_t<pi_ext_t>& ring_cmd);

    void propose(const ref_t<pi_ext_t>& udp_cmd);

    void continue_pending_vote(const ref_t<acceptor_instance_t>& instance);

    void apply_vote_and_send_to_next(const ref_t<acceptor_instance_t>& instance,
                                     acceptor_instance_t::vote_t vote);

    void commit(const ref_t<pi_ext_t>& udp_cmd);

};

} // namespace phantom
#pragma GCC visibility pop
