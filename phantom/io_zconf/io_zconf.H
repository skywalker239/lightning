// Copyright (C) 2012, Alexander Kharitonov <alexander.kharitonov@gmail.com>
// Copyright (C) 2012, YANDEX LLC.
// This code may be distributed under the terms of the GNU GPL v3.
// See ‘http://www.gnu.org/licenses/gpl.html’.
// vim: set tabstop=4 expandtab:
#pragma once

#include <phantom/io_zclient/io_zclient.H>

#include <unordered_map>

#pragma GCC visibility push(default)

namespace phantom {

class io_zconf_t : public io_zclient_t {
public:
    struct config_t : public io_zclient_t::config_t {
        string_t path;

        inline config_t() throw() {}
        inline ~config_t() throw() {}
        void check(const in_t::ptr_t&) const;
    };

    io_zconf_t(const string_t& name, const config_t& config);
    ~io_zconf_t();

    struct stat_t {
        bool valid;
        string_t value;
        struct Stat stat;

        stat_t()
            : valid(false), ref_count(0)
        {}
        
        //! HACK for map::insert to work
        stat_t(const stat_t& rhs)
            : valid(rhs.valid),
              value(rhs.value),
              stat(rhs.stat),
              ref_count(rhs.ref_count)
        {}

        ~stat_t() {
        }

        bq_cond_t cond;
        int ref_count;
    };

    stat_t* add_var_ref(const string_t& key);
    void remove_var_ref(const string_t& key);

    //! If version is -1, overwrites unconditionally.
    //  Returns true on success.
    bool set(const string_t& key, const string_t& value, int version);
private:
    void new_session();

    string_t full_path(const string_t& key) const;

    //! Callback for node creation/deletion/modification watches.
    static void node_watcher(zhandle_t* zh,
                             int type,
                             int state,
                             const char* path,
                             void* ctx);

    static void data_callback(int rc,
                              const char* value,
                              int vallen,
                              const struct Stat* stat,
                              const void* data);

    static void stat_callback(int rc,
                              const struct Stat* stat,
                              const void* data);

    void update_node(const string_t& key,
                     const char* value,
                     int vallen,
                     const struct Stat* stat);

    void set_no_node(const string_t& key);

    static void set_callback(int rc,
                             const struct Stat* stat,
                             const void* data);

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
                               stat_t,
                               string_hash_t,
                               string_cmp_t>
            var_map_t;

    var_map_t var_map_;
    thr::spinlock_t var_map_lock_;

    const string_t path_;

    class __hidden get_item_t;
    class __hidden set_item_t;
    class __hidden watch_item_t;

    class __hidden callback_data_t;
    class __hidden set_data_t;
    
    friend class get_item_t;
    friend class set_item_t;
};

}  // namespace phantom

#pragma GCC visibility pop
