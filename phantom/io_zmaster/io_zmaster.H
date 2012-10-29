#pragma once

#include <pd/base/thr.H>

#include <pd/bq/bq_cond.H>

#include <pd/lightning/var_base.H>

#include <phantom/io_zconf/io_zconf.H>
#include <phantom/io_zclient/io_zclient.H>

#pragma GCC visibility push(default)

namespace phantom {

class io_zmaster_t : public io_zclient_t {
public:
    struct config_t : public io_zclient_t::config_t {
        int host_id;
        string_t election_path;
        string_t current_master_path;
        config::objptr_t<io_zconf_t> zconf;

        config_t();
        ~config_t() throw() {}
        void check(const in_t::ptr_t& p) const;
    };

    io_zmaster_t(const string_t& name, const config_t& config);
    ~io_zmaster_t() throw();

    //! Volunteers to be master.
    //  If current state is not idle
    //  (i.e. another activate() or deactivate() is in progress)
    //  blocks until the next deactivate() completes.
    //
    //  MUST ONLY BE CALLED FROM A BQ THREAD.
    void activate();

    //! Steps down from master election asynchronously
    //  Cancels activate() in progress, if any.
    //  Returns immediately.
    void deactivate();

private:
    const string_t seq_node_path_base() const;
    const string_t seq_node_value() const;
    const string_t make_full_path(const char* node_name) const;

    //! The part of activate called under lock.
    void do_activate();
    //! The part of deactivate called under lock.
    void do_deactivate();

    //! Final part of master stepdown. Resets seq_node_path,
    //  sets state to INACTIVE, increments the epoch number and
    //  signals the waiters.
    //  Called with state_cond_ locked.
    //  Asserts that state is CANCELING.
    void advance_epoch();

    virtual void new_session();

    static void children_callback(int rc,
                                  const struct String_vector* strings,
                                  const void* data);

    static void create_callback(int rc,
                                const char* value,
                                const void* data);

    static void delete_callback(int rc,
                                const void* data);

    static void watch_callback(int rc,
                               const struct Stat* stat,
                               const void* data);

    static void deletion_watch(zhandle_t* zh,
                               int type,
                               int state,
                               const char* path,
                               void* watcherCtx);

    int predecessor_index(const string_t& key,
                          const struct String_vector* keys) const;

    class __hidden activate_item_t;
    class __hidden callback_data_t;
    class __hidden create_item_t;
    class __hidden delete_item_t;
    class __hidden get_children_item_t;
    class __hidden watch_item_t;
    class __hidden set_master_item_t;

    const int host_id_;
    const string_t election_path_;

    var_base_t current_master_;

    enum state_t {
        INACTIVE,
        ACTIVATING,
        REGISTERING,
        REGISTERED,
        GETTING_CHILDREN,
        SETTING_WATCH,
        WAITING_WATCH,
        WATCHING,
        MASTER,
        CANCELING
    };

    state_t state_;
    int current_epoch_;
    string_t seq_node_path_;
    bq_cond_t state_cond_;

    friend class activate_item_t;
    friend class create_item_t;
    friend class get_children_item_t;
    friend class delete_item_t;
};

}  // namespace phantom

#pragma GCC visibility pop
