// Copyright (C) 2012, Alexander Kharitonov <alexander.kharitonov@gmail.com>
// Copyright (C) 2012, YANDEX LLC.
// This code may be distributed under the terms of the GNU GPL v3.
// See ‘http://www.gnu.org/licenses/gpl.html’.
// vim: set tabstop=4 expandtab:
#include <phantom/io_blob_sender/out_udp.H>

#include <pd/base/exception.H>
#include <pd/bq/bq.H>

#include <unistd.h>

namespace phantom {

out_udp_t::out_udp_t(char* buffer,
                           size_t buffer_size,
                           uint32_t blob_size,
                           int fd,
                           const netaddr_t& address,
                           uint64_t guid,
                           uint64_t* bytes_sent,
                           uint64_t* packets_sent,
                           uint64_t* dups)
    : out_t(buffer, buffer_size),
      blob_size_(blob_size),
      fd_(fd),
      address_(address),
      guid_(guid),
      bytes_sent_(bytes_sent),
      packets_sent_(packets_sent),
      dups_(dups),
      sent_bytes_(0),
      sent_parts_(0)
{}

out_udp_t::~out_udp_t() throw() {
    safe_run(*this, &out_udp_t::flush_all);
}

void out_udp_t::flush() {
    iovec outvec[3];
    int outcount = 1;

    uint32_t flushed_bytes = 0;

    if(rpos < size) {
        outvec[outcount].iov_base = data + rpos;
        outvec[outcount].iov_len = ((wpos > rpos) ? wpos : size) - rpos;
        flushed_bytes += outvec[outcount].iov_len;
        ++outcount;
    }

    if(wpos > 0 && wpos <= rpos) {
        outvec[outcount].iov_base = data;
        outvec[outcount].iov_len = wpos;
        flushed_bytes += outvec[outcount].iov_len;
        ++outcount;
    }

    header_t header = { guid_, blob_size_, sent_bytes_, sent_parts_ };

    outvec[0].iov_base = &header;
    outvec[0].iov_len = sizeof(header);

    if(outcount > 1) {
        struct msghdr msg_hdr = {
            (void*) address_.sa,
            address_.sa_len,
            outvec,
            outcount,
            NULL,
            0,
            0
        };

        ssize_t res = do_sendmsg(&msg_hdr);
        if(res <= 0) {
            if(res == 0) errno = EREMOTEIO;
            throw exception_sys_t(log::error, errno, "sendmsg: %m");
        }

        if(res != flushed_bytes + (uint32_t)sizeof(header_t)) {
            throw exception_log_t(log::error, "sendmsg sent %ld bytes of %d", res, flushed_bytes);
        }

        rpos += flushed_bytes;
        if(rpos > size) {
            rpos -= size;
        }

        sent_bytes_ += flushed_bytes;
        sent_parts_ += 1;
        if(sent_bytes_ > blob_size_) {
            throw exception_log_t(log::error, "out_udp_t overflow(sent=%d, blob=%d)", sent_bytes_, blob_size_);
        }
        __sync_fetch_and_add(bytes_sent_, res);
        __sync_fetch_and_add(packets_sent_, 1);
    }

    if(rpos == wpos) {
        wpos = 0;
        rpos = size;
    }
}

ssize_t out_udp_t::do_sendmsg(const struct msghdr* msg) {
    ssize_t res = sendmsg(fd_, msg, 0);
    if(res < 0) {
        if(errno == EAGAIN) {
            __sync_fetch_and_add(dups_, 1);
            int fd2 = ::dup(fd_);
            if(fd2 < 0) {
                throw exception_sys_t(log::error, errno, "dup: %m");
            }
            res = bq_sendmsg(fd2, msg, 0, NULL);
            if(::close(fd2) < 0) {
                throw exception_sys_t(log::error, errno, "close: %m");
            }
            return res;
        } else {
            throw exception_sys_t(log::error, errno, "sendmsg: %m");
        }
    }
    return res;
}

ssize_t out_udp_t::bq_sendmsg(int fd, const struct msghdr* msg, int flags, interval_t* timeout) {
    while(true) {
        ssize_t res = sendmsg(fd, msg, flags);
        if(res < 0 && errno == EAGAIN) {
            short int events = POLLOUT;
            if(bq_success(bq_do_poll(fd, events, timeout, "sendmsg"))) {
                continue;
            }
        }

        return res;
    }
}

}  // namespace phantom
