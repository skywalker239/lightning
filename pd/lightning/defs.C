#include "defs.H"

#include <limits>

#include <pd/base/assert.H>

namespace pd {

ballot_id_t next_ballot_id(ballot_id_t old, host_id_t host_id) {
    assert(host_id <= MAX_HOST_ID);
    assert(old < (std::numeric_limits<ballot_id_t>::max() - (1 << (HOST_ID_BITS + 1))));

    return (old % (1 << HOST_ID_BITS)) +
           (1 << HOST_ID_BITS) +
           host_id;
}

} // namespace
