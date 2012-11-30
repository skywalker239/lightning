// Copyright (C) 2012, Alexander Kharitonov <alexander.kharitonov@gmail.com>
// Copyright (C) 2012, YANDEX LLC.
// This code may be distributed under the terms of the GNU GPL v3.
// See ‘http://www.gnu.org/licenses/gpl.html’.
// vim: set tabstop=4 expandtab:
#include "instance.H"
#include <pd/base/assert.H>

namespace pd {

const uint64_t instance_t::invalid_instance_id;

instance_t::instance_t()
    : instance_id_(invalid_instance_id),
      committed_(false)
 {}

void instance_t::set_value(const string_t& value_id,
                           const string_t& value)
{
    assert(instance_id_ != invalid_instance_id);
    assert(!committed_);
    value_id_ = value_id;
    value_    = value;
}

bool instance_t::commit(const string_t& value_id) {
    assert(instance_id_ != invalid_instance_id);
    if(!string_t::cmp_eq<ident_t>(value_id_, value_id)) {
        assert(!committed_);
        return false;
    }
    committed_ = true;
    return true;
}

bool instance_t::committed() const {
    assert(instance_id_ != invalid_instance_id);
    return committed_;
}

void instance_t::reset(uint64_t instance_id) {
    instance_id_ = instance_id;
    value_id_ = string_t::empty;
    value_ = string_t::empty;
    committed_ = false;
}

uint64_t instance_t::instance_id() const {
    return instance_id_;
}

}  // namespace pd
