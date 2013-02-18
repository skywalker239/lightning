#include "defs.H"

#include <limits>

#include <pd/base/assert.H>

namespace pd {

ballot_id_t next_ballot_id(ballot_id_t old, host_id_t host_id) {
    assert(host_id <= kMaxHostId);
    assert(old < (std::numeric_limits<ballot_id_t>::max() - (1 << (kHostIdBits + 1))));

    return (old % (1 << kHostIdBits)) +
           (1 << kHostIdBits) +
           host_id;
}

} // namespace
