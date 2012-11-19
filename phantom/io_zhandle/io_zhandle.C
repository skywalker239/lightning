// vim: set tabstop=4 expandtab:
#include <phantom/io_zhandle/io_zhandle.H>
#include <phantom/io_zhandle/zoo_util.H>
#include <phantom/io_zclient/io_zclient.H>

#include <pd/base/exception.H>
#include <pd/base/log.H>

#include <phantom/module.H>

namespace phantom {

MODULE(io_zhandle);

void io_zhandle_t::config_t::check(const in_t::ptr_t& p) const {
    io_t::config_t::check(p);
    if(servers.size() == 0) {
        config::error(p, "servers must be set");
    }
}

io_zhandle_t::io_zhandle_t(const string_t& name, const config_t& config)
    : io_t(name, config),
      servers_z_(string_t::ctor_t(config.servers.size() + 1)(config.servers)('\0')),
      zookeeper_log_path_(config.zookeeper_log),
      zhandle_(NULL),
      zookeeper_log_(NULL),
      zookeeper_log_level_(config.zookeeper_log_level),
      connected_(false),
      session_is_fresh_(true),
      clients_(NULL)
{}

io_zhandle_t::~io_zhandle_t() {
    assert(clients_ == NULL);
}

zhandle_t* io_zhandle_t::wait() {
    bq_cond_guard_t guard(connected_cond_);
    if(!connected_) {
        if(!bq_success(connected_cond_.wait(NULL))) {
            throw exception_sys_t(log::error, errno, "connected_cond.wait: %m");
        }
    }
    assert(connected_ && zhandle_);
    return zhandle_;
}

void io_zhandle_t::register_client(io_zclient_t* client) {
    thr::spinlock_guard_t guard(clients_lock_);
    if((client->next_ = clients_)) {
        clients_->me_ = &client->next_;
    }
    *(client->me_ = &clients_) = client;
}

void io_zhandle_t::deregister_client(io_zclient_t* client) {
    thr::spinlock_guard_t guard(clients_lock_);
    if((*client->me_ = client->next_)) {
        client->next_->me_ = client->me_;
    }
    guard.relax();
    client->next_ = NULL;
    client->me_ = NULL;
}

void io_zhandle_t::init() {
    zoo_set_debug_level(zookeeper_log_level_);
    if(zookeeper_log_path_.size() != 0) {
        MKCSTR(log_path_z, zookeeper_log_path_);
        zookeeper_log_ = fopen(log_path_z, "a");
        if(zookeeper_log_ == NULL) {
            throw exception_sys_t(log::error, errno, "fopen: %m");
        }
        zoo_set_log_stream(zookeeper_log_);
    }

    bq_cond_guard_t guard(connected_cond_);
    do_connect();
}

void io_zhandle_t::fini() {
    bq_cond_guard_t guard(connected_cond_);
    do_shutdown();
    guard.relax();
    if(zookeeper_log_) {
        fflush(zookeeper_log_);
        fclose(zookeeper_log_);
    }
}

void io_zhandle_t::run() {
}

void io_zhandle_t::stat(out_t&, bool) {
}

void io_zhandle_t::do_connect() {
    const int kRecvTimeout = 5000;

    assert(zhandle_ == NULL);
    log_debug("io_zhandle connecting to %s", servers_z_.ptr());
    zhandle_ = zookeeper_init(servers_z_.ptr(),
                              &io_zhandle_t::global_watcher,
                              kRecvTimeout,
                              NULL,
                              this,
                              0);
    if(!zhandle_) {
        throw exception_sys_t(log::error, errno, "zookeeper_init: %m");
    }
}

void io_zhandle_t::do_shutdown() {
    if(zhandle_) {
        int rc = zookeeper_close(zhandle_);
        zhandle_ = NULL;
        if(rc != ZOK) {
            log_warning("zookeeper_close returned %d: %m", rc);
        }
    }
    connected_ = false;
    session_is_fresh_ = true;
}

void io_zhandle_t::new_session_notify() {
    thr::spinlock_guard_t guard(clients_lock_);

    for(io_zclient_t* client = clients_; client; client = client->next_) {
        client->new_session();
    }
}

void io_zhandle_t::global_watcher(zhandle_t*,
                                  int type,
                                  int state,
                                  const char* path,
                                  void* ctx)
{
    io_zhandle_t* iozh = reinterpret_cast<io_zhandle_t*>(ctx);
    log_debug("io_zhandle(%p): (%s, %s) at '%s'",
              iozh,
              zoo_util::event_string(type),
              zoo_util::state_string(state),
              path);

    if(type == ZOO_SESSION_EVENT) {
        if(state == ZOO_CONNECTED_STATE) {
            bq_cond_guard_t guard(iozh->connected_cond_);
            assert(!iozh->connected_);
            iozh->connected_ = true;
            iozh->connected_cond_.send(true);
            log_debug("zhandle(%p): connected.", iozh);
            if(iozh->session_is_fresh_) {
                iozh->new_session_notify();
                iozh->session_is_fresh_ = false;
            }
        } else if(state == ZOO_CONNECTING_STATE) {
            bq_cond_guard_t guard(iozh->connected_cond_);
            assert(iozh->connected_);
            log_debug("zhandle(%p): disconnected.", iozh);
            iozh->connected_ = false;
        } else if(state == ZOO_EXPIRED_SESSION_STATE) {
            bq_cond_guard_t guard(iozh->connected_cond_);
            log_debug("zhandle(%p): session expired.", iozh);
            iozh->do_shutdown();
            iozh->do_connect();
        } else {
            log_error("Unknown state %s", zoo_util::state_string(state));
            fatal("unknown state");
        }
    } else {
        log_error("Spurious event (%s, %s) at '%s' at global watcher",
                  zoo_util::event_string(type),
                  zoo_util::state_string(state),
                  path);
        fatal("Unhandled zookeeper event at global watcher");
    }
}

namespace io_zhandle {
config_enum_sname(ZooLogLevel);
config_enum_value(ZooLogLevel, ZOO_LOG_LEVEL_ERROR);
config_enum_value(ZooLogLevel, ZOO_LOG_LEVEL_WARN);
config_enum_value(ZooLogLevel, ZOO_LOG_LEVEL_INFO);
config_enum_value(ZooLogLevel, ZOO_LOG_LEVEL_DEBUG);
config_binding_sname(io_zhandle_t);
config_binding_value(io_zhandle_t, servers);
config_binding_value(io_zhandle_t, zookeeper_log);
config_binding_value(io_zhandle_t, zookeeper_log_level);
config_binding_parent(io_zhandle_t, io_t, 1);
config_binding_ctor(io_t, io_zhandle_t);
}  // namespace io_zhandle

}  // namespace phantom
