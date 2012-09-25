#include <phantom/io_toy_zk_client/io_toy_zk_client.H>

#include <pd/base/time.H>

#include <pd/bq/bq_job.H>
#include <pd/bq/bq_util.H>

#include <phantom/module.H>


namespace phantom {

MODULE(io_toy_zk_client);

void io_toy_zk_client_t::config_t::check(const in_t::ptr_t& ptr) const {
    io_t::config_t::check(ptr);
    if(!zookeeper) {
        config::error(ptr, "zookeeper must be set");
    }
    if(set_key.size() == 0) {
        config::error(ptr, "set_key must be set");
    }
}

io_toy_zk_client_t::io_toy_zk_client_t(const string_t& name,
                                       const config_t& config)
    : io_t(name, config),
      zookeeper_(*config.zookeeper),
      set_var_(config.set_key, &zookeeper_)
{
    log_info("io_zk_client_t ctor");
    for(config::list_t<string_t>::ptr_t p = config.keys.ptr();
        p;
        ++p)
    {
        MKCSTR(key_z, p.val());
        log_info("creating var %s", key_z);
        vars_.push_back(new var_base_t(p.val(), &zookeeper_));
    }
}

io_toy_zk_client_t::~io_toy_zk_client_t() {
    for(size_t i = 0; i < vars_.size(); ++i) {
        delete vars_[i];
    }
}

void io_toy_zk_client_t::init() {}

void io_toy_zk_client_t::run() {
    for(size_t i = 0; i < vars_.size(); ++i) {
        string_t job_name = string_t::ctor_t(vars_[i]->key().size() + 5 + 2)
                                (CSTR("watch["))(vars_[i]->key())(']');

        bq_job_t<typeof(&io_toy_zk_client_t::loop)>::create(
            job_name,
            scheduler.bq_thr(),
            *this,
            &io_toy_zk_client_t::loop,
            *vars_[i]);
    }

    string_t set_job_name = string_t::ctor_t(set_var_.key().size() + 3 + 2)
                                (CSTR("set["))(set_var_.key())(']');
    bq_job_t<typeof(&io_toy_zk_client_t::set_loop)>::create(
        set_job_name,
        scheduler.bq_thr(),
        *this,
        &io_toy_zk_client_t::set_loop);
}

void io_toy_zk_client_t::loop(var_base_t& var) {
    MKCSTR(key_z, var.key());
    log_info("loop(%s)", key_z);
    while(true) {
        int ver = var.update();
        MKCSTR(value_z, var.value_string());
        log_info("'%s' = (%d, '%s')", key_z, ver, value_z);
        var.wait(ver);
    }
}

void io_toy_zk_client_t::set_loop() {
    int c = 0;
    while(true) {
        string_t new_value = string_t::ctor_t(1024).print(c++);
        set_var_.set(new_value);
        auto sleep_interval = interval_second;
        bq_sleep(&sleep_interval);
    }
}


void io_toy_zk_client_t::fini() {}

namespace io_toy_zk_client {
config_binding_sname(io_toy_zk_client_t);
config_binding_value(io_toy_zk_client_t, zookeeper);
config_binding_value(io_toy_zk_client_t, keys);
config_binding_value(io_toy_zk_client_t, set_key);
config_binding_parent(io_toy_zk_client_t, io_t, 1);
config_binding_ctor(io_t, io_toy_zk_client_t);
}  // namespace io_toy_zk_client

}  // namespace phantom
