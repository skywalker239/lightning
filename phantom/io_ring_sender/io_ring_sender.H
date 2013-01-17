// Copyright (C) 2012, Korotkiy Fedor <prime@yandex-team.ru>
// Copyright (C) 2012, YANDEX LLC.
// This code may be distributed under the terms of the GNU GPL v3.
// See ‘http://www.gnu.org/licenses/gpl.html’.
// vim: set tabstop=4 expandtab:
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

#pragma GCC visibility push(default)

namespace phantom {

/**
 * Sends blobs to next acceptor in the ring.
 *
 * Doesn't guarantee delivery.
 */
class io_ring_sender_t : public io_t {
public:
    struct config_t : public io_t::config_t {
        size_t queue_size;
        size_t number_of_connections;
        size_t obuf_size;
        interval_t net_timeout;

        config_t() throw()
            : queue_size(512),
              number_of_connections(4),
              obuf_size(sizeval_kilo),
              net_timeout(100 * interval_millisecond) {}
        ~config_t() throw() {}
        void check(const in_t::ptr_t& p) const;
    };

    io_ring_sender_t(const string_t& name, const config_t& config);
    ~io_ring_sender_t() throw();

    //! Send blob to next acceptor
    //!
    //! Blocks if internal queue is full
    //!
    //! Does nothing if host is not in the ring
    void send(const ref_t<pi_ext_t>& blob);

    //! Shutdown connections and disable ring_sender_t.
    void exit_ring();

    //! Shutdown old connections and join new ring.
    void join_ring(netaddr_ipv4_t next_in_the_ring);

    virtual void init();
    virtual void run();
    virtual void fini();

    virtual void stat(out_t&, bool) { /* TODO(prime@) */ }

private:
    blocking_queue_t<ref_t<pi_ext_t>> cmd_queue_;

    std::vector<ref_t<ring_link_t>> active_links_;

    size_t number_of_connections_;
    size_t obuf_size_;
    interval_t net_timeout_;

    void shutdown_links();

    void start_links(const ring_var_t& ring);
};

}

#pragma GCC visibility pop
