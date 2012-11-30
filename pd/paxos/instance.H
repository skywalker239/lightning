// Copyright (C) 2012, Alexander Kharitonov <alexander.kharitonov@gmail.com>
// Copyright (C) 2012, YANDEX LLC.
// This code may be distributed under the terms of the GNU GPL v3.
// See ‘http://www.gnu.org/licenses/gpl.html’.
// vim: set tabstop=4 expandtab:
#pragma once

#include <pd/base/ref.H>
#include <pd/base/string.H>
#include <stdint.h>

namespace pd {

class instance_t : public ref_count_atomic_t {
public:
    instance_t();

    void set_value(const string_t& value_id,
                   const string_t& value);

    bool commit(const string_t& value_id);

    bool committed() const;

    const string_t& value() const;

    void reset(uint64_t instance_id);

    uint64_t instance_id() const;

    static const uint64_t invalid_instance_id = ~0ULL;
private:
    uint64_t instance_id_;
    string_t value_id_;
    string_t value_;
    bool committed_;

    friend class ref_t<instance_t>;
};

}  // namespace pd
