#pragma once

#include <pd/lightning/var_base.H>
#include <pd/base/netaddr_ipv4.H>

namespace pd {

//! Represents a static list of hosts participating in Paxos.
//  The hosts are numbered with host ids in the range [0, num_of_hosts).
class hosts_var_t : public var_base_t {
public:
    hosts_var_t(const string_t& key, phantom::io_zconf_t* io_zconf);

    //! False iff failed to parse from the Zookeeper node.
    bool valid() const;

    //! Total number of hosts.
    size_t size() const;

    //! A host is defined by a name, a datacenter name and an IPv4 address.
    struct host_t {
        string_t hostname;
        string_t datacenter;
        address_ipv4_t address;
    };

    //! Returns data for host host_id.
    const host_t& host(size_t host_id) const;
private:
    virtual void update_impl();
};

}  // namespace pd
