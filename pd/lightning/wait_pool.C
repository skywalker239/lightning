// Copyright (C) 2012, Alexander Kharitonov <alexander.kharitonov@gmail.com>
// Copyright (C) 2012, YANDEX LLC.
// This code may be distributed under the terms of the GNU GPL v3.
// See ‘http://www.gnu.org/licenses/gpl.html’.
// vim: set tabstop=4 expandtab:
#include <pd/lightning/wait_pool.H>
#include <pd/base/cmp.H>

namespace pd {

wait_pool_t::data_t::data_t(request_id_t id)
    : ref_count_atomic_t(),
      id_(id),
      reply_()
{}

wait_pool_t::data_t::~data_t()
{}

void wait_pool_t::data_t::send(ref_t<pi_ext_t> reply) {
    bq_cond_guard_t guard(cond_);
    reply_ = reply;
    cond_.send();
}


ref_t<pi_ext_t> wait_pool_t::data_t::wait(interval_t* timeout) {
    bq_cond_guard_t guard(cond_);
    while(!reply_) {
        if(!bq_success(cond_.wait(timeout))) {
            if(errno != ETIMEDOUT) {
                throw exception_sys_t(log::error, errno, "wait_pool::data_t.wait: %m");
            }
            return reply_;
        }
    }
    
    return reply_;
}

wait_pool_t::item_t::item_t(wait_pool_t& wait_pool, request_id_t id)
    : ref_t<data_t>(new data_t(id)),
      slot(wait_pool.slot(id))
{
    thr::spinlock_guard_t guard(slot.lock);
    if((next = slot.list)) {
        next->me = &next;
    }
    *(me = &slot.list) = this;
}

wait_pool_t::item_t::~item_t() throw() {
    thr::spinlock_guard_t guard(slot.lock);
    if((*me = next)) {
        next->me = me;
    }
}

ref_t<wait_pool_t::data_t> wait_pool_t::lookup(request_id_t id) {
    slot_t& request_slot = slot(id);

    thr::spinlock_guard_t guard(request_slot.lock);
    for(const item_t* item = request_slot.list;
        item;
        item = item->next)
    {
        if((*item)->id_ == id) return (*item);
    }
    return NULL;
}

wait_pool_t::wait_pool_t(size_t slots)
    : slots_(slots),
      slots_number_(slots)
{}

wait_pool_t::~wait_pool_t()
{}

wait_pool_t::slot_t::slot_t()
    : lock(),
      list(NULL)
{}

wait_pool_t::slot_t::~slot_t() {
    assert(list == NULL);
}

wait_pool_t::slot_t& wait_pool_t::slot(request_id_t request_id) {
    fnv_t hasher;
    char* p = (char*)&request_id;
    for(size_t i = 0; i < sizeof(request_id); ++i) {
        hasher(p[i]);
    }
    return slots_[uint64_t(hasher) % slots_number_];
}

}  // namespace pd
