// Copyright (C) 2012, Alexander Kharitonov <alexander.kharitonov@gmail.com>
// Copyright (C) 2012, Alexey Pervushin <billyevans@yandex-team.ru>
// Copyright (C) 2012, YANDEX LLC.
// This code may be distributed under the terms of the GNU GPL v3.
// See ‘http://www.gnu.org/licenses/gpl.html’.
// vim: set tabstop=4 expandtab:
#pragma once

#include <pd/base/thr.H>
#include <pd/lightning/defs.H>

namespace pd {

class guid_generator_t {
public:
    guid_generator_t(host_id_t host_id) throw();
    ~guid_generator_t() throw() { };

    uint64_t get_guid();
private:
    thr::spinlock_t last_musec_lock_;
    uint64_t last_musec_;
    host_id_t host_id_;

    guid_generator_t(const guid_generator_t&) = delete;
    guid_generator_t& operator=(const guid_generator_t&) = delete;
};

}
