// Copyright (C) 2012, Fedor Korotkiy <prime@yandex-team.ru>
// Copyright (C) 2012, YANDEX LLC.
// This code may be distributed under the terms of the GNU GPL v3.
// See ‘http://www.gnu.org/licenses/gpl.html’.
// vim: set tabstop=4 expandtab:
#include <limits>

#include <pd/bq/bq_job.H>

#include <phantom/io_phase1_batch_executor/io_phase1_batch_executor.H>
#include <phantom/module.H>

namespace phantom {

io_phase1_batch_executor_t::io_phase1_batch_executor_t(
        const string_t& name,
        const config_t& config)
    : io_t(name, config),
      cmd_wait_pool_(config.wait_pool_size),
      received_cmd_queue_(config.cmd_queue_size),
      pending_pool_(config.pending_pool),
      instance_pool_(config.instance_pool),
      ring_sender_(config.ring_sender),
      request_id_generator_(config.host_id),
      host_id_(config.host_id),
      num_proposer_jobs_(config.num_proposer_jobs),
      num_acceptor_jobs_(config.num_acceptor_jobs),
      batch_size_(config.batch_size),
      ring_reply_timeout_(config.ring_reply_timeout) {}


void io_phase1_batch_executor_t::init() {}

void io_phase1_batch_executor_t::start_proposer(instance_id_t start_iid) {
    next_batch_start_ = start_iid;

    proposer_jobs_count_.started(num_proposer_jobs_);

    for(uint32_t i = 0; i < num_proposer_jobs_; ++i) {
        bq_job_t<typeof(&io_phase1_batch_executor_t::run_proposer)>::create(
            STRING("io_phase1_batch_executor_t::run_proposer"),
            bq_thr_get(),
            *this,
            &io_phase1_batch_executor_t::run_proposer
        );
    }
}

void io_phase1_batch_executor_t::wait_proposer_stop() {
    proposer_jobs_count_.wait_for_all_to_finish();
}

ref_t<pi_ext_t> io_phase1_batch_executor_t::propose_batch(
        instance_id_t batch_start) {
    for(ballot_id_t ballot = 1; is_master(); ++ballot) {
        request_id_t request_id = request_id_generator_.get_guid();
        ring_state_t ring_state = ring_state_snapshot();

        wait_pool_t::item_t wait_reply(cmd_wait_pool_, request_id);

        accept_batch_cmd(build_ring_batch_cmd(
            {
                request_id: request_id,
                ring_id: ring_state.ring_id,
                dst_host_id: host_id_
            },
            {
                start_instance_id: batch_start,
                end_instance_id: batch_start + batch_size_,
                ballot_id: ballot,
                failed_instances: std::vector<failed_instance_t>()
            }
        ));

        interval_t timeout = ring_reply_timeout_;

        ref_t<pi_ext_t> reply = wait_reply->wait(&timeout);
        if(reply) {
            return reply; // received reply from ring
        }

        if(ballot >= 256 && ballot % 256 == 0) {
            log_warning("ballot for batch [%ld, %ld) grow to %d",
                        batch_start,
                        batch_start + batch_size_,
                        ballot);
        }

        assert(ballot != std::numeric_limits<ballot_id_t>::max());
    }

    return NULL;
}

void io_phase1_batch_executor_t::run_proposer() {
    while(is_master()) {
        instance_id_t batch_start = next_batch_start_.fetch_add(batch_size_);

        // TODO(prime@): ask instance_pool_t if it ok to run batch

        ref_t<pi_ext_t> reply = propose_batch(batch_start);

        if(reply) {
            push_to_instance_pool(reply);
        }
    }

    proposer_jobs_count_.finish();
}

void io_phase1_batch_executor_t::push_to_instance_pool(
        const ref_t<pi_ext_t>& ) {
    // TODO(prime@): push to instance pool
}

bool io_phase1_batch_executor_t::accept_one_instance(
        instance_id_t iid,
        ballot_id_t ballot_id,
        failed_instance_t* failed) {
    acceptor_instance_store_t::err_t err;
    ref_t<acceptor_instance_t> instance = pending_pool_->lookup(iid, &err);

    if(err == acceptor_instance_store_t::err_t::IID_TOO_LOW) {
        failed->status = IID_TOO_LOW;
    } else if(err == acceptor_instance_store_t::err_t::IID_TOO_HIGH) {
        failed->status = IID_TOO_HIGH;
    } else {
        assert(instance); // not low, not high, must be ok

        ballot_id_t highest_voted = kInvalidBallotId;
        ballot_id_t highest_promised = kInvalidBallotId;

        instance->next_ballot(ballot_id,
                              &highest_promised,
                              &highest_voted,
                              NULL);

        if(highest_promised == ballot_id && highest_voted == kInvalidBallotId) {
            // acceptor make promise and never voted before => success
            return true;
        } else {
            // something went wrong
            failed->status = RESERVED;
            failed->highest_promised = highest_promised;
        }
    }

    failed->iid = iid;
    return false;
}

void io_phase1_batch_executor_t::update_and_send_to_next(
        const ref_t<pi_ext_t>& received_cmd,
        const std::vector<failed_instance_t>& localy_failed) {
    std::vector<failed_instance_t> all_failed = merge_failed_instances(
        localy_failed,
        failed_instances_pi_to_vector(failed_instances(received_cmd))
    );

    ring_state_t ring_state = ring_state_snapshot();

    if(ring_state.ring_id != ring_id(received_cmd)) {
        return; // ring changed, drop packet
    }

    ring_sender_->send(build_ring_batch_cmd(
        {
            request_id: request_id(received_cmd),
            ring_id: ring_id(received_cmd),
            dst_host_id: ring_state.next_in_the_ring
        },
        {
            start_instance_id: start_instance_id(received_cmd),
            end_instance_id: end_instance_id(received_cmd),
            ballot_id: ballot_id(received_cmd),
            failed_instances: all_failed
        }
    ));
}

void io_phase1_batch_executor_t::accept_batch_cmd(const ref_t<pi_ext_t>& ring_cmd) {
    std::vector<failed_instance_t> failed;

    for(instance_id_t iid = start_instance_id(ring_cmd);
        iid < end_instance_id(ring_cmd);
        ++iid)
    {
        failed_instance_t fail;

        if(!accept_one_instance(iid, ballot_id(ring_cmd), &fail)) {
            failed.push_back(fail);
        }
    }

    update_and_send_to_next(ring_cmd, failed);
}

void io_phase1_batch_executor_t::run_acceptor() {
    while(true) {
        ref_t<pi_ext_t> ring_cmd;
        received_cmd_queue_.pop(&ring_cmd);

        accept_batch_cmd(ring_cmd);
    }
}

void io_phase1_batch_executor_t::run() {
    for(uint32_t i = 0; i < num_acceptor_jobs_; ++i) {
        bq_job_t<typeof(&io_phase1_batch_executor_t::run_acceptor)>::create(
            STRING("io_phase1_batch_executor_t::run_acceptor"),
            bq_thr_get(),
            *this,
            &io_phase1_batch_executor_t::run_acceptor
        );
    }
}

void io_phase1_batch_executor_t::fini() {}

void io_phase1_batch_executor_t::handle_cmd(const ref_t<pi_ext_t>& ring_cmd) {
    if(is_master()) {
        ref_t<wait_pool_t::data_t> data = cmd_wait_pool_.lookup(request_id(ring_cmd));

        if(data) {
            data->send(ring_cmd);
        }
    } else {
        received_cmd_queue_.push(ring_cmd);
    }
}

bool io_phase1_batch_executor_t::is_master() {
    bq_cond_guard_t ring_state_guard(ring_state_changed_);
    return ring_state_.is_master;
}

io_phase1_batch_executor_t::ring_state_t io_phase1_batch_executor_t::ring_state_snapshot() {
    bq_cond_guard_t ring_state_guard(ring_state_changed_);
    return ring_state_;
}

} // namespace phantom
