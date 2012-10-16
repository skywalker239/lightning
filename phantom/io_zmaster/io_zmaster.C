#include <phantom/io_zmaster/io_zmaster.H>

#include <pd/base/exception.H>
#include <pd/base/log.H>

#include <phantom/module.H>

namespace phantom {

MODULE(io_zmaster);

namespace {

template<typename T>
class free_guard_t {
public:
    free_guard_t(T* ptr)
        : ptr_(ptr)
    {}

    ~free_guard_t() {
        free((void*)ptr_);
    }
private:
    T* ptr_;
};

template<typename T>
class delete_guard_t {
public:
    delete_guard_t(T* ptr)
        : ptr_(ptr)
    {}

    ~delete_guard_t() {
        delete ptr_;
    }
private:
    T* ptr_;
};

class Stat_guard_t {
public:
    Stat_guard_t(const struct Stat* stat)
        : stat_(stat)
    {}

    ~Stat_guard_t() {
        deallocate_Stat((Stat*) stat_);
    }
private:
    const struct Stat* stat_;
};

class String_vector_guard_t {
public:
    String_vector_guard_t(const struct String_vector* sv)
        : sv_(sv)
    {}

    ~String_vector_guard_t() {
        deallocate_String_vector((String_vector*) sv_);
    }
private:
    const struct String_vector* sv_;
};

const char* state_string(int state) {
    if(state == ZOO_EXPIRED_SESSION_STATE) {
            return "ZOO_EXPIRED_SESSION_STATE";
    } else if(state == ZOO_AUTH_FAILED_STATE) {
            return "ZOO_AUTH_FAILED_STATE";
    } else if(state == ZOO_CONNECTING_STATE) {
            return "ZOO_CONNECTING_STATE";
    } else if(state == ZOO_ASSOCIATING_STATE) {
            return "ZOO_ASSOCIATING_STATE";
    } else if(state == ZOO_CONNECTED_STATE) {
            return "ZOO_CONNECTED_STATE";
    } else {
            return "ZOO_UNKNOWN_STATE";
    }
}

const char* event_string(int type) {
    if(type == ZOO_CREATED_EVENT) {
            return "ZOO_CREATED_EVENT";
    } else if(type == ZOO_DELETED_EVENT) {
            return "ZOO_DELETED_EVENT";
    } else if(type == ZOO_CHANGED_EVENT) {
            return "ZOO_CHANGED_EVENT";
    } else if(type == ZOO_CHILD_EVENT) {
            return "ZOO_CHILD_EVENT";
    } else if(type == ZOO_SESSION_EVENT) {
            return "ZOO_SESSION_EVENT";
    } else if(type == ZOO_NOTWATCHING_EVENT) {
            return "ZOO_NOTWATCHING_EVENT";
    } else {
            return "ZOO_UNKNOWN_EVENT";
    }
}

}  // anonymous namespace

struct io_zmaster_t::callback_data_t {
    io_zmaster_t* zmaster;
    int epoch;
    callback_data_t* next;

    callback_data_t(io_zmaster_t* _zmaster, int _epoch)
        : zmaster(_zmaster),
          epoch(_epoch),
          next(NULL)
    {}

