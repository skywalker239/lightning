// Copyright (C) 2012, Alexander Kharitonov <alexander.kharitonov@gmail.com>
// Copyright (C) 2012, YANDEX LLC.
// This code may be distributed under the terms of the GNU GPL v3.
// See ‘http://www.gnu.org/licenses/gpl.html’.
// vim: set tabstop=4 expandtab:
#pragma once

#include <stdint.h>

namespace pd {

typedef uint64_t iid_t;
typedef int32_t replica_id_t;
typedef uint64_t term_t;

const replica_id_t INVALID_REPLICA_ID = 0;
const iid_t INVALID_IID = ~0ULL;

typedef uint64_t instance_id_t;
typedef uint32_t ballot_id_t;
typedef uint64_t request_id_t;
typedef uint64_t value_id_t;

const instance_id_t INVALID_INSTANCE_ID = ~0ULL;
const ballot_id_t INVALID_BALLOT_ID = 0U;
const request_id_t INVALID_REQUEST_ID = 0;
const value_id_t  INVALID_VALUE_ID  = 0ULL;

typedef uint32_t host_id_t;
typedef uint32_t ring_id_t;

const ring_id_t INVALID_RING_ID = 0;
const host_id_t INVALID_HOST_ID = ~0U;
const int HOST_ID_BITS = 12;
const host_id_t MAX_HOST_ID = (1 << HOST_ID_BITS) - 1;

ballot_id_t next_ballot_id(ballot_id_t old, host_id_t host_id);

}  // namespace pd
