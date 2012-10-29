#include <phantom/io_zmaster/io_zmaster.H>
#include <phantom/io_zhandle/zoo_util.H>

#include <pd/base/exception.H>
#include <pd/base/log.H>

#include <phantom/module.H>

namespace phantom {

MODULE(io_zmaster);

namespace {

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

class io_zmaster_t::activate_item_t : public io_zclient_t::todo_item_t {
public:
    activate_item_t(io_zmaster_t* zmaster, int epoch)
        : todo_item_t(zmaster),
          zmaster_(*zmaster),
          epoch_(epoch)
    {
        log_debug("activate_item_t ctor(%p)", this);
    }

    virtual void apply() {
        bq_cond_guard_t guard(zmaster_.state_cond_);

        if(zmaster_.current_epoch_ == epoch_ ||
           (zmaster_.current_epoch_ == epoch_ + 1 && zmaster_.state_ == io_zmaster_t::INACTIVE))
        {
            zmaster_.do_activate();
        } else {
            log_info("activate_item_t(%d): current epoch is %d and state %d, not activating",
                     epoch_,
                     zmaster_.current_epoch_,
                     zmaster_.state_);
        }
    }
private:
    io_zmaster_t& zmaster_;
    int epoch_;
};

class io_zmaster_t::delete_item_t : public io_zclient_t::todo_item_t {
public:
    delete_item_t(io_zmaster_t* zmaster, int epoch)
        : todo_item_t(zmaster),
          zmaster_(*zmaster),
          epoch_(epoch)
    {
        log_debug("delete_item_t ctor(%p)", this);
    }

    virtual void apply() {
        zhandle_guard_t zguard(zmaster_.zhandle_);
        zhandle_t* zhandle = zmaster_.zhandle_.wait();

        bq_cond_guard_t guard(zmaster_.state_cond_);
        log_debug("delete_item_t::apply %d", epoch_);

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
            log_debug("delete_item_t: zhandle is in ZINVALIDSTATE, retrying");
            zmaster_.schedule(new delete_item_t(&zmaster_, epoch_));
            return;
        } else {
            log_error("zoo_adelete returned %d (%s)", rc, zerror(rc));
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

    if(rc == ZOK || rc == ZNONODE) {
        log_debug("delete_callback(%s): success", zerror(rc));
        zmaster.advance_epoch();
    } else if(rc == ZOPERATIONTIMEOUT || rc == ZCONNECTIONLOSS) {
        log_debug("delete_callback: timed out, retrying");
        guard.relax();
        zmaster.schedule(new delete_item_t(&zmaster, epoch));
    } else if(rc == ZCLOSING) {
        log_debug("delete_callback: ZK closing");
    } else {
        log_error("delete_callback: unknown rc %d (%s)", rc, zerror(rc));
        fatal("unknown state in delete_callback");
    }
}

class io_zmaster_t::create_item_t : public io_zclient_t::todo_item_t {
public:
    create_item_t(io_zmaster_t* zmaster, int epoch)
        : todo_item_t(zmaster),
          zmaster_(*zmaster),
          epoch_(epoch)
    {
        log_debug("create_item_t ctor(%p)", this);
    }

