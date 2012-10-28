#include <phantom/io_zcluster_status/io_zcluster_status.H>

#include <phantom/io_zhandle/zoo_util.H>

#include <pd/base/exception.H>
#include <pd/base/log.H>

#include <pd/zookeeper/bq_zookeeper.H>

#include <phantom/module.H>

namespace phantom {

MODULE(io_zcluster_status);

string_t io_zcluster_status_t::status_node_full_path() const {
    return string_t::ctor_t(path_.size() + 16)(path_)('/').print(host_id_);
}

string_t io_zcluster_status_t::make_full_path(const string_t& node_name) const {
    return string_t::ctor_t(path_.size() + node_name.size() + 1)(path_)('/')(node_name);
}

bool io_zcluster_status_t::parse_host_id(const string_t& path, int* host_id) {
    in_t::ptr_t p(path);
    in_t::ptr_t p0(p);
    size_t limit = path.size();
    while(p.scan("/", 1, limit)) {
        p0 = ++p;
    }
    return p0.parse<int>(*host_id);
}

class io_zcluster_status_t::get_item_t : public io_zclient_t::todo_item_t {
public:
    get_item_t(io_zcluster_status_t* zcluster_status,
               const string_t& path,
               int session)
        : todo_item_t(zcluster_status),
          zcluster_status_(*zcluster_status),
          path_(path),
          session_(session)
    {}
private:
    void apply_status(int host_id, const string_t& value) {
        bq_cond_guard_t guard(zcluster_status_.state_cond_);
        if(zcluster_status_.current_status_session_ > session_) {
            log_info("get_item_t::apply_status: session mismatch (%d, %d)",
                     session_,
                     zcluster_status_.current_status_session_);
            return;
        }
        ref_t<host_status_list_t> new_status =
            zcluster_status_.current_status_->amend_host(host_id, value);
        zcluster_status_.current_status_ = new_status;
        guard.relax();
        zcluster_status_.signal_listeners(new_status);
    }

    virtual void apply() {
        int host_id = -1;
        if(!zcluster_status_.parse_host_id(path_, &host_id)) {
            MKCSTR(path_z, path_);
            log_info("get_item_t::apply: cannot get host id from path '%s'", path_z);
            return;
        }
        
        zhandle_guard_t zguard(zcluster_status_.zhandle_);
        zhandle_t* zh = zcluster_status_.zhandle_.wait();
        while(true) {
            string_t value;
            struct Stat st;

            int rc = bq_zoo_wget(zh,
                                 path_,
                                 &io_zcluster_status_t::node_watch,
                                 &zcluster_status_,
                                 &value,
                                 &st);
            switch(rc) {
                case ZOK:
                {
                    MKCSTR(path_z, path_);
                    MKCSTR(value_z, value);
                    log_info("get_item_t::apply: '%s'(%d) = '%s'", path_z, host_id, value_z);
                    apply_status(host_id, value);
                    return;
                    break;
                }
                case ZNONODE:
                {
                    MKCSTR(path_z, path_);
                    log_info("get_item_t::apply: node '%s' has gone away", path_z);
                    break;
                }
                case ZINVALIDSTATE:
                    log_info("get_item_t::apply: ZINVALIDSTATE, aborting");
                    return;
                    break;
                case ZCONNECTIONLOSS: case ZOPERATIONTIMEOUT: case ZCLOSING:
                    log_info("get_item_t::apply: retry (%s)",
                             zerror(rc));
                    break;
                default:
                    log_error("get_item_t::apply: unknown rc %d (%s)",
                              rc,
                              zerror(rc));
                    fatal("unknown state in get_item_t::apply");
            }
        }
    }

