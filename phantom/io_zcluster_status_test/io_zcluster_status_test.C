// Copyright (C) 2012, Alexander Kharitonov <alexander.kharitonov@gmail.com>
// Copyright (C) 2012, YANDEX LLC.
// This code may be distributed under the terms of the GNU GPL v3.
// See ‘http://www.gnu.org/licenses/gpl.html’.
// vim: set tabstop=4 expandtab:
#include <phantom/io_zcluster_status_test/io_zcluster_status_test.H>

#include <pd/base/exception.H>
#include <pd/base/random.H>
#include <pd/base/string.H>
#include <pd/bq/bq_util.H>

#include <phantom/module.H>

namespace phantom {

MODULE(io_zcluster_status_test);

void io_zcluster_status_test_t::config_t::check(const in_t::ptr_t& p) const {
    if(!cluster_status) {
        config::error(p, "cluster_status must be set");
    }
}

io_zcluster_status_test_t::io_zcluster_status_test_t(const string_t& name, const config_t& config)
    : io_t(name, config),
      cluster_status_(*config.cluster_status)
{}

void io_zcluster_status_test_t::init() {
    cluster_status_.add_listener(this);
}

void io_zcluster_status_test_t::fini() {
    cluster_status_.remove_listener(this);
}

void io_zcluster_status_test_t::run() {
    int c = 0;
    while(true) {
        string_t status = string_t::ctor_t(16).print(c++);
        cluster_status_.update(status);
        interval_t to_sleep = (random_U() % 10) * interval_second;
        if(bq_sleep(&to_sleep) < 0) {
            throw exception_sys_t(log::error, errno, "bq_sleep: %m");
        }
    }
}

const string_t io_zcluster_status_test_t::print_status(ref_t<host_status_t> status) {
    if(!status) {
        return string_t::empty;
    }
    return string_t::ctor_t(1024)(' ').print(status->host_id())(':')(' ')('{')(status->status())('}')(' ')(print_status(status->next()))('\0');
}

void io_zcluster_status_test_t::notify(ref_t<host_status_list_t> status) {
    if(!status->head()) {
        log_info("NOTIFY: (null)");
    } else {
        log_info("NOTIFY:");
        ref_t<host_status_t> head = status->head();
        while(head) {
            MKCSTR(status_z, head->status());
            log_info("%d: %s", head->host_id(), status_z);
            head = head->next();
        }
    }
}

namespace io_zcluster_status_test {
config_binding_sname(io_zcluster_status_test_t);
config_binding_value(io_zcluster_status_test_t, cluster_status);
config_binding_parent(io_zcluster_status_test_t, io_t, 1);
config_binding_ctor(io_t, io_zcluster_status_test_t);
}  // namespace io_zcluster_status_test

}  // namespace phantom
