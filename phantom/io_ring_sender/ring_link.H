#pragma once

#include <pd/base/ref.H>
#include <pd/base/netaddr_ipv4.H>
#include <pd/base/thr.H>

#include <pd/bq/bq_conn_fd.H>

#include <pd/lightning/pi_ext.H>
#include <pd/lightning/blocking_queue.H>

#include <phantom/pd.H>

namespace phantom {


/**
 * Single connection to next acceptor in the ring.
 *
 * Takes blobs one by one from queue and sends them to next acceptor.
 * Restarts connection if send operation didn't succeeded in
 * net_timeout.
 */
class ring_link_t : ref_count_atomic_t {
public:
    //! @param queue - ptr to queue from which link takes blobs to
    //! send
    //! @param next_in_the_ring - network address of next acceptor
    //! @param net_timeout - timeout after which connection is
    //! considered broken and restarted
    //! @param obuf_size - size of buffer allocated on stack and used
    //! internally by bq_out_t(in bytes)
    ring_link_t(blocking_queue_t<ref_t<pi_ext_t>>* queue,
                netaddr_ipv4_t next_in_the_ring,
                interval_t net_timeout,
                size_t obuf_size)
        : queue_(queue),
          next_in_the_ring_(next_in_the_ring),
          net_timeout_(net_timeout),
          obuf_size_(obuf_size),
          shutdown_(false) {};

    //! Main loop of link, all work is done here.
    //! Takes ref_t to itself to simplify destruction of stopped
    //! links(just shutdown link, delete reference to it and link will
    //! destroy itself after return from this function).
    //!
    //! Returns after shutdown() is called
    void loop(ref_t<ring_link_t> me);

    //! Shutdown this link.
    //! Link might stay blocked on queue, call queue.deactivate() to
    //! ensure that link is fully stopped.
    void shutdown();

private:
    blocking_queue_t<ref_t<pi_ext_t>>* queue_;
    netaddr_ipv4_t next_in_the_ring_;
    interval_t net_timeout_;
    size_t obuf_size_;
    bool shutdown_;
    thr::spinlock_t shutdown_lock_;

    bool is_stopped();

    //! Loop that takes blobs from queue and send then over network.
    void send_loop(bq_conn_t* conn);

    friend class ref_t<ring_link_t>;
};

}  // namespace phantom
