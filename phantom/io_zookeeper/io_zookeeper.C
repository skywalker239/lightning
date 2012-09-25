#include <phantom/io_zookeeper/io_zookeeper.H>

#include <pd/paxos/var.H>

#include <phantom/module.H>

#include <pd/base/assert.H>
#include <pd/base/exception.H>
#include <pd/base/log.H>
#include <pd/base/string.H>

#include <string.h>

namespace phantom {

MODULE(io_zookeeper);

void io_zookeeper_t::config_t::check(const in_t::ptr_t& p) const {
    if(servers.size() == 0) {
        config::error(p, "servers must be set");
    }
}

io_zookeeper_t::io_zookeeper_t(const string_t& name, const config_t& config)
    : io_t(name, config),
      servers_z_(string_t::ctor_t(config.servers.size() + 1)(config.servers)('\0')),
      zhandle_(NULL),
      zookeeper_log_path_(config.zookeeper_log),
      zookeeper_log_(NULL),
      connected_(false),
      need_to_reset_watches_(true),
      todo_list_(NULL),
      todo_last_(&todo_list_)
{
    for(config::list_t<string_t>::ptr_t p = config.keys.ptr(); p; ++p) {
        (void) add_var_ref(p.val());
    }
}

io_zookeeper_t::stat_t* io_zookeeper_t::add_var_ref(const string_t& key) {
    thr::spinlock_guard_t map_guard(var_map_lock_);
    auto iter = var_map_.insert(std::make_pair(key, stat_t()));
    stat_t& var_stat = iter.first->second;

    if(iter.second) {
        bq_cond_guard_t todo_guard(todo_cond_);
        new todo_item_t(todo_item_t::WATCH, key, *this);
        todo_cond_.send();
    }

    bq_cond_guard_t ref_guard(var_stat.cond);
    int rc = ++var_stat.ref_count;
    ref_guard.relax();
    map_guard.relax();

    MKCSTR(key_z, key);
    log_info("Added key '%s', refcount is %d", key_z, rc);

    return &var_stat;
}

void io_zookeeper_t::remove_var_ref(const string_t& key) {
    thr::spinlock_guard_t map_guard(var_map_lock_);
    auto iter = var_map_.find(key);

    assert(iter != var_map_.end());
    stat_t& var_stat = iter->second;
    bq_cond_guard_t ref_guard(var_stat.cond);
    int rc = --var_stat.ref_count;
    if(rc == 0) {
        var_map_.erase(key);
    }
    ref_guard.relax();
    map_guard.relax();

    MKCSTR(key_z, key);
    log_info("Removed key '%s', refcount is now %d", key_z, rc);
}
    
void io_zookeeper_t::init() {
    zoo_set_debug_level(ZOO_LOG_LEVEL_DEBUG);
    if(zookeeper_log_path_.size() != 0) {
        string_t zookeeper_log_path_z =
            string_t::ctor_t(zookeeper_log_path_.size() + 1)(zookeeper_log_path_)('\0');
        zookeeper_log_ = fopen(zookeeper_log_path_z.ptr(), "a");
        if(zookeeper_log_ == NULL) {
            throw exception_sys_t(log::error, errno, "fopen(%s): %m", zookeeper_log_path_z.ptr());
        }
        zoo_set_log_stream(zookeeper_log_);
    }

    bq_cond_guard_t guard(connected_cond_);
    do_connect();
}

void io_zookeeper_t::do_connect() {
    assert(zhandle_ == NULL);
    log_info("zookeeper_t::do_connect(), connecting to %s", servers_z_.ptr());
    zhandle_ = zookeeper_init(servers_z_.ptr(),
                              &io_zookeeper_t::global_watcher,
                              5000,
                              NULL,
                              this,
                              0);
    if(!zhandle_) {
        throw exception_sys_t(log::error, errno, "zookeeper_init: %m");
    }
}


void io_zookeeper_t::do_shutdown() {
    log_info("do_shutdown");
    assert(zhandle_);
    int rc = zookeeper_close(zhandle_);
    zhandle_ = NULL;
    if(rc != ZOK) {
        log_warning("zookeeper_close returned %d (%m)", rc);
    }
    connected_ = false;
    need_to_reset_watches_ = true;
}

void io_zookeeper_t::fini() {
    bq_cond_guard_t guard(connected_cond_);
    do_shutdown();
    guard.relax();
    if(zookeeper_log_) {
        fflush(zookeeper_log_);
        fclose(zookeeper_log_);
    }

    while(todo_list_) {
        delete todo_list_;
    }
}

void io_zookeeper_t::run() {
    log_info("processing todo items");
    while(true) {
        bq_cond_guard_t guard(todo_cond_);
        
        if(!todo_list_) {
            if(!bq_success(todo_cond_.wait(NULL))) {
                throw exception_sys_t(log::error, errno, "todo_cond.wait: %m");
            }
        }

        assert(todo_list_);
        todo_item_t::type_t type = todo_list_->type;
        string_t key = todo_list_->key;
        delete todo_list_;
        guard.relax();

        switch(type) {
            case todo_item_t::WATCH:
                watch_node(key);
                break;
            case todo_item_t::GET:
                get_node(key);
                break;
            default:
                throw exception_log_t(log::error, "unknown type %d", type);
                break;
        }
    }
}

namespace {

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

struct callback_data_t {
    io_zookeeper_t* iozk;
    string_t path;

