// Copyright (C) 2012, Fedor Korotkiy <prime@yandex-team.ru>
// Copyright (C) 2012, YANDEX LLC.
// This code may be distributed under the terms of the GNU GPL v3.
// See ‘http://www.gnu.org/licenses/gpl.html’.
// vim: set tabstop=4 expandtab:
#include "io_acceptor_store.H"

#include <pd/base/op.H>
#include <pd/base/assert.H>

#include <phantom/module.H>

namespace phantom {

MODULE(io_acceptor_store);

bool io_acceptor_store_t::config_t::check(const in_t::ptr_t& ptr) const {
    io_t::config_t::check(ptr);

    // TODO(prime@): you know what to do :)
    return true;
}

io_acceptor_store_t::io_acceptor_store_t(const string_t& name,
                                   const config_t& config)
    : io_t(name, config),
      ring_buffer_(config.size),
      begin_(0),
      birth_(kInvalidInstanceId),
      last_snapshot_(0),
      wall_(0),
      min_not_committed_iid_(0),
      next_to_max_touched_iid_(0) {}

io_acceptor_store_t::err_t io_acceptor_store_t::lookup(
        instance_id_t iid,
        ref_t<acceptor_instance_t>* instance) {
    thr::spinlock_guard_t guard(lock_);

    if(iid < birth_) {
        return err_t::DEAD;
    } else if(iid < begin_) {
        return err_t::FORGOTTEN;
    } else if(iid >= wall_) {
        return err_t::BEHIND_WALL;
    } else if(iid >= last_snapshot_ + ring_buffer_.size()) {
        return err_t::UNREACHABLE;
    }

    try_expand_to(iid);

    *instance = init_and_fetch(iid);
    assert(check_rep());
    return err_t::OK;
}

void io_acceptor_store_t::set_birth(instance_id_t birth) {
    thr::spinlock_guard_t guard(lock_);

    wall_ = begin_ = birth_ = birth;

    min_not_committed_iid_ = next_to_max_touched_iid_ = birth;

    // can't participate in them any way
    last_snapshot_ = birth;

    assert(check_rep());
}

void io_acceptor_store_t::notify_commit() {
    thr::spinlock_guard_t guard(lock_);

    while(min_not_committed_iid_ < begin_ + ring_buffer_.size() &&
          init_and_fetch(min_not_committed_iid_)->committed())
    {
        ++min_not_committed_iid_;
    }

    assert(check_rep());
}

bool io_acceptor_store_t::check_rep() {
    return (birth_ != kInvalidInstanceId) &&
           (birth_ <= begin_) &&
           (begin_ <= last_snapshot_) &&
           (next_to_max_touched_iid_ <= wall_) &&
           (next_to_max_touched_iid_ <= begin_ + ring_buffer_.size()) &&
           (begin_ <= min_not_committed_iid_);
           (min_not_committed_iid_ <= next_to_max_touched_iid_);
}

void io_acceptor_store_t::move_wall_to(instance_id_t wall) {
    thr::spinlock_guard_t guard(lock_);

    wall_ = max(wall_, wall);
    assert(check_rep());
}

void io_acceptor_store_t::move_last_snapshot_to(instance_id_t last_snapshot) {
    thr::spinlock_guard_t guard(lock_);

    last_snapshot_ = max(last_snapshot_, last_snapshot);
    assert(check_rep());
}

size_t io_acceptor_store_t::size() {
    thr::spinlock_guard_t guard(lock_);
    return ring_buffer_.size();
}

instance_id_t io_acceptor_store_t::birth() {
    thr::spinlock_guard_t guard(lock_);
    return birth_;
}

instance_id_t io_acceptor_store_t::next_to_max_touched_iid() {
    thr::spinlock_guard_t guard(lock_);
    return next_to_max_touched_iid_;
}

instance_id_t io_acceptor_store_t::min_not_committed_iid() {
    thr::spinlock_guard_t guard(lock_);
    return min_not_committed_iid_;
}

void io_acceptor_store_t::try_expand_to(instance_id_t iid) {
    assert(iid < last_snapshot_ + ring_buffer_.size());

    if(iid >= begin_ + ring_buffer_.size()) {
        // condition implies iid >= ring_buffer_.size(), so there
        // is no int overflow in subtracton
        begin_ = iid - ring_buffer_.size() + 1;

        if(min_not_committed_iid_ < begin_) {
            min_not_committed_iid_ = begin_;
        }
    }
}

ref_t<acceptor_instance_t>& io_acceptor_store_t::init_and_fetch(instance_id_t iid) {
    size_t index = iid % ring_buffer_.size();
    ref_t<acceptor_instance_t>& stored_instance = ring_buffer_[index];

    if(!stored_instance || stored_instance->iid() != iid) {
        stored_instance = new acceptor_instance_t(iid);
    }

    next_to_max_touched_iid_ = max(next_to_max_touched_iid_, iid + 1);

    return stored_instance;
}

namespace acceptor_store {
config_binding_sname(io_acceptor_store_t);
config_binding_value(io_acceptor_store_t, size);

config_binding_parent(io_acceptor_store_t, io_t, 1);
config_binding_ctor(io_t, io_acceptor_store_t);
} // namespace acceptor_store

} // namespace phantom
