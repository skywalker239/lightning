#include <phantom/io_zmaster_test/io_zmaster_test.H>

#include <pd/base/exception.H>
#include <pd/base/random.H>
#include <pd/base/time.H>

#include <pd/bq/bq_job.H>
#include <pd/bq/bq_util.H>

#include <phantom/module.H>


namespace phantom {

MODULE(io_zmaster_test);

void io_zmaster_test_t::config_t::check(const in_t::ptr_t& ptr) const {
    io_t::config_t::check(ptr);
    if(!zmaster) {
        config::error(ptr, "zmaster must be set");
    }
}

io_zmaster_test_t::io_zmaster_test_t(const string_t& name,
                                       const config_t& config)
    : io_t(name, config),
      zmaster_(*config.zmaster)
{
}

io_zmaster_test_t::~io_zmaster_test_t() {
}

void io_zmaster_test_t::init() {}

void io_zmaster_test_t::run() {
    while(true) {
        zmaster_.activate();
        interval_t master_time = (random_D() % 30) * interval_second;
        bq_sleep(&master_time);
        zmaster_.deactivate();
    }
}

void io_zmaster_test_t::fini() {}

namespace io_zmaster_test {
config_binding_sname(io_zmaster_test_t);
config_binding_value(io_zmaster_test_t, zmaster);
config_binding_parent(io_zmaster_test_t, io_t, 1);
config_binding_ctor(io_t, io_zmaster_test_t);
}  // namespace io_zmaster_test

}  // namespace phantom
