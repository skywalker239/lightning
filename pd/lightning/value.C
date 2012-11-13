// vim: set tabstop=4 expandtab:
#include <pd/lightning/value.H>
#include <pd/base/string.H>
#include <pd/pi/pi_pro.H>

namespace pd {

value_t::value_t()
{}

void value_t::set(value_id_t value_id, const string_t& value) {
    pi_t::pro_t::item_t id_item(pi_t::pro_t::uint_t(value_id), NULL);
    const str_t& value_str = value.str();
    pi_t::pro_t::item_t value_item(value_str, &id_item);
    pi_t::pro_t pro(&value_item);
    value_ = pi_ext_t::__build(pro);
}

bool value_t::set(const pi_t& pi_value) {
    const value_id_t value_id = pi_value.s_ind(0).s_uint();
    const str_t& value_str = pi_value.s_ind(1).s_str();

    if(value_id == kInvalidValueId || value_str.size() == 0) {
        return false;
    }

    pi_t::pro_t::item_t id_item(pi_t::pro_t::uint_t(value_id), NULL);
    pi_t::pro_t::item_t value_item(value_str, &id_item);
    pi_t::pro_t pro(&value_item);
    value_ = pi_ext_t::__build(pro);
    return true;
}

bool value_t::valid() const {
    return value_ != NULL;
}

value_id_t value_t::value_id() const {
    return value_ ?
               value_->pi().s_ind(0).s_uint() :
               kInvalidValueId;
}

const string_t value_t::value() const {
    if(value_) {
        const str_t& value_str = value_->pi().s_ind(1).s_str();
        return string_t::ctor_t(value_str.size())(value_str);
    } else {
        return string_t::empty;
    }
}

const pi_t& value_t::pi_value() const {
    return value_->pi();
}

}  // namespace pd
