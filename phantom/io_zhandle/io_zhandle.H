#pragma once

#include <phantom/io.H>

#include <pd/base/thr.H>
#include <pd/base/string.H>

#include <pd/bq/bq_cond.H>

#include <stdio.h>

#include <zookeeper/zookeeper.h>

#pragma GCC visibility push(default)

namespace phantom {

class io_zclient_t;

class io_zhandle_t : public io_t {
public:
    struct config_t : public io_t::config_t {
        string_t servers;
        string_t zookeeper_log;

        inline config_t() throw() {}
        inline ~config_t() throw() {}

        void check(const in_t::ptr_t&) const;
    };

    io_zhandle_t(const string_t& name, const config_t& config);
    ~io_zhandle_t() throw();

    void lock();

    void unlock();

    zhandle_t* wait();

    void register_client(io_zclient_t* client);

    void deregister_client(io_zclient_t* client);
private:
    virtual void init();

    virtual void run();

    virtual void fini();

    virtual void stat(out_t&, bool);

    void do_connect();
    void do_shutdown();

    //! Notifies all clients of this zhandle about session expiration.
    //  Used to reset watches etc.
    void new_session_notify();

    static void global_watcher(zhandle_t* zh,
                               int type,
                               int state,
                               const char* path,
                               void* ctx);

    string_t servers_z_;
    string_t zookeeper_log_path_;

    zhandle_t* zhandle_;
    FILE* zookeeper_log_;

    bool connected_;
    bool fresh_session_;
    bq_cond_t connected_cond_;

    io_zclient_t* clients_;
    thr::spinlock_t clients_lock_;
};

typedef thr::guard_t<io_zhandle_t> zhandle_guard_t;

}  // namespace phantom

#pragma GCC visibility pop
