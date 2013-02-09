#pragma once
// Copyright (C) 2012, Fedor Korotkiy <prime@yandex-team.ru>
// Copyright (C) 2012, YANDEX LLC.
// This code may be distributed under the terms of the GNU GPL v3.
// See ‘http://www.gnu.org/licenses/gpl.html’.
// vim: set tabstop=4 expandtab:
#include <pd/base/config.H>
#include <pd/bq/bq_mutex.H>
#include <pd/bq/bq_thr.H>

#include <pd/lightning/defs.H>
#include <pd/lightning/pi_ext.H>
#include <pd/lightning/pi_ring_cmd.H>

#include <phantom/io.H>
#include <phantom/io_paxos_executor/io_paxos_executor.H>

#pragma GCC visibility push(default)
namespace phantom {

/**
 * Phase1 batch executor.
 *
 *
 */
class io_phase1_batch_executor_t : public io_paxos_executor_t {
public:
    struct config_t : public io_paxos_executor_t::config_t {
        uint32_t batch_size;

        config_t() throw()
            : batch_size(1024) {}
        ~config_t() throw() {}
        void check(const in_t::ptr_t& p) const;
    };

    io_phase1_batch_executor_t(const string_t& name,
                               const config_t& config);

    void set_start_iid(instance_id_t start_iid);

private:
    const uint32_t batch_size_;

    instance_id_t next_batch_start_;
    bq_mutex_t next_batch_start_lock_;

    virtual void run_proposer();
    virtual void run_acceptor();
    virtual void accept_ring_cmd(const ref_t<pi_ext_t>& ring_cmd);

    bool next_batch_start(instance_id_t* start);
    ref_t<pi_ext_t> propose_batch(instance_id_t batch_start);
    void push_to_proposer_pool(const ref_t<pi_ext_t>& ring_reply);
    bool accept_one_instance(instance_id_t iid,
                             ballot_id_t ballot_id,
                             batch_fail_t* fail);
    void update_and_send_to_next(const ref_t<pi_ext_t>& received_cmd,
                                 const std::vector<batch_fail_t>& fails);

};

} // namespace phantom
#pragma GCC visibility pop
