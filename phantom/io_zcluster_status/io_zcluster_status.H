#pragma once

#include <pd/base/thr.H>

#include <phantom/io_zcluster_status/host_status.H>
#include <phantom/io_zcluster_status/status_listener.H>

#include <phantom/io_zclient/io_zclient.H>

namespace phantom {

class io_zcluster_status_t : public io_zclient_t {
public:
    struct config_t : public io_zclient_t::config_t {
        int host_id;
        string_t path;

        config_t();
        ~config_t() throw() {}
        void check(const in_t::ptr_t& p) const;
    };

    io_zcluster_status_t(const string_t& name, const config_t& config);
    ~io_zcluster_status_t();

    void add_listener(status_listener_t* listener);
    void remove_listener(status_listener_t* listener);

    ref_t<host_status_list_t> cluster_status() const;

    void update(const string_t& new_status);
private:
    string_t status_node_full_path() const;

    string_t make_full_path(const string_t& node_name) const;

    static bool parse_host_id(const string_t& path, int* host_id);
    
    virtual void new_session();

    virtual void run();

    static void node_watch(zhandle_t* zh,
                           int type,
                           int state,
                           const char* path,
                           void* watcherCtx);

    static void children_watch(zhandle_t* zh,
                               int type,
                               int state,
                               const char* path,
                               void* watcherCtx);

    const int host_id_;
    const string_t path_;

    ref_t<host_status_list_t> current_status_;
    int current_status_session_;
    int current_session_;
    string_t last_set_status_;
    int last_successful_set_number_;
    int current_set_number_;
    mutable bq_cond_t state_cond_;

    class __hidden new_session_item_t;
    class __hidden get_item_t;
    class __hidden del_item_t;
    class __hidden set_item_t;

    void signal_listeners(ref_t<host_status_list_t> status);

    status_listener_t* listeners_;
    mutable thr::spinlock_t listeners_lock_;
    
    friend class new_session_item_t;
    friend class del_item_t;
    friend class get_item_t;
    friend class set_item_t;
};

}  // namespace phantom
