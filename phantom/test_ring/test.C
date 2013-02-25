// Copyright (C) 2012, Korotkiy Fedor <prime@yandex-team.ru>
// Copyright (C) 2012, YANDEX LLC.
// This code may be distributed under the terms of the GNU GPL v3.
// See ‘http://www.gnu.org/licenses/gpl.html’.
// vim: set tabstop=4 expandtab:
#include <sys/types.h>
#include <signal.h>
#include <unistd.h>

#include <vector>

#include <pd/bq/bq_job.H>
#include <pd/bq/bq_util.H>
#include <pd/base/thr.H>
#include <pd/base/log.H>
#include <pd/base/assert.H>
#include <pd/lightning/pi_ext.H>
#include <pd/lightning/pi_ring_cmd.H>
#include <pd/lightning/finished_counter.H>

#include <phantom/io.H>
#include <phantom/module.H>
#include <phantom/io_ring_sender/io_ring_sender.H>
#include <phantom/ring_handler/ring_handler.H>
#include <phantom/ring_handler/ring_handler_proto.H>

namespace phantom {

MODULE(test_ring);

struct test_ring_handler_t : public ring_handler_t {
    struct config_t {
        void check(const in_t::ptr_t&) const {};
    };

	inline test_ring_handler_t(string_t const &, config_t const &) {}

    virtual void handle_ring_cmd(const ref_t<pi_ext_t>& ring_cmd) {
        thr::spinlock_guard_t guard(lock_);

        received_.push_back(ring_cmd);
    }

    size_t size() {
        thr::spinlock_guard_t guard(lock_);

        return received_.size();
    }

    void clear() {
        thr::spinlock_guard_t guard(lock_);

        received_.clear();
    }

    std::vector<ref_t<pi_ext_t>> received_;

    thr::spinlock_t lock_;
};

namespace test_ring_handler {
config_binding_sname(test_ring_handler_t);
config_binding_ctor(ring_handler_t, test_ring_handler_t);
}

static const uint16_t kProto1Port = 34843;
static const uint16_t kProto2Port = 34844;
static const host_id_t kProto1Host = 12;
static const host_id_t kProto2Host = 14;

class io_ring_sender_and_handler_test_t : public io_t {
public:
    struct config_t : public io_t::config_t {
        config::objptr_t<io_ring_sender_t> sender;

        config::objptr_t<ring_handler_proto_t> proto1;
        config::objptr_t<test_ring_handler_t> proto1_handler;

        config::objptr_t<ring_handler_proto_t> proto2;
        config::objptr_t<test_ring_handler_t> proto2_handler;

        void check(const in_t::ptr_t& ) const {}
    };

    io_ring_sender_and_handler_test_t(const string_t& name,
                                      const config_t& c)
        : io_t(name, c),
          sender_(c.sender),
          proto1_(c.proto1),
          proto1_handler_(c.proto1_handler),
          proto2_(c.proto2),
          proto2_handler_(c.proto2_handler) {}

    virtual void run() {
        test_ring_sender_ring_switch();
        test_ring_sender_exit_ring();
        test_stress();
        test_benchmark();

        log_info("All tests finished");
        log_info("Sending SIGQUIT");
        kill(getpid(), SIGQUIT);
    }

    void test_stress() {
        reset_all();

        for(ring_id_t ring_id = 1; ring_id < 100; ++ring_id) {
            uint16_t port = ring_id % 2 == 0 ? kProto1Port : kProto2Port;
            host_id_t host = ring_id % 2 == 0 ? kProto1Host : kProto2Host;

            sender_->join_ring(
                netaddr_ipv4_t(address_ipv4_t(2130706433 /* 127.0.0.1 */),
                               port));

            for(int i = 0; i < 100; ++i) {
                sender_->send(build_cmd(ring_id, host));
            }
        }
    }