    virtual void apply() {
        const string_t path = zmaster_.seq_node_path_base();
        const string_t value = zmaster_.seq_node_value();

        zhandle_guard_t zguard(zmaster_.zhandle_);
        zhandle_t* zhandle = zmaster_.zhandle_.wait();

        bq_cond_guard_t guard(zmaster_.state_cond_);
        log_debug("create_item_t::apply %d", epoch_);

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
            log_debug("create_item_t: zhandle is in ZINVALIDSTATE, retrying");
            zmaster_.schedule(new create_item_t(&zmaster_, epoch_));
            return;
        } else {
            log_error("zoo_acreate returned %d (%s)", rc, zerror(rc));
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
    {
        log_debug("get_children_item_t ctor(%p)", this);
    }

    void apply() {
        zhandle_guard_t zguard(zmaster_.zhandle_);
        zhandle_t* zhandle = zmaster_.zhandle_.wait();

        bq_cond_guard_t guard(zmaster_.state_cond_);
        log_debug("get_children_item_t::apply %d", epoch_);

        if(zmaster_.current_epoch_ != epoch_) {
            log_info("get_children_item_t: epoch mismatch (%d, %d)",
                     zmaster_.current_epoch_,
                     epoch_);
            return;
        }
        if(zmaster_.state_ == io_zmaster_t::CANCELING) {
            log_info("get_children_item_t(%d) canceled", epoch_);
            guard.relax();
            zmaster_.schedule(new delete_item_t(&zmaster_, epoch_));
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
            log_debug("io_zmaster_t::get_children_item_t: zhandle is in ZINVALIDSTATE, retrying");
            zmaster_.schedule(new get_children_item_t(&zmaster_, epoch_));
        } else {
            log_error("zoo_aget_children returned %d (%s)", rc, zerror(rc));
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
          key_(key)
    {
        log_debug("watch_item_t ctor(%p)", this);
    }

    virtual void apply() {
        zhandle_guard_t zguard(zmaster_.zhandle_);
        zhandle_t* zhandle = zmaster_.zhandle_.wait();

        bq_cond_guard_t guard(zmaster_.state_cond_);
        log_debug("watch_item_t::apply %d", epoch_);

        if(zmaster_.current_epoch_ != epoch_) {
            log_info("watch_item_t: epoch mismatch (%d, %d)",
                     zmaster_.current_epoch_,
                     epoch_);
            return;
        }
        if(zmaster_.state_ == io_zmaster_t::CANCELING) {
            log_info("watch_item_t(%d) canceled", epoch_);
            guard.relax();
            zmaster_.schedule(new delete_item_t(&zmaster_, epoch_));
            return;
        }
        assert(zmaster_.state_ == io_zmaster_t::SETTING_WATCH);

        callback_data_t* watch_context = new callback_data_t(&zmaster_, epoch_);
        callback_data_t* watch_callback_data = new callback_data_t(&zmaster_, epoch_);
        watch_callback_data->next = watch_context;
        MKCSTR(key_z, key_);
        int rc = zoo_awexists(zhandle,
                              key_z,
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
            log_debug("watch_item_t: zhandle in ZINVALIDSTATE, retrying");
            zmaster_.schedule(new watch_item_t(&zmaster_, epoch_, key_));
        } else {
            log_error("zoo_awexists returned %d (%s)", rc, zerror(rc));
            fatal("unknown state in watch_item_t");
        }
    }
private:
    io_zmaster_t& zmaster_;
    int epoch_;
    const string_t key_;
};

class io_zmaster_t::set_master_item_t : public io_zclient_t::todo_item_t {
public:
    set_master_item_t(io_zmaster_t* zmaster, int epoch)
        : todo_item_t(zmaster),
          zmaster_(*zmaster),
          epoch_(epoch)
    {
        log_debug("set_master_item_t ctor(%p)", this);
    }

    virtual void apply() {
        bq_cond_guard_t guard(zmaster_.state_cond_);
        log_debug("set_master_item_t::apply %d", epoch_);

        if(zmaster_.current_epoch_ != epoch_) {
            log_info("set_master_item_t: epoch mismatch (%d, %d)",
                     zmaster_.current_epoch_,
                     epoch_);
            return;
        }
        if(zmaster_.state_ == io_zmaster_t::CANCELING) {
            log_info("set_master_item_t(%d) canceled", epoch_);
            guard.relax();
            zmaster_.schedule(new delete_item_t(&zmaster_, epoch_));
            return;
        }
        assert(zmaster_.state_ == io_zmaster_t::MASTER);
        guard.relax();

        zmaster_.current_master_.set(zmaster_.host_id_, -1);
        log_debug("set_master_item_t: success");
    }
private:
    io_zmaster_t& zmaster_;
    int epoch_;
};

void io_zmaster_t::activate() {
    log_info("io_zmaster_t::activate()");
    bq_cond_guard_t guard(state_cond_);
    do_activate();
}

void io_zmaster_t::do_activate() {
    log_info("io_zmaster_t::do_activate()");
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
    schedule(new create_item_t(this, current_epoch_));
}

void io_zmaster_t::deactivate() {
    bq_cond_guard_t guard(state_cond_);
    log_info("io_zmaster_t::deactivate()");
    do_deactivate();
}

void io_zmaster_t::do_deactivate() {
    log_info("io_zmaster_t::do_deactivate()");

    if(state_ != INACTIVE) {
        state_t old_state = state_;
        state_ = CANCELING;
        log_info("do_deactivate: [%d](%d) -> CANCELING(%d)",
                 old_state,
                 current_epoch_,
                 current_epoch_);
        if(old_state == MASTER || old_state == WATCHING) {
            schedule(new delete_item_t(this, current_epoch_));
        }
    }
}

void io_zmaster_t::create_callback(int rc, const char* value, const void* data) {
    callback_data_t* callback_data = (callback_data_t*) data;
    delete_guard_t<callback_data_t> cbguard(callback_data);

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
            zmaster.schedule(new delete_item_t(&zmaster, epoch));
        } else {
            assert(zmaster.state_ == REGISTERING);
            log_info("io_zmaster_t: REGISTERING(%d) -> REGISTERED(%d)",
                     epoch,
                     epoch);
            zmaster.state_ = REGISTERED;
            guard.relax();
    
            zmaster.schedule(new get_children_item_t(&zmaster, epoch));
        }
    } else if(rc == ZOPERATIONTIMEOUT || rc == ZCONNECTIONLOSS) {
        if(zmaster.state_ == CANCELING) {
            log_info("io_zmaster_t::create_callback(failed) canceled");
            zmaster.advance_epoch();
        } else {
            assert(zmaster.state_ == REGISTERING);
            log_debug("io_zmaster_t::create_callback timeout, retrying");
            guard.relax();
    
            zmaster.schedule(new create_item_t(&zmaster, epoch));
        }
    } else if(rc == ZCLOSING) {
        log_debug("create_callback: ZK closing");
    } else {
        log_error("io_zmaster_t::create_callback got unknown rc %d (%s)", rc, zerror(rc));
        fatal("unknown state in create_callback");
    }
}

void io_zmaster_t::children_callback(int rc,
                                     const struct String_vector* strings,
                                     const void* data)
{
    callback_data_t* callback_data = (callback_data_t*) data;
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
        zmaster.schedule(new delete_item_t(&zmaster, epoch));
        return;
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
            zmaster.schedule(new set_master_item_t(&zmaster, epoch));
        } else {
            log_info("io_zmaster_t: GETTING_CHILDREN(%d) -> SETTING_WATCH(%d)",
                     epoch,
                     epoch);
            string_t watch_key = zmaster.make_full_path(strings->data[predecessor_index]);
            zmaster.state_ = SETTING_WATCH;
            guard.relax();
            zmaster.schedule(new watch_item_t(&zmaster, epoch, watch_key));
        }
    } else if(rc == ZOPERATIONTIMEOUT || rc == ZCONNECTIONLOSS) {
        log_debug("children_callback: timed out, retrying");
        zmaster.schedule(new get_children_item_t(&zmaster, epoch));
    } else if(rc == ZCLOSING) {
        log_debug("children_callback: ZK closing");
    } else {
        log_error("children_callback: unknown rc %d (%s)", rc, zerror(rc));
        fatal("unknown state in children_callback");
    }
}


void io_zmaster_t::watch_callback(int rc,
                                  const struct Stat* /* stat */,
                                  const void* data)
{
    callback_data_t* callback_data = (callback_data_t*) data;
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
        zmaster.schedule(new delete_item_t(&zmaster, epoch));
        return;
    }
    
