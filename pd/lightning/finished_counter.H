// Copyright (C) 2012, Korotkiy Fedor <prime@yandex-team.ru>
// Copyright (C) 2012, YANDEX LLC.
// This code may be distributed under the terms of the GNU GPL v3.
// See ‘http://www.gnu.org/licenses/gpl.html’.
// vim: set tabstop=4 expandtab:
#pragma once

#include <pd/base/exception.H>
#include <pd/bq/bq_cond.H>

namespace pd {

class finished_counter_t {
public:
    finished_counter_t() : count_(0) {}

    void start(int n);

    void finish();

    void wait_for_all_to_finish();

private:
    int count_;
    bq_cond_t all_finished_;
};

} // namespace pd
