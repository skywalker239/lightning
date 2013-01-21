#pragma once
// Copyright (C) 2012, Fedor Korotkiy <prime@yandex-team.ru>
// Copyright (C) 2012, YANDEX LLC.
// This code may be distributed under the terms of the GNU GPL v3.
// See ‘http://www.gnu.org/licenses/gpl.html’.
// vim: set tabstop=4 expandtab:
#include <pd/lightning/defs.H>
#include <pd/lightning/pi_ext.H>
#include <pd/lightning/pi_ring_cmd.H>
#include <pd/lightning/ring_var.H>
#include <pd/lightning/wait_pool.H>
#include <pd/lightning/blocking_queue.H>
#include <pd/lightning/pending_pool.H>

#include <pd/bq/bq_cond.H>

#include <phantom/io.H>
#include <phantom/io_transport_config/io_transport_config.H>
#include <phantom/io_ring_sender/io_ring_sender.H>

#pragma GCC visibility push(default)
namespace phantom {

class io_phase1_batch_executor_t : public io_t {

public:
    struct config_t : public io_t::config_t {
        host_id_t host_id;
        config::objptr_t<io_transport_config_t> transport_config;

        uint32_t wait_pool_size;
        uint32_t cmd_queue_size;

        uint32_t num_proposer_threads;
        uint32_t num_acceptor_threads;

        config_t() throw()
            : host_id(kInvalidHostId),
              wait_pool_size(64),
              cmd_queue_size(512),
              num_proposer_threads(1),
              num_acceptor_threads(1) {}
        ~config_t() throw() {}
        void check(const in_t::ptr_t& p) const;
    };

    io_phase1_batch_executor_t(const string_t& name,
                             const config_t& config);

    void handle_cmd(const ref_t<pi_ext_t>& ring_cmd);

    virtual void init();
    virtual void run();
    virtual void fini();
private:
    struct ring_state_t {
        ring_state_t()
            : is_master(false),
              prev_host_id(kInvalidHostId),
              ring_id(kInvalidRingId) {}

        bq_cond_t ring_state_changed;
        bool is_master;
        host_id_t prev_host_id;
        ring_id_t ring_id;
    };

    ring_var_t ring_;
    int ring_version_;
    ring_state_t ring_state_;

    wait_pool_t cmd_wait_pool_;
    blocking_queue_t<ref_t<pi_ext_t>> received_cmd_queue_;

    pending_pool_t* pending_pool_;
    io_ring_sender_t* ring_sender_;

    const host_id_t host_id_;
    const uint32_t num_proposer_threads_;
    const uint32_t num_acceptor_threads_;

    void run_proposer();
    void run_acceptor();

    void wait_becoming_master();

    void watch_ring_state();
    void update_ring_state();
};

} // namespace phantom
#pragma GCC visibility pop
