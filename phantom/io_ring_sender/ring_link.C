// Copyright (C) 2012, Korotkiy Fedor <prime@yandex-team.ru>
// Copyright (C) 2012, YANDEX LLC.
// This code may be distributed under the terms of the GNU GPL v3.
// See ‘http://www.gnu.org/licenses/gpl.html’.
// vim: set tabstop=4 expandtab:
#include <phantom/io_ring_sender/ring_link.H>

#include <pd/base/fd_guard.H>
#include <pd/base/log.H>

#include <pd/bq/bq_conn_fd.H>
#include <pd/bq/bq_out.H>
#include <pd/bq/bq_util.H>

namespace phantom {

void ring_link_t::loop(ref_t<ring_link_t> /* me */) {
    while(!is_stopped()) {
        int fd = socket(next_in_the_ring_.sa->sa_family, SOCK_STREAM, 0);
        if(fd < 0) {
            throw exception_sys_t(log::error, errno, "socket: %m");
        }

        fd_guard_t fd_guard(fd);
        bq_fd_setup(fd);

        interval_t connect_timeout = net_timeout_;
        if(bq_connect(fd,
                      next_in_the_ring_.sa,
                      next_in_the_ring_.sa_len,
                      &connect_timeout) < 0) {
            if(errno == ECANCELED) {
                throw exception_sys_t(log::error, errno, "bq_connect: %m");
            }

            log_warning("bq_connect: %m");
            continue;
        }

        bq_conn_fd_t conn(fd, NULL, log::warning);

        try {
            send_loop(&conn);
        } catch (const exception_sys_t& exception) {
            exception.log();
            if (exception.errno_val == ECANCELED) {
                throw;
            }
        }
    }
}

void ring_link_t::send_loop(bq_conn_t* conn) {
    char obuf[obuf_size_];
    bq_out_t out(*conn, obuf, sizeof(obuf), net_timeout_);

    while(!is_stopped()) {
        ref_t<pi_ext_t> blob;
        if (queue_->pop(&blob)) {
            pi_t::print_app(out, &blob->root());
            out.flush_all();
            out.timeout_reset();
        }
    }
}

void ring_link_t::shutdown() {
    thr::spinlock_guard_t guard(shutdown_lock_);
    shutdown_ = true;
}

bool ring_link_t::is_stopped() {
    thr::spinlock_guard_t guard(shutdown_lock_);
    return shutdown_;
}

}  // namespace phantom
