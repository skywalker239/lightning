// Copyright (C) 2013, Fedor Korotkiy <prime@yandex-team.ru>
// Copyright (C) 2013, YANDEX LLC.
// This code may be distributed under the terms of the GNU GPL v3.
// See ‘http://www.gnu.org/licenses/gpl.html’.
// vim: set tabstop=4 expandtab:
#include "io_phase1_executor.H"

#include <pd/base/config.H>
#include <pd/lightning/pi_ring_cmd.H>

#include <phantom/module.H>


namespace phantom {

MODULE(io_phase1_executor);

namespace io_phase1_executor {
config_binding_sname(io_phase1_executor_t);
config_binding_parent(io_phase1_executor_t, io_paxos_executor_t, 1);
config_binding_ctor(io_t, io_phase1_executor_t);
} // namespace io_phase1_executor

using namespace cmd;

io_phase1_executor_t::io_phase1_executor_t(const string_t& name,
                                           const config_t& config)
    : io_paxos_executor_t(name, config) {}


void io_phase1_executor_t::run_proposer() {
    while(true) {
        request_id_t request_id = request_id_generator_->get_guid();
        ring_state_t ring_state = ring_state_snapshot();

        instance_id_t iid;
        ballot_id_t ballot_id;

        if(!proposer_pool_->pop_failed(&iid, &ballot_id)) {
            return;
        }

        wait_pool_t::item_t wait_reply(cmd_wait_pool_, request_id);

        accept_ring_cmd(promise::build(
            {
                request_id: request_id,
                ring_id: ring_state.ring_id,
                dst_host_id: host_id_
            },
            {
                iid: iid,
                ballot_id: ballot_id,
                status: promise::status_t::SUCCESS,
                fail: NULL
            }
        ));

        interval_t timeout = ring_reply_timeout_;
        ref_t<pi_ext_t> reply = wait_reply->wait(&timeout);
        if(!reply) {
            proposer_pool_->push_failed(iid, next_ballot_id(ballot_id, host_id_));
        }

        if(promise::status(reply) == promise::status_t::SUCCESS) {
            proposer_pool_->push_open(iid, ballot_id);
        } else {
            if(promise::highest_promised(reply) == ballot_id) {
                proposer_pool_->push_reserved(iid, ballot_id, promise::last_proposal(reply));
            } else {
                proposer_pool_->push_failed(iid, next_ballot_id(promise::highest_promised(reply), host_id_));
            }
        }
    }
}

void io_phase1_executor_t::accept_ring_cmd(const ref_t<pi_ext_t>& ring_cmd) {
    ref_t<acceptor_instance_t> instance;

    promise::status_t status = promise::status_t::FAILED;
    ballot_id_t highest_promised, highest_proposed;
    value_t old_proposal;

    io_acceptor_store_t::err_t err = acceptor_store_->lookup(promise::iid(ring_cmd),
                                                             &instance);

    switch(err) {
      case io_acceptor_store_t::DEAD:
      case io_acceptor_store_t::FORGOTTEN:
        return; // ignore cmd

      case io_acceptor_store_t::UNREACHABLE:
      case io_acceptor_store_t::BEHIND_WALL:
        status = promise::status_t::FAILED;
        highest_promised = promise::ballot_id(ring_cmd);
        break;

      case io_acceptor_store_t::OK:
        if(instance->promise(promise::ballot_id(ring_cmd),
                             &highest_promised,
                             &highest_proposed,
                             &old_proposal)) {
            status = promise::status_t::SUCCESS;
        } else {
            status = promise::status_t::FAILED;
        }
        break;
    }

    promise::body_t body;
    body.iid = promise::iid(ring_cmd);
    body.ballot_id = promise::ballot_id(ring_cmd);

    promise::fail_t fail;

    if(promise::status(ring_cmd) == promise::status_t::SUCCESS &&
       status == promise::status_t::SUCCESS) {
        body.status = promise::status_t::SUCCESS;
        body.fail = NULL;
    } else {
        body.fail = &fail;
        fail.highest_promised = std::max(highest_promised,
                                         promise::highest_promised(ring_cmd));

        if(highest_proposed > promise::highest_proposed(ring_cmd)) {
            fail.highest_proposed = highest_proposed;
            fail.last_proposal = &old_proposal;
        } else {
            old_proposal = promise::last_proposal(ring_cmd);

            fail.highest_proposed = promise::highest_proposed(ring_cmd);
            fail.last_proposal = &old_proposal;
        }
    }

    ring_state_t ring_state = ring_state_snapshot();
    if(ring_state.ring_id != ring::ring_id(ring_cmd)) {
        return;
    }

    ring_sender_->send(promise::build(
        {
            request_id: ring::request_id(ring_cmd),
            ring_id: ring_state.ring_id,
            dst_host_id: ring_state.next_in_ring
        },
        body
    ));
}

} // namespace phantom
