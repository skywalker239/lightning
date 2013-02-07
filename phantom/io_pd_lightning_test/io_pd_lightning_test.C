// Copyright (C) 2012, Korotkiy Fedor <prime@yandex-team.ru>
// Copyright (C) 2012, YANDEX LLC.
// This code may be distributed under the terms of the GNU GPL v3.
// See ‘http://www.gnu.org/licenses/gpl.html’.
// vim: set tabstop=4 expandtab:
#include <sys/types.h>
#include <signal.h>
#include <unistd.h>

#include <vector>

#include <phantom/pd.H>
#include <phantom/io.H>
#include <phantom/module.H>

#include <pd/base/ref.H>
#include <pd/base/time.H>
#include <pd/base/log.H>
#include <pd/bq/bq_job.H>
#include <pd/bq/bq_cond.H>
#include <pd/bq/bq_util.H>
#include <pd/lightning/blocking_queue.H>
#include <pd/lightning/pi_ring_cmd.H>

namespace phantom {

MODULE(io_pd_lightning_test);

#ifdef ASSERT
#error ***ASSERT macros already defined***
#endif

#define ASSERT(value) \
do { \
  if (!(value)) { \
    log_error("FAIL %s in %s:%d", __func__, __FILE__, __LINE__); \
  } \
} while(0) \


class io_pd_lightning_test_t : public io_t {
public:
    struct config_t : public io_t::config_t {
        inline void check(const in_t::ptr_t& ptr) const {
            io_t::config_t::check(ptr);
        }
    };

    io_pd_lightning_test_t(const string_t& name, const config_t& config)
        : io_t(name, config) {}

    virtual void init() {}

    virtual void fini() {}

    virtual void run() {
        log_info("Running tests");

        test_blocking_queue();

        test_ring_cmd();

        log_info("All tests finished");
        log_info("Sending SIGQUIT");
        kill(getpid(), SIGQUIT);
    }

    virtual void stat(out_t& /*out*/, bool /*clear*/) {

    }
private:
    // ===== blocking_queue_t =====
    void test_blocking_queue() {
        log_info("Testing blocking_queue_t");

        test_blocking_queue_simple();
        test_blocking_queue_timeout();
        test_blocking_queue_concurrent();
        test_blocking_queue_deactivation();
        test_deactivation_unblocks();

        log_info("Finished testing blocking_queue_t");
    }

    void test_blocking_queue_simple() {
        const int QUEUE_SIZE = 10, N_ITERATIONS = 10;
        blocking_queue_t<int> queue(QUEUE_SIZE);

        bool fail = false;
        for (int j = 0; j < N_ITERATIONS; ++j) {
            for (int i = 0; i < QUEUE_SIZE; ++i) {
                if (!queue.push(i)) {
                    fail = true;
                }
            }

            for (int i = 0; i < QUEUE_SIZE; ++i) {
                int value = -1;
                if (!queue.pop(&value) || value != i) {
                    fail = true;
                }
            }
        }

        ASSERT(!fail);
    }

    void test_blocking_queue_timeout() {
        blocking_queue_t<int> queue(1);
        interval_t timeout = interval_millisecond;

        int value;
        ASSERT(!queue.pop(&value, &timeout));

        timeout = interval_millisecond;
        ASSERT(queue.push(0, &timeout));

        timeout = interval_millisecond;
        ASSERT(!queue.push(1, &timeout));

        timeout = interval_millisecond;
        ASSERT(queue.pop(&value, &timeout));
    }

    void test_blocking_queue_concurrent() {
        static const int QUEUE_SIZE = 8,
                         N_READERS = 50, N_WRITERS = 50,
                         // per one reader / writer
                         N_WRITES = 10000, N_READS = 10000;
        ASSERT(N_READERS * N_READS == N_WRITERS * N_WRITES);

        blocking_queue_t<int> queue(QUEUE_SIZE);

        int readers_stopped = 0;
        bq_cond_t readers_stop_cond;

        std::vector<int> poped_elements(N_WRITERS * N_WRITES, 0);

        for (int pusher = 0; pusher < N_WRITERS; ++pusher) {
            bq_job_t<typeof(&io_pd_lightning_test_t::test_blocking_queue_pusher)>::create(
                STRING("test_blocking_queue_pusher"),
                bq_thr_get(),
                *this,
                &io_pd_lightning_test_t::test_blocking_queue_pusher,
                &queue,
                N_WRITES * pusher,
                N_WRITES * (pusher + 1));
        }

        for (int reader = 0; reader < N_READERS; ++reader) {
            bq_job_t<typeof(&io_pd_lightning_test_t::test_blocking_queue_reader)>::create(
                STRING("test_blocking_queue_reader"),
                bq_thr_get(),
                *this,
                &io_pd_lightning_test_t::test_blocking_queue_reader,
                &queue,
                N_READS,
                &poped_elements,
                &readers_stopped,
                &readers_stop_cond);
        }

        bq_cond_guard_t readers_guard(readers_stop_cond);
        while(readers_stopped != N_READERS) {
            readers_stop_cond.wait(NULL);
        }

        bool fail = false;
        for (int element : poped_elements) {
            if (element != 1) {
                fail = true;
            }
        }
        ASSERT(!fail);
    }

