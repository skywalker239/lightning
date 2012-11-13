// vim: set tabstop=4 expandtab:
#include <pd/lightning/snapshot_var.H>
#include <pd/pi/pi.H>
#include <pd/lightning/pi_ext.H>

namespace pd {

snapshot_var_t::snapshot_var_t(const string_t& key, phantom::io_zconf_t* io_zconf)
    : var_base_t(key, io_zconf),
      snapshot_version_(0),
      location_(string_t::empty),
      valid_(false)
{}

bool snapshot_var_t::valid() const {
    return valid_;
}

uint64_t snapshot_var_t::snapshot_version() const {
    return snapshot_version_;
}

const string_t& snapshot_var_t::location() const {
    return location_;
}

void snapshot_var_t::set_invalid() {
    snapshot_version_ = 0;
    location_ = string_t::empty;
    valid_ = false;
}

void snapshot_var_t::update_impl() {
    ref_t<pi_ext_t> parsed;
    try {
        in_t::ptr_t p(value_string_);
        parsed = pi_ext_t::parse(p, &pi_t::parse_text);
    } catch(const pi_t::exception_t& ex) {
        ex.log();
    } catch(const exception_t& ex) {
        ex.log();
    }

    if(parsed == NULL) {
        MKCSTR(value_z, value_string_);
        log_warning("snapshot_var_t parse(%s) failed", value_z);
        set_invalid();
        return;
    }

    const pi_t& pi = parsed->pi();
    snapshot_version_ = pi.s_ind(0).s_uint();
    str_t location_str = pi.s_ind(1).s_str();
    location_ = string_t::ctor_t(location_str.size())(location_str);

    if(snapshot_version_ == 0 || location_.size() == 0) {
        MKCSTR(value_z, value_string_);
        log_warning("snapshot_var_t parse(%s) failed", value_z);
        set_invalid();
        return;
    }

    valid_ = true;
    MKCSTR(location_z, location_);
    log_debug("snapshot_var_t updated to (%ld, %s)", snapshot_version_, location_z);
}

}  // namespace pd
