// Copyright (C) 2012, Fedor Korotkiy <prime@yandex-team.ru>
// Copyright (C) 2012, YANDEX LLC.
// This code may be distributed under the terms of the GNU GPL v3.
// See ‘http://www.gnu.org/licenses/gpl.html’.
// vim: set tabstop=4 expandtab:
#include <pd/bq/bq_job.H>

#include <phantom/io_phase1_batch_executor/io_phase1_batch_executor.H>
#include <phantom/module.H>

namespace phantom {

io_phase1_batch_executor_t::io_phase1_batch_executor_t(
        const string_t& name,
        const config_t& config)
    : io_t(name, config),
      ring_(config.transport_config->ring()),
      cmd_wait_pool_(config.wait_pool_size),
      received_cmd_queue_(config.cmd_queue_size),
      host_id_(config.host_id),
      num_proposer_threads_(config.num_proposer_threads),
      num_acceptor_threads_(config.num_acceptor_threads) {}


void io_phase1_batch_executor_t::init() {}

void io_phase1_batch_executor_t::run_proposer() {

}

void io_phase1_batch_executor_t::run_acceptor() {
    while(true) {
        ref_t<pi_ext_t> ring_cmd;
        received_cmd_queue_.pop(&ring_cmd);

        std::vector<failed_instance_t> failed;
        for(instance_id_t iid = start_instance_id(ring_cmd);
                iid < end_instance_id(ring_cmd);
                ++iid) {
            ref_t<acceptor_instance_t> acceptor_instance =
                pending_pool_->lookup(iid);

            ballot_id_t promise = kInvalidBallotId;

            if(acceptor_instance) {
                acceptor_instance->next_ballot(
                    ballot_id(ring_cmd),
                    &promise,
                    NULL,
                    NULL);
            }

            if(promise > ballot_id(ring_cmd) || promise == kInvalidBallotId) {
                failed.push_back({ iid, promise });
            }
        }

        failed = merge_failed_instances(
            failed,
            failed_instances_pi_to_vector(failed_instances(ring_cmd))
        );

        ring_sender_->send(build_ring_batch_cmd(
            {
                request_id: request_id(ring_cmd),
                ring_id: ring_id(ring_cmd),
                host_id: host_id_
            },
            {
                start_instance_id: start_instance_id(ring_cmd),
                end_instance_id: end_instance_id(ring_cmd),
                ballot_id: ballot_id(ring_cmd),
                failed_instances: failed
            }
        ));
    }
}

void io_phase1_batch_executor_t::run() {
    ring_version_ = ring_.update();
    update_ring_state();

    bq_job_t<typeof(&io_phase1_batch_executor_t::watch_ring_state)>::create(
        STRING("watch_ring_state"),
        bq_thr_get(),
        *this,
        &io_phase1_batch_executor_t::watch_ring_state
    );

    for(size_t i = 0; i < num_proposer_threads_; ++i) {
        bq_job_t<typeof(&io_phase1_batch_executor_t::run_proposer)>::create(
            STRING("run_proposer"),
            bq_thr_get(),
            *this,
            &io_phase1_batch_executor_t::run_proposer
        );
    }

    for(size_t i = 0; i < num_acceptor_threads_; ++i) {
        bq_job_t<typeof(&io_phase1_batch_executor_t::run_acceptor)>::create(
            STRING("run_acceptor"),
            bq_thr_get(),
            *this,
            &io_phase1_batch_executor_t::run_acceptor
        );
    }
}

void io_phase1_batch_executor_t::fini() {}

void io_phase1_batch_executor_t::handle_cmd(const ref_t<pi_ext_t>& ring_cmd) {
    bq_cond_guard_t ring_state_guard(ring_state_.ring_state_changed);

    if(ring_id(ring_cmd) != ring_state_.ring_id ||
       host_id(ring_cmd) != ring_state_.prev_host_id) {
        return;
    }

    bool is_master = ring_state_.is_master;
    ring_state_guard.relax();

    if(is_master) {
        ref_t<wait_pool_t::data_t> data = cmd_wait_pool_.lookup(request_id(ring_cmd));

        if(data) {
            data->send(ring_cmd);
        }
    } else {
        received_cmd_queue_.push(ring_cmd);
    }
}

void io_phase1_batch_executor_t::watch_ring_state() {
    while(true) {
        ring_version_ = ring_.wait(ring_version_);
        update_ring_state();
    }
}

void io_phase1_batch_executor_t::update_ring_state() {
    while(!ring_.valid()) {
       ring_version_ = ring_.wait(ring_version_);
    }

    bq_cond_guard_t ring_state_guard(ring_state_.ring_state_changed);

    ring_state_.is_master = (ring_.master_host_id() == host_id_);
    ring_state_.prev_host_id = ring_.prev_host_id(host_id_);
    ring_state_.ring_id = ring_.ring_id();

    ring_state_.ring_state_changed.send(true);
}

void io_phase1_batch_executor_t::wait_becoming_master() {
    bq_cond_guard_t ring_state_guard(ring_state_.ring_state_changed);
    while(!ring_state_.is_master) {
        if(!bq_success(ring_state_.ring_state_changed.wait(NULL))) {
            throw exception_sys_t(log::error,
                                  errno,
                                  "wait_becoming_master");
        }
    }
}

} // namespace phantom
