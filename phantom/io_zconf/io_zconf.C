//TODO(skywalker): prepend path to keys, handle session reset
// 
#include <phantom/io_zconf/io_zconf.H>

#include <pd/base/exception.H>
#include <pd/base/log.H>

#include <phantom/module.H>

#include <zookeeper.h>

namespace phantom {

MODULE(io_zconf);

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

struct io_zconf_t::callback_data_t {
    io_zconf_t* iozc;
    string_t path;

    callback_data_t(io_zconf_t* _iozc,
                    const char* _path)
        : iozc(_iozc),
          path(string_t::ctor_t(strlen(_path))(str_t(_path, strlen(_path))))
    {}

    callback_data_t(io_zconf_t* _iozc,
                    const string_t& _path)
        : iozc(_iozc),
          path(_path)
    {}
};

struct io_zconf_t::set_data_t {
    io_zconf_t* iozc;
    string_t value;
    bq_cond_t cond;

    set_data_t(io_zconf_t* _iozc, const string_t& _value)
        : iozc(_iozc),
          value(_value)
    {}
};

class io_zconf_t::watch_item_t : public io_zclient_t::todo_item_t {
public:
    watch_item_t(io_zconf_t* zconf,
                 const string_t& key)
        : todo_item_t(zconf),
          zconf_(*zconf),
          key_(key)
    {}

    virtual void apply() {
        MKCSTR(key_z, key_);
        const auto& var_map = zconf_.var_map_;
        thr::spinlock_guard_t guard(zconf_.var_map_lock_);
        if(var_map.find(key_) == var_map.end()) {
            log_info("key '%s' removed, not watching", key_z);
            return;
        }
        guard.relax();

        zhandle_guard_t zguard(zconf_.zhandle_);

        zhandle_t* zhandle = zconf_.zhandle_.wait();
        int rc = zoo_awexists(zhandle,
                              key_z,
                              &io_zconf_t::node_watcher,
                              (void*) &zconf_,
                              &io_zconf_t::stat_callback,
                              (void*) new callback_data_t(&zconf_, key_));
        zguard.relax();

        if(rc == ZOK) {
            return;
        } else if(rc == ZINVALIDSTATE) {
            new watch_item_t(&zconf_, key_);
        } else {
            throw exception_sys_t(log::error,
                                  errno,
                                  "zoo_awexists returned %d (%m)",
                                  rc);
        }
    }
private:
    io_zconf_t& zconf_;
    string_t key_;
};

class io_zconf_t::get_item_t : public io_zclient_t::todo_item_t {
public:
    get_item_t(io_zconf_t* zconf,
               const string_t& key)
        : todo_item_t(zconf),
          zconf_(*zconf),
          key_(key)
    {}

    virtual void apply() {
        MKCSTR(key_z, key_);
        const auto& var_map = zconf_.var_map_;
        thr::spinlock_guard_t guard(zconf_.var_map_lock_);
        if(var_map.find(key_) == var_map.end()) {
            log_info("key '%s' removed, not getting", key_z);
            return;
        }
        guard.relax();

        zhandle_guard_t zguard(zconf_.zhandle_);

        zhandle_t* zhandle = zconf_.zhandle_.wait();
        int rc = zoo_awget(zhandle,
                           key_z,
                           &io_zconf_t::node_watcher,
                           (void*) &zconf_,
                           &io_zconf_t::data_callback,
                           (void*) new callback_data_t(&zconf_, key_));
        zguard.relax();

        if(rc == ZOK) {
            return;
        } else if(rc == ZINVALIDSTATE) {
            new get_item_t(&zconf_, key_);
        } else {
            throw exception_sys_t(log::error,
                                  errno,
                                  "zoo_awget returned %d (%m)",
                                  rc);
        }
    }
private:
    io_zconf_t& zconf_;
    string_t key_;
};

class io_zconf_t::set_item_t : public io_zclient_t::todo_item_t {
public:
    set_item_t(io_zconf_t* zconf,
               const string_t& key,
               const string_t& value,
               set_data_t& set_data)
        : todo_item_t(zconf),
          zconf_(*zconf),
          key_(key),
          value_(value),
          set_data_(set_data)
    {}

