// Copyright (C) 2012, Philipp Sinitsyn <prime@yandex-team.ru>
// Copyright (C) 2012, YANDEX LLC.
// This code may be distributed under the terms of the GNU GPL v3.
// See ‘http://www.gnu.org/licenses/gpl.html’.
// vim: set tabstop=4 expandtab:
#pragma once
#include <tuple>

#include <pd/lightning/defs.H>
#include <pd/lightning/value.H>
#include <pd/lightning/concurrent_heap.H>

#include <phantom/pd.H>
#include <phantom/io.H>
#include <pd/bq/bq_cond.H>

#pragma GCC visibility push(default)

namespace phantom {

/**
 * Maintains a container of paxos instance pools along with some
 * additional information.
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
 *    other arguments unmodified.
 * 
 * Implements standart size, empty and clear container functions for 
 * whole pool and separate heaps.
 */

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
        log_info(" **** Hi, im proposer!");
    }

    void push_failed(instance_id_t instance_id, ballot_id_t ballot_hint);
    bool pop_failed(instance_id_t* instance_id, ballot_id_t* ballot_hint);

    void push_open(instance_id_t instance_id, ballot_id_t ballot_id);
    bool pop_open(instance_id_t* instance_id, ballot_id_t* ballot_id);

    void push_reserved(instance_id_t instance_id,
                       ballot_id_t ballot_id,
                       value_t value);
    bool pop_reserved(instance_id_t* instance_id,
                      ballot_id_t* ballot_id,
                      value_t* value);


    size_t size();
    size_t open_size(); 
    size_t failed_size();
    size_t reserved_size(); 


    bool empty();
    bool open_empty();
    bool failed_empty();
    bool reserved_empty();

    void clear();

    virtual void init() {}
    virtual void run()  {}
    virtual void fini() {}
    virtual void stat(out_t&, bool) {}
 private:
    struct iid_cmp_t {
        template <typename T>
        bool operator() (const T& left, const T& right) const {
            return std::get<0>(left) < std::get<0>(right);    
        }
    };

    concurrent_heap_t<std::tuple<instance_id_t, ballot_id_t>, iid_cmp_t> open_instances_;
    concurrent_heap_t<std::tuple<instance_id_t, ballot_id_t>, iid_cmp_t> failed_instances_;
    concurrent_heap_t<std::tuple<instance_id_t, ballot_id_t, value_t>, iid_cmp_t> reserved_instances_;

    bool active;
    static const int MAX_INSTANCESS_IN_PROCESSING_ = 10000;
};

} // namespace phantom
