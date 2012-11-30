// Copyright (C) 2012, Alexander Kharitonov <alexander.kharitonov@gmail.com>
// Copyright (C) 2012, YANDEX LLC.
// This code may be distributed under the terms of the GNU GPL v3.
// See ‘http://www.gnu.org/licenses/gpl.html’.
// vim: set tabstop=4 expandtab:
#pragma once

#include <pd/base/string.H>
#include <pd/lightning/defs.H>
#include <pd/lightning/pi_ext.H>

namespace pd {

//! Represents a Paxos value.
//  A value is simply a string with a unique identifier.
class value_t {
public:
    //! Constructs an empty (invalid) value.
    value_t();

    //! Sets the value to (value_id, value).
    void set(value_id_t value_id, const string_t& value);

    //! Sets the value from a serialized value.
    //  Returns false iff the serialized value was malformed.
    bool set(const pi_t& pi_value);

    //! Returns whether this value is not empty.
    bool valid() const;

    //! Value id.
    value_id_t value_id() const;

    //! Value data.
    const string_t value() const;

    //! Serialized value.
    const pi_t& pi_value() const;
private:
    ref_t<pi_ext_t> value_;
};

}  // namespace pd
