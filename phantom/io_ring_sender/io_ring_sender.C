#include <pd/bq/bq_job.H>

#include <phantom/io_ring_sender/io_ring_sender.H>
#include <phantom/module.H>

namespace phantom {

MODULE(io_ring_sender);

void io_ring_sender_t::config_t::check(const in_t::ptr_t& p) const {
    io_t::config_t::check(p);

    if(!transport_config) {
        config::error(p, "io_ring_sender_t.transport_config must be set");
    }

    if(host_id == ring_var_t::kInvalidHostId) {
        config::error(p, "io_ring_sender_t.host_id must be set");
    }
}

io_ring_sender_t::io_ring_sender_t(const string_t& name, const config_t& config)
    : io_t(name, config),
      host_id_(config.host_id),
      transport_config_(*config.transport_config),
      queue_(NULL) {}

io_ring_sender_t::~io_ring_sender_t() {}

void io_ring_sender_t::init() {
    simple_var_t<size_t> queue_size = transport_config_.ring_sender_queue_size();
    queue_size.update();

    queue_ = new blocking_queue_t<ref_t<pi_ext_t>>(queue_size.value());

    simple_var_t<size_t> number_of_connections =
        transport_config_.ring_sender_number_of_connections();
    number_of_connections.update();
    number_of_connections_ = number_of_connections.value();

    simple_var_t<size_t> output_buffer_size =
        transport_config_.ring_sender_output_buffer_size();
    output_buffer_size.update();
    obuf_size_ = output_buffer_size.value();

    simple_var_t<interval_t> net_timeout =
        transport_config_.ring_sender_net_timeout();
    net_timeout.update();
    net_timeout_ = net_timeout.value();
}

void io_ring_sender_t::run() {
    ring_var_t ring = transport_config_.ring();

    int ring_version = ring.update();

    while(true) {
        while(!ring.valid()) {
            ring_version = ring.wait(ring_version);
        }

        if (ring.is_in_ring(host_id_)) {
            queue_->activate();
            start_links(ring);
        } else {
            queue_->deactivate();
            queue_->clear();
        }

        ring_version = ring.wait(ring_version);

        shutdown_links();
    }
}

void io_ring_sender_t::fini() {
    active_links_.clear();

    delete queue_;
    queue_ = NULL;
}

void io_ring_sender_t::send(const ref_t<pi_ext_t>& blob) {
    queue_->push(blob);
}

void io_ring_sender_t::start_links(const ring_var_t& ring) {
    uint32_t next_host_id = ring.next_host_id(host_id_);
    netaddr_ipv4_t next_in_the_ring = transport_config_.ring_address(next_host_id);

    for (size_t link = 0; link < number_of_connections_; ++link) {
        active_links_.push_back(ref_t<ring_link_t>(
            new ring_link_t(queue_, next_in_the_ring, net_timeout_, obuf_size_)));

        bq_job_t<typeof(&ring_link_t::loop)>::create(
            STRING("ring_link_t"),
            bq_thr_get(),
            *active_links_.back(),
            &ring_link_t::loop,
            active_links_.back()
        );
    }
}

void io_ring_sender_t::shutdown_links() {
    for (ref_t<ring_link_t>& link : active_links_) {
        link->shutdown();
    }

    active_links_.clear();
}

namespace io_ring_sender {
config_binding_sname(io_ring_sender_t);
config_binding_value(io_ring_sender_t, host_id);
config_binding_value(io_ring_sender_t, transport_config);
config_binding_parent(io_ring_sender_t, io_t, 1);
config_binding_ctor(io_t, io_transport_config_t);
}  // namespace io_ring_sender

}  // namespace phantom
