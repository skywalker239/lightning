#pragma once

#include <pd/base/ref.H>
#include <pd/base/thr.H>
#include <pd/lightning/defs.H>
#include <pd/lightning/pi_ext.H>
#include <pd/lightning/value.H>

namespace pd {

//! Represents a single Paxos instance in the acceptor.
class acceptor_instance_t : public ref_count_atomic_t {
public:
    acceptor_instance_t(instance_id_t instance_id);

    //! Performs Paxos Phase 1 on this instance with ballot proposed_ballot.
    //  Return values:
    //    * highest_promised_ballot is set to the ballot number for which
    //      the acceptor promises not to participate in any ballot with a
    //      smaller number.
    //      Upon a successful Phase 1, it will be equal to the proposed ballot,
    //      otherwise it will be higher than proposed_ballot and other return
    //      values will be undefined.
    //
    //    * highest_voted_ballot is set to the highest ballot number in which
    //      this acceptor has voted. If the acceptor has not voted in any
    //      ballot, this will be set to kInvalidBallotId and last_vote
    //      will be undefined.
    //
    //    * last_vote is set to the value for which the acceptor has voted
    //      in ballot highest_voted_ballot.
    void next_ballot(ballot_id_t  proposed_ballot,  
                     ballot_id_t* highest_promised_ballot,
                     ballot_id_t* highest_voted_ballot,
                     value_t*     last_vote);

    //! Starts Paxos Phase 2 on this instance with given (ballot, value).
    //  Return value:
    //    * highest_promised_ballot will be set to ballot upon success
    //      and to some higher ballot number upon failure (if this acceptor
    //      has already promised not to vote until some higher ballot).
    void begin_ballot(ballot_id_t  ballot,
                      value_t      value,
                      ballot_id_t* highest_promised_ballot);

    //! Applies a vote for (ballot, value_id) to this instance (for Ring Paxos).
    //  Returns true on success, false on failure (no value for value_id or
    //  already promised not to participate in ballot).
    //
    //  If we do not violate any promises with this vote but simply have not yet
    //  received its value, the pending vote will be stored in this instance
    //  for later retrieval.
    //
    //  request_id is the ring request id for this vote which needs to be stored
    //  to forward this vote later.
    bool vote(request_id_t request_id,
              ballot_id_t ballot,
              value_id_t value_id);

    //! Tries to commit value with value id value_id at ballot ballot.
    //  Returns true on success, false on failure (no value for this value_id).
    bool commit(value_id_t value_id);

    //! Id of this instance.
    instance_id_t instance_id() const;

    //! True iff committed.
    bool committed() const;

    //! Returns value_t() if not committed.
    value_t committed_value() const;

    //! If there is a pending vote stored, clears it, sets the output parameters
    //  to those of the vote and returns true.
    //  Otherwise returns false.
    bool pending_vote(request_id_t* request_id,
                      ballot_id_t* ballot,
                      value_id_t* value_id);

    //! Used to recover a committed instance from some other acceptor.
    void recover(ref_t<pi_ext_t> data);

    //! Serialize for recovery.
    //  Returns NULL if not committed.
    ref_t<pi_ext_t> pi_instance() const;

private:
    acceptor_instance_t(const acceptor_instance_t&) = delete;
    acceptor_instance_t& operator=(const acceptor_instance_t&) = delete;

    const instance_id_t instance_id_;

    ballot_id_t highest_promised_ballot_;
    ballot_id_t highest_voted_ballot_;
    value_t last_vote_;
    bool committed_;

    request_id_t pending_vote_request_id_;
    ballot_id_t  pending_vote_ballot_;
    value_id_t   pending_vote_value_id_;

    mutable thr::spinlock_t lock_;

    friend class ref_t<acceptor_instance_t>;
};

}  // namespace pd
