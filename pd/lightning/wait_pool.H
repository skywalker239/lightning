// vim: set tabstop=4 expandtab:
#pragma once

#include <pd/lightning/pi_ext.H>
#include <pd/lightning/defs.H>
#include <pd/base/ref.H>
#include <pd/bq/bq_cond.H>
#include <vector>

namespace pd {

/**
 * A wait pool for pi-valued requests for which replies are
 * received elsewhere (not on the same connection over
 * which the request has been sent, e.g. over a Ring Paxos ring).
 * Each request has a unique request id (of type request_id_t).
 * The usage is as follows:
 *
 * // In the code that sends the request //
 * // request_id is this request' id.
 *    wait_pool_t::item_t item(pool, request_id);
 *    do_send_request(request);
 *    interval_t timeout = ...;
 *    // NULL reply indicates that the request has timed out.
 *    ref_t<pi_ext_t> reply = item.wait(&timeout);
 *
 * // In the code that handles the incoming replies //
 *    (ref_t<pi_ext_t> reply, request_id_t id) =
 *        parse_reply_from_somewhere(...);
 *
 *    ref_t<wait_pool_t::data_t> data = wait_pool.lookup(id);
 *    // NULL indicates that this request has timed out or
 *    // the reply is a stray one.
 *    if(data != NULL) {
 *        data->send(reply);  // wakes the coroutine waiting for reply.
 *    }
 */
class wait_pool_t {
public:
    //! Holds a condition variable which the waiter waits on.
    class data_t : public ref_count_atomic_t {
    public:
        data_t(request_id_t id) throw();
        ~data_t() throw();

        //! Sets the reply and wakes the waiter.
        void send(ref_t<pi_ext_t> reply);

        //! Waits for reply until timeout expires.
        //  Returns the reply on success, NULL on timeout.
        ref_t<pi_ext_t> wait(interval_t* timeout);
    private:
        bq_cond_t cond_;
        request_id_t id_;
        ref_t<pi_ext_t> reply_;

        friend class ref_t<data_t>;
        friend class wait_pool_t;
    };

    struct slot_t;

    //! Represents a single data_t in a linked list with
    //  a spinlock.
    struct item_t : public ref_t<data_t> {
        slot_t& slot;
        item_t* next;
        item_t** me;

        item_t(wait_pool_t& pool, request_id_t id);
        ~item_t() throw();
    };
    
    //! A single hash chain with a spinlock.
    struct slot_t {
        thr::spinlock_t lock;
        item_t *list;
    
        slot_t() throw() {};
        ~slot_t() throw() {};
    };

    //! Looks up the data_t corresponding to request id `id'.
    //  Returns NULL on failure.
    ref_t<data_t> lookup(request_id_t id);

    //! The wait pool is a hash table with a fixed number of buckets.
    //  slots sets that number.
    wait_pool_t(size_t slots);
    ~wait_pool_t() throw();
private:
    wait_pool_t(const wait_pool_t&) = delete;
    wait_pool_t& operator=(const wait_pool_t&) = delete;

    //! The slot corresponding to request_id.
    slot_t& slot(request_id_t request_id); 

    std::vector<slot_t> slots_;
    const size_t slots_number_;
};

}  // namespace pd