    if(rc == ZOK) {
        log_info("watch_callback: WAITING_WATCH(%d) -> WATCHING(%d)",
                 epoch,
                 epoch);
        zmaster.state_ = WATCHING;
    } else if(rc == ZNONODE || rc == ZOPERATIONTIMEOUT || rc == ZCONNECTIONLOSS) {
        log_info("watch_callback(%s): WAITING_WATCH(%d) -> REGISTERED(%d)",
                 zerror(rc),
                 epoch,
                 epoch + 1);
        ++zmaster.current_epoch_;
        guard.relax();
        zmaster.state_ = REGISTERED;
        zmaster.schedule(new get_children_item_t(&zmaster, epoch + 1));
    } else if(rc == ZCLOSING) {
        log_debug("watch_callback: ZK closing");
    } else {
        log_error("watch_callback: unknown rc %d (%s)", rc, zerror(rc));
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

    log_info("io_zmaster_t::deletion_watch (%s, %s) at '%s'",
             zoo_util::event_string(type),
             zoo_util::state_string(state),
             path);

    if(type == ZOO_SESSION_EVENT) {
        log_debug("deletion_watch: ignoring SESSION_EVENT");
        return;
    }
    assert(state == ZOO_CONNECTED_STATE);
    if(type != ZOO_DELETED_EVENT) {
        log_error("deletion_watch: got bad event (%s, %s)", zoo_util::event_string(type), zoo_util::state_string(state));
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
    zmaster.schedule(new get_children_item_t(&zmaster, epoch + 1));
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

    //! In case of INACTIVE there is nothing to do.
    //  In case of CANCELING there is something down
    //  the queue that will complete the canceling.
    //  Otherwise we need to activate.
    if(state_ != INACTIVE && state_ != CANCELING) {
        do_deactivate();
        //! We cannot call do_activate() here directly
        //  because new_session is called from a
        //  non-bq thread and do_activate waits on a
        //  bq condition variable. So we schedule
        //  an activation.
        schedule(new activate_item_t(this, current_epoch_));
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
