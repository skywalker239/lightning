#pragma once

#include <pd/bq/bq_cond.H>

#include <pd/lightning/cluster_conf.H>

#include <phantom/io.H>
#include <phantom/io_lightning_network/io_lightning_network.H>
#include <phantom/io_raft_consensus/io_log_replica.H>

#pragma GCC visibility push(default)

namespace phantom {

//! Raft state machine.
//
// State consists of following parts.
//  * replica_id - id of this replica
//  * current_term - latest term server has seen
//  * vote_for - replica_id of candidates that received vote in
//               current term
//  * log[] - log entries
//  * cluster_conf - set of servers participating in protocol
//
class io_raft_state_t {
public:
    struct config_t : public io_t::config_t {
        config_t() {}
        void check(const in_t::ptr_t& p) const;
    };

    enum class replica_state_t {
        // Accepts APPEND cmds, don't participates in protocol.
        // kvs node or recovering lightning node is a witness.
        WITNESS,
        // Accepts APPEND cmds, participates in votes.
        FOLLOWER,
        // Tries to elect.
        CANDIDATE,
        // Sends APPEND cmds, accepts client requests.
        LEADER
    };

    io_raft_state_t(const string_t& name, const config_t& config);

    virtual void init();
    virtual void run();
    virtual void fini();

    virtual void stat(out_t&, bool);

    term_t wait_state(replica_state_t state);

    bool is_in_state(replica_state_t state, term_t term);

    //! Handle vote rpc from raft protocol.
    bool vote(term_t term,
              replica_id_t candidate,
              iid_t last_iid,
              term_t last_term);

    //! Handle first part of append rpc from raft protocol
    bool append(term_t term,
                iid_t prev_iid,
                term_t prev_term,
                ref_t<pi_ext_t> entry);

    //! Handle second part of append rpc from raft protocol
    //
    // Not sure if it is a good idea to split append rpc handler into
    // two parts
    //
    bool commit(term_t term, iid_t iid);

    //! Leader may appends entries to log without any checks.
    iid_t leader_append(ref_t<pi_ext_t> entry);

    //! Notify replica that it had won election in @param term.
    void won_election(term_t term);

    replica_id_t replica_id();
    term_t current_term();

    // Current
    lightning::cluster_conf_t get_cluster_conf();

private:
    bq_cond_t state_change_;

    replica_id_t replica_id_;
    // max seen term
    term_t current_term_;
    // vote in current_term, INVALID_REPLICA_ID if this replica hasn't
    // voted in this term
    replica_id_t vote_for_;

    replica_state_t state_;

    bool check_term(term_t term);

    void reset_election_timeout();

    io_log_replica_t* log_replica_;
};

} // namespace phantom
