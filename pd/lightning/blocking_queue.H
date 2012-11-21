#pragma once

#include <pd/base/ref.H>
#include <pd/base/queue.H>
#include <pd/base/exception.H>
#include <pd/bq/bq_cond.H>

namespace pd {

template<typename x_t>
class blocking_queue_t {
public:
    blocking_queue_t(size_t max_size)
            : max_size_(max_size),
              queue_(max_size),
              is_active_(true) {}

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

    bool empty() {
        bq_cond_guard_t guard(queue_cond_);

        return queue_.get_count() == 0;
    }

    void activate() {
        bq_cond_guard_t guard(queue_cond_);
        is_active_ = true;
    }

    void deactivate() {
        bq_cond_guard_t guard(queue_cond_);
        if (is_active_) {
            is_active_ = false;

            queue_cond_.send(true);
        }
    }

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
