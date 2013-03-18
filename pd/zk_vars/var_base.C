// Copyright (C) 2012, Alexander Kharitonov <alexander.kharitonov@gmail.com>
// Copyright (C) 2012, YANDEX LLC.
// This code may be distributed under the terms of the GNU GPL v3.
// See ‘http://www.gnu.org/licenses/gpl.html’.
// vim: set tabstop=4 expandtab:
#include <pd/zk_vars/var_base.H>
#include <pd/zk_vars/var_store.H>
#include <pd/zk_vars/var_handle.H>

#include <pd/base/exception.H>

namespace pd {

var_base_t::var_base_t(const string_t& key, var_store_t& store)
    : version_(kAnyVersion),
      value_string_(string_t::empty),
      key_(key),
      store_(&store),
      handle_(store_->add_var_ref(key_))
{}

var_base_t::var_base_t(const var_base_t& other)
    : version_(other.version_),
      value_string_(other.value_string_),
      key_(other.key_),
      store_(other.store_),
      handle_(store_->add_var_ref(key_))
{}

var_base_t& var_base_t::operator=(const var_base_t& other) {
    handle_ = NULL;
    store_->remove_var_ref(key_);
    store_ = NULL;

    version_ = other.version_;
    value_string_ = other.value_string_;
    key_ = other.key_;
    store_ = other.store_;
    handle_ = store_->add_var_ref(key_);
    return *this;
}

var_base_t::~var_base_t() {
    handle_ = NULL;
    store_->remove_var_ref(key_);
}

int var_base_t::update() {
    bq_cond_guard_t guard(handle_->cond_);
    update_internal();
    guard.relax();
    
    log_info("key '%*s' updated to '%*s' at version %d",
             (int) key_.size(),
             key_.ptr(),
             (int) value_string_.size(),
             value_string_.ptr(),
             version_);
    update_impl();

    return version_;
}

void var_base_t::update_internal() {
    if(!handle_->valid()) {
        log_info("waiting for key '%*s' to become available",
                 (int) key_.size(),
                 key_.ptr());
        if(!bq_success(handle_->cond_.wait(NULL))) {
            throw exception_sys_t(log::error, errno, "handle_cond.wait: %m");
        }
    }

    assert(handle_->valid());
    version_ = handle_->version();
    value_string_ = handle_->value();
}

int var_base_t::wait(int old_version) {
    if(version_ != old_version) {
        return version_;
    }

    bq_cond_guard_t guard(handle_->cond_);
    while(true) {
        update_internal();
        if(version_ != old_version) {
            break;
        }
        if(!bq_success(handle_->cond_.wait(NULL))) {
            throw exception_sys_t(log::error, errno, "handle_cond.wait: %m");
        }
    }
    guard.relax();

    log_info("key '%*s' updated to '%*s' at version %d",
             (int) key_.size(),
             key_.ptr(),
             (int) value_string_.size(),
             value_string_.ptr(),
             version_);
    update_impl();

    return version_;
}

bool var_base_t::do_set(const string_t& value, int version) {
    return store_->set(key_, value, version);
}

int var_base_t::version() const {
    return version_;
}

const string_t& var_base_t::value_string() const {
    return value_string_;
}

const string_t& var_base_t::key() const {
    return key_;
}

}  // namespace pd
