#pragma once

#include <phantom/io_zconf/io_zconf.H>
#include <phantom/io.H>
#include <pd/base/netaddr_ipv4.H>
#include <pd/bq/bq_cond.H>
#include <pd/lightning/hosts_var.H>
#include <pd/lightning/ring_var.H>
#include <pd/lightning/simple_var.H>
#include <pd/lightning/snapshot_var.H>

#pragma GCC visibility push(default)

namespace phantom {

//! This object is the single point of entry
//  for everything in lightning that depends on configuration
//  stored in Zookeeper.
//  It exports a bunch of variables for everything managed in ZK
//  and some helper functions.
class io_transport_config_t : public io_t {
public:
    struct config_t : public io_t::config_t {
        string_t path;
        config::objptr_t<io_zconf_t> zconf;

        config_t() throw() {}
        ~config_t() throw() {}
        void check(const in_t::ptr_t& p) const;
    };

    io_transport_config_t(const string_t& name, const config_t& config);
    ~io_transport_config_t() throw();

    //  ----------------------------------------------------------------------
    //  Static variables
    //  ----------------------------------------------------------------------

    //! The static host list for this instance.
    inline hosts_var_t hosts() const {
        return hosts_;
    }

    //! The multicast group for this instance.
    inline simple_var_t<address_ipv4_t> multicast_group() const {
        return multicast_group_;
    }

    //! The multicast port for this instance.
    inline simple_var_t<uint16_t> multicast_port() const {
        return multicast_port_;
    }

    //! The ring port for this instance.
    inline simple_var_t<uint16_t> ring_port() const {
        return ring_port_;
    }

    //! The recovery port for this instance.
    inline simple_var_t<uint16_t> recovery_port() const {
        return recovery_port_;
    }

    //! The client value port for this instance.
    inline simple_var_t<uint16_t> client_port() const {
        return client_port_;
    }

    //  ----------------------------------------------------------------------
    //  Dynamic variables
    //  ----------------------------------------------------------------------

    //! Current master for this instance.
    inline simple_var_t<uint32_t> master() const {
        return master_;
    }

    //! Current ring for this instance.
    inline ring_var_t ring() const {
        return ring_;
    }

    //! Current snapshot for this instance.
    inline snapshot_var_t snapshot() const {
        return snapshot_;
    }

    //  ----------------------------------------------------------------------
    //  Helper functions
    //  ----------------------------------------------------------------------
    
    //! The complete address of the ring service of host_id.
    const netaddr_ipv4_t ring_address(uint32_t host_id);

    //! The complete address to send multicasts.
    const netaddr_ipv4_t multicast_address();

    //! The complete address of the recovery service of host_id.
    const netaddr_ipv4_t recovery_address(uint32_t host_id);

private:
    //! The common code for *_update(host_id).
    const netaddr_ipv4_t host_address(uint32_t host_id, uint16_t port);

    //! path + '/' + node
    static const string_t node_name(const string_t& path, const string_t& node);

    virtual void init();
    virtual void fini();
    virtual void run();

    //! TODO(skywalker): dump current configuration.
    virtual void stat(out_t&, bool) {}

    hosts_var_t hosts_;
    simple_var_t<address_ipv4_t> multicast_group_;
    simple_var_t<uint16_t> multicast_port_;
    simple_var_t<uint16_t> ring_port_;
    simple_var_t<uint16_t> recovery_port_;
    simple_var_t<uint16_t> client_port_;

    simple_var_t<uint32_t> master_;
    ring_var_t ring_;
    snapshot_var_t snapshot_;
};

}  // namespace phantom

#pragma GCC visibility pop
