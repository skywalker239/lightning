#include <phantom/io_raft_consensus/io_raft_state.H>

#include <tuple>

namespace phantom {

void io_raft_state_t::init() {}

void io_raft_state_t::fini() {}

void io_raft_state_t::run() {}

void io_raft_state_t::stat(out_t&, bool) {}

term_t io_raft_state_t::wait_state(replica_state_t state) {
    bq_cond_guard_t guard(state_change_);
    while(state_ == state) {
        if(!bq_success(state_change_.wait(NULL))) {
            throw exception_sys_t(log::error, errno, "wait_state(): %m");
        }
    }
    return current_term_;
}

bool io_raft_state_t::is_in_state(replica_state_t state, term_t term) {
    bq_cond_guard_t guard(state_change_);
    return state == state_ && term == current_term_;
}

void io_raft_state_t::won_election(term_t term) {
    bq_cond_guard_t guard(state_change_);
    if(term == current_term_ && state_ == replica_state_t::CANDIDATE) {
        state_ = replica_state_t::LEADER;
        state_change_.send(true);
    }
}

bool io_raft_state_t::vote(term_t term,
                           replica_id_t candidate,
                           iid_t last_iid,
                           term_t last_term) {
    bq_cond_guard_t guard(state_change_);
    if(!check_term(term)) {
        return false;
    }

    iid_t local_iid;
    term_t local_term;

    std::tie(local_iid, local_term) = log_replica_->latest_entry();

    bool is_candidate_not_worse = last_term > local_term ||
                                  (last_term == local_term &&
                                   last_iid >= local_iid);

    if((vote_for_ == INVALID_REPLICA_ID || vote_for_ == candidate) && is_candidate_not_worse) {
        vote_for_ = candidate;
        reset_election_timeout();
        return true;
    } else {
        return false;
    }
}

bool io_raft_state_t::append(term_t term,
                             iid_t prev_iid,
                             term_t prev_term,
                             ref_t<pi_ext_t> entry) {
    bq_cond_guard_t guard(state_change_);
    if(!check_term(term)) {
        return false;
    }

    return log_replica_->append(prev_iid, prev_term, term, entry);
}

bool io_raft_state_t::commit(term_t term, iid_t iid) {
    bq_cond_guard_t guard(state_change_);
    if(!check_term(term)) {
        return false;
    }

    return log_replica_->commit(term, iid);
}

replica_id_t io_raft_state_t::replica_id() {
    bq_cond_guard_t guard(state_change_);
    return replica_id_;
}

term_t io_raft_state_t::current_term() {
    bq_cond_guard_t guard(state_change_);
    return current_term_;
}

bool io_raft_state_t::check_term(term_t term) {
    if(term > current_term_) {
        current_term_ = term;
        vote_for_ = INVALID_REPLICA_ID;

        if(state_ != replica_state_t::WITNESS) {
            state_ = replica_state_t::FOLLOWER;
        }
        state_change_.send(true);

        reset_election_timeout();

        return true;
    } else if(term == current_term_) {
        return true;
    } else {
        return false;
    }
}

void io_raft_state_t::reset_election_timeout() {

}

} // namespace phantom
