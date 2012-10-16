#pragma once

#include <phantom/io.H>

#include <pd/base/config_list.H>

#include <phantom/io_zmaster/io_zmaster.H>

#include <vector>

namespace phantom {

class io_zmaster_test_t : public io_t {
public:
    struct config_t : public io_t::config_t {
        config::objptr_t<io_zmaster_t> zmaster;

        inline config_t() throw() {}
        inline ~config_t() throw() {}

        void check(const in_t::ptr_t&) const;
    };

    io_zmaster_test_t(const string_t& name, const config_t& config);
    ~io_zmaster_test_t();
private:
    virtual void init();
    virtual void run();
    virtual void fini();

    virtual void stat(out_t&, bool) {}

    io_zmaster_t& zmaster_;
};

}  // namespace phantom
