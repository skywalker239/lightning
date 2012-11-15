// vim: set tabstop=4 expandtab:
#pragma once

#include <phantom/io.H>

#include <phantom/io_zhandle/io_zhandle.H>

#pragma GCC visibility push(default)

namespace phantom {

//! An abstract base class for phantom objects that want to access Zookeeper.
//
//  Holds a reference to an io_zhandle_t and processes a queue of actions
//  that do something with a Zookeeper session handle.
//
//  The actions are to be defined in descendant classes as descendants of
//  todo_item_t (see below).
//
class io_zclient_t : public io_t {
public:
    struct config_t : public io_t::config_t {
        config::objptr_t<io_zhandle_t> zhandle;

        inline config_t() throw() {}
        inline ~config_t() throw() {}

        void check(const in_t::ptr_t&) const;
    };

    //! An abstract action acting on a zhandle.
    class todo_item_t {
    public:
        todo_item_t();
        virtual ~todo_item_t();

        //! Attaches this item to the action queue of
        //  zclient.
        void attach(io_zclient_t* zclient) throw();
        //!  Detaches this item from its current queue
        //   (called by the zclient that it is attached to).
        void detach() throw();

        //! Applies this action, using zhandle as the
        //  current Zookeeper session handle.
        //  (called by the zclient that has just detached this item)
        virtual void apply(zhandle_t* zhandle) = 0;
    private:
        io_zclient_t* zclient_;
        todo_item_t* next_;
        todo_item_t** me_;

        friend class io_zclient_t;
    };

    io_zclient_t(const string_t& name, const config_t& config);
    ~io_zclient_t() throw();
protected:
    io_zhandle_t& zhandle_;

    //! Puts a new action on the queue.
    void schedule(todo_item_t* todo_item);
    
private:
    //! Registers this client with io_zhandle.
    //  DO NOT OVERRIDE.
    virtual void init();
    //! Processes the action queue.
    virtual void run();
    //! Deregisters this client with io_zhandle.
    //  DO NOT OVERRIDE.
    virtual void fini();

    virtual void stat(out_t&, bool);

    todo_item_t* next_todo();

    zhandle_t*   get_zhandle();

    //! Handles Zookeeper session reestablishment.
    //  (resets watches etc)
    //  This function completely blocks io_zhandle,
    //  so it should be fast.
    virtual void new_session() = 0;

    io_zclient_t* next_;
    io_zclient_t** me_;
    
    todo_item_t* todo_list_;
    todo_item_t** todo_last_;
    bq_cond_t todo_cond_;

    friend class io_zhandle_t;
    friend class todo_item_t;
};

}  // namespace phantom

#pragma GCC visibility pop
