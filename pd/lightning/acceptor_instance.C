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
      highest_promised_ballot_(kInvalidBallotId),
      highest_voted_ballot_(kInvalidBallotId),
      last_vote_(),
      committed_(false),
      pending_vote_request_id_(0),
      pending_vote_ballot_(kInvalidBallotId),
      pending_vote_value_id_(kInvalidValueId)
{}

void acceptor_instance_t::next_ballot(ballot_id_t  proposed_ballot,
                                      ballot_id_t* highest_promised_ballot,
                                      ballot_id_t* highest_voted_ballot,
                                      value_t*     last_vote)
{
    thr::spinlock_guard_t guard(lock_);

    if(proposed_ballot <= highest_promised_ballot_) {
        *highest_promised_ballot = highest_promised_ballot_;
        return;
    }

    highest_promised_ballot_ = proposed_ballot;
    *highest_promised_ballot = highest_promised_ballot_;
    *highest_voted_ballot    = highest_voted_ballot_;
    
    if(highest_voted_ballot_ != kInvalidBallotId) {
        *last_vote = last_vote_;
    }
}

void acceptor_instance_t::begin_ballot(ballot_id_t  ballot,
                                       value_t      value,
                                       ballot_id_t* highest_promised_ballot)
{
    thr::spinlock_guard_t guard(lock_);

    if(ballot < highest_promised_ballot_) {
        *highest_promised_ballot = highest_promised_ballot_;
        return;
    }

    highest_promised_ballot_ = ballot;
    *highest_promised_ballot = highest_promised_ballot_;

    if(ballot > highest_voted_ballot_) {
        highest_voted_ballot_ = ballot;
        last_vote_ = value;
    }
}

bool acceptor_instance_t::vote(request_id_t request_id,
                               ballot_id_t ballot,
                               value_id_t value_id)
{
    thr::spinlock_guard_t guard(lock_);

    if(ballot < highest_promised_ballot_) {
        return false;
    }

    if(ballot != highest_voted_ballot_ || last_vote_.value_id() != value_id) {
        pending_vote_request_id_ = request_id;
        pending_vote_ballot_ = ballot;
        pending_vote_value_id_ = value_id;
        return false;
    }

    return true;
}

bool acceptor_instance_t::commit(value_id_t value)
{
    thr::spinlock_guard_t guard(lock_);

    if(last_vote_.value_id() != value) {
        return false;
    }

    committed_ = true;
    return true;
}

instance_id_t acceptor_instance_t::instance_id() const {
    return instance_id_;
}

bool acceptor_instance_t::committed() const {
    thr::spinlock_guard_t guard(lock_);
    return committed_;
}

value_t acceptor_instance_t::committed_value() const {
    thr::spinlock_guard_t guard(lock_);
    return committed_ ? last_vote_ : value_t();
}

bool acceptor_instance_t::pending_vote(request_id_t* request_id,
                                       ballot_id_t*  ballot,
                                       value_id_t*   value_id)
{
    thr::spinlock_guard_t guard(lock_);

    if(pending_vote_ballot_ != kInvalidBallotId) {
        bool has_pending_vote = (pending_vote_ballot_ >= highest_promised_ballot_);
        if(has_pending_vote) {
            *request_id = pending_vote_request_id_;
            *ballot = pending_vote_ballot_;
            *value_id = pending_vote_value_id_;
        }
        pending_vote_request_id_ = 0;
        pending_vote_ballot_ = kInvalidBallotId;
        pending_vote_value_id_ = kInvalidValueId;
        return has_pending_vote;
    } else {
        return false;
    }
}

void acceptor_instance_t::recover(ref_t<pi_ext_t> data) {
    const pi_t& pi = data->pi();
    instance_id_t iid = pi.s_ind(0).s_uint();
    ballot_id_t   promised = pi.s_ind(1).s_uint();
    ballot_id_t   voted    = pi.s_ind(2).s_uint();
    const pi_t&   value    = pi.s_ind(3);

    thr::spinlock_guard_t guard(lock_);
    assert(iid == instance_id_);

    highest_promised_ballot_ = promised;
    highest_voted_ballot_    = voted;
    last_vote_.set(value);
    committed_ = true;
}

ref_t<pi_ext_t> acceptor_instance_t::pi_instance() const {
    thr::spinlock_guard_t guard(lock_);

    if(!committed_) {
        return NULL;
    }

    pi_t::pro_t::item_t iid(pi_t::pro_t::uint_t(instance_id_), NULL),
                        promised(pi_t::pro_t::uint_t(highest_promised_ballot_), &iid),
                        voted(pi_t::pro_t::uint_t(highest_voted_ballot_), &promised),
                        value(last_vote_.pi_value(), &voted);
    pi_t::pro_t pro(&value);
    return pi_ext_t::__build(pro);
}


}  // namespace pd
