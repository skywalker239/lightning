// Copyright (C) 2012, Korotkiy Fedor <prime@yandex-team.ru>
// Copyright (C) 2012, YANDEX LLC.
// This code may be distributed under the terms of the GNU GPL v3.
// See ‘http://www.gnu.org/licenses/gpl.html’.
// vim: set tabstop=4 expandtab:
#include <sys/types.h>
#include <signal.h>
#include <unistd.h>

#include <pd/base/log.H>
#include <pd/base/config.H>
#include <pd/bq/bq_job.H>
#include <pd/lightning/pi_ext.H>
#include <pd/lightning/finished_counter.H>

#include <phantom/io.H>
#include <phantom/module.H>
#include <phantom/io_blob_sender/io_blob_sender.H>
#include <phantom/io_blob_receiver/handler.H>
#include <phantom/io_blob_receiver/blob_fragment_pool.H>

namespace phantom {

MODULE(test_blob_multicast);

class io_blob_multicast_test_t : public io_t,
                                 public io_blob_receiver::handler_t {
public:
    struct config_t : public io_t::config_t {
        void check(const in_t::ptr_t& ) const {}

        config::objptr_t<io_blob_sender_t> sender;
    };

    io_blob_multicast_test_t(const string_t& name,
                             const config_t& c)
        : io_t(name, c) {}

    virtual void run() {
        benchmark_blob_fragment_pool();

        log_info("All tests finished");
        log_info("Sending SIGQUIT");
        kill(getpid(), SIGQUIT);
    }

    virtual void init() {}
    virtual void fini() {}
    virtual void stat(pd::out_t& , bool) {}

    virtual void handle(ref_t<pi_ext_t> /*blob*/,
                        const netaddr_t& /*remote_addr*/) {

    }

    virtual ~io_blob_multicast_test_t() {}

private:

    void benchmark_blob_fragment_pool() {
        blob_fragment_pool_t pool(128 * 32, 128);

        finished_counter_t counter;

        const int N_WORKERS = 16, BLOBS_PER_WORKER = 128 * 1024 / N_WORKERS;

        counter.started(N_WORKERS);

        for(int i = 0; i < N_WORKERS; ++i) {
            bq_job_t<typeof(&io_blob_multicast_test_t::mess_with_pool)>::create(
                STRING("worker"),
                scheduler.bq_thr(),
                *this,
                &io_blob_multicast_test_t::mess_with_pool,
                N_WORKERS * BLOBS_PER_WORKER,
                BLOBS_PER_WORKER,
                &pool,
                &counter
            );
        }

        counter.wait_for_all_to_finish();
    }

    void mess_with_pool(int start, int n_blobs,
                        blob_fragment_pool_t* pool,
                        finished_counter_t* counter) {
        const size_t BLOB_SIZE = 1024 * 8;

        for(int i = 0; i < n_blobs; ++i) {
            pool->lookup(start + i, BLOB_SIZE);

            pool->remove(start - i);
        }

        counter->finish();
    }
};

namespace io_blob_multicast_test {
config_binding_sname(io_blob_multicast_test_t);

config_binding_parent(io_blob_multicast_test_t, io_t, 1);
config_binding_ctor(io_t, io_blob_multicast_test_t);
} // namespace io_ring_sender

} // namespace phantom