    void test_blocking_queue_pusher(blocking_queue_t<int>* queue,
                                    int start,
                                    int end) {
        for (int i = start; i < end; ++i) {
            queue->push(i);
        }
    }

    void test_blocking_queue_reader(blocking_queue_t<int>* queue,
                                    int number_of_elements_to_read,
                                    std::vector<int>* poped_elements,
                                    int* readers_stopped,
                                    bq_cond_t* stop_cond) {
        for (int elements_read = 0;
             elements_read < number_of_elements_to_read;
             ++elements_read) {

            int value = 0;
            queue->pop(&value);
            (*poped_elements)[value] += 1;
        }

        bq_cond_guard_t stop_guard(*stop_cond);
        ++(*readers_stopped);
        stop_cond->send();
    }

    void test_blocking_queue_deactivation() {
        blocking_queue_t<int> queue(1);

        ASSERT(queue.push(0));

        queue.deactivate();

        queue.activate();

        int value;
        ASSERT(queue.pop(&value));

        queue.deactivate();

        ASSERT(!queue.pop(&value));
        ASSERT(!queue.push(1));
    }

    void test_deactivation_unblocks() {
        blocking_queue_t<int>* queue = new blocking_queue_t<int>(1);

        bq_job_t<typeof(&io_pd_lightning_test_t::deactivation_unblocks)>::create(
                STRING("deactivation_unblocks"),
                bq_thr_get(),
                *this,
                &io_pd_lightning_test_t::deactivation_unblocks,
                queue);

        interval_t timeout = 10 * interval_millisecond;
        bq_sleep(&timeout);
        queue->deactivate();

    }

    void deactivation_unblocks(blocking_queue_t<int>* queue) {
        int value;
        ASSERT(!queue->pop(&value));
        delete queue;
    }

    // ====== pd/lightning/pi_ring_cmd.H ======
    void test_ring_cmd() {
        log_info("Testing pi_ring_cmd.H");

        test_batch_ring_cmd();
        test_batch_ring_cmd_empty_failed_instances();

        log_info("Finished testing pi_ring_cmd.H");
    }

    void test_batch_ring_cmd_empty_failed_instances() {
        ref_t<pi_ext_t> cmd = build_ring_batch_cmd(
            {
                request_id: 52,
                ring_id: 21,
                dst_host_id: 12
            },
            {
                start_iid: 1024,
                end_iid: 2048,
                ballot_id: 7,
                fails: std::vector<batch_fail_t>()
            }
        );

        ASSERT(ring_cmd_type(cmd) == PHASE1_BATCH);

        ASSERT(request_id(cmd) == 52);
        ASSERT(ring_id(cmd) == 21);
        ASSERT(dst_host_id(cmd) == 12);

        ASSERT(batch_start_iid(cmd) == 1024);
        ASSERT(batch_end_iid(cmd) == 2048);
        ASSERT(batch_ballot_id(cmd) == 7);

        ASSERT(batch_fails(cmd)._count() == 0);
    }

    void test_batch_ring_cmd() {
        std::vector<batch_fail_t> failed{
            { 1050, 9, LOW_BALLOT_ID },
            { 1051, 10, RESERVED },
            { 1052, 11, IID_TOO_LOW }
        };

        ref_t<pi_ext_t> cmd = build_ring_batch_cmd(
            {
                request_id: 52,
                ring_id: 21,
                dst_host_id: 12
            },
            {
                start_iid: 1024,
                end_iid: 2048,
                ballot_id: 7,
                fails: failed
            }
        );

        ASSERT(ring_cmd_type(cmd) == PHASE1_BATCH);

        ASSERT(request_id(cmd) == 52);
        ASSERT(ring_id(cmd) == 21);
        ASSERT(dst_host_id(cmd) == 12);

        ASSERT(batch_start_iid(cmd) == 1024);
        ASSERT(batch_end_iid(cmd) == 2048);
        ASSERT(batch_ballot_id(cmd) == 7);

        std::vector<batch_fail_t> fi = fails_pi_to_vector(batch_fails(cmd));

        ASSERT(fi[0].iid == 1050);
        ASSERT(fi[0].highest_promised == 9);
        ASSERT(fi[0].status == LOW_BALLOT_ID);

        ASSERT(fi[1].iid == 1051);
        ASSERT(fi[1].highest_promised == 10);
        ASSERT(fi[1].status == RESERVED);

        ASSERT(fi[2].iid == 1052);
        ASSERT(fi[2].highest_promised == 11);
        ASSERT(fi[2].status == IID_TOO_LOW);

        ASSERT(is_ring_cmd_valid(cmd));
    }
};

#undef ASSERT

namespace io_pd_lightning_test {
config_binding_sname(io_pd_lightning_test_t);
config_binding_parent(io_pd_lightning_test_t, io_t, 1);
config_binding_ctor(io_t, io_pd_lightning_test_t);
}


} // namespace phantom
