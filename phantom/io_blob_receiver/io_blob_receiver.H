// Copyright (C) 2012, Alexander Kharitonov <alexander.kharitonov@gmail.com>
// Copyright (C) 2012, YANDEX LLC.
// This code may be distributed under the terms of the GNU GPL v3.
// See ‘http://www.gnu.org/licenses/gpl.html’.
// vim: set tabstop=4 expandtab:
#pragma once

#include <pd/lightning/pi_ext.H>

#include <pd/base/config_enum.H>
#include <pd/base/ipv4.H>
#include <pd/base/netaddr_ipv4.H>
#include <pd/base/ref.H>
#include <pd/base/time.H>

#include <phantom/io.H>

#include <phantom/io_blob_receiver/blob_fragment_pool.H>

namespace phantom {

namespace io_blob_receiver {

class handler_t;

}  // namespace io_blob_receiver

class io_blob_receiver_t : public io_t {
public:
    typedef io_blob_receiver::handler_t handler_t;

    struct config_t : public io_t::config_t {
        config_binding_type_ref(handler_t);

        address_ipv4_t address;
        uint16_t port;
        config::enum_t<bool> multicast;
        interval_t reassembly_timeout;
        config::objptr_t<handler_t> handler;
        size_t pending_blob_limit;

        config_t();
        ~config_t() throw() {}
        void check(const in_t::ptr_t& p) const;
    };

    io_blob_receiver_t(const string_t& name, const config_t& config);
    ~io_blob_receiver_t() throw();
private:
    struct header_t {
        uint64_t guid;
        uint32_t size;
        uint32_t begin;
        uint32_t part_number;
    } __attribute__((packed));

    virtual void init();
    virtual void fini();
    virtual void run();
    virtual void stat(out_t& out, bool clear);

    void timeout_blob(uint64_t guid);

    const netaddr_ipv4_t address_;
    const bool multicast_;
    const interval_t reassembly_timeout_;
    handler_t& handler_;

    blob_fragment_pool_t fragment_pool_;

    int fd_;

};

}  // namespace phantom
