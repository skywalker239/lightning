// vim: set tabstop=4 expandtab:
#pragma once

#include <pd/base/config.H>

#include <pd/lightning/var_store.H>
#include <pd/lightning/var_store_backend.H>

#include <phantom/io.H>

#pragma GCC visibility push(default)

namespace phantom {

namespace io_var_store {

typedef var_store_backend_t backend_t;

}  // namespace io_var_store

class io_var_store_t : public io_t,
                       public var_store_t 
{
public:
    typedef io_var_store::backend_t backend_t;

    struct config_t : public io_t::config_t {
        config_binding_type_ref(backend_t);
        config::objptr_t<backend_t> backend;

        inline config_t() throw();
        inline ~config_t() throw() {}
        void check(const in_t::ptr_t&) const;
    };

    io_var_store_t(const string_t& name, const config_t& config);
    ~io_var_store_t();
private:
    virtual void init();
    virtual void run();
    virtual void fini();

    virtual void stat(out_t&, bool);

    backend_t& backend_;
};

}  // namespace phantom

#pragma GCC visibility pop
