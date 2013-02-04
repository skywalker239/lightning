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
      pending_pool_(config.pending_pool),
      instance_pool_(config.instance_pool),
      ring_sender_(config.ring_sender),
      guid_generator_(config.host_id),
      host_id_(config.host_id),
      num_proposer_threads_(config.num_proposer_threads),
      num_acceptor_threads_(config.num_acceptor_threads),
      batch_size_(config.batch_size),
      ring_reply_timeout_(config.ring_reply_timeout) {}


void io_phase1_batch_executor_t::init() {}

void io_phase1_batch_executor_t::run_proposer() {
    while(true) {
        ring_id_t ring_id = wait_becoming_master_and_get_ring_id();

        instance_id_t batch_start = next_batch_start_.fetch_add(batch_size_);

        ref_t<pi_ext_t> reply;

        for(ballot_id_t ballot = 1; is_master(); ++ballot) {
            request_id_t request_id = guid_generator_.get_guid();

            wait_pool_t::item_t wait_reply(cmd_wait_pool_, request_id);

            accept_batch_cmd(build_ring_batch_cmd(
                {
                    request_id: request_id,
                    ring_id: ring_id,
                    host_id: host_id_
                },
                {
                    start_instance_id: batch_start,
                    end_instance_id: batch_start + batch_size_,
                    ballot_id: ballot,
                    failed_instances: std::vector<failed_instance_t>()
                }
            ));

            interval_t timeout = ring_reply_timeout_;
            reply = wait_reply.wait(&ring_reply_timeout);
            if(reply) {
                break;
            }
        }

        for (instance_id_t iid = start_instance_id(reply);
             iid < end_instance_id(reply);
             ++iid)
        {
            
        }
    }
}

void io_phase1_batch_executor_t::accept_batch_cmd(const ref_t<pi_ext_t>& ring_cmd) {
    std::vector<failed_instance_t> failed;
    for(instance_id_t iid = start_instance_id(ring_cmd);
        iid < end_instance_id(ring_cmd);
        ++iid)
    {
        ballot_id_t retry_ballot = kInvalidBallotId;
        failed_instance_status_t instance_status;

        ref_t<acceptor_instance_t> acceptor_instance = pending_pool_->lookup(iid);

        if(iid < instance_pool_->ancient_instance_id()) {
            instance_status = IID_TO_LOW;
        } else if(acceptor_instance) {
            // batcher on master run too far ahead, so we just
            // ignore this packet
            return;
        } else {
            ballot_id_t highest_voted = kInvalidBallotId;
            ballot_id_t highest_promised = kInvalidBallotId;

            acceptor_instance->next_ballot(ballot_id(ring_cmd),
                                           &highest_promised,
                                           &highest_voted,
                                           NULL);

            if(highest_voted != kInvalidBallot) {
                instance_status = RESERVED;
            }

            if(highest_promised == ballot_id(ring_cmd)) {
                retry_ballot = kInvalidBallotId;
            } else {
                retry_ballot = highest_promised;
            }
        }

        if(retry_ballot != kInvalidBallotId) {
            failed.push_back({
                iid, retry_ballot, instance_status
            });
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

void io_phase1_batch_executor_t::run_acceptor() {
    while(true) {
        ref_t<pi_ext_t> ring_cmd;
        received_cmd_queue_.pop(&ring_cmd);

        accept_ring_batch_cmd(ring_cmd);
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

    bool is_master = (ring_.master_host_id() == host_id_);

    if(is_master && !ring_state_.is_master) {
        instance_pool_->activate();
        next_batch_start_ = pending_pool_->min_not_commited_instance_id();
    } else if (!is_master && ring_state_.is_master) {
        instance_pool_->deactivate_and_clear();
    }

    ring_state_.is_master = is_master;
    ring_state_.prev_host_id = ring_.prev_host_id(host_id_);
    ring_state_.ring_id = ring_.ring_id();

    ring_state_.ring_state_changed.send(true);
}

bool io_phase1_batch_executor_t::is_master() {
    bq_cond_guard_t ring_state_guard(ring_state_.ring_state_changed);
    return is_master();
}

ring_id_t io_phase1_batch_executor_t::wait_becoming_master_and_get_ring_id() {
    bq_cond_guard_t ring_state_guard(ring_state_.ring_state_changed);
    while(!ring_state_.is_master) {
        if(!bq_success(ring_state_.ring_state_changed.wait(NULL))) {
            throw exception_sys_t(log::error,
                                  errno,
                                  "wait_becoming_master");
        }
    }

    return ring_state_.ring_id;
}

} // namespace phantom
