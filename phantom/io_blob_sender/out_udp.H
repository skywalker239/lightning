#pragma once

#include <pd/base/out.H>
#include <pd/base/netaddr.H>
#include <pd/base/time.H>
#include <phantom/pd.H>

#include <sys/types.h>
#include <sys/socket.h>

namespace phantom {

class out_udp_t : public out_t {
public:
    out_udp_t(char* buffer,
                 size_t buffer_size,
                 uint32_t blob_size,
                 int fd,
                 const netaddr_t& address,
                 uint64_t guid,
                 uint64_t* bytes_sent,
                 uint64_t* packets_sent,
                 uint64_t* dups);
    ~out_udp_t() throw();
private:
    struct header_t {
        uint64_t guid;
        uint32_t size;
        uint32_t begin;
        uint32_t part_number;
    } __attribute__((packed));

    virtual void flush();

    ssize_t do_sendmsg(const struct msghdr* msg);

    ssize_t bq_sendmsg(int fd, const struct msghdr* msg, int flags, interval_t* timeout);

    const uint32_t blob_size_;
    const int fd_;
    const netaddr_t& address_;
    const uint64_t guid_;
    
    uint64_t* bytes_sent_;
    uint64_t* packets_sent_;
    uint64_t* dups_;

    uint32_t sent_parts_;
    uint32_t sent_bytes_;
};

}  // namespace phantom