    virtual void apply() {
        MKCSTR(key_z, key_);
        const auto& var_map = zconf_.var_map_;
        thr::spinlock_guard_t guard(zconf_.var_map_lock_);
        if(var_map.find(key_) == var_map.end()) {
            log_info("key '%s' removed, not setting", key_z);
            bq_cond_guard_t set_guard(set_data_.cond);
            set_data_.cond.send(); // XXX ignoring nonwatched sets
            return;
        }
        guard.relax();

        zhandle_guard_t zguard(zconf_.zhandle_);

        zhandle_t* zhandle = zconf_.zhandle_.wait();
        int rc = zoo_aset(zhandle,
                          key_z,
                          value_.ptr(),
                          value_.size(),
                          -1,
                          &io_zconf_t::set_callback,
                          (void*) &set_data_);
        zguard.relax();

        if(rc == ZOK) {
            return;
        } else if(rc == ZINVALIDSTATE) {
            new set_item_t(&zconf_, key_, value_, set_data_);
        } else {
            throw exception_sys_t(log::error,
                                  errno,
                                  "zoo_aset returned %d (%m)",
                                  rc);
        }
    }
private:
    io_zconf_t& zconf_;
    string_t key_;
    string_t value_;
    set_data_t& set_data_;
};


void io_zconf_t::config_t::check(const in_t::ptr_t& p) const {
    io_zclient_t::config_t::check(p);
    if(path.size() == 0) {
        config::error(p, "path must be set");
    }
}

io_zconf_t::io_zconf_t(const string_t& name, const config_t& config)
    : io_zclient_t(name, config)
{}

io_zconf_t::~io_zconf_t()
{}

io_zconf_t::stat_t* io_zconf_t::add_var_ref(const string_t& key) {
    thr::spinlock_guard_t map_guard(var_map_lock_);
    auto iter = var_map_.insert(std::make_pair(key, stat_t()));
    stat_t& var_stat = iter.first->second;

    if(iter.second) {
        new watch_item_t(this, key);
    }

    bq_cond_guard_t ref_guard(var_stat.cond);
    int rc = ++var_stat.ref_count;
    ref_guard.relax();
    map_guard.relax();

    MKCSTR(key_z, key);
    log_info("Added key '%s', refcount is %d", key_z, rc);

    return &var_stat;
}

void io_zconf_t::remove_var_ref(const string_t& key) {
    thr::spinlock_guard_t map_guard(var_map_lock_);
    auto iter = var_map_.find(key);

    assert(iter != var_map_.end());
    stat_t& var_stat = iter->second;
    bq_cond_guard_t ref_guard(var_stat.cond);
    int rc = --var_stat.ref_count;
    ref_guard.relax();
    if(rc == 0) {
        var_map_.erase(key);
    }
    map_guard.relax();

    MKCSTR(key_z, key);
    log_info("Removed key '%s', refcount is now %d", key_z, rc);
}

void io_zconf_t::set(const string_t& key, const string_t& value) {
    set_data_t set_data(this, value);

    bq_cond_guard_t set_guard(set_data.cond);
    new set_item_t(this, key, value, set_data);

    if(!bq_success(set_data.cond.wait(NULL))) {
        throw exception_sys_t(log::error, errno, "set_data.cond.wait: %m");
    }
}

void io_zconf_t::node_watcher(zhandle_t*,
                              int type,
                              int state,
                              const char* path,
                              void* ctx)
{
    io_zconf_t* iozc = reinterpret_cast<io_zconf_t*>(ctx);

    free_guard_t<const char> path_guard(path);
    log_info("node watch: state %s, event %s at '%s'",
             state_string(state),
             event_string(type),
             path);
    size_t path_len = strlen(path);
    string_t key = string_t::ctor_t(path_len)(str_t(path, path_len));

    if(type == ZOO_SESSION_EVENT) {
        log_info("node watch for '%s' doing nothing for ZOO_SESSION_EVENT",
                 path);
    } else if(type == ZOO_CREATED_EVENT || type == ZOO_CHANGED_EVENT) {
        new get_item_t(iozc, key);
    } else if(type == ZOO_DELETED_EVENT) {
        new watch_item_t(iozc, key);
    } else {
        throw exception_log_t(log::error,
                              "Unknown event %d at node_watcher",
                              type);
    }
}

void io_zconf_t::data_callback(int rc,
                               const char* value,
                               int vallen,
                               const struct Stat* stat,
                               const void* data)
{
    callback_data_t* callback_data = (callback_data_t*) data;
    delete_guard_t<callback_data_t> cbdata_guard(callback_data);
    free_guard_t<const char> value_guard(value);
    Stat_guard_t stat_guard(stat);

    io_zconf_t* iozc = callback_data->iozc;
    const string_t& path = callback_data->path;

    if(rc == ZOK) {
        iozc->update_node(path, value, vallen, stat);
    } else if(rc == ZNONODE) {
        iozc->set_no_node(path);
        new watch_item_t(iozc, path);
    } else {
        throw exception_log_t(log::error,
                              "data_callback got rc %d",
                              rc);
    }
}

void io_zconf_t::stat_callback(int rc,
                               const struct Stat* stat,
                               const void* data)
{
    callback_data_t* callback_data = (callback_data_t*) data;

    delete_guard_t<callback_data_t> cbdata_guard(callback_data);
    Stat_guard_t stat_guard(stat);

    io_zconf_t* iozc = callback_data->iozc;
    const string_t& path = callback_data->path;

    if(rc == ZOK) {
        new get_item_t(iozc, path);
    } else if(rc == ZNONODE) {
        iozc->set_no_node(path);
    } else {
        throw exception_log_t(log::error,
                              "stat_callback got rc %d",
                              rc);
    }
}

void io_zconf_t::set_callback(int rc,
                              const struct Stat* stat,
                              const void* data)
{
    set_data_t* set_data = (set_data_t*) data;

    Stat_guard_t stat_guard(stat);

    if(rc == ZOK) {
        bq_cond_guard_t set_guard(set_data->cond);
        set_data->cond.send();
    } else {
        throw exception_log_t(log::error,
                              "set_callback got rc %d",
                              rc);
    }
}

void io_zconf_t::update_node(const string_t& key,
                             const char* value,
                             int vallen,
                             const struct Stat* stat)
{
    MKCSTR(key_z, key);
    thr::spinlock_guard_t map_guard(var_map_lock_);
    auto iter = var_map_.find(key);
    if(iter == var_map_.end()) {
        log_info("update_node: key '%s' not found", key_z);
        return;
    }
    stat_t& var_stat = iter->second;

    bq_cond_guard_t var_guard(var_stat.cond);
    map_guard.relax();

    var_stat.valid = true;
    var_stat.exists = true;
    var_stat.value = string_t::ctor_t(vallen)(str_t(value, vallen));
    var_stat.stat = *stat;

    var_stat.cond.send(true);
    var_guard.relax();

    MKCSTR(val_z, var_stat.value);
    log_info("update_node '%s' = '%s' (czxid=%ld mzxid=%ld ctime=%ld mtime=%ld version=%d cversion=%d aversion=%d ephemeralOwner=%ld dataLength=%d numChildren=%d pzxid=%ld)", key_z, val_z, stat->czxid, stat->mzxid, stat->ctime, stat->mtime, stat->version, stat->cversion, stat->aversion, stat->ephemeralOwner, stat->dataLength, stat->numChildren, stat->pzxid);
}

void io_zconf_t::set_no_node(const string_t& key) {
    MKCSTR(key_z, key);
    thr::spinlock_guard_t map_guard(var_map_lock_);
    auto iter = var_map_.find(key);
    if(iter == var_map_.end()) {
        log_info("set_no_node: key '%s' not found", key_z);
        return;
    }
    stat_t& var_stat = iter->second;

    bq_cond_guard_t var_guard(var_stat.cond);
    map_guard.relax();

    var_stat.valid = true;
    var_stat.exists = false;
    var_stat.value = string_t::empty;

    var_stat.cond.send(true);
    var_guard.relax();

    log_info("set_no_node '%s'", key_z);
}

namespace io_zconf {
config_binding_sname(io_zconf_t);
config_binding_value(io_zconf_t, path);
config_binding_parent(io_zconf_t, io_zclient_t, 1);
config_binding_ctor(io_t, io_zhandle_t);
}  // namespace io_zconf

}  // namespace phantom
