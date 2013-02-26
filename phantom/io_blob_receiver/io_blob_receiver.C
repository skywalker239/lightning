// Copyright (C) 2012, Alexander Kharitonov <alexander.kharitonov@gmail.com>
// Copyright (C) 2012, YANDEX LLC.
// This code may be distributed under the terms of the GNU GPL v3.
// See ‘http://www.gnu.org/licenses/gpl.html’.
// vim: set tabstop=4 expandtab:
//#include <phantom/io_blob_receiver/io_blob_receiver.H>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

#include <memory>

#include <pd/base/exception.H>
#include <pd/bq/bq_util.H>

#include <phantom/io_blob_receiver/handler.H>
#include <phantom/module.H>

namespace phantom {

MODULE(io_blob_receiver);

// io_blob_receiver_t::config_t::config_t()
//     : port(0),
//       multicast(false),
//       reassembly_timeout(interval_zero),
//       pending_blob_limit(0)
// {}

// void io_blob_receiver_t::config_t::check(const in_t::ptr_t& p) const {
//     io_t::config_t::check(p);

//     if(!address) {
//         config::error(p, "io_blob_receiver_t.address must be set");
//     }
//     if(!port) {
//         config::error(p, "io_blob_receiver_t.port must be set");
//     }
//     if(reassembly_timeout == interval_zero) {
//         config::error(p, "io_blob_receiver_t.reassembly_timeout must be set");
//     }
//     if(!handler) {
//         config::error(p, "io_blob_receiver_t.handler must be set");
//     }
// }

// io_blob_receiver_t::io_blob_receiver_t(const string_t& name, const config_t& config)
//     : io_t(name, config),
//       address_(netaddr_ipv4_t(config.address, config.port)),
//       multicast_(config.multicast),
//       reassembly_timeout_(config.reassembly_timeout),
//       handler_(*config.handler),
//       fragment_pool_(config.pending_blob_limit),
//       fd_(-1)
// {}

// io_blob_receiver_t::~io_blob_receiver_t() throw() {
//     assert(fd_ == -1);
// }

// void io_blob_receiver_t::init() {
//     fd_ = ::socket(AF_INET, SOCK_DGRAM, 0);
//     if(fd_ < 0) {
//         throw exception_sys_t(log::error | log::trace, errno, "socket: %m");
//     }

//     if(bq_fd_setup(fd_) < 0) {
//         throw exception_sys_t(log::error | log::trace, errno, "bq_fd_setup: %m");
//     }

//     if(!multicast_) {
//         if(::bind(fd_, address_.sa, address_.sa_len) < 0) {
//             throw exception_sys_t(log::error | log::trace, errno, "bind: %m");
//         }
//     } else {
//         netaddr_ipv4_t bind_address(INADDR_ANY, address_.port());
//         if(::bind(fd_, bind_address.sa, bind_address.sa_len) < 0) {
//             throw exception_sys_t(log::error | log::trace, errno, "bind: %m");
//         }
//         struct ip_mreq mreq;
//         // TODO(skywalker): this binds to all interfaces.
//         mreq.imr_interface.s_addr = htonl(INADDR_ANY);
//         mreq.imr_multiaddr.s_addr = htonl(address_.address().value);
//         if(::setsockopt(fd_, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0) {
//             throw exception_sys_t(log::error | log::trace, errno, "setsockopt, IP_ADD_MEMBERSHIP: %m");
//         }
//     }
// }

// void io_blob_receiver_t::fini() {
//     if(fd_ >= 0) {
//         if(::close(fd_) < 0) {
//             log_error("close: %m");
//         }
//         fd_ = -1;
//     }
// }

// void io_blob_receiver_t::run() {
//     while(true) {
//         char buffer[8950];

//         std::unique_ptr<netaddr_ipv4_t> remote_addr(new netaddr_ipv4_t);

//         ssize_t res = bq_recvfrom(fd_, buffer, sizeof(buffer), remote_addr->sa, &(remote_addr->sa_len), NULL);
//         if(res < 0) {
//             throw exception_sys_t(log::error | log::trace, errno, "bq_recvfrom: %m");
//         }

//         if(res < sizeof(header_t)) {
//             continue;
//         }

//         header_t* header = (header_t*) buffer;
//         char* data = buffer + sizeof(header_t);
//         size_t data_length = res - sizeof(header_t);
//         if(header->begin > header->size ||
//            header->begin + data_length > header->size)
//         {
//             continue;
//         }

//         ref_t<blob_fragment_pool_t::data_t> data = fragment_pool_.lookup(header->guid);
//         if(!data) {
//             data = fragment_pool_.insert(guid);
//         }

//     }
// }

}  // namespace phantom