    ~callback_data_t() throw() {
        if(next) {
            delete next;
        }
    }
};

io_zmaster_t::config_t::config_t()
    : host_id(-1)
{}

void io_zmaster_t::config_t::check(const in_t::ptr_t& p) const {
    io_zclient_t::config_t::check(p);

    if(host_id == -1) {
        config::error(p, "host_id must be set");
    }
    if(election_path.size() == 0) {
        config::error(p, "election_path must be set");
    }
    if(current_master_path.size() == 0) {
        config::error(p, "current_master_path must be set");
    }
    if(!zconf) {
        config::error(p, "zconf must be set");
    }
}

io_zmaster_t::io_zmaster_t(const string_t& name, const config_t& config)
    : io_zclient_t(name, config),
      host_id_(config.host_id),
      election_path_(config.election_path),
      current_master_(config.current_master_path, config.zconf),
      state_(INACTIVE),
      current_epoch_(0)
{}

io_zmaster_t::~io_zmaster_t()
{}

const string_t io_zmaster_t::seq_node_path_base() const {
    return string_t::ctor_t(election_path_.size() + 4)
                           (election_path_)
                           ('/')('e')('_')('\0');
}

const string_t io_zmaster_t::seq_node_value() const {
    return string_t::ctor_t(16).print(host_id_);
}

const string_t io_zmaster_t::make_full_path(const char* node_name) const {
    const size_t len = strlen(node_name);
    return string_t::ctor_t(election_path_.size() + 1 + len)
                           (election_path_)
                           ('/')
                           (str_t(node_name, len));
}

void io_zmaster_t::advance_epoch() {
    assert(state_ == CANCELING);
    log_info("CANCELING(%d) -> INACTIVE(%d)",
             current_epoch_,
             current_epoch_ + 1);
    seq_node_path_ = string_t::empty;
    state_ = INACTIVE;
    ++current_epoch_;
    state_cond_.send(true);
}

class io_zmaster_t::delete_item_t : public io_zclient_t::todo_item_t {
public:
    delete_item_t(io_zmaster_t* zmaster, int epoch)
        : todo_item_t(zmaster),
          zmaster_(*zmaster),
          epoch_(epoch)
    {}

    virtual void apply() {
        log_debug("delete_item_t::apply %d", epoch_);

        zhandle_guard_t zguard(zmaster_.zhandle_);
        zhandle_t* zhandle = zmaster_.zhandle_.wait();

        bq_cond_guard_t guard(zmaster_.state_cond_);
        if(zmaster_.current_epoch_ != epoch_) {
            log_info("delete_item_t: epoch mismatch (%d, %d)",
                     zmaster_.current_epoch_,
                     epoch_);
            return;
        }
        assert(zmaster_.state_ == io_zmaster_t::CANCELING);
        MKCSTR(seq_node_path_z, zmaster_.seq_node_path_);

        int rc = zoo_adelete(zhandle,
                             seq_node_path_z,
                             -1,
                             &io_zmaster_t::delete_callback,
                             (void*) new callback_data_t(&zmaster_, epoch_));
        if(rc == ZOK) {
            log_debug("delete_item_t: CANCELING(%d), called zoo_adelete",
                     epoch_);
            return;
        } else if(rc == ZINVALIDSTATE) {
            log_debug("delete_item_t: zhandle is in ZINVALIDSTATE, nothing to do");
            zmaster_.advance_epoch();
            return;
        } else {
            log_error("zoo_adelete returned %d (%m)", rc);
            fatal("unknown state in delete_item_t");
        }
    }
private:
    io_zmaster_t& zmaster_;
    int epoch_;
};

void io_zmaster_t::delete_callback(int rc, const void* data) {
    callback_data_t* callback_data = (callback_data_t*) data;
    delete_guard_t<callback_data_t> cbguard(callback_data);

    io_zmaster_t& zmaster = *(callback_data->zmaster);
    const int epoch = callback_data->epoch;

    bq_cond_guard_t guard(zmaster.state_cond_);
    if(zmaster.current_epoch_ != epoch) {
        log_info("delete_callback: epoch mismatch (%d, %d)",
                 zmaster.current_epoch_,
                 epoch);
        return;
    }
    assert(zmaster.state_ == CANCELING);

    if(rc == ZOK) {
        log_debug("delete_callback: success");
        zmaster.advance_epoch();
    } else if(rc == ZOPERATIONTIMEOUT) {
        log_debug("delete_callback: timed out, retrying");
        guard.relax();
        new delete_item_t(&zmaster, epoch);
    } else {
        log_error("delete_callback: unknown rc %d", rc);
        fatal("unknown state in delete_callback");
    }
}

class io_zmaster_t::create_item_t : public io_zclient_t::todo_item_t {
public:
    create_item_t(io_zmaster_t* zmaster, int epoch)
        : todo_item_t(zmaster),
          zmaster_(*zmaster),
          epoch_(epoch)
    {}

