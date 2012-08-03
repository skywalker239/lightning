#include "../io_datagram.H"

#include <phantom/module.H>

#include <pd/base/netaddr_ipv4.H>

namespace phantom {

MODULE(io_datagram_ipv4);

class io_datagram_ipv4_t : public io_datagram_t {
public:
    struct config_t : io_datagram_t::config_t {
        address_ipv4_t address;
        uint16_t port;

        inline config_t() throw() :
            io_datagram_t::config_t(),
            address(),
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

    inline io_datagram_ipv4_t(const string_t& name, const config_t& config)
        : io_datagram_t(name, config),
          netaddr_(config.address, config.port)
    {}

    inline ~io_datagram_ipv4_t() throw()
    {}
private:
    virtual const netaddr_t& bind_addr() const throw();
    virtual netaddr_t* new_netaddr() const;
    virtual void fd_setup(int fd) const;

    netaddr_ipv4_t netaddr_;
};

const netaddr_t& io_datagram_ipv4_t::bind_addr() const throw() {
    return netaddr_;
}

netaddr_t* io_datagram_ipv4_t::new_netaddr() const {
    return new netaddr_ipv4_t;
}

void io_datagram_ipv4_t::fd_setup(int /* fd */) const {
}

namespace io_datagram_ipv4 {
config_binding_sname(io_datagram_ipv4_t);
config_binding_value(io_datagram_ipv4_t, address);
config_binding_value(io_datagram_ipv4_t, port);
config_binding_parent(io_datagram_ipv4_t, io_datagram_t, 1);
config_binding_ctor(io_t, io_datagram_ipv4_t);
}  // namespace io_datagram_ipv4

}  // namespace phantom