    io_zcluster_status_t& zcluster_status_;
    const string_t path_;
    int session_;
};

class io_zcluster_status_t::set_item_t : public io_zclient_t::todo_item_t {
public:
    set_item_t(io_zcluster_status_t* zcluster_status,
               const string_t& value,
               int set_number)
        : todo_item_t(zcluster_status),
          zcluster_status_(*zcluster_status),
          value_(value),
          set_number_(set_number)
    {}

private:
    virtual void apply() {
        {
            bq_cond_guard_t guard(zcluster_status_.state_cond_);
            if(zcluster_status_.last_successful_set_number_ > set_number_) {
                log_info("set_item_t::apply: set number too old (%d, %d)",
                         set_number_,
                         zcluster_status_.last_successful_set_number_);
                return;
            }
        }

        zhandle_guard_t zguard(zcluster_status_.zhandle_);
        zhandle_t* zh = zcluster_status_.zhandle_.wait();
        while(true) {
            struct Stat stat;
            int rc = bq_zoo_set(zh,
                                zcluster_status_.status_node_full_path(),
                                value_,
                                -1,
                                &stat);
            switch(rc) {
                case ZOK:
                {
                    MKCSTR(value_z, value_);
                    log_info("set_item_t::apply: set to '%s'", value_z);
                    bq_cond_guard_t guard(zcluster_status_.state_cond_);
                    zcluster_status_.last_successful_set_number_ = set_number_;
                    return;
                    break;
                }
                case ZNONODE:
                {
                    log_info("set_item_t::apply: ZNONODE, retrying");
                    //! Need to reschedule because this item blocks the possible pending create.
                    zcluster_status_.schedule(new set_item_t(&zcluster_status_, value_, set_number_));
                    return;
                    break;
                }
                case ZINVALIDSTATE:
                {
                    log_info("set_item_t::apply: ZINVALIDSTATE, aborting");
                    return;
                    break;
                }
                case ZCONNECTIONLOSS: case ZOPERATIONTIMEOUT: case ZCLOSING:
                    log_info("set_item_t::apply: retry (%s)",
                             zerror(rc));
                    break;
                default:
                    log_error("set_item_t::apply: unknown rc %d (%s)",
                              rc,
                              zerror(rc));
                    fatal("unknown state in set_item_t::apply");
            }
        }
    }

    io_zcluster_status_t& zcluster_status_;
    const string_t value_;
    int set_number_;
};

class io_zcluster_status_t::new_session_item_t : public io_zclient_t::todo_item_t {
public:
    new_session_item_t(io_zcluster_status_t* zcluster_status,
                       int session_number,
                       const string_t& our_status,
                       bool create_node)
        : todo_item_t(zcluster_status),
          zcluster_status_(*zcluster_status),
          session_number_(session_number),
          our_status_(our_status),
          create_node_(create_node),
          zhandle_(NULL),
          new_status_(new host_status_list_t)
    {}
private:
    bool create_node() {
        while(true) {
            string_t result;
            int rc = bq_zoo_create(zhandle_,
                                   zcluster_status_.status_node_full_path(),
                                   our_status_,
                                   &ZOO_OPEN_ACL_UNSAFE,
                                   ZOO_EPHEMERAL,
                                   &result);
            switch(rc) {
                case ZOK:
                {
                    MKCSTR(result_z, result);
                    log_info("new_session_item::create_node: created '%s'",
                             result_z);
                    return true;
                    break;
                }
                case ZINVALIDSTATE:
                {
                    log_info("new_session_item::create_node: ZINVALIDSTATE, aborting");
                    return false;
                    break;
                }
                case ZCONNECTIONLOSS: case ZOPERATIONTIMEOUT: case ZCLOSING:
                {
                    log_info("new_session_item::create_node: retry (%s)",
                             zerror(rc));
                    break;
                }
                default:
                {
                    log_error("new_session_item::create_node: unknown rc %d (%s)", rc, zerror(rc));
                    fatal("unknown state in new_session_item::create_node");
                    break;
                }
            }
        }
    }

    bool get_children() {
        while(true) {
            int rc = bq_zoo_wget_children(zhandle_,
                                          zcluster_status_.path_,
                                          &io_zcluster_status_t::children_watch,
                                          &zcluster_status_,
                                          &children_);
            switch(rc) {
                case ZOK:
                    log_info("new_session_item::get_children: got children list");
                    return true;
                    break;
                case ZINVALIDSTATE:
                    log_info("new_session_item::get_children: ZINVALIDSTATE, aborting");
                    return false;
                    break;
                case ZCONNECTIONLOSS: case ZOPERATIONTIMEOUT: case ZCLOSING:
                    log_info("new_session_item::get_children: retry (%s)",
                             zerror(rc));
                    break;
                default:
                    log_error("new_session_item::get_children: unknown rc %d (%s)",
                              rc,
                              zerror(rc));
                    fatal("unknown state in new_session_item::get_children");
            }
        }
    }

    bool parse_host_id(const string_t& node, int* result) {
        in_t::ptr_t p(node);
        return p.parse<int>(*result);
    }

