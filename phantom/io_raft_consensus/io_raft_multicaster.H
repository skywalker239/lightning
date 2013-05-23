#pragma once

#pragma GCC visibility push(default)

#include <pd/lightning/pi_ext.H>

namespace phantom {

class io_raft_state_t;
class io_follower_log_replica_t;

class io_raft_multicaster_t {
public:
    virtual void run();

private:
    io_raft_state_t* raft_state_;
    io_follower_log_replica_t* log_replica_;
};

} // namespace phantom

#pragma GCC visibility pop
