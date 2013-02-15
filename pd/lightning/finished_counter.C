// Copyright (C) 2012, Korotkiy Fedor <prime@yandex-team.ru>
// Copyright (C) 2012, YANDEX LLC.
// This code may be distributed under the terms of the GNU GPL v3.
// See ‘http://www.gnu.org/licenses/gpl.html’.
// vim: set tabstop=4 expandtab:
#include "finished_counter.H"

#include <pd/base/assert.H>

namespace pd {

void finished_counter_t::started(int n) {
    assert(n >= 0);

    bq_cond_guard_t guard(all_finished_);
    count_ += n;
}

void finished_counter_t::finish() {
    bq_cond_guard_t guard(all_finished_);

    assert(count_ > 0);

    --count_;
    if(count_ == 0) {
        all_finished_.send();
    }
}

void finished_counter_t::wait_for_all_to_finish() {
    bq_cond_guard_t guard(all_finished_);

    while(count_ != 0) {
        if(!bq_success(all_finished_.wait(NULL))) {
            throw exception_sys_t(log::error,
                                  errno,
                                  "finished_counter_t::wait_all_finished: %m");
        }
    }
}

}
