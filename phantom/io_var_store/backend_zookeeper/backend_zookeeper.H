// vim: set tabstop=4 expandtab:
#pragma once

#include <phantom/io_var_store/io_var_store.H>

#include <phantom/io_zclient/io_zclient.H>

#include <pd/bq/bq_mutex.H>

#include <unordered_map>

#pragma GCC visibility push(default)

namespace phantom {
namespace io_var_store {

//! Manages the variables stored as files in a
//  Zookeeper directory.
class backend_zookeeper_t : public backend_t,
                            public io_zclient_t
{
public:
    struct config_t {
        //! The root directory for this var store.
        string_t path;

        config_t() throw();
        ~config_t() throw();
        void check(const in_t::ptr_t&) const;
    };

    backend_zookeeper_t(const string_t& name, const config_t& config);

private:
    string_t full_path(const string_t& key);

    virtual void add_key(const string_t& key, var_handle_t* handle);

    virtual void remove_key(const string_t& key);

    virtual bool set(const string_t& key,
                     const string_t& value,
                     int version);

    virtual void new_session();

    static void  node_watcher(zhandle_t*,
                              int type,
                              int state,
                              const char* path,
                              void* ctx);

    struct string_cmp_t {
        bool operator()(const string_t& a, const string_t& b) const {
            return string_t::cmp_eq<ident_t>(a, b);
        }
    };

    struct string_hash_t {
        size_t operator()(const string_t& s) const {
            return s.str().fnv<ident_t>();
        }
    };

    typedef std::unordered_map<string_t,
                               var_handle_t*,
                               string_hash_t,
                               string_cmp_t>
            var_handle_map_t;

    var_handle_map_t var_handle_map_;
    bq_mutex_t       var_handle_map_lock_;

    const string_t path_;

    class watch_item_t;
    class get_item_t;
    class set_item_t;

    friend class watch_item_t;
    friend class get_item_t;
    friend class set_item_t;
};

}  // namespace io_var_store
}  // namespace phantom

#pragma GCC visibility pop
