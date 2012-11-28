#pragma once

#include <pd/base/ref.H>
#include <pd/base/queue.H>
#include <pd/base/exception.H>
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
        bq_cond_guard_t guard(queue_cond_);

        if (!is_active_) {
            return false;
        }

        while(queue_.get_count() >= max_size_) {
            if(!bq_success(queue_cond_.wait(timeout))) {
                if(errno == ETIMEDOUT || !is_active_) {
                    return false;
                } else {
                    throw exception_sys_t(log::error, errno, "blocking_queue_t.push: %m");
                }
            }
        }

        queue_.insert(value);
        queue_cond_.send(true);
        return true;
    }

    //! Pop value from the queue. Blocks if queue is empty until other
    //! coroutine push() element into the queue or timeout expires or
    //! queue goes into inactive state.
    //!
    //! @return true on success, false otherwise
    //!
    //! @throws exception_sys_t on phantom shutdown or other error.
    bool pop(x_t* value, interval_t* timeout = NULL) {
        bq_cond_guard_t guard(queue_cond_);

        if (!is_active_) {
            return false;
        }

        while(queue_.get_count() == 0) {
            if(!bq_success(queue_cond_.wait(timeout))) {
                if(errno == ETIMEDOUT || !is_active_) {
                    return false;
                } else {
                    throw exception_sys_t(log::error, errno, "blocking_queue_t.pop: %m");
                }
            }
        }

        *value = queue_.remove();
        queue_cond_.send(true);
        return true;
    }

    //! @return true iff queue is empty
    bool empty() {
        bq_cond_guard_t guard(queue_cond_);

        return queue_.get_count() == 0;
    }

    //! Switch state to active
    void activate() {
        bq_cond_guard_t guard(queue_cond_);
        is_active_ = true;
    }

    //! Switch state to inactive. Unblocks blocked pop() and push() as
    //! if corresponding call timeout expires.
    //!
    //! Note that if queue deactivates and activates again some pop()
    //! and push() calls might stay blocked.
    void deactivate() {
        bq_cond_guard_t guard(queue_cond_);
        if (is_active_) {
            is_active_ = false;

            queue_cond_.send(true);
        }
    }

    //! Removes all elements from queue. After this call queue still
    //! stores max_size of default constructed x_t.
    void clear() {
        bq_cond_guard_t guard(queue_cond_);
        while (queue_.get_count() != 0) {
            queue_.remove();
        }
        queue_cond_.send(true);
    }

private:
    size_t max_size_;
    queue_t<x_t> queue_;
    bq_cond_t queue_cond_;
    bool is_active_;
};

}  // namespace phantom
