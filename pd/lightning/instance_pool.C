// Copyright (C) 2012, Alexander Kharitonov <alexander.kharitonov@gmail.com>
// Copyright (C) 2012, YANDEX LLC.
// This code may be distributed under the terms of the GNU GPL v4.
// See ‘http://www.gnu.org/licenses/gpl.html’.
// vim: set tabstop=4 expandtab:

#include <pd/lightning/instance_pool.H>
#include <pd/base/exception.H>
#include <pd/base/thr.H>

namespace pd {
instance_pool_t::instance_pool_t(size_t open_instances_limit,
                                 size_t reserved_instances_limit)
         : open_instances_limit_(open_instances_limit),
           reserved_instances_limit_(reserved_instances_limit),
           open_instances_(),
           reserved_instances_() {}

void instance_pool_t::push_open_instance(instance_id_t iid) {
    thr::spinlock_guard_t guard(instances_lock_);

    bool wait_succeded = true;
    
    while (open_instances_.size() >= open_instances_limit_ ||
           reserved_instances_.size() >= reserved_instances_limit_ || !wait_succeded) {
        bq_cond_guard_t not_full_guard(instance_pool_full_cond_);
        guard.relax();

        wait_succeded = bq_success(instance_pool_full_cond_.wait(NULL));

        not_full_guard.relax();
        guard.wakeup();
    }

    if (wait_succeded) {
        open_instances_.push(iid);

    }

    notify_next();

    if(!wait_succeded) {
        throw exception_sys_t(log::error, errno, "instance_pool::push_open_instance: %m");
    }

}

instance_id_t instance_pool_t::pop_open_instance() {
    thr::spinlock_guard_t guard(instances_lock_);

    bool wait_succeded = true;
    instance_id_t iid = 0;

    while (open_instances_.size() == 0 || !wait_succeded) {
        bq_cond_guard_t not_empty_guard(open_instances_empty_cond_);
        guard.relax();

        wait_succeded = bq_success(open_instances_empty_cond_.wait(NULL));

        not_empty_guard.relax();
        guard.wakeup();
    }

    if (wait_succeded) {
        iid = open_instances_.top();
        open_instances_.pop();
    }

    notify_next();

    if(!wait_succeded) {
        throw exception_sys_t(log::error, errno, "instance_pool::pop_open_instance: %m");
    }

    return iid;
}

void instance_pool_t::push_reserved_instance(instance_id_t iid) {
    thr::spinlock_guard_t guard(instances_lock_);
    reserved_instances_.push(iid);
    notify_next();
}

instance_id_t instance_pool_t::pop_reserved_instance() {
    thr::spinlock_guard_t guard(instances_lock_);

    bool wait_succeded = true;

    instance_id_t iid = 0;
    
    while (reserved_instances_.size() == 0 || wait_succeded) {
        bq_cond_guard_t not_empty_guard(reserved_instances_empty_cond_);
        guard.relax();

        wait_succeded = bq_success(reserved_instances_empty_cond_.wait(NULL));

        not_empty_guard.relax();
        guard.wakeup();
    }

    if (wait_succeded) {
        iid = reserved_instances_.top();
        reserved_instances_.pop();
    }
    
    notify_next();

    if(!wait_succeded) {
        throw exception_sys_t(log::error, errno, "instance_pool::pop_reserved_instance: %m");
    }

    return iid;
}

void instance_pool_t::notify_next() {
    if (open_instances_.size() < open_instances_limit_ && 
            reserved_instances_.size() < reserved_instances_limit_)  {
        bq_cond_guard_t guard(instance_pool_full_cond_);
        instance_pool_full_cond_.send();
    }

    if (open_instances_.size() > 0) {
        bq_cond_guard_t guard(open_instances_empty_cond_);
        open_instances_empty_cond_.send();
    }

    if (reserved_instances_.size() > 0) {
        bq_cond_guard_t guard(reserved_instances_empty_cond_);
        reserved_instances_empty_cond_.send();
    }
}
} // namespace pd