    bool fetch_children() {
        for(auto i = children_.begin(); i != children_.end(); ++i) {
            int host_id = -1;
            if(!parse_host_id(*i, &host_id)) {
                MKCSTR(node_z, *i);
                log_info("failed to parse host id from '%s'", node_z);
                return false;
            }

            string_t result;
            bool exists = false;
            if(!fetch_item(*i, &result, &exists)) {
                log_info("new_session_item:fetch_children failed");
                return false;
            }
            if(exists) {
                new_status_ = new_status_->amend_host(host_id, result);
            }
        }
        return true;
    }

    bool fetch_item(const string_t& name, string_t* result, bool* exists) {
        while(true) {
            const string_t path = zcluster_status_.make_full_path(name);

            struct Stat st;
            int rc = bq_zoo_wget(zhandle_,
                                 path,
                                 &io_zcluster_status_t::node_watch,
                                 &zcluster_status_,
                                 result,
                                 &st);
            switch(rc) {
                case ZOK:
                {
                    MKCSTR(name_z, name);
                    MKCSTR(value_z, *result);
                    log_info("new_session_item::fetch_item(%s): '%s'",
                             name_z,
                             value_z);
                    *exists = true;
                    return true;
                    break;
                }
                case ZNONODE:
                {
                    MKCSTR(name_z, name);
                    log_info("new_session_item::fetch_item: node '%s' has gone away",
                    name_z);
                    *exists = false;
                    return true;
                }
                case ZINVALIDSTATE:
                    log_info("new_session_item::fetch_item: ZINVALIDSTATE, aborting");
                    return false;
                    break;
                case ZCONNECTIONLOSS: case ZOPERATIONTIMEOUT: case ZCLOSING:
                    log_info("new_session_item::fetch_item: retry (%s)", zerror(rc));
                    break;
                default:
                    log_error("new_session_item::fetch_item: unknown rc %d (%s)",
                              rc,
                              zerror(rc));
                    fatal("new_session_item::fetch_item: unknown state");
            }
        }
    }

    virtual void apply() {
        zhandle_guard_t zguard(zcluster_status_.zhandle_);

        zhandle_ = zcluster_status_.zhandle_.wait();
        zguard.relax();

        if(create_node_) {
            if(!create_node()) {
                return;
            }
        }

        if(!get_children()) {
            return;
        }
        if(!fetch_children()) {
            return;
        }

        bq_cond_guard_t guard(zcluster_status_.state_cond_);
        if(zcluster_status_.current_status_session_ > session_number_) {
            log_info("new_session_item: session %d, obj has %d, aborting",
                     session_number_,
                     zcluster_status_.current_status_session_);
            return;
        }
        zcluster_status_.current_status_ = new_status_;
        zcluster_status_.current_status_session_ = session_number_;
        zcluster_status_.state_cond_.send(true);
        guard.relax();
        zcluster_status_.signal_listeners(new_status_);
    }

