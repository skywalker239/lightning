// Copyright (C) 2012, Fedor Korotkiy <prime@yandex-team.ru>
// Copyright (C) 2012, YANDEX LLC.
// This code may be distributed under the terms of the GNU GPL v3.
// See ‘http://www.gnu.org/licenses/gpl.html’.
// vim: set tabstop=4 expandtab:
#include <pd/bq/bq_job.H>

#include <phantom/io_phase_executor_base/io_phase_executor_base.H>
#include <phantom/module.H>

namespace phantom {

io_phase_executor_base_t::io_phase_executor_base_t(
        const string_t& name,
        const config_t& config)
    : io_t(name, config),
      cmd_wait_pool_(config.wait_pool_size),
      received_cmd_queue_(config.cmd_queue_size),
      ring_(config.transport_config->ring()),
      num_proposer_threads_(config.num_proposer_threads),
      num_acceptor_threads_(config.num_acceptor_threads) {}


void io_phase_executor_base_t::init() {}

void io_phase_executor_base_t::run_proposer_base() {
    run_proposer();
}

void io_phase_executor_base_t::run_acceptor_base() {
    run_acceptor();
}

void io_phase_executor_base_t::run() {
    ring_version_ = ring_.update();
    update_ring_state();

    bq_job_t<typeof(&io_phase_executor_base_t::watch_ring_state)>::create(
        STRING("watch_ring_state"),
        bq_thr_get(),
        *this,
        &io_phase_executor_base_t::watch_ring_state
    );

    for(uint32_t i = 0; i < num_proposer_threads_; ++i) {
        bq_job_t<typeof(&io_phase_executor_base_t::run_proposer_base)>::create(
            STRING("run_proposer"),
            bq_thr_get(),
            *this,
            &io_phase_executor_base_t::run_proposer_base
        );
    }

    for(uint32_t i = 0; i < num_acceptor_threads_; ++i) {
        bq_job_t<typeof(&io_phase_executor_base_t::run_acceptor_base)>::create(
            STRING("run_acceptor"),
            bq_thr_get(),
            *this,
            &io_phase_executor_base_t::run_acceptor_base
        );
    }
}

void io_phase_executor_base_t::fini() {}

void io_phase_executor_base_t::handle_cmd(const ref_t<pi_ext_t>& ring_cmd) {
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

void io_phase_executor_base_t::watch_ring_state() {
    while(true) {
        ring_version_ = ring_.wait(ring_version_);
        update_ring_state();
    }
}

void io_phase_executor_base_t::update_ring_state() {
    while(!ring_.valid()) {
       ring_version_ = ring_.wait(ring_version_);
    }

    bq_cond_guard_t ring_state_guard(ring_state_.ring_state_changed);

    ring_state_.is_master = (ring_.master_host_id() == host_id_);
    ring_state_.prev_host_id = ring_.prev_host_id(host_id_);
    ring_state_.ring_id = ring_.ring_id();

    ring_state_.ring_state_changed.send(true);
}

void io_phase_executor_base_t::wait_becoming_master() {
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
