#pragma once

#include <pd/base/ref.H>
#include <pd/base/netaddr_ipv4.H>
#include <pd/base/thr.H>

#include <pd/bq/bq_conn_fd.H>

#include <pd/lightning/pi_ext.H>
#include <pd/lightning/blocking_queue.H>

#include <phantom/pd.H>
#include <phantom/io_ring_sender/io_ring_sender.H>

namespace phantom {

class ring_link_t : ref_count_atomic_t {
public:
    ring_link_t(blocking_queue_t<ref_t<pi_ext_t>>* queue,
                netaddr_ipv4_t next_in_the_ring,
                interval_t net_timeout,
                size_t obuf_size)
        : queue_(queue),
          next_in_the_ring_(next_in_the_ring),
          net_timeout_(net_timeout),
          obuf_size_(obuf_size),
          shutdown_(false) {};

    void loop(ref_t<ring_link_t>* me);

    void shutdown();

private:
    blocking_queue_t<ref_t<pi_ext_t>>* queue_;
    netaddr_ipv4_t next_in_the_ring_;
    interval_t net_timeout_;
    size_t obuf_size_;
    bool shutdown_;
    thr::spinlock_t shutdown_lock_;

    bool is_stopped();

    void send_loop(bq_conn_t& conn);

    friend class ref_t<ring_link_t>;
};

}  // namespace phantom
