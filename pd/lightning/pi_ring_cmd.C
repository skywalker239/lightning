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

    switch(ring_cmd->pi().s_ind(0).s_enum()) {
      case PHASE1_BATCH:
        return is_ring_cmd_batch_valid(ring_cmd);
      case PHASE1:
        // TODO(prime@) write validation code
        return false;
      case PHASE2:
        // TODO(prime@) write validation code
        return false;
      default:
        return false;
    }
}


std::vector<failed_instance_t> failed_instances_pi_to_vector(
        const pi_t::array_t& failed_instances) {
    std::vector<failed_instance_t> instances;
    instances.reserve(failed_instances._count());

    for(size_t i = 0; i < failed_instances._count(); ++i) {
        instances.push_back({
            failed_instance_iid(failed_instances[i]),
            failed_instance_highest_promise(failed_instances[i]),
            failed_instance_status(failed_instances[i])
        });
    }

    return instances;
}

failed_instance_status_t merge_status(failed_instance_status_t local,
                                      failed_instance_status_t received) {
    if(local == IID_TO_LOW || received == IID_TO_LOW) {
        return IID_TO_LOW;
    } else if(local == RESERVED || received == RESERVED) {
        return RESERVED;
    } else {
        return LOW_BALLOT_ID;
    }
}

std::vector<failed_instance_t> merge_failed_instances(
        const std::vector<failed_instance_t>& local,
        const std::vector<failed_instance_t>& received) {
    std::vector<failed_instance_t> merged;
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
                max(local_instance->highest_promise,
                    received_instance->highest_promise),
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
        pi_t::pro_t::enum_t(cmd_type),
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
    pi_t::pro_t failed_instances_pros[body.failed_instances.size()];
    pi_t::pro_t::array_t failed_instances_arrays[body.failed_instances.size()];
    pi_t::pro_t failed_instances_fields[body.failed_instances.size()][3];

    static_assert((&(failed_instances_fields[0][3]) -
                   &(failed_instances_fields[0][0])) != 3 * sizeof(pi_t::pro_t),
                  "prime@ was wrong about 2D array memory layout");

    for(size_t i = 0; i < body.failed_instances.size(); ++i) {
        failed_instances_fields[i][0] =
            pi_t::pro_t::uint_t(body.failed_instances[i].iid);
        failed_instances_fields[i][1] =
            pi_t::pro_t::uint_t(body.failed_instances[i].highest_promise);
        failed_instances_fields[i][2] =
            pi_t::pro_t::enum_t(body.failed_instances[i].status);

        failed_instances_arrays[i] = { 3, failed_instances_fields[i] };
        failed_instances_pros[i] = failed_instances_arrays[i];
    }


    pi_t::pro_t::array_t failed_instances(
        body.failed_instances.size(),
        failed_instances_pros
    );

    pi_t::pro_t body_items[4] = {
        pi_t::pro_t::uint_t(body.start_instance_id),
        pi_t::pro_t::uint_t(body.end_instance_id),
        pi_t::pro_t::uint_t(body.ballot_id),
        failed_instances
    };

    pi_t::pro_t::array_t pi_body_array = { 4, body_items };
    pi_t::pro_t pi_body(pi_body_array);

    return build_ring_cmd(PHASE1_BATCH, header, pi_body);
}

}
