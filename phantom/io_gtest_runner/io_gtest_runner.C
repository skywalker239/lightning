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

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace phantom {

MODULE(io_gtest_runner);

class io_gtest_runner_t : public io_t {
public:
    struct config_t : public io_t::config_t {
        void check(const in_t::ptr_t& ) const {}
    };

    io_gtest_runner_t(const string_t& name, const config_t& c)
        : io_t(name, c) {}

    virtual void init() {
        int argc = 1;
        char const* argv[] = { "gtest_runner", NULL };

        testing::InitGoogleMock(&argc, const_cast<char**>(argv));
    }

    virtual void fini() {}

    virtual void stat(pd::out_t&, bool) {}

    virtual void run() {
        if(0 == RUN_ALL_TESTS()) {
            log_info("Tests finished OK");
            log_info("Sending SIGQUIT");
            kill(getpid(), SIGQUIT);
        } else {
            log_info("Tests finished BAD");
            log_info("Sending SIGKILL");
            kill(getpid(), SIGKILL);
        }

    }

private:
};

namespace io_gtest_runner {
config_binding_sname(io_gtest_runner_t);

config_binding_parent(io_gtest_runner_t, io_t, 1);
config_binding_ctor(io_t, io_gtest_runner_t);
} // namespace io_gtest_runner

} // namespace phantom
