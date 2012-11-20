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
#include <pd/lightning/blocking_queue.H>

#pragma GCC visibility push(default)

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
                int value;
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
        int QUEUE_SIZE = 10, N_READERS = 10, N_PUSHERS = 3, N_WRITES = 1000;
        blocking_queue_t<int> queue(QUEUE_SIZE);

        int pushers_finished = 0;
        bq_cond_t pusher_finished_cond;

        bool stop_readers = false;
        int readers_stopped = 0;
        bq_cond_t readers_stop_cond;

        std::vector<int> poped_elements(N_PUSHERS * N_WRITES, 0);

        for (int pusher = 0; pusher < N_PUSHERS; ++pusher) {
            bq_job_t<typeof(&io_pd_lightning_test_t::test_blocking_queue_pusher)>::create(
                STRING("test_blocking_queue_pusher"), bq_thr_get(), *this,
                &io_pd_lightning_test_t::test_blocking_queue_pusher,
                &queue, N_WRITES * pusher, N_WRITES * (pusher + 1),
                &pushers_finished, &pusher_finished_cond);
        }

        for (int reader = 0; reader < N_READERS; ++reader) {
            bq_job_t<typeof(&io_pd_lightning_test_t::test_blocking_queue_reader)>::create(
                STRING("test_blocking_queue_reader"), bq_thr_get(), *this,
                &io_pd_lightning_test_t::test_blocking_queue_reader,
                &queue, &stop_readers, &readers_stopped,
                &poped_elements, &readers_stop_cond);
        }

        bq_cond_guard_t guard1(pusher_finished_cond);
        while(pushers_finished != N_PUSHERS) {
            pusher_finished_cond.wait(NULL);
        }

        bq_cond_guard_t guard2(readers_stop_cond);
        stop_readers = true;
        while(readers_stopped != N_READERS) {
            readers_stop_cond.wait(NULL);
        }

        bool fail = false;
        for (int element : poped_elements) {
            if (element == 0) {
                fail = true;
            }
        }
        ASSERT(!fail);
    }

    void test_blocking_queue_pusher(blocking_queue_t<int>* queue,
                                    int start, int end,
                                    int* finished,
                                    bq_cond_t* finished_cond) {
        for (int i = start; i < end; ++i) {
            
            queue->push(i);
        }

        bq_cond_guard_t guard(*finished_cond);
        ++(*finished);
        finished_cond->send();
    }

    void test_blocking_queue_reader(blocking_queue_t<int>* queue,
                                    bool* stop, int* readers_stopped,
                                    std::vector<int>* poped_elements,
                                    bq_cond_t* stop_cond) {
        interval_t timeout;
        while (true) {
            {
                bq_cond_guard_t guard(*stop_cond);
                if (*stop && queue->empty()) {
                    ++(*readers_stopped);
                    stop_cond->send();
                    break;
                }
            }

            timeout = interval_millisecond;
            int value;
            if (queue->pop(&value, &timeout)) {
                (*poped_elements)[value] = 1;
            }
        }
    }
};

#undef ASSERT

namespace io_pd_lightning_test {
config_binding_sname(io_pd_lightning_test_t);
config_binding_parent(io_pd_lightning_test_t, io_t, 1);
config_binding_ctor(io_t, io_pd_lightning_test_t);
}


} // namespace phantom

#pragma GCC visibility pop
