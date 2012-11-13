// vim: set tabstop=4 expandtab:
#pragma once

#include <phantom/io.H>

#include <phantom/io_zhandle/io_zhandle.H>

#pragma GCC visibility push(default)

namespace phantom {

class io_zclient_t : public io_t {
public:
    struct config_t : public io_t::config_t {
        config::objptr_t<io_zhandle_t> zhandle;

        inline config_t() throw() {}
        inline ~config_t() throw() {}

        void check(const in_t::ptr_t&) const;
    };

    class todo_item_t {
        io_zclient_t* zclient_;
        todo_item_t* next_;
        todo_item_t** me_;
    public:
        todo_item_t(io_zclient_t* zclient);
        virtual ~todo_item_t();

        void attach() throw();
        void detach() throw();

        virtual void apply() = 0;
    };

    io_zclient_t(const string_t& name, const config_t& config);
    ~io_zclient_t() throw();
protected:
    io_zhandle_t& zhandle_;

    void schedule(todo_item_t* todo_item);
    
private:
    virtual void init();
    virtual void run();
    virtual void fini();
    virtual void stat(out_t&, bool);

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
