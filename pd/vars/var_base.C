#include <pd/vars/var_base.H>
#include <pd/base/exception.H>

namespace pd {

using ::phantom::io_zookeeper_t;

var_base_t::var_base_t(const string_t& key, io_zookeeper_t* io_zookeeper)
    : version_(kInvalidVersion),
      key_(key),
      io_zookeeper_(io_zookeeper),
      stat_(io_zookeeper_->add_var_ref(key))
{}

var_base_t::~var_base_t() {
    io_zookeeper_->remove_var_ref(key_);
    stat_ = NULL;
}

int var_base_t::update() {
    bq_cond_guard_t guard(stat_->cond);
    update_internal();
    guard.relax();
    
    MKCSTR(key_z, key_);
    MKCSTR(value_z, value_string_);
    log_info("key '%s' updated to '%s' at version %d",
             key_z,
             value_z,
             version_);
    update_impl();

    return version_;
}

void var_base_t::update_internal() {
    MKCSTR(key_z, key_);

    if(!stat_->valid) {
        log_info("waiting for key '%s' to become available", key_z);
        if(!bq_success(stat_->cond.wait(NULL))) {
            throw exception_sys_t(log::error, errno, "stat_cond.wait: %m");
        }
    }

    assert(stat_->valid);
    version_ = stat_->exists ? stat_->stat.version : kDeletedVersion;
    value_string_ = stat_->exists ? stat_->value : string_t::empty;
}

int var_base_t::wait(int old_version) {
    if(version_ != old_version) {
        return version_;
    }

    bq_cond_guard_t guard(stat_->cond);
    while(true) {
        update_internal();
        if(version_ != old_version) {
            break;
        }
        if(!bq_success(stat_->cond.wait(NULL))) {
            throw exception_sys_t(log::error, errno, "stat_cond.wait: %m");
        }
    }
    guard.relax();

    MKCSTR(key_z, key_);
    MKCSTR(value_z, value_string_);
    log_info("key '%s' updated to '%s' at version %d",
             key_z,
             value_z,
             version_);
    update_impl();

    return version_;
}

bool var_base_t::set(const string_t&, int) {
    return false;
}

bool var_base_t::exists() const {
    return version_ >= 0;
}

int var_base_t::version() const {
    return version_;
}

const string_t& var_base_t::value_string() const {
    return value_string_;
}

}  // namespace pd
