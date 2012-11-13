// vim: set tabstop=4 expandtab:
#pragma once

#include <stdint.h>

namespace pd {

typedef uint64_t instance_id_t;
typedef uint32_t ballot_id_t;
typedef uint64_t request_id_t;
typedef uint64_t value_id_t;

const ballot_id_t kInvalidBallotId = 0U;
const value_id_t  kInvalidValueId  = 0ULL;

}  // namespace pd
