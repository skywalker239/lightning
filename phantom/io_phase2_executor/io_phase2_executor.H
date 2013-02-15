#pragma once
// Copyright (C) 2012, Fedor Korotkiy <prime@yandex-team.ru>
// Copyright (C) 2012, YANDEX LLC.
// This code may be distributed under the terms of the GNU GPL v3.
// See ‘http://www.gnu.org/licenses/gpl.html’.
// vim: set tabstop=4 expandtab:
#include <pd/base/config.H>
#include <pd/base/netaddr_ipv4.H>
#include <pd/lightning/defs.H>

#include <phantom/pd.H>
#include <phantom/io_blob_sender/io_blob_sender.H>
#include <phantom/io_blob_receiver/handler.H>

#pragma GCC visibility push(default)
namespace phantom {

class io_phase2_executor_t : public io_paxos_executor_t,
                             public io_blob_handler::handler_t {
public:
    struct config_t : public io_t::config_t {
        config::objptr_t<io_blob_sender_t> blob_sender;
    };

    io_phase2_executor_t(const string_t& name,
                         const config_t& config);


private:
    io_blob_sender_t* blob_sender_;
};

} // namespace phantom
#pragma GCC visibility pop
