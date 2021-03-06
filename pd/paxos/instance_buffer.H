// Copyright (C) 2012, Alexander Kharitonov <alexander.kharitonov@gmail.com>
// Copyright (C) 2012, YANDEX LLC.
// This code may be distributed under the terms of the GNU GPL v3.
// See ‘http://www.gnu.org/licenses/gpl.html’.
// vim: set tabstop=4 expandtab:
#pragma once

#include "commit_tracker.H"
#include "instance.H"
#include <pd/base/ref.H>
#include <pd/base/thr.H>
#include <vector>

namespace pd {

class instance_buffer_t {
public:
    instance_buffer_t(const commit_tracker_t& commit_tracker,
                      const size_t buffer_size);

    enum result_t {
        too_old = 0,
        too_new,
        ok,
        failed
    };

    result_t set(uint64_t instance_id,
                 const string_t& value_id,
                 const string_t& value);

    result_t commit(uint64_t instance_id,
                    const string_t& value_id);

    result_t begin(uint64_t instance_id,
                   const string_t& value_id,
                   const string_t& value);

    result_t committed(uint64_t instance_id);

    result_t get(uint64_t instance_id,
                 ref_t<instance_t>* instance) const;
private:
    result_t lookup(uint64_t instance_id,
                    instance_t** instance);

    const commit_tracker_t& commit_tracker_;

    const size_t max_size_;
    size_t begin_;

    std::vector<ref_t<instance_t> > instances_;

    mutable thr::spinlock_t lock_;
};

}  // namespace pd
