// Copyright (C) 2012, Alexander Kharitonov <alexander.kharitonov@gmail.com>
// Copyright (C) 2012, YANDEX LLC.
// This code may be distributed under the terms of the GNU GPL v3.
// See ‘http://www.gnu.org/licenses/gpl.html’.
// vim: set tabstop=4 expandtab:
#pragma once

#include <phantom/io.H>

#include <pd/base/thr.H>
#include <pd/base/string.H>

#include <pd/bq/bq_cond.H>

#include <pd/zookeeper/zhandle_holder.H>

#include <stdio.h>

#include <zookeeper/zookeeper.h>

#pragma GCC visibility push(default)

namespace phantom {

class io_zclient_t;

//! Provides a single Zookeeper session handle to other
//  objects.
//  Manages the session state:
//    * Tries to reconnect upon connection loss
//    * Notifies clients about ZK session expiration.
class io_zhandle_t : public io_t,
                     public zhandle_holder_t
{
public:
    struct config_t : public io_t::config_t {
        //! ZK hosts string, host:port, comma separated.
        string_t servers;
        //! Path to the zookeeper log file.
        string_t zookeeper_log;
        //! Zookeeper log level.
        config::enum_t<ZooLogLevel> zookeeper_log_level;

        inline config_t() throw()
            : zookeeper_log_level(ZOO_LOG_LEVEL_ERROR)
        {}
        inline ~config_t() throw() {}

        void check(const in_t::ptr_t&) const;
    };

    io_zhandle_t(const string_t& name, const config_t& config);
    ~io_zhandle_t() throw();

    //! If there is a valid open Zookeeper session, returns its zhandle
    //  immediately.
    //  Otherwise blocks until a session is established.
    virtual zhandle_t* wait();

    //! Clients must register themselves with io_zhandle
    //  to be notified of ZK session expirations
    //  (because all pending watches must be reset then).
    //
    //  io_zclient_t calls this in its init().
    void register_client(io_zclient_t* client);

    //! Removes client from the registered clients list.
    //
    //  io_zclient_t calls this in its fini().
    void deregister_client(io_zclient_t* client);
private:
    virtual void init();

    virtual void run();

    virtual void fini();

    virtual void stat(out_t&, bool);

    void do_connect();
    void do_shutdown();

    //! Notifies all clients of this zhandle about session expiration.
    //  Clients use this to reset watches etc.
    //  Blocks.
    void new_session_notify();

    //! The global watcher for the ZK session.
    //  Handles automatic session (re)establishment.
    //
    //  Clients MUST NOT register watches that will fall through
    //  to the global watcher (i.e. they MUST use local watch objects).
    //
    //  Any spurious watch event that reaches this watcher will cause
    //  a fatal error.
    static void global_watcher(zhandle_t* zh,
                               int type,
                               int state,
                               const char* path,
                               void* ctx);

    string_t servers_z_;
    string_t zookeeper_log_path_;

    zhandle_t* zhandle_;
    FILE* zookeeper_log_;
    ZooLogLevel zookeeper_log_level_;

    bool connected_;
    bool session_is_fresh_;
    bq_cond_t connected_cond_;

    io_zclient_t* clients_;
    thr::spinlock_t clients_lock_;
};

}  // namespace phantom

#pragma GCC visibility pop