    io_zcluster_status_t& zcluster_status_;
    int session_number_;
    const string_t our_status_;
    bool create_node_;
    zhandle_t* zhandle_;
    std::list<string_t> children_;
    ref_t<host_status_list_t> new_status_;
};

io_zcluster_status_t::config_t::config_t()
    : host_id(-1)
{}

void io_zcluster_status_t::config_t::check(const in_t::ptr_t& p) const {
    io_zclient_t::config_t::check(p);

    if(host_id == -1) {
        config::error(p, "host_id must be set");
    }
    if(path.size() == 0) {
        config::error(p, "path must be set");
    }
}

io_zcluster_status_t::io_zcluster_status_t(const string_t& name, const config_t& config)
    : io_zclient_t(name, config),
      host_id_(config.host_id),
      path_(config.path),
      current_status_(new host_status_list_t),
      current_status_session_(-1),
      current_session_(0),
      last_set_status_(string_t::ctor_t(3)(CSTR("{}"))),
      last_successful_set_number_(-1),
      current_set_number_(0),
      listeners_(NULL)
{}

io_zcluster_status_t::~io_zcluster_status_t() {
    assert(listeners_ == NULL);
}

void io_zcluster_status_t::add_listener(status_listener_t* listener) {
    thr::spinlock_guard_t guard(listeners_lock_);

    if((listener->next_ = listeners_)) {
        listeners_->me_ = &listener->next_;
    }
    *(listener->me_ = &listeners_) = listener;
}

void io_zcluster_status_t::remove_listener(status_listener_t* listener) {
    thr::spinlock_guard_t guard(listeners_lock_);

    if((*listener->me_ = listener->next_)) {
        listener->next_->me_ = listener->me_;
    }

    guard.relax();
    listener->next_ = NULL;
    listener->me_ = NULL;
}

ref_t<host_status_list_t> io_zcluster_status_t::cluster_status() const {
    bq_cond_guard_t guard(state_cond_);

    while(current_status_session_ == -1) {
        if(!bq_success(state_cond_.wait(NULL))) {
            throw exception_sys_t(log::error, errno, "state_cond_.wait: %m");
        }
    }

    ref_t<host_status_list_t> status = current_status_;
    return status;
}

void io_zcluster_status_t::update(const string_t& new_status) {
    bq_cond_guard_t guard(state_cond_);
    int set_number = current_set_number_++;
    last_set_status_ = new_status;
    guard.relax();

    MKCSTR(status_z, new_status);
    log_info("io_zcluster_status_t::update ('%s', %d)", status_z, set_number);
    schedule(new set_item_t(this, new_status, set_number));
}

void io_zcluster_status_t::new_session() {
    bq_cond_guard_t guard(state_cond_);
    int session_number = current_session_++;
    log_info("io_zcluster_status_t::new_session(%d)", session_number);
    schedule(new new_session_item_t(this, session_number, last_set_status_, true));
}

void io_zcluster_status_t::node_watch(zhandle_t* /* zh */,
                                      int type,
                                      int state,
                                      const char* path,
                                      void* watcherCtx)
{
    if(state != ZOO_CONNECTED_STATE ||
      (state == ZOO_CONNECTED_STATE && type == ZOO_SESSION_EVENT))
    {
        log_info("zcluster_status::node_watch ignoring state (%s, %s)", zoo_util::event_string(type), zoo_util::state_string(state));
        return;
    }

    io_zcluster_status_t* zcluster_status = (io_zcluster_status_t*) watcherCtx;
    size_t path_len = strlen(path);
    string_t path_str = string_t::ctor_t(path_len)(str_t(path, path_len));

    if(type == ZOO_DELETED_EVENT) {
        log_info("doing nothing for ZOO_DELETED_EVENT at %s", path);
    } else if(type == ZOO_CHANGED_EVENT) {
        bq_cond_guard_t guard(zcluster_status->state_cond_);
        zcluster_status->schedule(new get_item_t(zcluster_status, path_str, zcluster_status->current_status_session_));
    } else {
        log_error("Unhandled event (%s, %s) at '%s'",
                  zoo_util::event_string(type),
                  zoo_util::state_string(state),
                  path);
        fatal("unknown state in node_watch");
    }
}

void io_zcluster_status_t::children_watch(zhandle_t* /* zh */,
                                          int type,
                                          int state,
                                          const char* path,
                                          void* watcherCtx)
{
    if(state != ZOO_CONNECTED_STATE ||
       (state == ZOO_CONNECTED_STATE && type == ZOO_SESSION_EVENT))
    {
        log_info("zcluster_status::children_watch ignoring state (%s, %s)", zoo_util::event_string(type), zoo_util::state_string(state));
        return;
    }

    io_zcluster_status_t* zcluster_status = (io_zcluster_status_t*) watcherCtx;

    if(type == ZOO_CHILD_EVENT) {
        bq_cond_guard_t guard(zcluster_status->state_cond_);
        int session_number = zcluster_status->current_session_++;
        zcluster_status->schedule(new new_session_item_t(zcluster_status, session_number, zcluster_status->last_set_status_, false));
    } else {
        log_error("Unhandled event (%s, %s) at '%s'",
                  zoo_util::event_string(type),
                  zoo_util::state_string(state),
                  path);
        fatal("unknown state in children_watch");
    }
}

void io_zcluster_status_t::signal_listeners(ref_t<host_status_list_t> status) {
    thr::spinlock_guard_t guard(listeners_lock_);
    for(status_listener_t* l = listeners_; l; l = l->next_) {
        l->notify(status);
    }
}

namespace io_zcluster_status {
config_binding_sname(io_zcluster_status_t);
config_binding_value(io_zcluster_status_t, host_id);
config_binding_value(io_zcluster_status_t, path);
config_binding_parent(io_zcluster_status_t, io_zclient_t, 1);
config_binding_ctor(io_t, io_zcluster_status_t);
}  // namespace io_zcluster_status

}  // namespace phantom
