// Copyright (C) 2012, Alexander Kharitonov <alexander.kharitonov@gmail.com>
// Copyright (C) 2012, YANDEX LLC.
// This code may be distributed under the terms of the GNU GPL v3.
// See ‘http://www.gnu.org/licenses/gpl.html’.
// vim: set tabstop=4 expandtab:
#pragma once

#include <phantom/io_zcluster_status/host_status.H>

#pragma GCC visibility push(default)

namespace phantom {

class status_listener_t {
public:
    virtual ~status_listener_t() {}

    virtual void notify(ref_t<host_status_list_t> cluster_status) = 0;
private:
    status_listener_t* next_;
    status_listener_t** me_;

    friend class io_zcluster_status_t;
};

}  // namespace phantom

#pragma GCC visibility pop
