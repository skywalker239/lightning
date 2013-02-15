// Copyright (C) 2012, Alexander Kharitonov <alexander.kharitonov@gmail.com>
// Copyright (C) 2012, YANDEX LLC.
// This code may be distributed under the terms of the GNU GPL v3.
// See ‘http://www.gnu.org/licenses/gpl.html’.
// vim: set tabstop=4 expandtab:
#include <pd/lightning/acceptor_instance.H>
#include <pd/base/assert.H>
#include <pd/pi/pi_pro.H>

namespace pd {

acceptor_instance_t::acceptor_instance_t(instance_id_t instance_id)
    : instance_id_(instance_id),
      highest_promised_ballot_(0),
      highest_proposed_ballot_(kInvalidBallotId),
      last_proposal_(),
      committed_(false),
      pending_vote_()
{}

bool acceptor_instance_t::promise(ballot_id_t ballot,
                                  ballot_id_t* highest_promised_ballot,
                                  ballot_id_t* highest_proposed_ballot,
                                  value_t* last_proposal)
{
    thr::spinlock_guard_t guard(lock_);

    bool promise_succeeded = (ballot > highest_promised_ballot_);

    if(promise_succeeded) {
        highest_promised_ballot_ = ballot;
    }

    if(highest_promised_ballot) {
        *highest_promised_ballot = highest_promised_ballot_;
    }

    if(highest_proposed_ballot) {
        *highest_proposed_ballot = highest_proposed_ballot_;
    }

    if(highest_proposed_ballot_ != kInvalidBallotId && last_proposal) {
        *last_proposal = last_proposal_;
    }

    return promise_succeeded;
}

bool acceptor_instance_t::propose(ballot_id_t ballot, value_t value) {
    thr::spinlock_guard_t guard(lock_);

    if(ballot < highest_promised_ballot_) {
        return false;
    }

    highest_proposed_ballot_ = ballot;
    last_proposal_ = value;
    return true;
}

bool acceptor_instance_t::vote(vote_t vote) {
    thr::spinlock_guard_t guard(lock_);

    if(vote.value_id == last_proposal_.value_id()) {
        return true;
    } else {
        if(vote.ballot_id >= highest_promised_ballot_) {
            pending_vote_ = vote;
        }

        return false;
    }
}

bool acceptor_instance_t::commit(value_id_t value_id) {
    thr::spinlock_guard_t guard(lock_);

    if(last_proposal_.value_id() != value_id) {
        return false;
    } else {
        committed_ = true;
        return true;
    }
}

instance_id_t acceptor_instance_t::iid() const {
    return instance_id_;
}

bool acceptor_instance_t::committed() const {
    thr::spinlock_guard_t guard(lock_);
    return committed_;
}

value_t acceptor_instance_t::committed_value() const {
    thr::spinlock_guard_t guard(lock_);
    return committed_ ? last_proposal_ : value_t();
}

bool acceptor_instance_t::pending_vote(vote_t* vote) {
    thr::spinlock_guard_t guard(lock_);

    if(pending_vote_.ballot_id != kInvalidBallotId &&
       pending_vote_.ballot_id >= highest_promised_ballot_)
    {
        *vote = pending_vote_;
        pending_vote_ = vote_t();
        return true;
    } else {
        return false;
    }
}

// void acceptor_instance_t::recover(ref_t<pi_ext_t> data) {
//     const pi_t& pi = data->pi();
//     instance_id_t iid = pi.s_ind(0).s_uint();
//     ballot_id_t   promised = pi.s_ind(1).s_uint();
//     ballot_id_t   voted    = pi.s_ind(2).s_uint();
//     const pi_t&   value    = pi.s_ind(3);

//     thr::spinlock_guard_t guard(lock_);
//     assert(iid == instance_id_);

//     highest_promised_ballot_ = promised;
//     highest_voted_ballot_    = voted;
//     last_vote_.set(value);
//     committed_ = true;
// }

// ref_t<pi_ext_t> acceptor_instance_t::pi_instance() const {
//     thr::spinlock_guard_t guard(lock_);

//     if(!committed_) {
//         return NULL;
//     }

//     pi_t::pro_t::item_t iid(pi_t::pro_t::uint_t(instance_id_), NULL),
//                         promised(pi_t::pro_t::uint_t(highest_promised_ballot_), &iid),
//                         voted(pi_t::pro_t::uint_t(highest_voted_ballot_), &promised),
//                         value(last_vote_.pi_value(), &voted);
//     pi_t::pro_t pro(&value);
//     return pi_ext_t::__build(pro);
// }


}  // namespace pd
