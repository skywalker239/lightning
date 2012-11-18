// vim: set tabstop=4 expandtab:
#include <phantom/io_var_store/backend_zookeeper/backend_zookeeper.H>

#include <pd/zookeeper/bq_zookeeper.H>

#include <pd/bq/bq_cond.H>

namespace phantom {
namespace io_var_store {

namespace {

struct set_data_t {
    bq_cond_t cond;
    bool result;
    set_data_t()
        : result(false)
    {}
};

}  // anonymous namespace

class backend_zookeeper_t::watch_item_t : public io_zclient_t::todo_item_t {
public:
    watch_item_t(backend_zookeeper_t* backend,
                 const string_t& key)
        : backend_(*backend),
          key_(key)
    {}

    bool apply(zhandle_t* zhandle) {
        if(!key_is_watched()) {
            return true;
        }

        struct Stat stat;
        int rc = bq_zoo_wexists(zhandle,
                                key_,
                                &backend_zookeeper_t::node_watch,
                                &backend_,
                                &stat);
        switch(rc) {
            case ZOK:
                backend_.schedule(new get_item_t(&backend_, key));
                return true;
                break;
            case ZINVALIDSTATE: case ZOPERATIONTIMEOUT: case ZCONNECTIONLOSS:
            case ZCLOSING:
                return false;
                break;
            default:
                log_error("watch_item_t got rc %d (%s)", rc, zerror(rc));
                fatal("unknown state in watch_item_t");
        }
    }
private:
    bool key_is_watched() {
        bq_mutex_guard_t guard(backend_.var_handle_map_lock_);
        return backend_.var_handle_map_.find(key) !=
               backend_.var_handle_map_.end();
    }

    backend_zookeeper_t& backend_;
    const string_t key_;
};

/*
class backend_zookeeper_t::get_item_t : public io_zclient_t::todo_item_t {
    //// TODO
public:
    get_item_t(backend_zookeeper_t* backend,
               const string_t& key)
        : backend_(&backend),
          key_(key)
    {}

    virtual bool apply(zhandle_t* zhandle) {
        struct Stat stat;
        string_t value;

        int rc = bq_zoo_wget(zhandle,
                             key_,
                             &backend_zookeeper_t::node_watch,
                             &backend_,
                             &value,
                             &stat);
        switch(rc) {
            case ZOK:
                backend_.schedule(new get_item_t(&backend_, key));
                return true;
                break;
            case ZINVALIDSTATE: case ZOPERATIONTIMEOUT: case ZCONNECTIONLOSS:
            case ZCLOSING:
                return false;
                break;
            default:
                log_error("watch_item_t got rc %d (%s)", rc, zerror(rc));
                fatal("unknown state in watch_item_t");
        }
    }
private:
    backend_zookeeper_t& backend_;
    const string_t key_;
    }
*/


backend_zookeeper_t::config_t::config_t()
    : zhandle(),
      path(string_t::empty)
{}

backend_zookeeper_t::config_t::~config_t()
{}

void backend_zookeeper_t::config_t::check(const in_t::ptr_t& p) const {
}

backend_zookeeper_t::backend_zookeeper_t(const string_t& name,
                                         const config_t& config)
    : io_zclient_t(name, config),
      path_(config.path)
{}

void backend_zookeeper_t::add_key(const string_t& key, var_handle_t* handle) {
    const string_t full_key = full_path(key);

    bq_mutex_guard_t guard(var_handle_map_lock_);

    auto insert_result = var_handle_map_.insert(
                            std::make_pair(full_key,
                                           handle));
    assert(insert_result.second);

    schedule(new watch_item_t(this, full_key));
}

void backend_zookeeper_t::remove_key(const string_t& key) {
    const string_t full_key = full_path(key);

    bq_mutex_guard_t guard(var_handle_map_lock_);

    auto iter = var_handle_map_.find(full_key);
    assert(iter != var_handle_map_.end());
    var_handle_map_.erase(iter);
}

bool backend_zookeeper_t::set(const string_t& key,
                              const string_t& value,
                              int version)
{
    const string_t full_key = full_path(key)
    {
        bq_mutex_guard_t vguard(var_handle_map_.lock);
        if(var_handle_map_.find(full_key) == var_handle_map_.end()) {
            return false;
        }
    }

    set_data_t data;
    bq_cond_guard_t guard(data.cond);
    schedule(new set_item_t(this, full_key, value, version, &data));
    if(!bq_success(data.cond.wait(NULL))) {
        throw exception_sys_t(log::error | log::trace,
                              errno,
                              "set_data.cond_wait: %m");
    }
    return data.result;
}

void backend_zookeeper_t::new_session() {
    bq_mutex_guard_t guard(var_handle_map_lock_);

    for(auto i = var_handle_map_.begin();
        i != var_handle_map_.end();
        ++i)
    {
        schedule(new watch_item_t(this, i->first));
    }
}

string_t backend_zookeeper_t::full_path(const string_t& key) {
    return string_t::ctor_t(path_.size() + key.size() + 1)(path_)('/')(key);
}

}  // namespace io_var_store
}  // namespace phantom
