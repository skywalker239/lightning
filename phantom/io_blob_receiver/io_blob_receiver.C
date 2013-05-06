// Copyright (C) 2012, Alexander Kharitonov <alexander.kharitonov@gmail.com>
// Copyright (C) 2012, YANDEX LLC.
// This code may be distributed under the terms of the GNU GPL v3.
// See ‘http://www.gnu.org/licenses/gpl.html’.
// vim: set tabstop=4 expandtab:
#include <phantom/io_blob_receiver/io_blob_receiver.H>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

#include <memory>

#include <pd/base/exception.H>
#include <pd/bq/bq_util.H>

#include <phantom/io_blob_receiver/handler.H>
#include <phantom/io_blob_sender/out_udp.H>
#include <phantom/module.H>

namespace phantom {

MODULE(io_blob_receiver);

io_blob_receiver_t::config_t::config_t()
    : port(0),
      multicast(false),
      pending_blob_limit(1024),
      hash_table_size(1024)
{}

void io_blob_receiver_t::config_t::check(const in_t::ptr_t& p) const {
    io_t::config_t::check(p);

    if(multicast) {
        if(!address) {
            config::error(p, "io_blob_receiver_t.address must be set");
        }

        if(!port) {
            config::error(p, "io_blob_receiver_t.port must be set");
        }
    }

    if(!handler) {
        config::error(p, "io_blob_receiver_t.handler must be set");
    }
}

io_blob_receiver_t::io_blob_receiver_t(const string_t& name, const config_t& config)
    : io_t(name, config),
      address_(netaddr_ipv4_t(config.address, config.port)),
      multicast_(config.multicast),
      handler_(config.handler),
      fragment_pool_(config.pending_blob_limit, config.hash_table_size),
      fd_(-1)
{}

io_blob_receiver_t::~io_blob_receiver_t() throw() {
    assert(fd_ == -1);
}

void io_blob_receiver_t::init() {
    fd_ = ::socket(AF_INET, SOCK_DGRAM, 0);
    if(fd_ < 0) {
        throw exception_sys_t(log::error, errno, "socket: %m");
    }

    if(bq_fd_setup(fd_) < 0) {
        throw exception_sys_t(log::error, errno, "bq_fd_setup: %m");
    }

    int i = 1;
    if(setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &i, sizeof(i)) < 0) {
        throw exception_sys_t(log::error, errno, "setsockopt, SO_REUSEADDR, 1: %m");
    }

    if(!multicast_) {
        if(::bind(fd_, address_.sa, address_.sa_len) < 0) {
            throw exception_sys_t(log::error, errno, "bind: %m");
        }
    } else {
        netaddr_ipv4_t bind_address(INADDR_ANY, address_.port());
        if(::bind(fd_, bind_address.sa, bind_address.sa_len) < 0) {
            throw exception_sys_t(log::error, errno, "bind: %m");
        }
        struct ip_mreq mreq;
        // TODO(skywalker): this binds to all interfaces.
        mreq.imr_interface.s_addr = htonl(INADDR_ANY);
        mreq.imr_multiaddr.s_addr = htonl(address_.address().value);
        if(::setsockopt(fd_, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0) {
            throw exception_sys_t(log::error, errno, "setsockopt, IP_ADD_MEMBERSHIP: %m");
        }
    }
}

void io_blob_receiver_t::fini() {
    if(fd_ >= 0) {
        if(::close(fd_) < 0) {
            log_error("close: %m");
        }
        fd_ = -1;
    }
}

void io_blob_receiver_t::stat(out_t& , bool ) {

}

void io_blob_receiver_t::run() {
    while(true) {
        char buffer[8950];

        netaddr_ipv4_t remote_addr;

        ssize_t res = bq_recvfrom(fd_, buffer, sizeof(buffer), remote_addr.sa, &(remote_addr.sa_len), NULL);

        if(res < 0) {
            throw exception_sys_t(log::error, errno, "bq_recvfrom: %m");
        }

        if(static_cast<size_t>(res) < sizeof(out_udp_t::header_t)) {
            continue;
        }

        out_udp_t::header_t* header = (out_udp_t::header_t*) buffer;
        char* data = buffer + sizeof(out_udp_t::header_t);
        size_t data_length = res - sizeof(out_udp_t::header_t);

        if(header->begin > header->size ||
           header->begin + data_length > header->size)
        {
            continue;
        }

//        log_debug("received blob part(guid=%ld, total_size=%d, begin=%d, part_num=%d, size=%ld)",
//                  header->guid, header->size, header->begin, header->part_num, data_length);

        ref_t<blob_fragment_pool_t::blob_t> blob = fragment_pool_.lookup(header->guid, header->size);

        // TODO(prime@): ugly with lots of copying, rewrite
        if(blob->update(header->begin, str_t(data, data_length))) {
//                log_debug("blob completed(guid=%ld)", header->guid);

                fragment_pool_.remove(header->guid);

                string_t blob_str = string(blob->get_data());

                ref_t<pi_ext_t> udp_cmd;
                try {
                    auto in = in_t::ptr_t(blob_str);

                    udp_cmd = pi_ext_t::parse(in, &pi_t::parse_app);

                    handler_->handle(udp_cmd, remote_addr);
                } catch(exception_t& ex) {
                    ex.log();
                }
        }
    }
}

namespace io_blob_receiver {
config_binding_sname(io_blob_receiver_t);

config_binding_parent(io_blob_receiver_t, io_t, 1);
config_binding_ctor(io_t, io_blob_receiver_t);

config_binding_type(io_blob_receiver_t, handler_t);

config_binding_value(io_blob_receiver_t, address);
config_binding_value(io_blob_receiver_t, port);
config_binding_value(io_blob_receiver_t, multicast);
config_binding_value(io_blob_receiver_t, handler);
config_binding_value(io_blob_receiver_t, pending_blob_limit);
config_binding_value(io_blob_receiver_t, hash_table_size);
} // namespace blob_receiver

}  // namespace phantom
