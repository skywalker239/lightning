#pragma once
// Copyright (C) 2012, Fedor Korotkiy <prime@yandex-team.ru>
// Copyright (C) 2012, YANDEX LLC.
// This code may be distributed under the terms of the GNU GPL v3.
// See ‘http://www.gnu.org/licenses/gpl.html’.
// vim: set tabstop=4 expandtab:
#include <atomic>

#include <pd/lightning/defs.H>
#include <pd/lightning/pi_ext.H>
#include <pd/lightning/pi_ring_cmd.H>
#include <pd/lightning/wait_pool.H>
#include <pd/lightning/blocking_queue.H>
#include <pd/lightning/pending_pool.H>
#include <pd/lightning/guid.H>
#include <pd/lightning/finished_counter.H>

#include <pd/bq/bq_cond.H>

#include <phantom/io.H>
#include <phantom/io_ring_sender/io_ring_sender.H>
#include <phantom/proposer_pool/proposer_pool.H>

#pragma GCC visibility push(default)
namespace phantom {

/**
 * Phase1 batch executor.
 */
class io_phase1_batch_executor_t : public io_t {
public:
    struct config_t : public io_t::config_t {
        host_id_t host_id;

        config::objptr_t<proposer_pool_t> proposer_pool;
        config::objptr_t<io_ring_sender_t> ring_sender;
        config::objptr_t<pending_pool_t> pending_pool;

        uint32_t wait_pool_size;
        uint32_t cmd_queue_size;

        uint32_t num_proposer_jobs;
        uint32_t num_acceptor_jobs;

        uint32_t batch_size;
        interval_t ring_reply_timeout;

        config_t() throw()
            : host_id(kInvalidHostId),
              wait_pool_size(8),
              cmd_queue_size(128),
              num_proposer_jobs(4),
              num_acceptor_jobs(16),
              batch_size(1024) {}
        ~config_t() throw() {}
        void check(const in_t::ptr_t& p) const;
    };

    io_phase1_batch_executor_t(const string_t& name,
                               const config_t& config);

    void handle_cmd(const ref_t<pi_ext_t>& ring_cmd);

    void ring_state_changed(ring_id_t ring_id,
                            host_id_t next_in_the_ring,
                            bool is_master);

    void wait_proposer_stop();

    void start_proposer(instance_id_t start_iid);

    virtual void init();
    virtual void run();
    virtual void fini();

private:
    struct ring_state_t {
        ring_state_t()
            : ring_id(kInvalidRingId),
              next_in_the_ring(kInvalidHostId),
              is_master(false) {}

        ring_id_t ring_id;
        host_id_t next_in_the_ring;
        bool is_master;
    };

    ring_state_t ring_state_;
    pd::bq_cond_t ring_state_changed_;

    wait_pool_t cmd_wait_pool_;
    blocking_queue_t<ref_t<pi_ext_t>> received_cmd_queue_;

    finished_counter_t proposer_jobs_count_;

    pending_pool_t* pending_pool_;
    proposer_pool_t* proposer_pool_;
    io_ring_sender_t* ring_sender_;

    guid_generator_t request_id_generator_;

    const host_id_t host_id_;
    const uint32_t num_proposer_jobs_;
    const uint32_t num_acceptor_jobs_;
    const uint32_t batch_size_;
    const interval_t ring_reply_timeout_;

    std::atomic<instance_id_t> next_batch_start_;

    void run_proposer();
    void run_acceptor();

    ring_state_t ring_state_snapshot();
    bool is_master();

    ref_t<pi_ext_t> propose_batch(instance_id_t batch_start);

    void push_to_proposer_pool(const ref_t<pi_ext_t>& ring_reply);

    void accept_batch_cmd(const ref_t<pi_ext_t>& ring_cmd);

    bool accept_one_instance(instance_id_t iid,
                             ballot_id_t ballot_id,
                             batch_fail_t* fail);

    void update_and_send_to_next(const ref_t<pi_ext_t>& received_cmd,
                                 const std::vector<batch_fail_t>& fails);

};

} // namespace phantom
#pragma GCC visibility pop
