// vim: set tabstop=4 expandtab:

#include <phantom/io_var_store/io_var_store.H>

#include <phantom/module.H>

namespace phantom {

MODULE(io_var_store);

io_var_store_t::config_t::config_t()
{}

void io_var_store_t::config_t::check(const in_t::ptr_t& p) const {
    if(!backend) {
        config::error(p, "io_var_store.backend must be set");
    }
}

io_var_store_t::io_var_store_t(const string_t& name, const config_t& config)
    : io_t(name, config),
      var_store_t(*config.backend),
      backend_(*config.backend)
{}

io_var_store_t::~io_var_store_t()
{}

void io_var_store_t::init()
{}

void io_var_store_t::fini()
{}

void io_var_store_t::run()
{}

void io_var_store_t::stat(out_t&, bool)
{}

namespace io_var_store {
config_binding_sname(io_var_store_t);
config_binding_type(io_var_store_t, backend_t);
config_binding_value(io_var_store_t, backend);
config_binding_parent(io_var_store_t, io_t, 1);
config_binding_ctor(io_t, io_var_store_t);
}  // namespace io_var_store

}  // namespace phantom
