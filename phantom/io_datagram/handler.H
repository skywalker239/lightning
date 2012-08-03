#pragma once

#include <phantom/pd.H>

#include <pd/base/netaddr.H>

namespace phantom {
namespace io_datagram {

class handler_t {
public:
    virtual void handle_datagram(
        const char* data,
        size_t length,
        const netaddr_t& local_addr,
        const netaddr_t& remote_addr) = 0;

    virtual void stat(out_t& out, bool clear) = 0;
protected:
    inline handler_t() throw() {}
    inline ~handler_t() throw() {}
private:
    void *operator new(size_t);
    void operator delete(void *);
};

}  // namespace io_datagram
}  // namespace phantom
