// Copyright (C) 2012, Fedor Korotkiy <prime@yandex-team.ru>
// Copyright (C) 2012, YANDEX LLC.
// This code may be distributed under the terms of the GNU GPL v3.
// See ‘http://www.gnu.org/licenses/gpl.html’.
// vim: set tabstop=4 expandtab:
#include "io_paxos_executor.H"

#include <pd/base/config.H>
#include <pd/bq/bq_job.H>
#include <pd/lightning/pi_ring_cmd.H>

#include <phantom/module.H>

namespace phantom {

using namespace pd::cmd::ring;

MODULE(io_paxos_executor);

io_paxos_executor_t::io_paxos_executor_t(const string_t& name,
                                         const config_t& config)
    : io_t(name, config),
      acceptor_store_(config.acceptor_store),
      proposer_pool_(config.proposer_pool),
      ring_sender_(config.ring_sender),
      request_id_generator_(config.request_id_generator),
      cmd_wait_pool_(config.wait_pool_size),
      received_cmd_queue_(config.cmd_queue_size),
      host_id_(config.host_id),
      num_proposer_jobs_(config.num_proposer_jobs),
      num_acceptor_jobs_(config.num_acceptor_jobs),
      ring_reply_timeout_(config.ring_reply_timeout) {}

void io_paxos_executor_t::init() {}

void io_paxos_executor_t::fini() {}

void io_paxos_executor_t::run() {
    string_t job_name = string_t::ctor_t(name.size() + 1 + 8 + 1)
        (name)('[').print(CSTR("acceptor"))(']');

    for(uint32_t i = 0; i < num_acceptor_jobs_; ++i) {
        bq_job_t<typeof(&io_paxos_executor_t::run_acceptor)>::create(
            job_name,
            scheduler.bq_thr(),
            *this,
            &io_paxos_executor_t::run_acceptor
        );
    }
}

void io_paxos_executor_t::start_proposer() {
    proposer_jobs_count_.started(num_proposer_jobs_);

    string_t job_name = string_t::ctor_t(name.size() + 1 + 8 + 1)
        (name)('[').print(CSTR("proposer"))(']');

    for(uint32_t i = 0; i < num_proposer_jobs_; ++i) {
        bq_job_t<typeof(&io_paxos_executor_t::count_and_run_proposer)>::create(
            job_name,
            scheduler.bq_thr(),
            *this,
            &io_paxos_executor_t::count_and_run_proposer
        );
    }

}

void io_paxos_executor_t::wait_proposer_stop() {
    proposer_jobs_count_.wait_for_all_to_finish();
}

void io_paxos_executor_t::handle_ring_cmd(const ref_t<pi_ext_t>& ring_cmd) {
    ring_state_t ring_state = ring_state_snapshot();

    if(ring_id(ring_cmd) != ring_state.ring_id ||
       dst_host_id(ring_cmd) != host_id_)
    {
       return;
    }

    if(ring_state.is_master) {
        ref_t<wait_pool_t::data_t> data = cmd_wait_pool_.lookup(
            request_id(ring_cmd)
        );

        if(data) {
            data->send(ring_cmd);
        }
    } else {
        received_cmd_queue_.push(ring_cmd);
    }
}

void io_paxos_executor_t::ring_state_changed(ring_id_t ring_id,
                                             host_id_t next_in_ring,
                                             bool is_master) {
    thr::spinlock_guard_t ring_state_guard(ring_state_lock_);

    ring_state_.ring_id = ring_id;
    ring_state_.next_in_ring = next_in_ring;
    ring_state_.is_master = is_master;
}

void io_paxos_executor_t::run_acceptor() {
    while(true) {
        ref_t<pi_ext_t> ring_cmd;
        received_cmd_queue_.pop(&ring_cmd);

        accept_ring_cmd(ring_cmd);
    }
}

bool io_paxos_executor_t::is_master() {
    thr::spinlock_guard_t ring_state_guard(ring_state_lock_);
    return ring_state_.is_master;
}

io_paxos_executor_t::ring_state_t io_paxos_executor_t::ring_state_snapshot() {
    thr::spinlock_guard_t ring_state_guard(ring_state_lock_);
    return ring_state_;
}

void io_paxos_executor_t::count_and_run_proposer() {
    log_info("proposer started");

    run_proposer();

    proposer_jobs_count_.finish();
    log_info("proposer finished");
}

namespace io_paxos_executor {
config_binding_sname(io_paxos_executor_t);

config_binding_value(io_paxos_executor_t, host_id);

config_binding_value(io_paxos_executor_t, proposer_pool);
config_binding_value(io_paxos_executor_t, ring_sender);
config_binding_value(io_paxos_executor_t, acceptor_store);
config_binding_value(io_paxos_executor_t, request_id_generator);

config_binding_value(io_paxos_executor_t, wait_pool_size);
config_binding_value(io_paxos_executor_t, cmd_queue_size);

config_binding_value(io_paxos_executor_t, num_proposer_jobs);
config_binding_value(io_paxos_executor_t, num_acceptor_jobs);

config_binding_value(io_paxos_executor_t, ring_reply_timeout);

config_binding_parent(io_paxos_executor_t, io_t, 1);
}

} // namespace phantom
