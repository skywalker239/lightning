#pragma once

#include <vector>

#include <pd/base/ref.H>
#include <pd/base/queue.H>
#include <pd/base/netaddr.H>
#include <pd/lightning/pi_ext.H>
#include <pd/lightning/blocking_queue.H>

#include <phantom/io.H>
#include <phantom/io_transport_config/io_transport_config.H>
#include <phantom/io_ring_sender/ring_link.H>

#pragma GCC visibitily push(default)

namespace phantom {

/**
 * Sends blobs to next acceptor in the ring.
 *
 * Maintains several TCP connections to next acceptor. Shuts them down
 * and starts new when zookeeper ring variable changes.
 *
 * When ring changes or TCP connection breaks some blobs might be lost
 * or sent to wrong acceptor.
 */
class io_ring_sender_t : public io_t {
public:
    struct config_t : public io_t::config_t {
        uint32_t host_id;
        config::objptr_t<io_transport_config_t> transport_config;

        config_t() throw() : host_id(ring_var_t::kInvalidHostId) {}
        ~config_t() throw() {}
        void check(const in_t::ptr_t& p) const;
    };

    io_ring_sender_t(const string_t& name, const config_t& config);
    ~io_ring_sender_t() throw();

    //! Send blob to next acceptor
    //!
    //! Blocks if internal queue is full.
    //!
    //! Return immediately and discard blob if current acceptor is not in the ring.
    void send(const ref_t<pi_ext_t>& blob);

    virtual void init();
    virtual void run();
    virtual void fini();

private:
    uint32_t host_id_;
    io_transport_config_t& transport_config_;

    blocking_queue_t<ref_t<pi_ext_t>>* queue_;

    std::vector<ref_t<ring_link_t>> active_links_;

    size_t number_of_connections_;
    size_t obuf_size_;
    interval_t net_timeout_;

    void shutdown_links();

    void start_links(const ring_var_t& ring);
};

}

#pragma GCC visibility pop
