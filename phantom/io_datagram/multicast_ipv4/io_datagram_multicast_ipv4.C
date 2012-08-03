#include "../io_datagram.H"

#include <phantom/module.H>

#include <pd/base/exception.H>
#include <pd/base/log.H>
#include <pd/base/ipv4.H>
#include <pd/base/netaddr_ipv4.H>

#include <netinet/in.h>

namespace phantom {

MODULE(io_datagram_multicast_ipv4);

class io_datagram_multicast_ipv4_t : public io_datagram_t {
public:
    struct config_t : io_datagram_t::config_t {
        address_ipv4_t multicast_group;
        uint16_t port;

        inline config_t() throw() :
            io_datagram_t::config_t(),
            multicast_group(),
            port(0)
        {}

        inline void check(const in_t::ptr_t& ptr) const {
            io_datagram_t::config_t::check(ptr);

            if(!port) {
                config::error(ptr, "port is required");
            }
        }

        inline ~config_t() throw()
        {}
    };

    inline io_datagram_multicast_ipv4_t(const string_t& name, const config_t& config)
        : io_datagram_t(name, config),
          multicast_addr_(config.multicast_group, config.port),
          bind_addr_(address_ipv4_t(INADDR_ANY), config.port)
    {}

    inline ~io_datagram_multicast_ipv4_t() throw()
    {}
private:
    virtual const netaddr_t& bind_addr() const throw();
    virtual netaddr_t* new_netaddr() const;
    virtual void fd_setup(int fd) const;

    netaddr_ipv4_t multicast_addr_;
    netaddr_ipv4_t bind_addr_;
};

const netaddr_t& io_datagram_multicast_ipv4_t::bind_addr() const throw() {
    return bind_addr_;
}

netaddr_t* io_datagram_multicast_ipv4_t::new_netaddr() const {
    return new netaddr_ipv4_t;
}

void io_datagram_multicast_ipv4_t::fd_setup(int fd) const {
    struct ip_mreq mreq;
    // TODO(skywalker): we bind to all interfaces for now.
    mreq.imr_interface.s_addr = htonl(INADDR_ANY);
    mreq.imr_multiaddr.s_addr = htonl(multicast_addr_.address().value);
    if(setsockopt(fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0) {
        throw exception_sys_t(log::error, errno, "setsockopt, IP_ADD_MEMBERSHIP: %m");
    }
}

namespace io_datagram_ipv4 {
config_binding_sname(io_datagram_multicast_ipv4_t);
config_binding_value(io_datagram_multicast_ipv4_t, multicast_group);
config_binding_value(io_datagram_multicast_ipv4_t, port);
config_binding_parent(io_datagram_multicast_ipv4_t, io_datagram_t, 1);
config_binding_ctor(io_t, io_datagram_multicast_ipv4_t);
}  // namespace io_datagram_ipv4

}  // namespace phantom
