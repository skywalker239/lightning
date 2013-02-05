// Copyright (C) 2012, Korotkiy Fedor <prime@yandex-team.ru>
// Copyright (C) 2012, YANDEX LLC.
// This code may be distributed under the terms of the GNU GPL v3.
// See ‘http://www.gnu.org/licenses/gpl.html’.
// vim: set tabstop=4 expandtab:
#pragma once

#include <pd/bq/bq_job.H>
#include <pd/bq/bq_cond.H>
#include <pd/base/exception.H>

namespace pd {

template<class x_t, class ...args_t>
class work_pool_t {
public:
    work_pool_t() : jobs_active_(0) {};

    void start(x_t& x, args_t&& ...args) {
        bq_job_t<typeof(&work_pool_t::run)>::create(
            STRING("work_pool_t"),
            bq_thr_get(),
            x,
            args
        );
    }

    void wait_all_stop() {
        pd::bq_cond_guard_t guard(job_num_change_);

        while(jobs_active_ != 0) {
            if(!bq_success(job_num_change_.wait(NULL))) {
                throw exception_sys_t(log::error,
                                      errno,
                                      "work_pool_t::wait_all_stop: %m");
            }
        }
    }

private:
    pd::bq_cond_t job_num_change_;
    int jobs_active_;

    void run(f_t& f) {
        {
            pd::bq_cond_guard_t guard(job_num_change_);
            ++jobs_active_;
        }

        f();

        {
            pd::bq_cond_guard_t guard(job_num_change_);
            --jobs_active_;
            if(jobs_active_ == 0) {
                job_num_change_.send();
            }
        }
    }
};

} // namespace pd
