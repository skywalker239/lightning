// Copyright (C) 2012, Korotkiy Fedor <prime@yandex-team.ru>
// Copyright (C) 2012, YANDEX LLC.
// This code may be distributed under the terms of the GNU GPL v3.
// See ‘http://www.gnu.org/licenses/gpl.html’.
// vim: set tabstop=4 expandtab:
#include <sys/types.h>
#include <signal.h>
#include <unistd.h>

#include <atomic>

#include <pd/base/log.H>
#include <pd/base/config.H>

#include <phantom/io.H>
#include <phantom/module.H>
#include <phantom/io_paxos_executor/io_paxos_executor.H>

namespace phantom {

MODULE(test_executors);

class test_paxos_executor_t : public io_paxos_executor_t {
public:
    struct config_t : public io_paxos_executor_t::config_t {};

    test_paxos_executor_t(const string_t& name, const config_t& config)
        : io_paxos_executor_t(name, config) {}

    std::atomic<int> proposers_started;

    std::vector<ref_t<pi_ext_t>> acceptor_cmds;
    std::vector<ref_t<pi_ext_t>> proposer_cmds;

    thr::spinlock_t lock_;
private:
    virtual void run_proposer() {
        ++proposers_started;

        while(ring_state_snapshot().is_master) {
            wait_pool_t::item_t wait_reply(cmd_wait_pool_, 1);

            interval_t timeout = interval_second / 10;
            ref_t<pi_ext_t> cmd = wait_reply->wait(&timeout);
            if(cmd) {
                thr::spinlock_guard_t guard(lock_);

                proposer_cmds.push_back(cmd);
            }
        }
    }

    virtual void accept_ring_cmd(const ref_t<pi_ext_t>& cmd) {
        thr::spinlock_guard_t guard(lock_);

        acceptor_cmds.push_back(cmd);
    }
};

namespace test_paxos_executor {
config_binding_sname(test_paxos_executor_t);
config_binding_parent(test_paxos_executor_t, io_paxos_executor_t, 1);
config_binding_ctor(io_t, test_paxos_executor_t);
} // namespace test_paxos_executor

class io_executors_test_t : public io_t {
public:
    struct config_t : public io_t::config_t {
        config::objptr_t<test_paxos_executor_t> test_executor;

        void check(const in_t::ptr_t& ) const {}
    };

    io_executors_test_t(const string_t& name,
                        const config_t& c)
        : io_t(name, c),
          test_executor_(c.test_executor) {}

    virtual void run() {
        log_info("All tests finished");
        log_info("Sending SIGQUIT");
        kill(getpid(), SIGQUIT);
    }

    virtual void init() {}
    virtual void fini() {}
    virtual void stat(pd::out_t& , bool) {}

    virtual ~io_executors_test_t() {}

private:
    test_paxos_executor_t* test_executor_;
};

namespace io_executors_test {
config_binding_sname(io_executors_test_t);

config_binding_value(io_executors_test_t, test_executor);

config_binding_parent(io_executors_test_t, io_t, 1);
config_binding_ctor(io_t, io_executors_test_t);
} // namespace io_ring_sender

} // namespace phantom
