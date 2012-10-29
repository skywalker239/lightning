#include <phantom/io_toy_zk_client/io_toy_zk_client.H>

#include <pd/base/exception.H>
#include <pd/base/time.H>

#include <pd/bq/bq_job.H>
#include <pd/bq/bq_util.H>

#include <phantom/module.H>


namespace phantom {

MODULE(io_toy_zk_client);

void io_toy_zk_client_t::config_t::check(const in_t::ptr_t& ptr) const {
    io_t::config_t::check(ptr);
    if(!zconf) {
        config::error(ptr, "zconf must be set");
    }
    if(set_key.size() == 0) {
        config::error(ptr, "set_key must be set");
    }
}

io_toy_zk_client_t::io_toy_zk_client_t(const string_t& name,
                                       const config_t& config)
    : io_t(name, config),
      zconf_(*config.zconf),
      set_var_(config.set_key, &zconf_),
      snapshot_var_(config.snapshot_var, &zconf_)
{
    log_info("io_zk_client_t ctor");
    for(config::list_t<string_t>::ptr_t p = config.keys.ptr();
        p;
        ++p)
    {
        MKCSTR(key_z, p.val());
        log_info("creating var %s", key_z);
        vars_.push_back(new simple_var_t<string_t>(p.val(), &zconf_));
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

    bq_job_t<typeof(&io_toy_zk_client_t::snap_loop)>::create(
        STRING("snapshot"),
        scheduler.bq_thr(),
        *this,
        &io_toy_zk_client_t::snap_loop);

/*
    string_t set_job_name = string_t::ctor_t(set_var_.key().size() + 3 + 2)
                                (CSTR("set["))(set_var_.key())(']');
    bq_job_t<typeof(&io_toy_zk_client_t::set_loop)>::create(
        set_job_name,
        scheduler.bq_thr(),
        *this,
        &io_toy_zk_client_t::set_loop);
*/
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

void io_toy_zk_client_t::snap_loop() {
    while(true) {
        int ver = snapshot_var_.update();
        log_info("snap_loop: update returned %d", ver);
        MKCSTR(loc_z, snapshot_var_.location());
        log_info("snapshot is (%ld, %s)", snapshot_var_.snapshot_version(), loc_z);
        snapshot_var_.wait(ver);
    }
}

void io_toy_zk_client_t::set_loop() {
    int c = 0;
    while(true) {
        string_t new_value = string_t::ctor_t(1024).print(c++);
        int v = set_var_.update();
        bool res = set_var_.set(new_value, v);
        log_info("set with version %d: %d", v, int(res));
        auto sleep_interval = interval_second;
        if(bq_sleep(&sleep_interval) < 0) {
            throw exception_sys_t(log::error,
                                  errno,
                                  "bq_sleep: %m");
        }
    }
}


void io_toy_zk_client_t::fini() {}

namespace io_toy_zk_client {
config_binding_sname(io_toy_zk_client_t);
config_binding_value(io_toy_zk_client_t, zconf);
config_binding_value(io_toy_zk_client_t, keys);
config_binding_value(io_toy_zk_client_t, snapshot_var);
config_binding_value(io_toy_zk_client_t, set_key);
config_binding_parent(io_toy_zk_client_t, io_t, 1);
config_binding_ctor(io_t, io_toy_zk_client_t);
}  // namespace io_toy_zk_client

}  // namespace phantom
