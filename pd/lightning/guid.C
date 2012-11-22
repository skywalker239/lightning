// Copyright (C) 2012, Alexander Kharitonov <alexander.kharitonov@gmail.com>
// Copyright (C) 2012, Alexey Pervushin <billyevans@yandex-team.ru>
// Copyright (C) 2012, YANDEX LLC.
// This code may be distributed under the terms of the GNU GPL v3.
// See ‘http://www.gnu.org/licenses/gpl.html’.
// vim: set tabstop=4 expandtab:
#include "guid.H"

#include <pd/base/time.H>

namespace pd {

guid_generator_t::guid_generator_t(uint32_t host_id) throw()
    : last_musec_(0), host_id_(host_id)
{ }

uint64_t guid_generator_t::get_guid()
{
    uint64_t musec = (timeval_current() - timeval_unix_origin) / interval_microsecond;

    {
        thr::spinlock_guard_t guard(last_musec_lock_);
        if(musec > last_musec_) {
            last_musec_ = musec;
        } else {
            musec = ++last_musec_;
        }
    }

    return (musec << 12) + host_id_;
}

}
