#pragma once
// Copyright (C) 2013, Fedor Korotkiy <prime@yandex-team.ru>
// Copyright (C) 2013, YANDEX LLC.
// This code may be distributed under the terms of the GNU GPL v3.
// See ‘http://www.gnu.org/licenses/gpl.html’.
// vim: set tabstop=4 expandtab:

#include <phantom/io_paxos_executor/io_paxos_executor.H>

#pragma GCC visibility push(default)
namespace phantom {

/**
 * Phase1 executor.
 *
 *
 */
class io_phase1_executor_t : public io_paxos_executor_t {
public:
    struct config_t : public io_paxos_executor_t::config_t {
        void check(const in_t::ptr_t& p) const;
    };

    io_phase1_executor_t(const string_t& name,
                         const config_t& config);

private:
    virtual void run_proposer();
    virtual void accept_ring_cmd(const ref_t<pi_ext_t>& ring_cmd);
};

} // namespace phantom
#pragma GCC visibility pop
