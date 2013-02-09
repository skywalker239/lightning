#pragma once
// Copyright (C) 2012, Fedor Korotkiy <prime@yandex-team.ru>
// Copyright (C) 2012, YANDEX LLC.
// This code may be distributed under the terms of the GNU GPL v3.
// See ‘http://www.gnu.org/licenses/gpl.html’.
// vim: set tabstop=4 expandtab:
#include <pd/base/config.H>
#include <pd/bq/bq_thr.H>
#include <pd/lightning/defs.H>
#include <pd/lightning/pending_pool.H>
#include <pd/lightning/finished_counter.H>
#include <pd/lightning/wait_pool.H>
#include <pd/lightning/blocking_queue.H>

#include <phantom/pd.H>
#include <phantom/io.H>
#include <phantom/io_ring_sender/io_ring_sender.H>
#include <phantom/ring_handler/ring_handler.H>
#include <phantom/proposer_pool/proposer_pool.H>
#include <phantom/io_guid/io_guid.H>

#pragma GCC visibility push(default)
namespace phantom {

class io_paxos_executor_t : public io_t, public ring_handler_t {
public:
    struct config_t : public io_t::config_t {
        host_id_t host_id;

        config::objptr_t<proposer_pool_t> proposer_pool;
        config::objptr_t<io_ring_sender_t> ring_sender;
        config::objptr_t<pending_pool_t> pending_pool;
        config::objptr_t<io_guid_t> request_id_generator;

        uint32_t wait_pool_size;
        uint32_t cmd_queue_size;

        uint32_t num_proposer_jobs;
        uint32_t num_acceptor_jobs;

        interval_t ring_reply_timeout;

        void check(const in_t::ptr_t& p) const;
    };

    io_paxos_executor_t(const string_t& name,
                        const config_t& config);

    virtual void handle_ring_cmd(const ref_t<pi_ext_t>& cmd);

    virtual void init();
    virtual void run();
    virtual void fini();

    void start_proposer();
    void wait_proposer_stop();

    void ring_state_changed(ring_id_t ring_id,
                            host_id_t next_in_ring,
                            bool is_master);
protected:
    struct ring_state_t {
        ring_state_t()
            : ring_id(kInvalidRingId),
              next_in_ring(kInvalidHostId),
              is_master(false) {}

        ring_id_t ring_id;
        host_id_t next_in_ring;
        bool is_master;
    };

    ring_state_t ring_state_;
    thr::spinlock_t ring_state_lock_;

    pending_pool_t* pending_pool_;
    proposer_pool_t* proposer_pool_;
    io_ring_sender_t* ring_sender_;
    io_guid_t* request_id_generator_;

    wait_pool_t cmd_wait_pool_;
    blocking_queue_t<ref_t<pi_ext_t>> received_cmd_queue_;

    finished_counter_t proposer_jobs_count_;

    const host_id_t host_id_;
    const uint32_t num_proposer_jobs_;
    const uint32_t num_acceptor_jobs_;
    const interval_t ring_reply_timeout_;

    virtual void run_proposer() = 0;
    virtual void accept_ring_cmd(const ref_t<pi_ext_t>& cmd) = 0;

    void run_acceptor();
    ring_state_t ring_state_snapshot();
    bool is_master();
};

} // namespace phantom
#pragma GCC visibility pop
