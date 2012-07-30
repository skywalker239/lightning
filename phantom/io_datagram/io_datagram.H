#pragma once

#include <phantom/io.H>

#include <pd/base/fd.H>
#include <pd/base/netaddr.H>
#include <pd/base/time.H>

namespace phantom {

namespace io_datagram {
class handler_t;
}  // namespace io_datagram

class io_datagram_t : public io_t {
public:
    typedef io_datagram::handler_t handler_t;

    struct config_t : io_t::config_t {
        config_binding_type_ref(handler_t);

        config::objptr_t<handler_t> handler;
        config::enum_t<bool> reuse_addr;
        interval_t poll_interval;

        config_t() throw();
        void check(const in_t::ptr_t& ptr) const;
        inline ~config_t() throw() {}
    };

    // 65535 - 8 (udp header) - 20 (ip header).
    static const size_t maximum_datagram_size = 65507;
protected:
    io_datagram_t(const string_t& name, const config_t& config);
    ~io_datagram_t() throw();

private:
    virtual const netaddr_t& bind_addr() const throw() = 0;
    virtual netaddr_t* new_netaddr() const = 0;
    virtual void fd_setup(int fd) const = 0;
    virtual const fd_ctl_t* ctl() const = 0;

    virtual void init();
    virtual void run();
    virtual void stat(out_t& out, bool clear);
    virtual void fini();

    void loop(int fd);

    int fd_;
    bool reuse_addr_;

    handler_t& handler_;
};

}  // namespace phantom
