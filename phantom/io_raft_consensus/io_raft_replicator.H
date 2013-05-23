#pragma once

#include <pd/lightning/defs.H>

#include <phantom/pd.H>
#include <phantom/io_lightning_network/io_lightning_network.H>
#include <phantom/io_raft_consensus/io_replica_set.H>

#pragma GCC visibility push(default)

namespace phantom {

class io_raft_state_t;
class io_follower_log_replica_t;

class io_raft_replicator_t {
public:
    virtual void run();

    void accept_client(ref_t<lightning_link_t> link);

private:
    io_raft_state_t* raft_state_;
    io_follower_log_replica_t* log_replica_;
    io_replica_set_t* replica_set_;
    io_lightning_network_t* network_;

    void accept_client(ref_t<lightning_link_t> link,
                       ref_t<io_replica_set_t::replica_info_t> replica_info);
};

} // namespace phantom

#pragma GCC visibility pop
