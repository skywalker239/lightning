// Copyright (C) 2012, Korotkiy Fedor <prime@yandex-team.ru>
// Copyright (C) 2012, YANDEX LLC.
// This code may be distributed under the terms of the GNU GPL v3.
// See ‘http://www.gnu.org/licenses/gpl.html’.
// vim: set tabstop=4 expandtab:
#include <pd/lightning/pi_ring_cmd.H>
#include <pd/base/op.H>
#include <pd/pi/pi_pro.H>

namespace pd {

namespace cmd {

namespace ring {

bool is_header_valid(const ref_t<pi_ext_t>& ring_cmd) {
    const pi_t& header = ring_cmd->pi().s_ind(1);

    if (header.type() != pi_t::_array ||
        header.__array()._count() != 3) {
        return false;
    }

    return true;
}

bool is_valid(const ref_t<pi_ext_t>& ring_cmd) {
    if(ring_cmd->pi().type() != pi_t::_array) {
        return false;
    }

    if(!is_header_valid(ring_cmd)) {
        return false;
    }

    switch(type(ring_cmd)) {
      case type_t::BATCH:
        return batch::is_body_valid(ring_cmd);
      case type_t::PROMISE:
        // TODO(prime@) write validation code
        return false;
      case type_t::VOTE:
        return vote::is_body_valid(ring_cmd);
      default:
        return false;
    }
}

ref_t<pi_ext_t> build(type_t cmd_type,
                      const header_t& header,
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

} // namespace ring

namespace batch {

using namespace pd::cmd::ring;

bool is_body_valid(const ref_t<pi_ext_t>& ring_cmd) {
    const pi_t& body = ring_cmd->pi().s_ind(2);

    if(body.type() != pi_t::_array ||
       body.__array()._count() != 4) {
        return false;
    }

    if(body.s_ind(3).type() != pi_t::_array) {
        return false;
    }

    auto fail = pi_t::array_t::c_ptr_t(body.s_ind(3).__array());
    for(; fail; ++fail) {
        if(fail->type() != pi_t::_array ||
           fail->__array()._count() != 2 ||
           start_iid(ring_cmd) > fail_iid(*fail) ||
           end_iid(ring_cmd) <= fail_iid(*fail))
        {
            return false;
        }
    }

    return true;
}


std::vector<fail_t> fails_pi_to_vector(const pi_t::array_t& fails) {
    std::vector<fail_t> instances;
    instances.reserve(fails._count());

    for(size_t i = 0; i < fails._count(); ++i) {
        instances.push_back({
            fail_iid(fails[i]),
            fail_highest_promise(fails[i]),
        });
    }

    return instances;
}

std::vector<fail_t> merge_fails(const std::vector<fail_t>& local,
                                const std::vector<fail_t>& received) {
    std::vector<fail_t> merged;
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
                    received_instance->highest_promised)
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

ref_t<pi_ext_t> build(const ring::header_t& header, const body_t& body) {
    pi_t::pro_t fails_pros[body.fails.size()];
    pi_t::pro_t::array_t fails_arrays[body.fails.size()];
    pi_t::pro_t fails_fields[body.fails.size()][2];

    static_assert((&(fails_fields[0][2]) - &(fails_fields[0][0])) != 2 * sizeof(pi_t::pro_t),
                  "prime@ was wrong about 2D array memory layout");

    for(size_t i = 0; i < body.fails.size(); ++i) {
        fails_fields[i][0] = pi_t::pro_t::uint_t(body.fails[i].iid);
        fails_fields[i][1] = pi_t::pro_t::uint_t(body.fails[i].highest_promised);

        fails_arrays[i] = { 2, fails_fields[i] };
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

    return ring::build(ring::type_t::BATCH, header, pi_body);
}

} // namespace batch

namespace promise {

ref_t<pi_ext_t> build(const ring::header_t& ,
                      const body_t& ) {
    // TODO(prime@):
    return NULL;
}

} // namespace promise

namespace vote {

ref_t<pi_ext_t> build(const ring::header_t& header, const body_t& body) {
    pi_t::pro_t body_items[3] = {
        pi_t::pro_t::uint_t(body.iid),
        pi_t::pro_t::uint_t(body.ballot_id),
        pi_t::pro_t::uint_t(body.value_id)
    };

    pi_t::pro_t::array_t body_array = { 3, body_items };
    pi_t::pro_t pi_body(body_array);

    return ring::build(ring::type_t::VOTE, header, pi_body);
}

bool is_body_valid(const ref_t<pi_ext_t>& cmd) {
    const pi_t& body = cmd->pi().s_ind(2);

    if(body.type() != pi_t::_array ||
       body.__array()._count() != 3) {
        return false;
    }

    return true;
}

} // namespace vote

} // namespace cmd

} // namespace pd
