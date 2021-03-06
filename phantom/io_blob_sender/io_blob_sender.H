// Copyright (C) 2012, Alexander Kharitonov <alexander.kharitonov@gmail.com>
// Copyright (C) 2012, YANDEX LLC.
// This code may be distributed under the terms of the GNU GPL v3.
// See ‘http://www.gnu.org/licenses/gpl.html’.
// vim: set tabstop=4 expandtab:
#pragma once

#include <pd/base/config_enum.H>
#include <pd/base/ipv4.H>
#include <pd/base/netaddr_ipv4.H>

#include <pd/lightning/pi_ext.H>

#include <phantom/io.H>
#include <phantom/io_blob_sender/out_udp.H>

#pragma GCC visibility push(default)

namespace phantom {

class io_blob_sender_t : public io_t {
public:
    struct config_t : public io_t::config_t {
        address_ipv4_t address;
        uint16_t port;
        uint32_t max_datagram_size;
        config::enum_t<bool> multicast;

        config_t() throw();
        ~config_t() throw() {};
        void check(const in_t::ptr_t& p) const;
    };

    io_blob_sender_t(const string_t& name, const config_t& config);
    ~io_blob_sender_t();

    void send(uint64_t guid,
              ref_t<pi_ext_t> value,
              const netaddr_ipv4_t& destination = netaddr_ipv4_t());
private:
    virtual void init();
    virtual void run();
    virtual void fini();

    virtual void stat(out_t& out, bool clear);

    const netaddr_ipv4_t address_;
    const bool multicast_;
    const uint32_t max_datagram_size_;

    int fd_;

    uint64_t blobs_sent_;
    uint64_t bytes_sent_;
    uint64_t packets_sent_;
    uint64_t dups_;
};

}  // namespace phantom

#pragma GCC visibility pop
