// Copyright (C) 2012, Alexander Kharitonov <alexander.kharitonov@gmail.com>
// Copyright (C) 2012, YANDEX LLC.
// This code may be distributed under the terms of the GNU GPL v3.
// See ‘http://www.gnu.org/licenses/gpl.html’.
// vim: set tabstop=4 expandtab:
#pragma once

#include <pd/zk_vars/var_base.H>
#include <pd/base/exception.H>
#include <pd/base/log.H>
#include <pd/base/string.H>

namespace pd {

template<typename T>
class simple_var_t : public var_base_t {
public:
    simple_var_t(const string_t& key, var_store_t& store)
        : var_base_t(key, store),
          value_()
    {}

    simple_var_t(const simple_var_t& other)
        : var_base_t(other),
          value_(other.value_)
    {}

    simple_var_t& operator=(const simple_var_t& other) {
        var_base_t::operator=(other);
        value_ = other.value_;
    }

    ~simple_var_t()
    {}

    bool set(const T& value, int version) {
        string_t::ctor_t string(1024);
        string.print(value);
        return do_set(string, version);
    }

    const T& value() const {
        return value_;
    }
private:
    virtual void update_impl() {
        in_t::ptr_t p(value_string_);
        if(!p.parse(value_)) {
            MKCSTR(value_z, value_string_);
            throw exception_log_t(log::error | log::trace, "could not parse value '%s'", value_z);
        }
    }

    T value_;
};

}  // namespace pd
