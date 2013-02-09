// Copyright (C) 2012, Korotkiy Fedor <prime@yandex-team.ru>
// Copyright (C) 2012, YANDEX LLC.
// This code may be distributed under the terms of the GNU GPL v3.
// See ‘http://www.gnu.org/licenses/gpl.html’.
// vim: set tabstop=4 expandtab:
#include <sys/types.h>
#include <signal.h>
#include <unistd.h>

#include <pd/base/log.H>

#include <phantom/io.H>
#include <phantom/module.H>

namespace phantom {

MODULE(test_executors);

class io_executors_test_t : public io_t {
public:
    struct config_t : public io_t::config_t {
        void check(const in_t::ptr_t& ) const {}
    };

    io_executors_test_t(const string_t& name,
                        const config_t& c)
        : io_t(name, c) {}

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

};

namespace io_executors_test {
config_binding_sname(io_executors_test_t);

config_binding_parent(io_executors_test_t, io_t, 1);
config_binding_ctor(io_t, io_executors_test_t);
} // namespace io_ring_sender

} // namespace phantom
