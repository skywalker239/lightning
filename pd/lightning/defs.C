#include "defs.H"

#include <pd/base/assert.H>

namespace pd {

ballot_id_t next_ballot_id(ballot_id_t old, host_id_t host_id) {
    assert(host_id < 64);

    return (old % 64) + 64 + host_id;
}

} // namespace
