// Copyright (C) 2012, Korotkiy Fedor <prime@yandex-team.ru>
// Copyright (C) 2012, YANDEX LLC.
// This code may be distributed under the terms of the GNU GPL v3.
// See ‘http://www.gnu.org/licenses/gpl.html’.
// vim: set tabstop=4 expandtab:
#pragma once

#include <pd/base/ref.H>
#include <pd/base/queue.H>
#include <pd/base/exception.H>
#include <pd/base/thr.H>
#include <pd/bq/bq_cond.H>

namespace pd {

/**
 * Fix-sized queue that can be safely accessed from different phantom
 * coroutines simultaneously.
 *
 * Can be in one of two states - active and inactive.
 *
 * In active state push() and pop() block coroutine until operation
 * can be performed of timeout expires.
 *
 * In inactive state push() and pop() return right away as if timeout
 * expires(event if no timeout was supplied). Calls to push() and
 * pop() that blocked while queue was in active state also return.
 * This state is usefull for stopping threads that access
 * queue(coroutine can just pass NULL as timeout and check return
 * value of pop() or push() instead of polling some *stop* flag).
 */
template<typename x_t>
class blocking_queue_t {
public:
    //! max_size - maximum number of elements queue can hold
    blocking_queue_t(size_t max_size)
            : max_size_(max_size),
              queue_(max_size),
              is_active_(true) {}

    //! Pushes value into the queue. Blocks if queue is full until
    //! other coroutine pop() element from the queue or timeout
    //! expires or queue goes into inactive state.
    //!
    //! @return true on success, false otherwise
    //!
    //! @throws exception_sys_t on phantom shutdown or other error.
    bool push(const x_t& value, interval_t* timeout = NULL) {
        thr::spinlock_guard_t guard(queue_lock_);

        bool wait_succeded = true;
        int wait_errno = 0;
        while(queue_.get_count() == max_size_ &&
              is_active_ &&
              wait_succeded) {
            bq_cond_guard_t not_full_guard(not_full_cond_);
            guard.relax();

            wait_succeded = bq_success(not_full_cond_.wait(timeout));
            wait_errno = errno;

            not_full_guard.relax();
            guard.wakeup();
        }

        if(is_active_ && wait_succeded) {
            queue_.insert(value);
        }

        send_to_next();

        if(!is_active_ || (!wait_succeded && wait_errno == ETIMEDOUT)) {
            return false;
        } else if(!wait_succeded) {
            throw exception_sys_t(log::error,
                                  errno,
                                  "blocking_queue_t::push: %m");
        } else {
            return true;
        }
    }

    //! Pop value from the queue. Blocks if queue is empty until other
    //! coroutine push() element into the queue or timeout expires or
    //! queue goes into inactive state.
    //!
    //! @return true on success, false otherwise
    //!
    //! @throws exception_sys_t on phantom shutdown or other error.
    bool pop(x_t* value, interval_t* timeout = NULL) {
        thr::spinlock_guard_t guard(queue_lock_);

        bool wait_succeded = true;
        int wait_errno = 0;
        while(queue_.get_count() == 0 &&
              is_active_ &&
              wait_succeded) {
            bq_cond_guard_t not_empty_guard(not_empty_cond_);
            guard.relax();

            wait_succeded = bq_success(not_empty_cond_.wait(timeout));
            wait_errno = errno;

            not_empty_guard.relax();
            guard.wakeup();
        }

        if(is_active_ && wait_succeded) {
            *value = queue_.remove();
        }

        send_to_next();

        if(!is_active_ || (!wait_succeded && wait_errno == ETIMEDOUT)) {
            return false;
        } else if(!wait_succeded) {
            throw exception_sys_t(log::error,
                                  errno,
                                  "blocking_queue_t::push: %m");
        } else {
            return true;
        }
    }

    bool empty() {
        thr::spinlock_guard_t guard(queue_lock_);
        return queue_.get_count() == 0;
    }

    //! Switch state to active
    void activate() {
        thr::spinlock_guard_t guard(queue_lock_);
        is_active_ = true;
    }

    //! Switch state to inactive. Unblocks blocked pop() and push() as
    //! if corresponding call timeout expires.
    //!
    //! Note that if queue deactivates and activates again some pop()
    //! and push() calls might stay blocked.
    void deactivate() {
        thr::spinlock_guard_t guard(queue_lock_);

        if(is_active_) {
            is_active_ = false;

            send_to_next();
        }
    }

    //! Removes all elements from queue. After this call queue still
    //! stores max_size of default constructed x_t.
    void clear() {
        thr::spinlock_guard_t guard(queue_lock_);
        while(queue_.get_count() != 0) {
            queue_.remove();
        }

        bq_cond_guard_t not_full_guard(not_full_cond_);
        not_full_cond_.send();
    }

private:
    size_t max_size_;
    queue_t<x_t> queue_;
    thr::spinlock_t queue_lock_;
    bq_cond_t not_empty_cond_, not_full_cond_;
    bool is_active_;

    void send_to_next() {
        if(queue_.get_count() > 0 || !is_active_) {
            bq_cond_guard_t not_empty_guard(not_empty_cond_);
            not_empty_cond_.send();
        }

        if(queue_.get_count() < max_size_ || !is_active_) {
            bq_cond_guard_t not_full_guard(not_full_cond_);
            not_full_cond_.send();
        }
    }
};

}  // namespace phantom
