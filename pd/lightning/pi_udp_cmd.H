#pragma once
// Copyright (C) 2012, Korotkiy Fedor <prime@yandex-team.ru>
// Copyright (C) 2012, YANDEX LLC.
// This code may be distributed under the terms of the GNU GPL v3.
// See ‘http://www.gnu.org/licenses/gpl.html’.
// vim: set tabstop=4 expandtab:
#include <pd/lightning/pi_ext.H>
#include <pd/lightning/defs.H>
#include <pd/lightning/value.H>

namespace pd {

namespace cmd {

namespace udp {
    // NOTE: enum send over network, do not change existing values
    enum class type_t {
        PROPOSE = 1,
        COMMIT = 2
    };

    bool is_valid(const ref_t<pi_ext_t>& cmd);
    type_t type(const ref_t<pi_ext_t>& cmd);
} // namespace udp

namespace propose {
    request_id_t request_id(const ref_t<pi_ext_t>& cmd);
    instance_id_t iid(const ref_t<pi_ext_t>& cmd);
    ballot_id_t ballot_id(const ref_t<pi_ext_t>& cmd);

    value_id_t value_id(const ref_t<pi_ext_t>& cmd);
    const pi_t::string_t& value_str(const ref_t<pi_ext_t>& cmd);

    value_t value(const ref_t<pi_ext_t>& cmd);

    ref_t<pi_ext_t> build(request_id_t request_id,
                          instance_id_t iid,
                          ballot_id_t ballot_id,
                          const value_t& value);
} // namespace propose

namespace commit {
    instance_id_t iid(const ref_t<pi_ext_t>& cmd);
    value_id_t value_id(const ref_t<pi_ext_t>& cmd);

    ref_t<pi_ext_t> build(instance_id_t iid, value_id_t value_id);
} // namespace commit

} // namespace cmd

} // namespace pd
