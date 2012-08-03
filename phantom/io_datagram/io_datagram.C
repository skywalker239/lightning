#include "io_datagram.H"
#include "handler.H"

#include <phantom/module.H>

#include <pd/base/exception.H>
#include <pd/base/fd_guard.H>
#include <pd/base/log.H>
#include <pd/bq/bq_job.H>
#include <pd/bq/bq_util.H>

namespace phantom {

MODULE(io_datagram);

io_datagram_t::config_t::config_t() throw()
    : io_t::config_t(),
      handler(),
      reuse_addr(true),
      poll_interval(interval_second)
{}

void io_datagram_t::config_t::check(const in_t::ptr_t& ptr) const {
    io_t::config_t::check(ptr);

    if(!handler) {
        config::error(ptr, "handler is required");
    }

    if(poll_interval > interval_minute) {
        config::error(ptr, "poll interval is too big");
    }
}

namespace io_datagram {
config_binding_sname(io_datagram_t);
config_binding_type(io_datagram_t, handler_t);
config_binding_value(io_datagram_t, handler);
config_binding_value(io_datagram_t, reuse_addr);
config_binding_value(io_datagram_t, poll_interval);
config_binding_parent(io_datagram_t, io_t, 1);
}  // namespace io_datagram

io_datagram_t::io_datagram_t(const string_t& name, const config_t& config)
    : io_t(name, config),
      fd_(-1),
      reuse_addr_(config.reuse_addr),
      handler_(*config.handler)
{}

io_datagram_t::~io_datagram_t() throw()
{
    if(fd_ >= 0) {
        if(::close(fd_) < 0) {
            log_error("~io_datagram_t, close: %m");
        }
        fd_ = -1;
    }
}

void io_datagram_t::init() {
    const netaddr_t& netaddr = bind_addr();

    fd_ = socket(netaddr.sa->sa_family, SOCK_DGRAM, 0);
    if(fd_ < 0) {
        throw exception_sys_t(log::error, errno, "socket: %m");
    }

    try {
        if(reuse_addr_) {
            int i = 1;
            if(setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &i, sizeof(i)) < 0) {
                throw exception_sys_t(log::error, errno, "setsockopt, SO_REUSEADDR, 1: %m");
            }
        }

            if(::bind(fd_, netaddr.sa, netaddr.sa_len) < 0) {
                throw exception_sys_t(log::error, errno, "bind: %m");
            }
    } catch(...) {
        ::close(fd_);
        fd_ = -1;
        throw;
    }
}

void io_datagram_t::run() {
    // No multithreaded recv for now.
    int fd = ::dup(fd_);
    fd_guard_t fd_guard(fd);

    bq_job_t<typeof(&io_datagram_t::loop)>::create(
        name,
        scheduler.bq_thr(),
        *this,
        &io_datagram_t::loop,
        fd);
    fd_guard.relax();
}

void io_datagram_t::loop(int fd) const {
    fd_guard_t fd_guard(fd);

    bq_fd_setup(fd);
    fd_setup(fd);

    const netaddr_t& local_addr = bind_addr();

    while(true) {
        netaddr_t* remote_addr = new_netaddr();

        class netaddr_guard_t {
        public:
            inline netaddr_guard_t(netaddr_t* addr) throw() : addr_(addr) { }
            inline ~netaddr_guard_t() throw() { delete addr_; }
        private:
            netaddr_t* addr_;
        } netaddr_guard(remote_addr);

        char buffer[maximum_datagram_size];
        ssize_t read_bytes = bq_recvfrom(fd,
                                         buffer,
                                         sizeof(buffer),
                                         remote_addr->sa,
                                         &remote_addr->sa_len,
                                         NULL);
        if(read_bytes < 0) {
            throw exception_sys_t(log::error, errno, "recvfrom: %m");
        }

        handler_.handle_datagram(buffer, read_bytes, local_addr, *remote_addr);
    }
}

void io_datagram_t::fini() {
    if(::close(fd_) < 0) {
        throw exception_sys_t(log::error, errno, "close(): %m");
    }
    fd_ = -1;
}

void io_datagram_t::stat(out_t& out, bool clear) {
    handler_.stat(out, clear);
}

}  // namespace phantom
