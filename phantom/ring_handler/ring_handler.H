#pragma once
// Copyright (C) 2012, Korotkiy Fedor <prime@yandex-team.ru>
// Copyright (C) 2012, YANDEX LLC.
// This code may be distributed under the terms of the GNU GPL v3.
// See ‘http://www.gnu.org/licenses/gpl.html’.
// vim: set tabstop=4 expandtab:
#include <pd/lightning/pi_ext.H>

#pragma GCC visibility push(default)

namespace phantom {

namespace ring_handler_proto {

class ring_handler_t {
public:
    virtual void handle_ring_cmd(const ref_t<pi_ext_t>& ring_cmd) = 0;
};

} // namespace ring_handler_proto

using ring_handler_proto::ring_handler_t;

} // namespace phantom

#pragma GCC visibility pop
