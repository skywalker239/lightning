// Copyright (C) 2012, Alexander Kharitonov <alexander.kharitonov@gmail.com>
// Copyright (C) 2012, YANDEX LLC.
// This code may be distributed under the terms of the GNU GPL v3.
// See ‘http://www.gnu.org/licenses/gpl.html’.
// vim: set tabstop=4 expandtab:
#pragma once

#include <pd/base/ref.H>
#include <pd/base/thr.H>
#include <pd/lightning/defs.H>
#include <pd/lightning/pi_ext.H>
#include <pd/lightning/value.H>

namespace pd {

/**
 * Represents a single Paxos instance in the acceptor.
 */
class acceptor_instance_t : public ref_count_atomic_t {
public:
    acceptor_instance_t(instance_id_t instance_id);

    /**
     * Performs Paxos Phase 1 on this instance.
     *
     * @return true on success, false otherwise.
     *
     * Output arguments are set if not NULL.
     */
    bool promise(ballot_id_t ballot,
                 ballot_id_t* highest_promised_ballot,
                 ballot_id_t* highest_proposed_ballot,
                 value_t* last_proposal);

    /**
     * Starts Paxos Phase 2 on this instance with given (ballot,
     * value).
     */
    bool propose(ballot_id_t ballot, value_t value);

    struct vote_t {
        vote_t()
            : request_id(kInvalidRequestId),
              ring_id(kInvalidRingId),
              ballot_id(kInvalidBallotId),
              value_id(kInvalidValueId) {}

        vote_t(request_id_t request_id,
               ring_id_t ring_id,
               ballot_id_t ballot_id,
               value_id_t value_id)
            : request_id(request_id),
              ring_id(ring_id),
              ballot_id(ballot_id),
              value_id(value_id) {}

        // Information needed to continue pending vote
        request_id_t request_id;
        ring_id_t ring_id;
        ballot_id_t ballot_id;

        // Actually metters in paxos
        value_id_t value_id;
    };

    bool vote(vote_t vote);

    //! Tries to commit value with value id value_id at ballot ballot.
    //  Returns true on success, false on failure (no value for this value_id).
    bool commit(value_id_t value_id);

    //! Id of this instance.
    instance_id_t iid() const;

    //! True iff committed.
    bool committed() const;

    //! Returns value_t() if not committed.
    value_t committed_value() const;

    //! If there is a pending vote stored, clears it, sets the output parameters
    //  to those of the vote and returns true.
    //  Otherwise returns false.
    bool pending_vote_ready(vote_t* vote);

    // //! Used to recover a committed value from some other acceptor.
    // void recover(ref_t<pi_ext_t> value);

    // //  Returns NULL if not committed.
    // ref_t<pi_ext_t> pi_instance() const;

private:
    acceptor_instance_t(const acceptor_instance_t&) = delete;
    acceptor_instance_t& operator=(const acceptor_instance_t&) = delete;

    const instance_id_t instance_id_;

    ballot_id_t highest_promised_ballot_;
    ballot_id_t highest_proposed_ballot_;
    value_t last_proposal_;
    bool committed_;

    vote_t pending_vote_;

    mutable thr::spinlock_t lock_;

    friend class ref_t<acceptor_instance_t>;
};

}  // namespace pd
