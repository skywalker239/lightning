// Copyright (C) 2012, Alexander Kharitonov <alexander.kharitonov@gmail.com>
// Copyright (C) 2012, YANDEX LLC.
// This code may be distributed under the terms of the GNU GPL v3.
// See ‘http://www.gnu.org/licenses/gpl.html’.
// vim: set tabstop=4 expandtab:
#pragma once


#include <phantom/io_zcluster_status/io_zcluster_status.H>
#include <phantom/io_zcluster_status/status_listener.H>

#include <phantom/io.H>

#include <pd/base/config.H>

namespace phantom {

class io_zcluster_status_test_t : public io_t, public status_listener_t {
public:
    struct config_t : public io_t::config_t {
        config::objptr_t<io_zcluster_status_t> cluster_status;

        config_t() {}
        ~config_t() throw() {}

        void check(const in_t::ptr_t& p) const;
    };

    io_zcluster_status_test_t(const string_t& name, const config_t& config);

private:
    virtual void init();
    virtual void fini();
    virtual void run();
    virtual void stat(out_t&, bool) {}

    io_zcluster_status_t& cluster_status_;

    const string_t print_status(ref_t<host_status_t> status);

    virtual void notify(ref_t<host_status_list_t> status);
};

}  // namespace phantom
