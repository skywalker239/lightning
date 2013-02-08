// Copyright (C) 2012, Alexander Kharitonov <alexander.kharitonov@gmail.com>
// Copyright (C) 2012, YANDEX LLC.
// This code may be distributed under the terms of the GNU GPL v3.
// See ‘http://www.gnu.org/licenses/gpl.html’.
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

typedef uint32_t host_id_t;
typedef uint32_t ring_id_t;

const ring_id_t kInvalidRingId = 0;
const host_id_t kInvalidHostId = ~0U;
const int kHostIdBits = 12;
const host_id_t kMaxHostId = (1 << kHostIdBits) - 1;

ballot_id_t next_ballot_id(ballot_id_t old, host_id_t host_id);

}  // namespace pd
