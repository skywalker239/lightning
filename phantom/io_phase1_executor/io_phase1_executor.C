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

using namespace cmd;

void io_phase1_executor_t::run_proposer() {

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
            fail.highest_proposed = promise::highest_proposed(ring_cmd);
            fail.last_proposal = &promise::last_proposal(ring_cmd);
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
