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

    io_zclient_t(const string_t& name, const config_t& config);
    ~io_zclient_t() throw() {}
private:
    virtual void init();
    virtual void run();
    virtual void fini();
    virtual void stat(out_t&, bool);

    virtual void new_session();

    io_zclient_t* next_;
    io_zclient_t** me_;
    
    io_zhandle_t* zhandle_;

    friend class io_zhandle_t;
};

}  // namespace phantom

#pragma GCC visibility pop
