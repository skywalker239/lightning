// Copyright (C) 2012, Fedor Korotkiy <prime@yandex-team.ru>
// Copyright (C) 2012, YANDEX LLC.
// This code may be distributed under the terms of the GNU GPL v3.
// See ‘http://www.gnu.org/licenses/gpl.html’.
// vim: set tabstop=4 expandtab:
#include "io_phase2_executor.H"

#include <pd/lightning/pi_ring_cmd.H>
#include <pd/lightning/pi_udp_cmd.H>

#include <phantom/module.H>

namespace phantom {

MODULE(io_phase2_executor);

namespace io_phase2_executor {
config_binding_sname(io_phase2_executor_t);
config_binding_parent(io_phase2_executor_t, io_paxos_executor_t, 1);
config_binding_ctor(io_t, io_phase2_executor_t);
//config_binding_ctor(io_blob_receiver::handler_t, io_phase2_executor_t);

config_binding_value(io_phase2_executor_t, blob_sender);
config_binding_value(io_phase2_executor_t, udp_guid_generator);
} // namespace io_phase2_executor

using namespace pd::cmd;

io_phase2_executor_t::io_phase2_executor_t(const string_t& name,
                                           const config_t& config)
    : io_paxos_executor_t(name, config),
      blob_sender_(config.blob_sender),
      udp_guid_generator_(config.udp_guid_generator) {}


void io_phase2_executor_t::run_proposer() {
    while(true) {
        ring_state_t ring_state = ring_state_snapshot();

        if(!ring_state.is_master) {
            break;
        }

        instance_id_t iid;
        ballot_id_t ballot_id;
        value_t value;
        if(!proposer_pool_->pop_reserved(&iid, &ballot_id, &value)) {
            break;
        }

        request_id_t request_id = request_id_generator_->get_guid();

        ref_t<pi_ext_t> propose_cmd = propose::build(
            request_id,
            iid,
            ballot_id,
            value
        );

        blob_sender_->send(udp_guid_generator_->get_guid(), propose_cmd);
        bool propose_succeeded = propose(propose_cmd);

        ref_t<pi_ext_t> reply;

        if(propose_succeeded) {
            wait_pool_t::item_t wait_reply(cmd_wait_pool_, request_id);

            accept_ring_cmd(vote::build(
                {
                    request_id: request_id,
                    ring_id: ring_state.ring_id,
                    dst_host_id: host_id_
                },
                {
                    iid: iid,
                    ballot_id: ballot_id,
                    value_id: value.value_id()
                }
            ));

            interval_t timeout = ring_reply_timeout_;
            reply = wait_reply->wait(&timeout);
        }

        if(propose_succeeded && reply) {
            ref_t<pi_ext_t> commit_cmd = commit::build(iid, value.value_id());

            blob_sender_->send(udp_guid_generator_->get_guid(), commit_cmd);
            commit(commit_cmd);
        } else {
            proposer_pool_->push_failed(iid, next_ballot_id(ballot_id, host_id_));
        }
    }
}

void io_phase2_executor_t::accept_ring_cmd(const ref_t<pi_ext_t>& ring_cmd) {
    ref_t<acceptor_instance_t> instance;
    auto err = acceptor_store_->lookup(vote::iid(ring_cmd), &instance);
    if(err != io_acceptor_store_t::OK) {
        log_warning("iid is too high or too low (iid = %ld)(accept_ring_cmd)",
                    vote::iid(ring_cmd));
        return;
    }

    apply_vote_and_send_to_next(
        instance,
        acceptor_instance_t::vote_t(
            ring::request_id(ring_cmd),
            ring::ring_id(ring_cmd),
            vote::ballot_id(ring_cmd),
            vote::value_id(ring_cmd)
        )
    );
}

void io_phase2_executor_t::handle(ref_t<pi_ext_t> udp_cmd,
                                  const netaddr_t& /* remote_addr */) {
    if(is_master()) {
        log_debug("ignoring udp cmd on master");
    }

    if(!udp::is_valid(udp_cmd)) {
        log_warning("received invalid udp cmd");
        return;
    }

    switch(udp::type(udp_cmd)) {
    case udp::type_t::PROPOSE:
        propose(udp_cmd);
        break;
    case udp::type_t::COMMIT:
        commit(udp_cmd);
        break;
    default:
        log_error("unknown udp cmd type");
        break;
    }
}

bool io_phase2_executor_t::propose(const ref_t<pi_ext_t>& udp_cmd) {
    ref_t<acceptor_instance_t> instance;
    auto err = acceptor_store_->lookup(propose::iid(udp_cmd), &instance);
    if(err != io_acceptor_store_t::OK) {
        log_warning("iid is too high or too low (iid=%ld)(begin_ballot)",
                    propose::iid(udp_cmd));
        return false;
    }

    if(!instance->propose(propose::ballot_id(udp_cmd),
                          propose::value(udp_cmd))) {
        log_debug("propose failed(iid=%ld)", instance->iid());
        return false;
    }

    acceptor_instance_t::vote_t vote;
    if(instance->pending_vote_ready(&vote)) {
        log_debug("continuing pending vote(iid=%ld)", instance->iid());
        apply_vote_and_send_to_next(instance, vote);
    }

    return true;
}

void io_phase2_executor_t::apply_vote_and_send_to_next(
        const ref_t<acceptor_instance_t>& instance,
        acceptor_instance_t::vote_t vote) {
    ring_state_t ring_state = ring_state_snapshot();

    if(ring_state.ring_id != vote.ring_id) {
        log_debug("ignoring vote because ring_id has changed(iid=%ld)",
                  instance->iid());
        return;
    }

    if(instance->vote(vote)) {
        ring_sender_->send(vote::build(
            {
                request_id: vote.request_id,
                ring_id: vote.ring_id,
                dst_host_id: ring_state.next_in_ring
            },
            {
                iid: instance->iid(),
                ballot_id: vote.ballot_id,
                value_id: vote.value_id
            }
        ));
    } else {
        log_debug("vote failed for iid=%ld", instance->iid());
    }
}

void io_phase2_executor_t::commit(const ref_t<pi_ext_t>& cmd) {
    ref_t<acceptor_instance_t> instance;
    auto err = acceptor_store_->lookup(commit::iid(cmd), &instance);
    if(err != io_acceptor_store_t::OK) {
        log_warning("iid is too high or too low (iid = %ld)(commit)",
                    commit::iid(cmd));
        return;
    }

    if(instance->commit(commit::value_id(cmd))) {
        acceptor_store_->notify_commit();
    } else {
        log_debug("commit failed for iid=%ld", instance->iid());
        // TODO(prime@): maybe start recovery
    }
}

} // namespace phantom
