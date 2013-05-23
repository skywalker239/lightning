#pragma once

#include <pd/base/ref.H>
#include <pd/base/netaddr_ipv4.H>
#include <pd/lightning/pi_ext.H>

#include <phantom/io.H>

#pragma GCC visibility push(default)

namespace phantom {

class lightning_link_t : public ref_count_atomic_t {
public:
    virtual ref_t<pi_ext_t> send(ref_t<pi_ext_t> request) = 0;
    virtual void close() = 0;

    friend class ref_t<lightning_link_t>;
};

class io_lightning_network_t : public io_t {
public:
    struct config_t : public io_t::config_t {
        config_t() {}
        void check(const in_t::ptr_t& p) const;
    };

    io_lightning_network_t(const string_t& name, const config_t& config);

    virtual void init();
    virtual void run();
    virtual void fini();

    virtual void stat(out_t&, bool);

    ref_t<lightning_link_t> connect(netaddr_ipv4_t addr);
private:
};

} // namespace phantom

#pragma GCC visibility pop