    void test_ring_sender_ring_switch() {
        reset_all();

        sender_->join_ring(
            netaddr_ipv4_t(address_ipv4_t(2130706433 /* 127.0.0.1 */),
                           kProto1Port));
        sender_->send(build_cmd(30, kProto1Host));
        sleep();

        sender_->exit_ring();
        sleep();

        sender_->join_ring(
            netaddr_ipv4_t(address_ipv4_t(2130706433 /* 127.0.0.1 */),
                           kProto2Port));
        sleep();

        sender_->send(build_cmd(31, kProto2Host));

        sleep();
        sender_->exit_ring();

        assert(proto1_handler_->size() == 1);
        assert(proto2_handler_->size() == 1);
    }

    void test_ring_sender_exit_ring() {
        reset_all();

        sender_->join_ring(
            netaddr_ipv4_t(address_ipv4_t(2130706433 /* 127.0.0.1 */),
                           kProto1Port));
        sender_->send(build_cmd(30, kProto1Host));
        sleep();

        sender_->exit_ring();
        sleep();

        sender_->send(build_cmd(30, kProto1Host));
        sleep();

        assert(proto1_handler_->size() == 1);
    }

    void test_benchmark() {
        log_info("Starting test_benchmark()");

        reset_all();
        sender_->join_ring(
            netaddr_ipv4_t(address_ipv4_t(2130706433 /* 127.0.0.1 */),
                           kProto1Port));

        sleep();

        const int N_SENDERS = 10;
        const int CMD_PER_SENDER = 100000;

        finished_counter_t count;
        count.started(N_SENDERS);

        for(int i = 0; i < N_SENDERS; ++i) {
            bq_job_t<typeof(&io_ring_sender_and_handler_test_t::send_many)>::create(
                STRING("sender"),
                scheduler.bq_thr(),
                *this,
                &io_ring_sender_and_handler_test_t::send_many,
                CMD_PER_SENDER,
                &count
            );
        }

        count.wait_for_all_to_finish();

        log_info("test_benchmark() finished");
        log_info("%d cmds send, %lu cmds received",
                 N_SENDERS * CMD_PER_SENDER,
                 proto1_handler_->size());
    }

    void send_many(int cmd_count, finished_counter_t* active_senders) {
        ref_t<pi_ext_t> cmd = build_cmd(30, kProto1Host);

        for(int i = 0; i < cmd_count; ++i) {
            sender_->send(cmd);
        }

        active_senders->finish();
    }

    ref_t<pi_ext_t> build_cmd(ring_id_t ring_id, host_id_t dst_host_id) {
        ref_t<pi_ext_t> cmd = cmd::batch::build(
            {
                request_id: 52,
                ring_id: ring_id,
                dst_host_id: dst_host_id
            },
            {
                start_iid: 1024,
                end_iid: 2048,
                ballot_id: 7,
                fails: std::vector<cmd::batch::fail_t>()
            }
        );

        return cmd;
    }

    void reset_all() {
        sender_->exit_ring();

        sleep();

        proto1_handler_->clear();
        proto2_handler_->clear();
    }

    void sleep() {
        interval_t timeout = 100 * interval_millisecond;
        bq_sleep(&timeout);
    }

    virtual void init() {}
    virtual void fini() {}
    virtual void stat(pd::out_t& , bool) {}

    virtual ~io_ring_sender_and_handler_test_t() {}

private:
    io_ring_sender_t* sender_;
    ring_handler_proto_t* proto1_;
    test_ring_handler_t* proto1_handler_;
    ring_handler_proto_t* proto2_;
    test_ring_handler_t* proto2_handler_;
};

namespace io_ring_sender_and_handler_test {
config_binding_sname(io_ring_sender_and_handler_test_t);

config_binding_value(io_ring_sender_and_handler_test_t, sender);
config_binding_value(io_ring_sender_and_handler_test_t, proto1);
config_binding_value(io_ring_sender_and_handler_test_t, proto1_handler);
config_binding_value(io_ring_sender_and_handler_test_t, proto2);
config_binding_value(io_ring_sender_and_handler_test_t, proto2_handler);

config_binding_parent(io_ring_sender_and_handler_test_t, io_t, 1);
config_binding_ctor(io_t, io_ring_sender_and_handler_test_t);
} // namespace io_ring_sender

} // namespace phantom
