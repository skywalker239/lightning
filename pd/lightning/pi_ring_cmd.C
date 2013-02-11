// Copyright (C) 2012, Korotkiy Fedor <prime@yandex-team.ru>
// Copyright (C) 2012, YANDEX LLC.
// This code may be distributed under the terms of the GNU GPL v3.
// See ‘http://www.gnu.org/licenses/gpl.html’.
// vim: set tabstop=4 expandtab:
#include <pd/lightning/pi_ring_cmd.H>
#include <pd/base/op.H>
#include <pd/pi/pi_pro.H>

namespace pd {

bool is_ring_cmd_header_valid(const ref_t<pi_ext_t>& ring_cmd) {
    const pi_t& header = ring_cmd->pi().s_ind(1);

    if (header.type() != pi_t::_array ||
        header.__array()._count() != 3) {
        return false;
    }

    // TODO(prime@): check element types
    return true;
}

bool is_ring_cmd_batch_valid(const ref_t<pi_ext_t>& ring_cmd) {
    const pi_t& batch_data = ring_cmd->pi().s_ind(2);

    if(batch_data.type() != pi_t::_array ||
       batch_data.__array()._count() != 4) {
        return false;
    }

    if(batch_data.s_ind(3).type() != pi_t::_array) {
        return false;
    }

    // TODO(prime@) check failed instances
    return true;
}

bool is_ring_cmd_valid(const ref_t<pi_ext_t>& ring_cmd) {
    if(ring_cmd->pi().type() != pi_t::_array) {
        return false;
    }

    if(!is_ring_cmd_header_valid(ring_cmd)) {
        return false;
    }

    switch(ring_cmd_type(ring_cmd)) {
      case ring_cmd_type_t::PHASE1_BATCH:
        return is_ring_cmd_batch_valid(ring_cmd);
      case ring_cmd_type_t::PHASE1:
        // TODO(prime@) write validation code
        return false;
      case ring_cmd_type_t::PHASE2:
        // TODO(prime@) write validation code
        return false;
      default:
        return false;
    }
}


std::vector<batch_fail_t> fails_pi_to_vector(
        const pi_t::array_t& fails) {
    std::vector<batch_fail_t> instances;
    instances.reserve(fails._count());

    for(size_t i = 0; i < fails._count(); ++i) {
        instances.push_back({
            fail_iid(fails[i]),
            fail_highest_promise(fails[i]),
            fail_status(fails[i])
        });
    }

    return instances;
}

instance_status_t merge_status(instance_status_t local,
                               instance_status_t received) {
    if(local == instance_status_t::OPEN && received == instance_status_t::OPEN) {
        return instance_status_t::OPEN;
    } else if(local == instance_status_t::IID_TOO_LOW ||
              received == instance_status_t::IID_TOO_LOW) {
        return instance_status_t::IID_TOO_LOW;
    } else if(local == instance_status_t::IID_TOO_HIGH ||
              received == instance_status_t::IID_TOO_HIGH) {
        return instance_status_t::IID_TOO_HIGH;
    } else if(local == instance_status_t::RESERVED ||
              received == instance_status_t::RESERVED) {
        return instance_status_t::RESERVED;
    } else {
        return instance_status_t::LOW_BALLOT_ID;
    }
}

std::vector<batch_fail_t> merge_fails(
        const std::vector<batch_fail_t>& local,
        const std::vector<batch_fail_t>& received) {
    std::vector<batch_fail_t> merged;
    merged.reserve(local.size() + received.size());

    auto local_instance = local.begin();
    auto received_instance = received.begin();

    while(local_instance != local.end() &&
          received_instance != received.end()) {
        if(local_instance->iid < received_instance->iid) {
            merged.push_back(*local_instance);
            ++local_instance;
        } else if(local_instance->iid > received_instance->iid) {
            merged.push_back(*received_instance);
            ++received_instance;
        } else {
            merged.push_back({
                local_instance->iid,
                max(local_instance->highest_promised,
                    received_instance->highest_promised),
                merge_status(local_instance->status,
                             received_instance->status)
            });
            ++local_instance;
            ++received_instance;
        }
    }

    while(local_instance != local.end()) {
        merged.push_back(*local_instance);
        ++local_instance;
    }

    while(received_instance != received.end()) {
        merged.push_back(*received_instance);
        ++received_instance;
    }

    return merged;
}

inline ref_t<pi_ext_t> build_ring_cmd(
        ring_cmd_type_t cmd_type,
        const ring_cmd_header_t& header,
        pi_t::pro_t& pi_body) {
    pi_t::pro_t header_items[3] = {
        pi_t::pro_t::uint_t(header.request_id),
        pi_t::pro_t::uint_t(header.ring_id),
        pi_t::pro_t::uint_t(header.dst_host_id)
    };

    pi_t::pro_t::array_t pi_header_array = { 3, header_items };
    pi_t::pro_t pi_header(pi_header_array);

    pi_t::pro_t cmd_items[3] = {
        pi_t::pro_t::uint_t(static_cast<uint8_t>(cmd_type)),
        pi_header,
        pi_body
    };

    pi_t::pro_t::array_t cmd_array = { 3, cmd_items };
    pi_t::pro_t cmd(cmd_array);

    return pi_ext_t::__build(cmd);
}

ref_t<pi_ext_t> build_ring_batch_cmd(
        const ring_cmd_header_t& header,
        const ring_batch_cmd_body_t& body) {
    pi_t::pro_t fails_pros[body.fails.size()];
    pi_t::pro_t::array_t fails_arrays[body.fails.size()];
    pi_t::pro_t fails_fields[body.fails.size()][3];

    static_assert((&(fails_fields[0][3]) - &(fails_fields[0][0])) != 3 * sizeof(pi_t::pro_t),
                  "prime@ was wrong about 2D array memory layout");

    for(size_t i = 0; i < body.fails.size(); ++i) {
        fails_fields[i][0] = pi_t::pro_t::uint_t(body.fails[i].iid);
        fails_fields[i][1] = pi_t::pro_t::uint_t(body.fails[i].highest_promised);
        fails_fields[i][2] = pi_t::pro_t::uint_t(
            static_cast<uint8_t>(body.fails[i].status)
        );

        fails_arrays[i] = { 3, fails_fields[i] };
        fails_pros[i] = fails_arrays[i];
    }


    pi_t::pro_t::array_t fails(
        body.fails.size(),
        fails_pros
    );

    pi_t::pro_t body_items[4] = {
        pi_t::pro_t::uint_t(body.start_iid),
        pi_t::pro_t::uint_t(body.end_iid),
        pi_t::pro_t::uint_t(body.ballot_id),
        fails
    };

    pi_t::pro_t::array_t pi_body_array = { 4, body_items };
    pi_t::pro_t pi_body(pi_body_array);

    return build_ring_cmd(ring_cmd_type_t::PHASE1_BATCH, header, pi_body);
}

}
