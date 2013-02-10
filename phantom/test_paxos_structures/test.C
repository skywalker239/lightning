// Copyright (C) 2012, Korotkiy Fedor <prime@yandex-team.ru>
// Copyright (C) 2012, YANDEX LLC.
// This code may be distributed under the terms of the GNU GPL v3.
// See ‘http://www.gnu.org/licenses/gpl.html’.
// vim: set tabstop=4 expandtab:
#include <sys/types.h>
#include <signal.h>
#include <unistd.h>

#include <pd/base/log.H>
#include <pd/base/config.H>
#include <pd/base/assert.H>

#include <phantom/io.H>
#include <phantom/module.H>
#include <phantom/acceptor_store/acceptor_store.H>

namespace phantom {

MODULE(test_paxos_structures);

class io_paxos_structures_test_t : public io_t {
public:
    struct config_t : public io_t::config_t {
        config::objptr_t<acceptor_store_t> acceptor_store;

        void check(const in_t::ptr_t& p) const {
            io_t::config_t::check(p);
        }
    };

    io_paxos_structures_test_t(const string_t& name,
                               const config_t& config)
        : io_t(name, config),
          acceptor_store_(config.acceptor_store) {}

    virtual void run() {
        log_info("Testing acceptor_store_t");
        log_info("Finished testing acceptor_store_t");

        log_info("All tests finished");
        log_info("Sending SIGQUIT");
        kill(getpid(), SIGQUIT);
    }

    void test_acceptor_store() {
        ref_t<acceptor_instance_t> instance;
        size_t size = acceptor_store_->size();

        assert(acceptor_store_->lookup(0, &instance) == acceptor_store_t::OK);
        assert(instance->instance_id() == 0);

        assert(acceptor_store_->lookup(size, &instance) == acceptor_store_t::IID_TOO_HIGH);

        acceptor_store_->set_last_snapshot(size);
        assert(acceptor_store_->lookup(size, &instance) == acceptor_store_t::OK);
        assert(instance->instance_id() == size);

        assert(acceptor_store_->lookup(size, &instance) == acceptor_store_t::IID_TOO_LOW);
    }

    virtual void init() {}
    virtual void fini() {}
    virtual void stat(pd::out_t& , bool) {}

    virtual ~io_paxos_structures_test_t() {}

private:
    acceptor_store_t* acceptor_store_;
};

namespace io_paxos_structures_test {
config_binding_sname(io_paxos_structures_test_t);
config_binding_value(io_paxos_structures_test_t, acceptor_store);
config_binding_parent(io_paxos_structures_test_t, io_t, 1);
config_binding_ctor(io_t, io_paxos_structures_test_t);
} // namespace io_ring_sender

} // namespace phantom
