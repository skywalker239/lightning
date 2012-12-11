// Copyright (C) 2012, Fedor Korotkiy <prime@yandex-team.ru>
// Copyright (C) 2012, YANDEX LLC.
// This code may be distributed under the terms of the GNU GPL v3.
// See ‘http://www.gnu.org/licenses/gpl.html’.
// vim: set tabstop=4 expandtab:
#pragma once

#include <pd/pi/pi.H>

namespace pd {

template<class key_enum_t, class value_enum_t>
inline bool get_enum_field(const pi_t::map_t& map,
                           key_enum_t key,
                           value_enum_t *field) {
    pi_t const* field_raw = map.lookup(pi_t(pi_t::_enum, key));

    if (field_raw->type() != pi_t::_enum) {
        return false;
    }

    *field = field_raw->__<value_enum_t>();
}

template<class key_enum_t, class int_t>
inline bool get_int_field(const pi_t::map_t& map,
                          key_enum_t key,
                          int_t *field) {
    pi_t const* field_raw = map.lookup(pi_t(pi_t::_enum, key));

    if (field_raw->type() != pi_t::_uint64) {
        return false;
    }

    *field = field_raw->__uint64();
}

}
