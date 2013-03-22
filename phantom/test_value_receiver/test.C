// Copyright (C) 2012, Pervushin Alexey <billyevans@yandex-team.ru>
// Copyright (C) 2012, YANDEX LLC.
// This code may be distributed under the terms of the GNU GPL v3.
// See ‘http://www.gnu.org/licenses/gpl.html’.
// vim: set tabstop=4 expandtab:
#include <sys/types.h>
#include <signal.h>
#include <unistd.h>

#include <pd/base/log.H>
#include <pd/base/assert.H>
#include <pd/lightning/pi_ext.H>
#include <pd/bq/bq_util.H>
#include <pd/pi/pi_pro.H>

#include <phantom/io.H>
#include <phantom/module.H>
#include <phantom/io_ring_sender/io_ring_sender.H>
#include <phantom/io_proposer_pool/io_proposer_pool.H>
#include <phantom/io_stream/proto_value_receiver/proto_value_receiver.H>

namespace phantom {

MODULE(test_value_receiver);

class io_test_sender_t : public io_t {
public:
    static const uint16_t value_receiver_port = 34813;
    struct config_t : public io_t::config_t {
        config::objptr_t<io_ring_sender_t> sender;
        config::objptr_t<io_proposer_pool_t> proposer_pool;
        config::objptr_t<io_stream::proto_value_receiver_t> value_receiver;

        void check(const in_t::ptr_t& ) const {}
    };

    io_test_sender_t(const string_t& name, const config_t& config)
        : io_t(name, config),
        sender_(*config.sender),
        proposer_pool_(*config.proposer_pool),
        value_recv_(*config.value_receiver)
    {}
    virtual void run() {
        push_pop_test();

        log_info("All tests finished");
        log_info("Sending SIGTERM");
        kill(getpid(), SIGTERM);
    }
    void sleep() {
        interval_t timeout = 100 * interval_millisecond;
        bq_sleep(&timeout);
    }
    ref_t<pi_ext_t> make_test_pi() {
        string_t s = STRING("[ 1 2 null { \"abc\" : 3 3 : \"abc\" }];");
        in_t::ptr_t ptr = s;
        ref_t<pi_ext_t> root = pi_ext_t::parse(ptr, &pi_t::parse_text);
        str_t str(CSTR("lkjhg"));
        pi_t::pro_t::item_t item1(str, NULL);
        pi_t::pro_t::item_t item2(pi_t::pro_t::uint_t(1234), &item1);
        pi_t::pro_t::item_t item3(root->root().value, &item2);

        pi_t::pro_t pro(&item3);
        return pi_ext_t::__build(pro);
    }
    void push_pop_test() {
        reset_all();

        instance_id_t iid_origin(5);
        ballot_id_t ballot_origin(2);

        proposer_pool_.push_open(iid_origin, ballot_origin);
        value_recv_.set_master(true);
        sender_.join_ring(
            netaddr_ipv4_t(address_ipv4_t(2130706433 /* 127.0.0.1 */),
                               value_receiver_port));
        ref_t<pi_ext_t> val = make_test_pi();
        sender_.send(val);
        sleep();
        { // compare
            instance_id_t iid;
            ballot_id_t ballot;
            value_t value;
            assert(proposer_pool_.pop_reserved(&iid, &ballot, &value));
            assert(iid == iid_origin);
            assert(ballot == ballot_origin);
            assert(value.valid());
            const pi_t::root_t *root = (const pi_t::root_t *)(value.value().ptr());
            assert(pi_t::cmp_eq(root->value, val->pi()));
            assert(value_recv_.get_recv_count() == 1);
            assert(value_recv_.get_last_pushed_value_id() == value.value_id());
        }
    }

    void reset_all() {
        sender_.exit_ring();

        sleep();
    }

    virtual void init() {}
    virtual void fini() {}
    virtual void stat(pd::out_t& , bool) {}

    virtual ~io_test_sender_t() {}

private:
    io_ring_sender_t& sender_; // use it just as tcp-sender
    io_proposer_pool_t& proposer_pool_;
    io_stream::proto_value_receiver_t& value_recv_;
};

namespace io_test_sender {
config_binding_sname(io_test_sender_t);
config_binding_parent(io_test_sender_t, io_t, 1);
config_binding_value(io_test_sender_t, sender);
config_binding_value(io_test_sender_t, proposer_pool);
config_binding_value(io_test_sender_t, value_receiver);
config_binding_ctor(io_t, io_test_sender_t);
} // namespace io_test_sender

} // namespace phantom
