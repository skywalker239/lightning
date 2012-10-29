#pragma once

#include <phantom/io.H>

#include <pd/base/config_list.H>
#include <pd/lightning/simple_var.H>
#include <pd/lightning/snapshot_var.H>

#include <phantom/io_zconf/io_zconf.H>

#include <vector>

namespace phantom {

class io_toy_zk_client_t : public io_t {
public:
    struct config_t : public io_t::config_t {
        config::objptr_t<io_zconf_t> zconf;
        config::list_t<string_t> keys;
        string_t snapshot_var;
        string_t set_key;

        inline config_t() throw() {}
        inline ~config_t() throw() {}

        void check(const in_t::ptr_t&) const;
    };

    io_toy_zk_client_t(const string_t& name, const config_t& config);
    ~io_toy_zk_client_t();
private:
    virtual void init();
    virtual void run();
    virtual void fini();

    virtual void stat(out_t&, bool) {}

    void loop(var_base_t& var);

    void snap_loop();

    void set_loop();

    io_zconf_t& zconf_;
    simple_var_t<string_t> set_var_;
    snapshot_var_t snapshot_var_;
    std::vector<simple_var_t<string_t>* > vars_;
};

}  // namespace phantom
