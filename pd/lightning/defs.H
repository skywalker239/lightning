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

enum ring_cmd_fields_t {
    RING_CMD_TYPE_FIELD = 0x30,
    RING_ID_FIELD,
    RING_SENDER_HOST_ID_FIELD,
    START_INSTANCE_ID_FIELD,
    END_INSTANCE_ID_FIELD,
    REQUEST_ID_FIELD,
    BALLOT_ID_FIELD,
    RESERVED_INSTANCES_FIELD
};

enum ring_cmd_type_t {
    PHASE1_BATCH = 0x30,
    PHASE1,
    PHASE2
};

}  // namespace pd
