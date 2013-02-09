// Copyright (C) 2012, Korotkiy Fedor <prime@yandex-team.ru>
// Copyright (C) 2012, YANDEX LLC.
// This code may be distributed under the terms of the GNU GPL v3.
// See ‘http://www.gnu.org/licenses/gpl.html’.
// vim: set tabstop=4 expandtab:
#pragma once

#include <pd/lightning/defs.H>
#include <pd/lightning/value.H>

#include <phantom/pd.H>

#pragma GCC visibility push(default)

namespace phantom {

/**
 * Maintains pool of active paxos instance ids along with some
 * additional information. Also responsible for throttling paxos
 * executors on proposer.
 *
 * Consists of 3 pools.
 *
 * 1) Failed pool - instances for which paxos failed for some
 * reason and must be started over. Stores (iid, ballot_id hint)
 * pairs.
 *
 * 2) Open pool - instances which successfully completed phase1
 * with no value received from acceptors. Stores (iid, ballot_id)
 * pairs.
 *
 * 3) Reserved pool - instances which successfully completed phase1
 * with some value received from acceptors or new value bounded from
 * client. Stores (iid, ballot_id, value) tuples.
 *
 * Can be ether in active or inactive state.
 *
 * 1) push_*() on inactive pool return immediately doing nothing.
 * 2) pop_*() on inactive pool return immediately false leaving
 *    other arguments undefined.
 */
class proposer_pool_t {
public:
    bool may_start_batch(instance_id_t start, instance_id_t end);

    void activate(instance_id_t start);

    void deactivate();

    void push_failed(instance_id_t instance_id, ballot_id_t ballot_hint);

    bool pop_failed(instance_id_t* instance_id, ballot_id_t* ballot_hint);

    void push_open(instance_id_t instance_id, ballot_id_t ballot_id);

    bool pop_open(instance_id_t* instance_id, ballot_id_t* ballot_id);

    void push_reserved(instance_id_t instance_id,
                       ballot_id_t ballot_id,
                       value_t value);

    bool pop_reserved(instance_id_t* instance_id,
                      ballot_id_t* ballot_id,
                      value_t* value);
};

} // namespace phantom
