// Copyright (C) 2012, Alexander Kharitonov <alexander.kharitonov@gmail.com>
// Copyright (C) 2012, YANDEX LLC.
// This code may be distributed under the terms of the GNU GPL v3.
// See ‘http://www.gnu.org/licenses/gpl.html’.
// vim: set tabstop=4 expandtab:
#pragma once

#include <phantom/pd.H>
#include <pd/base/netaddr.H>
#include <pd/lightning/pi_ext.H>

#pragma GCC visibility push(default)

namespace phantom {
namespace io_blob_receiver {

class handler_t {
public:
    virtual void handle(ref_t<pi_ext_t> blob,
                        const netaddr_t& remote_addr) = 0;
};

}}  // namespace phantom::io_blob_receiver

#pragma GCC visibility pop