    virtual void apply() {
        log_debug("create_item_t::apply %d", epoch_);

        const string_t path = zmaster_.seq_node_path_base();
        const string_t value = zmaster_.seq_node_value();

        zhandle_guard_t zguard(zmaster_.zhandle_);
        zhandle_t* zhandle = zmaster_.zhandle_.wait();

        bq_cond_guard_t guard(zmaster_.state_cond_);
        if(zmaster_.current_epoch_ != epoch_) {
            log_info("create_item_t: epoch mismatch (%d, %d)",
                     zmaster_.current_epoch_,
                     epoch_);
            return;
        }
        if(zmaster_.state_ == io_zmaster_t::CANCELING) {
            log_info("create_item_t(%d) canceled", epoch_);
            zmaster_.advance_epoch();
            return;
        }
        assert(zmaster_.state_ == io_zmaster_t::ACTIVATING);

        int rc = zoo_acreate(zhandle,
                             path.ptr(),
                             value.ptr(),
                             value.size(),
                             &ZOO_OPEN_ACL_UNSAFE, // TODO(skywalker)
                             ZOO_EPHEMERAL | ZOO_SEQUENCE,
                             &io_zmaster_t::create_callback,
                             (const void*) new callback_data_t(&zmaster_, epoch_));
        if(rc == ZOK) {
            zmaster_.state_ = io_zmaster_t::REGISTERING;
            log_info("zmaster: ACTIVATING(%d) -> REGISTERING(%d)", epoch_, epoch_);
            return;
        } else if(rc == ZINVALIDSTATE) {
            log_debug("create_item_t: zhandle is in ZINVALIDSTATE, doing nothing");
            return;
        } else {
            log_error("zoo_acreate returned %d (%m)", rc);
            fatal("unknown state in create_item_t");
        }
    }
private:
    io_zmaster_t& zmaster_;
    int epoch_;
};

class io_zmaster_t::get_children_item_t : public io_zclient_t::todo_item_t {
public:
    get_children_item_t(io_zmaster_t* zmaster, int epoch)
        : todo_item_t(zmaster),
          zmaster_(*zmaster),
          epoch_(epoch)
    {}

    void apply() {
        log_debug("get_children_item_t::apply %d", epoch_);

        zhandle_guard_t zguard(zmaster_.zhandle_);
        zhandle_t* zhandle = zmaster_.zhandle_.wait();

        bq_cond_guard_t guard(zmaster_.state_cond_);
        if(zmaster_.current_epoch_ != epoch_) {
            log_info("get_children_item_t: epoch mismatch (%d, %d)",
                     zmaster_.current_epoch_,
                     epoch_);
            return;
        }
        if(zmaster_.state_ == io_zmaster_t::CANCELING) {
            log_info("get_children_item_t(%d) canceled", epoch_);
            guard.relax();
            new delete_item_t(&zmaster_, epoch_);
            return;
        }
        assert(zmaster_.state_ == io_zmaster_t::REGISTERED);

        MKCSTR(path_z, zmaster_.election_path_);
        int rc = zoo_aget_children(zhandle,
                                   path_z,
                                   0,
                                   &io_zmaster_t::children_callback,
                                   (const void*) new callback_data_t(&zmaster_, epoch_));

        if(rc == ZOK) {
            log_info("zmaster: REGISTERED(%d) -> GETTING_CHILDREN(%d)",
                     epoch_,
                     epoch_);
            zmaster_.state_ = io_zmaster_t::GETTING_CHILDREN;
            return;
        } else if(rc == ZINVALIDSTATE) {
            log_debug("io_zmaster_t::get_children_item_t: zhandle is in ZINVALIDSTATE, doing nothing");
        } else {
            log_error("zoo_aget_children returned %d (%m)", rc);
            fatal("unknown state in zoo_aget_children");
        }
    }
private:
    io_zmaster_t& zmaster_;
    int epoch_;
};

class io_zmaster_t::watch_item_t : public io_zclient_t::todo_item_t {
public:
    watch_item_t(io_zmaster_t* zmaster, int epoch, const string_t& key)
        : todo_item_t(zmaster),
          zmaster_(*zmaster),
          epoch_(epoch),
          key_z_(string_t::ctor_t(key.size() + 1)(key)('\0'))
    {}

