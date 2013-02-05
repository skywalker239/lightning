// Copyright (C) 2012, Alexey Pervushin <billyevans@yandex-team.ru>
// Copyright (C) 2012, YANDEX LLC.
// This code may be distributed under the terms of the GNU GPL v3.
// See ‘http://www.gnu.org/licenses/gpl.html’.
// vim: set tabstop=4 expandtab:
#include "io_guid.H"

#include <phantom/module.H>

namespace phantom {

MODULE(io_guid);

io_guid_t::config_t::config_t() throw()
    : host_id(-1)
{}

void io_guid_t::config_t::check(const in_t::ptr_t& p) const {
    io_t::config_t::check(p);

    if(host_id == -1) {
        config::error(p, "host_id must be set");
    }

    if(host_id > 1 << 12) {
        config::error(p, "host_id is to big");
    }
}

io_guid_t::io_guid_t(const string_t& name, const config_t& config)
    : io_t(name, config), guid_(config.host_id)
{}

guid_generator_t& io_guid_t::generator() {
    return guid_;
}

void io_guid_t::init()
{}

void io_guid_t::fini()
{}

void io_guid_t::run()
{}

void io_guid_t::stat(out_t&, bool)
{}

namespace io_guid {
config_binding_sname(io_guid_t);
config_binding_value(io_guid_t, host_id);
config_binding_parent(io_guid_t, io_t, 1);
config_binding_ctor(io_t, io_guid_t);
}  // namespace io_guid

} // namespace pahntom
