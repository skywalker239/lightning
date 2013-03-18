// vim: set tabstop=4 expandtab:
#include <pd/zk_vars/var_handle.H>

namespace pd {

var_handle_t::var_handle_t(const string_t& key)
    : key_(key),
      valid_(false),
      value_(string_t::empty),
      version_(-1),
      cond_(),
      ref_count_(1)
{}

var_handle_t::~var_handle_t() {
    assert(ref_count_ == 0);
}

const string_t& var_handle_t::key() const {
    return key_;
}

void var_handle_t::reset(const string_t& value, int version) {
    bq_cond_guard_t guard(cond_);
    valid_ = true;
    value_ = value;
    version_ = version;
    cond_.send(true);
}

void var_handle_t::clear() {
    bq_cond_guard_t guard(cond_);
    valid_ = false;
    value_ = string_t::empty;
    version_ = -1;
    // do not wake waiters
}

bool var_handle_t::valid() const {
    bq_cond_guard_t guard(cond_);
    return valid_;
}

const string_t& var_handle_t::value() const {
    bq_cond_guard_t guard(cond_);
    return value_;
}

int var_handle_t::version() const {
    bq_cond_guard_t guard(cond_);
    return version_;
}

void var_handle_t::add_ref() {
    bq_cond_guard_t guard(cond_);
    ++ref_count_;
}

bool var_handle_t::remove_ref() {
    bq_cond_guard_t guard(cond_);
    return (--ref_count_ == 0);
}

}  // namespace pd
