#pragma once

#include <phantom/io.H>
#include <phantom/io_transport_config/io_transport_config.H>

#include <pd/base/ref.H>
#include <pd/base/queue.H>
#include <pd/base/netaddr.H>
#include <pd/lightning/pi_ext.H>

#pragma GCC visibitily push(default)

namespace phantom {

class io_ring_sender_t : public io_t {
public:
    struct config_t : public io_t::config_t {
        uint32_t host_id;
        config::objptr_t<io_transport_config_t> transport_config;

        config_t() throw() {}
        ~config_t() throw() {}
        void check(const in_t::ptr_t& p) const;
    };

    io_ring_sender_t(const string_t& name, const config_t& config);
    ~io_ring_sender_t() throw();

    void send_to_ring(ref_t<pi_ext_t> blob);

    virtual void init();
    virtual void run();
    virtual void fini();

private:
    uint32_t host_id_;
    io_transport_config_t& transport_config_;
};

}

#pragma GCC visibility pop
