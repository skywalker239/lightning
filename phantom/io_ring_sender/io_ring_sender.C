#include <phantom/io_ring_sender/io_ring_sender.H>
#include <phantom/module.H>

namespace phantom {

MODULE(io_ring_sender);

void io_ring_sender_t::config_t::check(const in_t::ptr_t& p) const {
    io_t::config_t::check(p);

    if(!transport_config) {
        config::error(p, "io_ring_sender_t.transport_config must be set");
    }

    if(!host_id) {
        config::error(p, "io_ring_sender_t.host_id must be set");

    }
}

io_ring_sender_t::io_ring_sender_t(const string_t& name, const config_t& config)
     : io_t(name, config),
       host_id_(config.host_id),
       transport_config_(*config.transport_config) {

}

namespace io_ring_sender {
config_binding_sname(io_ring_sender_t);
config_binding_value(io_ring_sender_t, host_id);
config_binding_value(io_ring_sender_t, transport_config);
config_binding_parent(io_ring_sender_t, io_t, 1);
config_binding_ctor(io_t, io_transport_config_t);
}  // namespace io_ring_sender

}  // namespace phantom
