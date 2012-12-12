#pragma once
// Copyright (C) 2012, Fedor Korokiy <prime@yandex-team.ru>
// Copyright (C) 2012, YANDEX LLC.
// This code may be distributed under the terms of the GNU GPL v3.
// See ‘http://www.gnu.org/licenses/gpl.html’.
// vim: set tabstop=4 expandtab:
#include <pd/lightning/defs.H>
#include <pd/lightning/pi_ext.H>
#include <pd/lightning/pi_ring_cmd.H>
#include <pd/lightning/ring_var.H>
#include <pd/lightning/wait_pool.H>
#include <pd/lightning/blocking_queue.H>

#include <phantom/io_phase_executor_base/io_phase_executor_base.H>

#pragma GCC visibility push(default)
namespace phantom {

/**
 * Executes phase1 batch commands.
 */
class io_phase1_batch_executor_t : public io_phase_executor_base_t {
public:
    struct config_t : public io_phase_executor_base_t::config_t {

        config_t() {}
        void check(const in_t::ptr_t& p) const;
    };

    io_phase1_batch_executor_t(const string_t& name,
                               const config_t& config);

private:
    virtual void run_proposer();
    virtual void run_acceptor();

    using io_phase_executor_base_t::host_id_;
    using io_phase_executor_base_t::cmd_wait_pool_;
    using io_phase_executor_base_t::received_cmd_queue_;
    using io_phase_executor_base_t::ring_state_;
};

} // namespace phantom
#pragma GCC visibility pop
