// Copyright (C) 2013, Korotkiy Fedor <prime@yandex-team.ru>
// Copyright (C) 2013, YANDEX LLC.
// This code may be distributed under the terms of the GNU GPL v3.
// See ‘http://www.gnu.org/licenses/gpl.html’.
// vim: set tabstop=4 expandtab:

#include <pd/base/config.H>
#include <pd/base/log.H>
#include <pd/base/netaddr.H>

#include <phantom/io.H>
#include <phantom/module.H>
#include <phantom/io_ring_sender/io_ring_sender.H>
#include <phantom/io_phase1_executor/io_phase1_executor.H>
#include <phantom/io_phase1_batch_executor/io_phase1_batch_executor.H>
#include <phantom/io_phase2_executor/io_phase2_executor.H>
#include <phantom/io_acceptor_store/io_acceptor_store.H>
#include <phantom/io_proposer_pool/io_proposer_pool.H>

namespace phantom {

MODULE(io_lightning_static_server);

class io_lightning_static_server_t : public io_t {
public:
    struct config_t : public io_t::config_t {
        void check(const in_t::ptr_t& ) const {}

        config::objptr_t<io_ring_sender_t> ring_sender;
        config::objptr_t<io_phase1_executor_t> phase1_executor;
        config::objptr_t<io_phase1_batch_executor_t> phase1_batch_executor;
        config::objptr_t<io_phase2_executor_t> phase2_executor;
        config::objptr_t<io_acceptor_store_t> acceptor_store;
        config::objptr_t<io_proposer_pool_t> proposer_pool;

        config::enum_t<bool> is_master;
        address_ipv4_t next_acceptor_host;
        uint16_t next_acceptor_port;
        host_id_t next_acceptor_host_id;

        config_t() : is_master(false) {}
    };

    io_lightning_static_server_t(const string_t& name,
                                 const config_t& config)
        : io_t(name, config),
          ring_sender_(config.ring_sender),
          phase1_executor_(config.phase1_executor),
          phase1_batch_executor_(config.phase1_batch_executor),
          phase2_executor_(config.phase2_executor),
          acceptor_store_(config.acceptor_store),
          proposer_pool_(config.proposer_pool),
          is_master_(config.is_master),
          next_acceptor_addr_(netaddr_ipv4_t(config.next_acceptor_host, config.next_acceptor_port)),
          next_acceptor_host_id_(config.next_acceptor_host_id) {}

    void init() {}

    void run() {
        ring_sender_->join_ring(next_acceptor_addr_);

        acceptor_store_->set_birth(0);

        phase1_executor_->ring_state_changed(42, next_acceptor_host_id_, is_master_);
        phase1_batch_executor_->ring_state_changed(42, next_acceptor_host_id_, is_master_);
        phase2_executor_->ring_state_changed(42, next_acceptor_host_id_, is_master_);

        if(is_master_) {
            proposer_pool_->activate();
            phase1_batch_executor_->set_start_iid(0);

            phase1_executor_->start_proposer();
            phase1_batch_executor_->start_proposer();
            phase2_executor_->start_proposer();
        }
    }

    void fini() {}

    void stat(out_t&, bool) {}

private:
    io_ring_sender_t* ring_sender_;
    io_phase1_executor_t* phase1_executor_;
    io_phase1_batch_executor_t* phase1_batch_executor_;
    io_phase2_executor_t* phase2_executor_;
    io_acceptor_store_t* acceptor_store_;
    io_proposer_pool_t* proposer_pool_;

    bool is_master_;
    netaddr_ipv4_t next_acceptor_addr_;
    host_id_t next_acceptor_host_id_;
};

namespace io_lightning_static_server {
config_binding_sname(io_lightning_static_server_t);
config_binding_parent(io_lightning_static_server_t, io_t, 1);
config_binding_ctor(io_t, io_lightning_static_server_t);

config_binding_value(io_lightning_static_server_t, ring_sender);
config_binding_value(io_lightning_static_server_t, phase1_executor);
config_binding_value(io_lightning_static_server_t, phase1_batch_executor);
config_binding_value(io_lightning_static_server_t, phase2_executor);
config_binding_value(io_lightning_static_server_t, acceptor_store);
config_binding_value(io_lightning_static_server_t, proposer_pool);

config_binding_value(io_lightning_static_server_t, is_master);
config_binding_value(io_lightning_static_server_t, next_acceptor_host);
config_binding_value(io_lightning_static_server_t, next_acceptor_port);
config_binding_value(io_lightning_static_server_t, next_acceptor_host_id);

} // namespace io_lightning_static_server

} // namespace phantom
