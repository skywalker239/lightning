#pragma once

#include <phantom/io.H>

#include <pd/base/config_list.H>
#include <pd/base/list.H>
#include <pd/base/thr.H>

#include <pd/bq/bq_cond.H>

#include <unordered_map>

#include <stdio.h>

#include <zookeeper.h>

namespace phantom {

class io_zookeeper_t : public io_t {
public:
    struct config_t : public io_t::config_t {
        string_t servers;
        string_t zookeeper_log;
        config::list_t<string_t> keys;


        inline config_t() throw() {}
        inline ~config_t() throw() {}
        void check(const in_t::ptr_t&) const;
    };

    io_zookeeper_t(const string_t& name, const config_t& config);
    ~io_zookeeper_t() throw() {}

private:
    virtual void init();

    //! Processes the action queue (todo_list_).
    virtual void run();

    virtual void fini();

    //! Assume connected_cond_ to be locked.
    void do_connect();
    void do_shutdown();

    //! Schedules setting the watches for all vars in var_map_.
    void set_watches();

    //! Calls zoo_awexists at key. Must only be called from run().
    void watch_node(const string_t& key);
    //! Calls zoo_awget at key. Must only be called from run().
    void get_node(const string_t& key);

    virtual void stat(out_t&, bool) {}

    //! The global callback for the zookeeper session.
    //  It will receive session connect/disconnect/expiration notifications.
    static void global_watcher(zhandle_t* zh,
                               int type,
                               int state,
                               const char* path,
                               void* ctx);


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

    void update_node(const string_t& key,
                     const char* value,
                     int vallen,
                     const struct Stat* stat);
    void set_no_node(const string_t& key);

    static void stat_callback(int rc,
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

    struct stat_t {
        bool valid;
        bool exists;
        string_t value;
        struct Stat stat;

        stat_t()
            : valid(false), exists(false)
        {}

        ~stat_t() {
        }
    };

    typedef std::unordered_map<string_t,
                               stat_t,
                               string_hash_t,
                               string_cmp_t>
            var_map_t;
    
    string_t servers_z_;
    zhandle_t* zhandle_;
    string_t zookeeper_log_path_;
    FILE* zookeeper_log_;

    bool connected_;
    bool need_to_reset_watches_;
    bq_cond_t connected_cond_;
    
    struct todo_item_t {
        todo_item_t* next;
        todo_item_t** me;

        enum type_t {
            WATCH,
            GET
        };
        
        type_t type;
        string_t key;

        io_zookeeper_t& io_zookeeper;

        todo_item_t(type_t _type,
                    const string_t& _key, 
                    io_zookeeper_t& _io_zookeeeper);

        ~todo_item_t();
    };
    
    todo_item_t* todo_list_;
    todo_item_t** todo_last_;
    bq_cond_t todo_cond_;

    var_map_t var_map_;
    thr::spinlock_t var_map_lock_;

    friend class todo_item_t;
};

}  // namespace phantom
