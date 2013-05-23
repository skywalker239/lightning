#pragma once

#pragma GCC visibility push(default)

#include <pd/lightning/pi_ext.H>

#include <phantom/pd.H>

namespace phantom {

class io_raft_state_t;

class io_raft_voter_t {
public:
    ref_t<pi_ext_t> handle_rpc(ref_t<pi_ext_t> cmd);

private:
    io_raft_state_t* raft_state_;
};

} // namespace phantom

#pragma GCC visibility pop