    callback_data_t(io_zookeeper_t* _iozk,
                    const char* _path)
        : iozk(_iozk),
          path(string_t::ctor_t(strlen(_path))(str_t(_path, strlen(_path))))
    {}

    callback_data_t(io_zookeeper_t* _iozk,
                    const string_t& _path)
        : iozk(_iozk),
          path(_path)
    {}
};

}  // anonymous namespace

void io_zookeeper_t::global_watcher(zhandle_t*,
                                    int type,
                                    int state,
                                    const char* path,
                                    void* ctx)
{
    io_zookeeper_t* iozk = reinterpret_cast<io_zookeeper_t*>(ctx);
    log_info("zookeeper(%p): state %s, event %s at '%s'", iozk, state_string(state), event_string(type), path);

    free_guard_t<const char> path_guard(path);
    bq_cond_guard_t guard(iozk->connected_cond_);

    if(type == ZOO_SESSION_EVENT) {
        if(state == ZOO_CONNECTED_STATE) {
            assert(!iozk->connected_);
            iozk->connected_ = true;
            iozk->connected_cond_.send(true);
            log_info("zookeeper(%p) reconnected to server.", iozk);
            if(iozk->need_to_reset_watches_) {
                iozk->set_watches();
                iozk->need_to_reset_watches_ = false;
            }
        } else if(state == ZOO_CONNECTING_STATE) {
            assert(iozk->connected_);
            log_info("zookeeper(%p) disconnected from server (%s).", iozk, state_string(state));
            iozk->connected_ = false;
        } else if(state == ZOO_EXPIRED_SESSION_STATE) {
            log_info("zookeeper(%p): session expired", iozk);
            iozk->do_shutdown();
            iozk->do_connect();
        } else {
            throw exception_log_t(log::error, "unknown state");
        }
    }
    
}

void io_zookeeper_t::set_watches() {
    log_info("set_watches");
    thr::spinlock_guard_t guard(var_map_lock_);

    for(auto i = var_map_.begin(); i != var_map_.end(); ++i) {
        bq_cond_guard_t cond_guard(todo_cond_);
        new todo_item_t(todo_item_t::WATCH, i->first, *this);
        todo_cond_.send();
    }
}

void io_zookeeper_t::watch_node(const string_t& key) {
    MKCSTR(key_z, key);
    thr::spinlock_guard_t map_guard(var_map_lock_);
    if(var_map_.find(key) == var_map_.end()) {
        log_info("Key '%s' removed, not watching", key_z);
        return;
    }
    map_guard.relax();

    bq_cond_guard_t connected_guard(connected_cond_);

    if(!connected_) {
        if(!bq_success(connected_cond_.wait(NULL))) {
            throw exception_sys_t(log::error, errno, "connected_cond.wait: %m");
        }
    }

    assert(connected_);
    int rc = zoo_awexists(zhandle_,
                          key_z,
                          &io_zookeeper_t::node_watcher,
                          (void*) this,
                          &io_zookeeper_t::stat_callback,
                          (void*) new callback_data_t(this, key));
    connected_guard.relax();
    log_info("watch_node(%s) = %d", key_z, rc);

    if(rc == ZOK) {
        return;
    } else if(rc == ZINVALIDSTATE) {
        bq_cond_guard_t todo_guard(todo_cond_);
        new todo_item_t(todo_item_t::WATCH, key, *this);
        todo_cond_.send();
    } else {
        throw exception_sys_t(log::error,
                              errno,
                              "zoo_awexists returned %d (%m)",
                              rc);
    }
}

void io_zookeeper_t::get_node(const string_t& key) {
    MKCSTR(key_z, key);
    thr::spinlock_guard_t map_guard(var_map_lock_);
    if(var_map_.find(key) == var_map_.end()) {
        log_info("Key '%s' removed, not getting", key_z);
        return;
    }
    map_guard.relax();

    bq_cond_guard_t connected_guard(connected_cond_);

    if(!connected_) {
        if(!bq_success(connected_cond_.wait(NULL))) {
            throw exception_sys_t(log::error, errno, "connected_cond.wait: %m");
        }
    }

    assert(connected_);
    int rc = zoo_awget(zhandle_,
                       key_z,
                       &io_zookeeper_t::node_watcher,
                       (void*) this,
                       &io_zookeeper_t::data_callback,
                       (void*) new callback_data_t(this, key));
    connected_guard.relax();

    if(rc == ZOK) {
        return;
    } else if(rc == ZINVALIDSTATE) {
        bq_cond_guard_t todo_guard(todo_cond_);
        new todo_item_t(todo_item_t::GET, key, *this);
        todo_cond_.send();
    } else {
        throw exception_sys_t(log::error,
                              errno,
                              "zoo_awget returned %d (%m)",
                              rc);
    }
}

void io_zookeeper_t::node_watcher(zhandle_t*,
                                  int type,
                                  int state,
                                  const char* path,
                                  void* ctx)
{
    io_zookeeper_t* iozk = reinterpret_cast<io_zookeeper_t*>(ctx);

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
        bq_cond_guard_t todo_guard(iozk->todo_cond_);
        new todo_item_t(todo_item_t::GET, key, *iozk);
        iozk->todo_cond_.send();
    } else if(type == ZOO_DELETED_EVENT) {
        bq_cond_guard_t todo_guard(iozk->todo_cond_);
        new todo_item_t(todo_item_t::WATCH, key, *iozk);
        iozk->todo_cond_.send();
    } else {
        throw exception_log_t(log::error,
                              "Unknown event %d at node_watcher",
                              type);
    }
}

