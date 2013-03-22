// Copyright (C) 2012, Alexey Pervushin <billyevans@yandex-team.ru>
// Copyright (C) 2012, YANDEX LLC.
// This code may be distributed under the terms of the GNU GPL v3.
// See ‘http://www.gnu.org/licenses/gpl.html’.
// vim: set tabstop=4 expandtab:
#pragma once

#include <atomic>

#include <phantom/io_stream/proto.H>
#include <phantom/module.H>
#include <phantom/io_proposer_pool/io_proposer_pool.H>
#include <phantom/io_guid/io_guid.H>

#include <pd/base/config.H>

namespace phantom { namespace io_stream {


//! Receive values from clients for commiting it to replica.
//  After receive value put it with guid in pending pool
//  Class is proto_t for io_stream

class proto_value_receiver_t : public proto_t {
public:
    struct config_t {
        config::objptr_t<io_proposer_pool_t> proposer_pool;
        config::objptr_t<io_guid_t> value_id_generator;

        inline config_t() throw() { }
        inline ~config_t() throw() { }
        void check(const in_t::ptr_t&) const;
    };

    proto_value_receiver_t(const string_t&, const config_t& config) throw();
    inline ~proto_value_receiver_t() throw() { }
    void set_master(bool master) throw();
    size_t get_recv_count() const throw();
    value_id_t get_last_pushed_value_id() const throw();
private:
    std::atomic<size_t> received_count_;
    std::atomic<value_id_t> last_pushed_id_;

    std::atomic<bool> master_;
    io_proposer_pool_t& proposer_pool_;
    io_guid_t& value_id_generator_;

    virtual bool request_proc(
        in_t::ptr_t& ptr, out_t&, const netaddr_t&, const netaddr_t&
    );

    virtual void stat(out_t &out, bool clear);
};

}} // namespace phantom::io_stream
