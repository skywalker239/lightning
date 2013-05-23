#pragma once

#pragma GCC visibility push(default)

#include <pd/lightning/pi_ext.H>
#include <pd/lightning/append_heap.H>

#include <phantom/pd.H>

namespace phantom {

class io_raft_state_t;

class io_raft_appender_t {
public:
    ref_t<pi_ext_t> handle_rpc(ref_t<pi_ext_t> cmd);
    void handle_multicast(ref_t<pi_ext_t> cmd);

    struct append_result_t {
        bool success;
        iid_t latest_iid;
        term_t latest_term;
    };

private:
    io_raft_state_t* raft_state_;

    lightning::append_heap_t append_heap_;

    append_result_t append(ref_t<pi_ext_t> cmd);
};

} // namespace phantom

#pragma GCC visibility pop
