// Copyright (C) 2012, Philipp Sinitsyn <prime@yandex-team.ru>
// Copyright (C) 2012, YANDEX LLC.
// This code may be distributed under the terms of the GNU GPL v3.
// See ‘http://www.gnu.org/licenses/gpl.html’.
// vim: set tabstop=4 expandtab:
#pragma once

#include <pd/lightning/defs.H>
#include <pd/lightning/value.H>

#include <phantom/pd.H>
#include <phantom/io.H>
#include <pd/bq/bq_cond.H>

#include <queue>
#include <vector>
#include <functional> 

#pragma GCC visibility push(default)

namespace phantom {

/**
 * Maintains pool of active paxos instance ids along with some
 * additional information. Also responsible for throttling paxos
 * executors on proposer.
 *
 * Consists of 3 pools.
 *
 * 1) Failed pool - instances for which paxos failed for some
 * reason and must be started over. Stores (iid, ballot_id hint)
 * pairs.
 *
 * 2) Open pool - instances which successfully completed phase1
 * with no value received from acceptors. Stores (iid, ballot_id)
 * pairs.
 *
 * 3) Reserved pool - instances which successfully completed phase1
 * with some value received from acceptors or new value bounded from
 * client. Stores (iid, ballot_id, value) tuples.
 *
 * Can be ether in active or inactive state.
 *
 * 1) push_*() on inactive pool return immediately doing nothing.
 * 2) pop_*() on inactive pool return immediately false leaving
 *    other arguments undefined.
 */

struct open_blob {
    instance_id_t iid;
    ballot_id_t   ballot;
    

    open_blob() {}
    open_blob(instance_id_t iid, ballot_id_t ballot) : iid(iid), 
                                                       ballot(ballot)
    {}

    virtual bool operator > (const open_blob& other) const {
        return iid > other.iid;    
    }
};

struct reserved_blob : open_blob {
    reserved_blob() : open_blob()
    {}
    reserved_blob(instance_id_t iid, ballot_id_t ballot, value_t value) :
        open_blob(iid, ballot), value(value)
    {}
    value_t value;
};

template <typename T>
class generic_pool_t {
 public:
     generic_pool_t () : active(true) 
     {}

     size_t size() {
        return pool_.size();
     }

     bool empty() {
        return pool_.empty(); 
     }

     void push(T value) {
        bq_cond_guard_t guard(cond_);
        pool_.push(value);
        cond_.send();
     }

     void pop(T* value) {
        bq_cond_guard_t guard(cond_);
        bool wait_ok = true;

        while(wait_ok && pool_.empty() && is_active()) {
            wait_ok = bq_success(cond_.wait(NULL));
        }
        
        T result = pool_.top();
        pool_.pop();

        (*value) = result;
     }

     void activate() {
        thr::spinlock_guard_t guard(lock_);
        active = true;
        cond_.send(true);
     }

     void deactivate() {
        thr::spinlock_guard_t guard(lock_);
        active = false;
        cond_.send(true);
     }

     bool is_active() {
        thr::spinlock_guard_t guard(lock_);
        return active;
     }

 private:
     bool active;
     thr::spinlock_t lock_;

     bq_cond_t cond_;
     std::priority_queue<T, std::vector<T>, std::greater<T> > pool_;
};

class io_proposer_pool_t : public io_t {
 public:
    io_proposer_pool_t (const string_t& name, const config_t& conf) : io_t(name, conf)
    {}

    struct config_t : io_t::config_t {
        config_t() {}        
    };

    void activate();
    void deactivate();
    void say_hi() {
        log_info("Hi!");
    }

    void push_failed(instance_id_t instance_id, ballot_id_t ballot_hint);
    bool pop_failed(instance_id_t* instance_id, ballot_id_t* ballot_hint);
    bool failed_empty();

    void push_open(instance_id_t instance_id, ballot_id_t ballot_id);
    bool pop_open(instance_id_t* instance_id, ballot_id_t* ballot_id);
    bool open_empty();

    void push_reserved(instance_id_t instance_id,
                       ballot_id_t ballot_id,
                       value_t value);
    bool pop_reserved(instance_id_t* instance_id,
                      ballot_id_t* ballot_id,
                      value_t* value);

    bool reserved_empty();

    virtual void init() {}
    virtual void run()  {}
    virtual void fini() {}
    virtual void stat(out_t&, bool) {}
 private:

    generic_pool_t<open_blob> open_instances_;
    generic_pool_t<open_blob> failed_instances_;
    generic_pool_t<reserved_blob> reserved_instances_;

    bool active;
    static const int MAX_INSTANCESS_IN_PROCESSING_ = 10000;
};

} // namespace phantom
