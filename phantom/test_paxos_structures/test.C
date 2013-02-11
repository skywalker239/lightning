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
#include <phantom/io_acceptor_store/io_acceptor_store.H>

namespace phantom {

MODULE(test_paxos_structures);

class io_paxos_structures_test_t : public io_t {
public:
    struct config_t : public io_t::config_t {
        config::objptr_t<io_acceptor_store_t> acceptor_store;

        void check(const in_t::ptr_t& p) const {
            io_t::config_t::check(p);
        }
    };

    io_paxos_structures_test_t(const string_t& name,
                               const config_t& config)
        : io_t(name, config),
          store_(config.acceptor_store) {}

    virtual void run() {
        log_info("Testing io_acceptor_store_t");
        test_acceptor_store();
        log_info("Finished testing io_acceptor_store_t");

        log_info("All tests finished");
        log_info("Sending SIGQUIT");
        kill(getpid(), SIGQUIT);
    }

    void test_acceptor_store() {
        ref_t<acceptor_instance_t> instance;
        size_t size = store_->size();

#define ASSERT_LOOKUP(iid, err)\
{\
    assert(store_->lookup((iid), &instance) == io_acceptor_store_t::err);\
}

#define ASSERT_LOOKUP_OK(iid)\
{\
    assert(store_->lookup((iid), &instance) == io_acceptor_store_t::OK);\
    assert(instance->instance_id() == (iid));\
}

#define ASSERT_IID(a, b)\
{\
    assert(store_->min_not_committed_iid() == (a));\
    assert(store_->next_to_max_touched_iid() == (b));\
}\

        store_->set_birth(0);
        ASSERT_IID(0, 0);

        ASSERT_LOOKUP(0, BEHIND_WALL);
        ASSERT_IID(0, 0);

        store_->move_wall_to(size + 1);

        ASSERT_LOOKUP_OK(0);
        ASSERT_LOOKUP(size, UNREACHABLE);
        ASSERT_LOOKUP(size + 1, BEHIND_WALL);
        ASSERT_IID(0, 1);

        store_->move_last_snapshot_to(size);
        store_->move_wall_to(2 * size);
        for(uint i = size; i < 2 * size; ++i) {
            ASSERT_LOOKUP_OK(i);
            ASSERT_IID(i - size + 1, i + 1);
        }

        ASSERT_LOOKUP(0, FORGOTTEN);
        ASSERT_LOOKUP(size - 1, FORGOTTEN);
        ASSERT_LOOKUP(2 * size, BEHIND_WALL);
        ASSERT_IID(size, 2 * size);

        store_->set_birth(1024);

        ASSERT_LOOKUP(0, DEAD);
        ASSERT_LOOKUP(1023, DEAD);
        ASSERT_IID(1024, 1024);

        store_->move_wall_to(1024 + 2 * size + 1);
        ASSERT_LOOKUP_OK(1024);
        ASSERT_IID(1024, 1025);
        store_->move_last_snapshot_to(1024 + 2 * size);
        ASSERT_LOOKUP_OK(1024 + 2 * size);
        ASSERT_IID(1024 + size + 1, 1024 + 2 * size + 1);

#undef ASSERT_LOOKUP
#undef ASSERT_LOOKUP_OK
#undef ASSERT_IID
        // TODO(prime@): test nofity_commit()
    }

    virtual void init() {}
    virtual void fini() {}
    virtual void stat(pd::out_t& , bool) {}

    virtual ~io_paxos_structures_test_t() {}

private:
    io_acceptor_store_t* store_;
};

namespace io_paxos_structures_test {
config_binding_sname(io_paxos_structures_test_t);
config_binding_value(io_paxos_structures_test_t, acceptor_store);
config_binding_parent(io_paxos_structures_test_t, io_t, 1);
config_binding_ctor(io_t, io_paxos_structures_test_t);
} // namespace io_ring_sender

} // namespace phantom