    virtual void apply() {
        log_debug("watch_item_t::apply %d", epoch_);

        zhandle_guard_t zguard(zmaster_.zhandle_);
        zhandle_t* zhandle = zmaster_.zhandle_.wait();

        bq_cond_guard_t guard(zmaster_.state_cond_);
        if(zmaster_.current_epoch_ != epoch_) {
            log_info("watch_item_t: epoch mismatch (%d, %d)",
                     zmaster_.current_epoch_,
                     epoch_);
            return;
        }
        if(zmaster_.state_ == io_zmaster_t::CANCELING) {
            log_info("watch_item_t(%d) canceled", epoch_);
            guard.relax();
            new delete_item_t(&zmaster_, epoch_);
            return;
        }
        assert(zmaster_.state_ == io_zmaster_t::SETTING_WATCH);

        callback_data_t* watch_context = new callback_data_t(&zmaster_, epoch_);
        callback_data_t* watch_callback_data = new callback_data_t(&zmaster_, epoch_);
        watch_callback_data->next = watch_context;
        int rc = zoo_awexists(zhandle,
                              key_z_.ptr(),
                              &io_zmaster_t::deletion_watch,
                              (void*) watch_context,
                              &io_zmaster_t::watch_callback,
                              (void*) watch_callback_data);
        if(rc == ZOK) {
            log_info("zmaster: GETTING_CHILDREN(%d) -> WAITING_WATCH(%d)",
                     epoch_,
                     epoch_);
            zmaster_.state_ = io_zmaster_t::WAITING_WATCH;
            return;
        } else if(rc == ZINVALIDSTATE) {
            log_debug("watch_item_t: zhandle in ZINVALIDSTATE, doing nothing");
        } else {
            log_error("zoo_awexists returned %d (%m)", rc);
            fatal("unknown state in watch_item_t");
        }
    }
private:
    io_zmaster_t& zmaster_;
    int epoch_;
    const string_t key_z_;
};

class io_zmaster_t::set_master_item_t : public io_zclient_t::todo_item_t {
public:
    set_master_item_t(io_zmaster_t* zmaster, int epoch)
        : todo_item_t(zmaster),
          zmaster_(*zmaster),
          epoch_(epoch)
    {}

