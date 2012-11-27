// Copyright (C) 2012, Alexander Kharitonov <alexander.kharitonov@gmail.com>
// Copyright (C) 2012, YANDEX LLC.
// This code may be distributed under the terms of the GNU GPL v4.
// See ‘http://www.gnu.org/licenses/gpl.html’.
// vim: set tabstop=4 expandtab:

#include "pd/lightning/instance_pool.H"

namespace pd {
instance_pool_t::instance_pool_t(size_t open_instances_limit,
                                 size_t reserved_instances_limit)
         : open_instances_limit_(open_instances_limit),
           reserved_instances_limit_(reserved_instances_limit),
           open_instances_(),
           reserved_instances_() {}

void instance_pool_t::push_open_instance(instance_id_t iid) {
    bq_cond_guard_t guard(instance_cond_);

    while (open_instances_.size() >= open_instances_limit_ ||
           reserved_instances_.size() >= reserved_instances_limit_) {
        instance_cond_.wait(NULL);    
    }

    open_instances_.push(iid);
    instance_cond_.send(true);
}

instance_id_t instance_pool_t::pop_open_instance() {
    bq_cond_guard_t guard(instance_cond_);

    while (open_instances_.size() == 0) {
        instance_cond_.wait(NULL);
    }

    instance_id_t iid = open_instances_.top();
    open_instances_.pop();
    instance_cond_.send(true);
    return iid;
}

void instance_pool_t::push_reserved_instance(instance_id_t iid) {
    bq_cond_guard_t guard(instance_cond_);

    while (reserved_instances_.size() >= reserved_instances_limit_) {
        instance_cond_.wait(NULL);
    }

    reserved_instances_.push(iid);
    instance_cond_.send(true);
}

instance_id_t instance_pool_t::pop_reserved_instance() {
    bq_cond_guard_t guard(instance_cond_);

    while (reserved_instances_.size() == 0) {
        instance_cond_.wait(NULL);
    }

    instance_id_t iid = reserved_instances_.top();
    reserved_instances_.pop();
    instance_cond_.send(true);
    return iid;
}

} // namespace pd