void io_zookeeper_t::data_callback(int rc,
                                   const char* value,
                                   int vallen,
                                   const struct Stat* stat,
                                   const void* data)
{
    callback_data_t* callback_data = (callback_data_t*) data;
    delete_guard_t<callback_data_t> cbdata_guard(callback_data);
    free_guard_t<const char> value_guard(value);
    Stat_guard_t stat_guard(stat);

    io_zookeeper_t* iozk = callback_data->iozk;
    const string_t& path = callback_data->path;

    if(rc == ZOK) {
        iozk->update_node(path, value, vallen, stat);
    } else if(rc == ZNONODE) {
        iozk->set_no_node(path);
        bq_cond_guard_t todo_guard(iozk->todo_cond_);
        new todo_item_t(todo_item_t::WATCH, path, *iozk);
        iozk->todo_cond_.send();
    } else {
        throw exception_log_t(log::error,
                              "data_callback got rc %d",
                              rc);
    }
}

void io_zookeeper_t::stat_callback(int rc,
                                   const struct Stat* stat,
                                   const void* data)
{
    callback_data_t* callback_data = (callback_data_t*) data;

    delete_guard_t<callback_data_t> cbdata_guard(callback_data);
    Stat_guard_t stat_guard(stat);

    io_zookeeper_t* iozk = callback_data->iozk;
    const string_t& path = callback_data->path;

    bq_cond_guard_t guard(iozk->connected_cond_);
    if(rc == ZOK) {
        bq_cond_guard_t todo_guard(iozk->todo_cond_);
        new todo_item_t(todo_item_t::GET, path, *iozk);
        iozk->todo_cond_.send();
    } else if(rc == ZNONODE) {
        iozk->set_no_node(path);
    } else {
        throw exception_log_t(log::error,
                              "stat_callback got rc %d",
                              rc);
    }
}

void io_zookeeper_t::update_node(const string_t& key,
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

void io_zookeeper_t::set_no_node(const string_t& key) {
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

io_zookeeper_t::todo_item_t::todo_item_t(todo_item_t::type_t _type,
                                         const string_t& _key,
                                         io_zookeeper_t& _io_zookeeper)
    : type(_type),
      key(_key),
      io_zookeeper(_io_zookeeper)
{
    *(me = io_zookeeper.todo_last_) = this;
    *(io_zookeeper.todo_last_ = &next) = NULL;
    MKCSTR(key_z, key);
    log_info("new todo item (%d, %s)", type, key_z);
}

io_zookeeper_t::todo_item_t::~todo_item_t() {
    if((*me = next)) {
        next->me = me;
    }
    if(io_zookeeper.todo_last_ == &next) {
        io_zookeeper.todo_last_ = me;
    }
    MKCSTR(key_z, key);
    log_info("todo item (%d, %s) dequeued", type, key_z);
}

namespace io_zookeeper {
config_binding_sname(io_zookeeper_t);
config_binding_value(io_zookeeper_t, servers);
config_binding_value(io_zookeeper_t, keys);
config_binding_value(io_zookeeper_t, zookeeper_log);
config_binding_parent(io_zookeeper_t, io_t, 1);
config_binding_ctor(io_t, io_zookeeper_t);
}  // namespace io_zookeeper

}  // namespace phantom
