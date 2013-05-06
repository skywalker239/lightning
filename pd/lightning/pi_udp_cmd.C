// Copyright (C) 2013, Korotkiy Fedor <prime@yandex-team.ru>
// Copyright (C) 2013, YANDEX LLC.
// This code may be distributed under the terms of the GNU GPL v3.
// See ‘http://www.gnu.org/licenses/gpl.html’.
// vim: set tabstop=4 expandtab:

#include <pd/pi/pi_pro.H>
#include <pd/lightning/pi_udp_cmd.H>

namespace pd { namespace cmd {

namespace udp {

bool is_valid(const ref_t<pi_ext_t>&) {
    // TODO(prime@):
    return true;
}

} // namespace udp

namespace propose {

ref_t<pi_ext_t> build(request_id_t request_id,
                      instance_id_t iid,
                      ballot_id_t ballot_id,
                      const value_t& value) {
    using namespace pd::pi_build;

    return ref_t<pi_ext_t>(pi_ext_t::__build(
        arr_t{
            int_t((int)udp::type_t::PROPOSE),
            int_t(request_id),
            int_t(iid),
            int_t(ballot_id),
            value.pi_value()
        }
    ));
}

} // namespace propose

namespace commit {

ref_t<pi_ext_t> build(instance_id_t iid, value_id_t value_id) {
    using namespace pd::pi_build;

    return ref_t<pi_ext_t>(pi_ext_t::__build(
        arr_t{
            int_t((int)udp::type_t::COMMIT),
            int_t(iid),
            int_t(value_id)
        }
    ));
}

} // namespace commit

}} // namespace pd::cmd
