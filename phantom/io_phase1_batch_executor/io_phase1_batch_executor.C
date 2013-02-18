// Copyright (C) 2012, Fedor Korotkiy <prime@yandex-team.ru>
// Copyright (C) 2012, YANDEX LLC.
// This code may be distributed under the terms of the GNU GPL v3.
// See ‘http://www.gnu.org/licenses/gpl.html’.
// vim: set tabstop=4 expandtab:
#include "io_phase1_batch_executor.H"

#include <pd/bq/bq_job.H>

#include <phantom/module.H>

namespace phantom {

MODULE(io_phase1_batch_executor);

void io_phase1_batch_executor_t::config_t::check(const in_t::ptr_t& ) const {
//    TODO(prime@): sanitize config
}

io_phase1_batch_executor_t::io_phase1_batch_executor_t(
        const string_t& name,
        const config_t& config)
    : io_paxos_executor_t(name, config),
      batch_size_(config.batch_size) {}

ref_t<pi_ext_t> io_phase1_batch_executor_t::propose_batch(
        instance_id_t batch_start) {
    for(ballot_id_t ballot = host_id_;
        is_master();
        ballot = next_ballot_id(ballot, host_id_))
    {
        request_id_t request_id = request_id_generator_->generator().get_guid();
        ring_state_t ring_state = ring_state_snapshot();

        wait_pool_t::item_t wait_reply(cmd_wait_pool_, request_id);

        accept_ring_cmd(build_ring_batch_cmd(
            {
                request_id: request_id,
                ring_id: ring_state.ring_id,
                dst_host_id: host_id_  // sending to local acceptor
            },
            {
                start_iid: batch_start,
                end_iid: batch_start + batch_size_,
                ballot_id: ballot,
                fails: std::vector<batch_fail_t>()
            }
        ));

        interval_t timeout = ring_reply_timeout_;

        ref_t<pi_ext_t> reply = wait_reply->wait(&timeout);
        if(reply) {
            return reply; // received reply from ring
        }

        if(ballot >= 1024 * kMaxHostId) {
            log_warning("ballot for batch [%ld, %ld) grow to %d",
                        batch_start,
                        batch_start + batch_size_,
                        ballot);
        }
    }

    return NULL;
}

bool io_phase1_batch_executor_t::next_batch_start(instance_id_t* start) {
    bq_mutex_guard_t guard(next_batch_start_lock_);

    *start = next_batch_start_;
    next_batch_start_ += batch_size_;

    // calling under lock, so batch intervals are strongly increasing
//    return proposer_pool_->may_start_batch(*start, next_batch_start_);
    return true;
}

void io_phase1_batch_executor_t::run_proposer() {
    while(is_master()) {
        instance_id_t batch_start;

        if(!next_batch_start(&batch_start)) {
            // this host is not master any more
            return;
        }

        ref_t<pi_ext_t> reply = propose_batch(batch_start);

        if(reply) {
            push_to_proposer_pool(reply);
        }
    }

    proposer_jobs_count_.finish();
}

void io_phase1_batch_executor_t::push_to_proposer_pool(
        const ref_t<pi_ext_t>& ring_reply) {
    auto fail_ptr = pi_t::array_t::c_ptr_t(batch_fails(ring_reply));

    for(instance_id_t iid = batch_start_iid(ring_reply);
        iid < batch_end_iid(ring_reply);
        ++iid)
    {
        if(fail_ptr && fail_iid(*fail_ptr) == iid) {
            switch(fail_status(*fail_ptr)) {
            case instance_status_t::IID_TOO_HIGH:
                log_warning("received IID_TOO_HIGH");

                proposer_pool_->push_failed(
                    iid,
                    next_ballot_id(batch_ballot_id(ring_reply), host_id_)
                );
                break;
            case instance_status_t::LOW_BALLOT_ID:
            case instance_status_t::RESERVED:
                proposer_pool_->push_failed(
                    iid,
                    next_ballot_id(fail_highest_promise(*fail_ptr), host_id_)
                );
                break;
            case instance_status_t::IID_TOO_LOW:
                log_error("batcher received IID_TOO_LOW, bug in ring selector");
                assert(!"batcher received IID_TOO_LOW, bug in ring selector");
                break;
            case instance_status_t::OPEN:
                log_error("batcher received OPEN, this should never happed");
                break;
            default:
                log_error("batcher received unknown instance status");
                break;
            }

            ++fail_ptr;
        } else {
            proposer_pool_->push_open(iid, batch_ballot_id(ring_reply));
        }
    }
}

bool io_phase1_batch_executor_t::accept_one_instance(
        instance_id_t iid,
        ballot_id_t ballot_id,
        batch_fail_t* fail) {
    ref_t<acceptor_instance_t> instance;
    io_acceptor_store_t::err_t err = acceptor_store_->lookup(iid, &instance);

    if(err == io_acceptor_store_t::DEAD ||
       err == io_acceptor_store_t::FORGOTTEN)
    {
        log_error("Instance %ld is DEAD or FORGOTTEN."
                  "Bug in ring selector, this should never happend.", iid);
        // master better kill himself, LOL
        fail->status = instance_status_t::IID_TOO_LOW;
    } else if(err == io_acceptor_store_t::BEHIND_WALL ||
              err == io_acceptor_store_t::UNREACHABLE) {
        fail->status = instance_status_t::IID_TOO_HIGH;
    } else {
        assert(instance);

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
            if(highest_voted != kInvalidBallotId) {
                fail->status = instance_status_t::RESERVED;
            } else {
                fail->status = instance_status_t::LOW_BALLOT_ID;
            }
            fail->highest_promised = highest_promised;
        }
    }

    fail->iid = iid;
    return false;
}

void io_phase1_batch_executor_t::update_and_send_to_next(
        const ref_t<pi_ext_t>& received_cmd,
        const std::vector<batch_fail_t>& localy_failed) {
    std::vector<batch_fail_t> all_failed = merge_fails(
        localy_failed,
        fails_pi_to_vector(batch_fails(received_cmd))
    );

    ring_state_t ring_state = ring_state_snapshot();

    if(ring_state.ring_id != ring_id(received_cmd)) {
        return; // ring changed, drop packet
    }

    ring_sender_->send(build_ring_batch_cmd(
        {
            request_id: request_id(received_cmd),
            ring_id: ring_id(received_cmd),
            dst_host_id: ring_state.next_in_ring
        },
        {
            start_iid: batch_start_iid(received_cmd),
            end_iid: batch_end_iid(received_cmd),
            ballot_id: batch_ballot_id(received_cmd),
            fails: all_failed
        }
    ));
}

void io_phase1_batch_executor_t::accept_ring_cmd(const ref_t<pi_ext_t>& ring_cmd) {
    std::vector<batch_fail_t> all_failed;

    for(instance_id_t iid = batch_start_iid(ring_cmd);
        iid < batch_end_iid(ring_cmd);
        ++iid)
    {
        batch_fail_t fail;

        if(!accept_one_instance(iid, batch_ballot_id(ring_cmd), &fail)) {
            all_failed.push_back(fail);
        }
    }

    update_and_send_to_next(ring_cmd, all_failed);
}

} // namespace phantom
