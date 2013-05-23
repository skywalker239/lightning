#pragma once

#pragma GCC visibility push(default)

#include <pd/bq/bq_cond.H>

#include <pd/lightning/pi_ext.H>
#include <pd/lightning/cluster_conf.H>

#include <phantom/pd.H>

namespace phantom {

class io_raft_state_t;
class io_replica_set_t;

class io_raft_leader_t {
public:
    virtual void run();

    void handle_client_rpc(ref_t<pi_ext_t> cmd);

private:
    class election_t : public ref_count_atomic_t {
    public:
        election_t(lightning::cluster_conf_t conf);

        bool voted(replica_id_t follower, bool vote);

        friend class ref_t<election_t>;
    };

    io_raft_state_t* raft_state_;
    io_replica_set_t* replica_set_;

    bool run_election(ref_t<election_t> election, term_t term);
};

} // namespace phantom

#pragma GCC visibility pop