    virtual void apply() {
        log_debug("set_master_item_t::apply %d", epoch_);

        bq_cond_guard_t guard(zmaster_.state_cond_);
        if(zmaster_.current_epoch_ != epoch_) {
            log_info("set_master_item_t: epoch mismatch (%d, %d)",
                     zmaster_.current_epoch_,
                     epoch_);
            return;
        }
        if(zmaster_.state_ == io_zmaster_t::CANCELING) {
            log_info("set_master_item_t(%d) canceled", epoch_);
            guard.relax();
            new delete_item_t(&zmaster_, epoch_);
            return;
        }
        assert(zmaster_.state_ == io_zmaster_t::MASTER);

        zmaster_.current_master_.set(zmaster_.seq_node_value(), -1);
        log_debug("set_master_item_t: success");
    }
private:
    io_zmaster_t& zmaster_;
    int epoch_;
};

void io_zmaster_t::activate() {
    log_info("io_zmaster_t::activate()");
    bq_cond_guard_t guard(state_cond_);
    do_activate(guard);
}

void io_zmaster_t::do_activate(bq_cond_guard_t& guard) {
    while(true) {
        if(state_ != INACTIVE) {
            log_info("io_zmaster_t::activate waiting for INACTIVE state");
            if(!bq_success(state_cond_.wait(NULL))) {
                throw exception_sys_t(log::error, errno, "state_cond_.wait: %m");
            }
        } else {
            break;
        }
    }
    log_info("io_zmaster_t: INACTIVE(%d) -> ACTIVATING(%d)",
             current_epoch_,
             current_epoch_);
    state_ = ACTIVATING;
    guard.relax();
    new create_item_t(this, current_epoch_);
}

void io_zmaster_t::deactivate() {
    log_info("io_zmaster_t::deactivate()");
    bq_cond_guard_t guard(state_cond_);

    if(state_ != INACTIVE) {
        state_t old_state = state_;
        state_ = CANCELING;
        if(old_state == MASTER || old_state == WATCHING) {
            new delete_item_t(this, current_epoch_);
        }
    }
}

void io_zmaster_t::create_callback(int rc, const char* value, const void* data) {
    callback_data_t* callback_data = (callback_data_t*) data;
    delete_guard_t<callback_data_t> cbguard(callback_data);
    free_guard_t<const char> value_guard(value);

    io_zmaster_t& zmaster = *(callback_data->zmaster);
    const int epoch = callback_data->epoch;

    log_info("io_zmaster_t::create_callback: rc %d value '%s'", rc, value);

    bq_cond_guard_t guard(zmaster.state_cond_);

    if(epoch != zmaster.current_epoch_) {
        log_info("io_zmaster_t::create_callback: epoch mismatch (%d, %d)",
                 zmaster.current_epoch_,
                 epoch);
        return;
    }

    if(rc == ZOK) {
        size_t seq_node_path_len = strlen(value);
        zmaster.seq_node_path_ =
            string_t::ctor_t(seq_node_path_len)(str_t(value, seq_node_path_len));
        if(zmaster.state_ == CANCELING) {
            log_info("io_zmaster_t::create_callback(success) canceled");
            new delete_item_t(&zmaster, epoch);
        } else {
            assert(zmaster.state_ == REGISTERING);
            log_info("io_zmaster_t: REGISTERING(%d) -> REGISTERED(%d)",
                     epoch,
                     epoch);
            zmaster.state_ = REGISTERED;
            guard.relax();
    
            new get_children_item_t(&zmaster, epoch);
        }
    } else if(rc == ZOPERATIONTIMEOUT) {
        if(zmaster.state_ == CANCELING) {
            log_info("io_zmaster_t::create_callback(failed) canceled");
            zmaster.advance_epoch();
        } else {
            assert(zmaster.state_ == REGISTERING);
            log_debug("io_zmaster_t::create_callback timeout, retrying");
            guard.relax();
    
            new create_item_t(&zmaster, epoch);
        }
    } else {
        log_error("io_zmaster_t::create_callback got unknown rc %d", rc);
        fatal("unknown state in create_callback");
    }
}

void io_zmaster_t::children_callback(int rc,
                                     const struct String_vector* strings,
                                     const void* data)
{
    callback_data_t* callback_data = (callback_data_t*) data;
    String_vector_guard_t svguard(strings);
    delete_guard_t<callback_data_t> cbguard(callback_data);

    io_zmaster_t& zmaster = *(callback_data->zmaster);
    const int epoch = callback_data->epoch;
    
    bq_cond_guard_t guard(zmaster.state_cond_);

    if(zmaster.current_epoch_ != epoch) {
        log_info("children_callback: epoch mismatch(%d, %d)",
                 zmaster.current_epoch_,
                 epoch);
        return;
    }
    if(zmaster.state_ == CANCELING) {
        log_info("children_callback(%d): canceled", epoch);
        guard.relax();
        new delete_item_t(&zmaster, epoch);
    }
    assert(zmaster.state_ == GETTING_CHILDREN);

    if(rc == ZOK) {
        int predecessor_index = zmaster.predecessor_index(zmaster.seq_node_path_,
                                                          strings);
        if(predecessor_index == -1) {
            log_info("io_zmaster_t: GETTING_CHILDREN(%d) -> MASTER(%d)",
                     epoch,
                     epoch);
            zmaster.state_ = MASTER;
            guard.relax(); // ???
            new set_master_item_t(&zmaster, epoch);
        } else {
            log_info("io_zmaster_t: GETTING_CHILDREN(%d) -> SETTING_WATCH(%d)",
                     epoch,
                     epoch);
            string_t watch_key = zmaster.make_full_path(strings->data[predecessor_index]);
            zmaster.state_ = SETTING_WATCH;
            guard.relax();
            new watch_item_t(&zmaster, epoch, watch_key);
        }
    } else if(rc == ZOPERATIONTIMEOUT) {
        log_debug("children_callback: timed out, retrying");
        new get_children_item_t(&zmaster, epoch);
    } else {
        log_error("children_callback: unknown rc %d", rc);
        fatal("unknown state in children_callback");
    }
}


void io_zmaster_t::watch_callback(int rc,
                                  const struct Stat* stat,
                                  const void* data)
{
    callback_data_t* callback_data = (callback_data_t*) data;
    Stat_guard_t stat_guard(stat);
    delete_guard_t<callback_data_t> cbguard(callback_data);

    io_zmaster_t& zmaster = *(callback_data->zmaster);
    const int epoch = callback_data->epoch;

    bq_cond_guard_t guard(zmaster.state_cond_);
    if(zmaster.current_epoch_ != epoch) {
        log_info("watch_callback: epoch mismatch (%d, %d)",
                 zmaster.current_epoch_,
                 epoch);
        return;
    }

    if(rc == ZOK) {
        callback_data->next = NULL; // do not delete watch context!
    }

    if(zmaster.state_ == CANCELING) {
        log_info("watch_callback(%d) canceled", epoch);
        guard.relax();
        new delete_item_t(&zmaster, epoch);
        return;
    }
    
    if(rc == ZOK) {
        log_info("watch_callback: WAITING_WATCH(%d) -> WATCHING(%d)",
                 epoch,
                 epoch);
        zmaster.state_ = WATCHING;
    } else if(rc == ZNONODE || rc == ZOPERATIONTIMEOUT) {
        log_info("watch_callback: WAITING_WATCH(%d) -> REGISTERED(%d)",
                 epoch,
                 epoch + 1);
        ++zmaster.current_epoch_;
        guard.relax();
        zmaster.state_ = REGISTERED;
        new get_children_item_t(&zmaster, epoch + 1);
    } else {
        log_error("watch_callback: unknown rc %d", rc);
        fatal("unknown state in watch_callback");
    }
}

void io_zmaster_t::deletion_watch(zhandle_t* /* zh */,
                                  int type,
                                  int state,
                                  const char* path,
                                  void* watcherCtx)
{
    callback_data_t* callback_data = (callback_data_t*) watcherCtx;
    delete_guard_t<callback_data_t> cbguard(callback_data);
    free_guard_t<const char> path_guard(path);

    log_info("io_zmaster_t::deletion_watch (%s, %s) at '%s'",
             event_string(type),
             state_string(state),
             path);

    if(type == ZOO_SESSION_EVENT) {
        log_debug("deletion_watch: ignoring SESSION_EVENT");
        return;
    }
    assert(state == ZOO_CONNECTED_STATE);
    if(type != ZOO_DELETED_EVENT) {
        log_error("deletion_watch: got bad event (%s, %s)", event_string(type), state_string(state));
        fatal("unknown state in deletion_watch");
    }

    io_zmaster_t& zmaster = *(callback_data->zmaster);
    const int epoch = callback_data->epoch;

    bq_cond_guard_t guard(zmaster.state_cond_);
    if(zmaster.current_epoch_ != epoch) {
        log_info("deletion_watch: epoch mismatch (%d, %d)",
                 zmaster.current_epoch_,
                 epoch);
        return;
    }
    if(zmaster.state_ == CANCELING) {
        log_info("deletion_watch(%d): canceled, doing nothing", epoch);
        return;
    }

    log_info("deletion_watch: [%d](%d) -> REGISTERED(%d)",
             zmaster.state_,
             epoch,
             epoch + 1);
    zmaster.state_ = REGISTERED;
    ++zmaster.current_epoch_;
    new get_children_item_t(&zmaster, epoch + 1);
}

int io_zmaster_t::predecessor_index(const string_t& key,
                                    const struct String_vector* keys) const
{
    int max_index = -1;
    string_t max_string = string_t::empty;

    for(int i = 0; i < keys->count; ++i) {
        const string_t candidate_key = make_full_path(keys->data[i]);
        if(string_t::cmp<ident_t>(candidate_key, key).is_less()) {
            if(string_t::cmp<ident_t>(candidate_key, max_string).is_greater()) {
                max_index = i;
                max_string = candidate_key;
            }
        }
    }
    return max_index;
}

void io_zmaster_t::new_session() {
    log_info("io_zmaster_t::new_session");
    bq_cond_guard_t guard(state_cond_);

    if(state_ != INACTIVE) {
        state_ = CANCELING; // asserted by advance_epoch
        advance_epoch();
        do_activate(guard);
    }
}

namespace io_zmaster {
config_binding_sname(io_zmaster_t);
config_binding_value(io_zmaster_t, host_id);
config_binding_value(io_zmaster_t, election_path);
config_binding_value(io_zmaster_t, current_master_path);
config_binding_value(io_zmaster_t, zconf);
config_binding_parent(io_zmaster_t, io_zclient_t, 1);
config_binding_ctor(io_t, io_zmaster_t);
}  // namespace io_zmaster

}  // namespace phantom
