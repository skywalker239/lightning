#pragma once

#include <phantom/pd.H>
#include <pd/base/netaddr.H>
#include <pd/lightning/pi_ext.H>

#pragma GCC visibility push(default)

namespace phantom {
namespace io_blob_receiver {

class handler_t {
public:
    virtual ~handler_t() throw() {}

    virtual void handle(ref_t<pi_ext_t> blob,
                        const netaddr_t& remote_addr) = 0;

    virtual void stat(out_t& out, bool clear) = 0;
private: // don't use
    void *operator new(size_t);
    void operator delete(void *);
};

}}  // namespace phantom::io_blob_receiver

#pragma GCC visibility pop
