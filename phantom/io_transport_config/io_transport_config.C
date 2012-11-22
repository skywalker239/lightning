// Copyright (C) 2012, Alexander Kharitonov <alexander.kharitonov@gmail.com>
// Copyright (C) 2012, YANDEX LLC.
// This code may be distributed under the terms of the GNU GPL v3.
// See ‘http://www.gnu.org/licenses/gpl.html’.
// vim: set tabstop=4 expandtab:
#include <phantom/io_transport_config/io_transport_config.H>

namespace phantom {

void io_transport_config_t::config_t::check(const in_t::ptr_t& p) const {
    if(path.size() == 0) {
        config::error(p, "io_transport_config_t.path must be set");
    }
    if(!zconf) {
        config::error(p, "io_transport_config_t.zconf must be set");
    }
}

io_transport_config_t::io_transport_config_t(const string_t& name, const config_t& config)
    : io_t(name, config),
      hosts_(node_name(config.path, STRING("hosts")), config.zconf),
      multicast_group_(node_name(config.path, STRING("multicast_group")), config.zconf),
      multicast_port_(node_name(config.path, STRING("multicast_port")), config.zconf),
      ring_port_(node_name(config.path, STRING("ring_port")), config.zconf),
      recovery_port_(node_name(config.path, STRING("recovery_port")), config.zconf),
      client_port_(node_name(config.path, STRING("client_port")), config.zconf),
      master_(node_name(config.path, STRING("master")), config.zconf),
      ring_(node_name(config.path, STRING("ring")), config.zconf),
      snapshot_(node_name(config.path, STRING("snapshot")), config.zconf)
{}

const string_t io_transport_config_t::node_name(const string_t& path, const string_t& node) {
    return string_t::ctor_t(path.size() + node.size() + 1)(path)('/')(node);
}

const netaddr_ipv4_t io_transport_config_t::ring_address(uint32_t host_id) {
    ring_port_.update();
    return host_address(host_id, ring_port_.value());
}

const netaddr_ipv4_t io_transport_config_t::recovery_address(uint32_t host_id) {
    recovery_port_.update();
    return host_address(host_id, recovery_port_.value());
}

const netaddr_ipv4_t io_transport_config_t::multicast_address() {
    multicast_group_.update();
    multicast_port_.update();
    return netaddr_ipv4_t(multicast_group_.value(), multicast_port_.value());
}

const netaddr_ipv4_t io_transport_config_t::host_address(uint32_t host_id, uint16_t port) {
    hosts_.update();
    const hosts_var_t::host_t& host = hosts_.host(host_id);
    return netaddr_ipv4_t(host.address, port);
}

void io_transport_config_t::init()
{}

void io_transport_config_t::fini()
{}

void io_transport_config_t::run()
{}

namespace io_transport_config {
config_binding_sname(io_transport_config_t);
config_binding_value(io_transport_config_t, path);
config_binding_value(io_transport_config_t, zconf);
config_binding_parent(io_transport_config_t, io_t, 1);
config_binding_ctor(io_t, io_transport_config_t);
}  // namespace io_transport_config

}  // namespace phantom
