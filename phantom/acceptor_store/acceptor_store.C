// Copyright (C) 2012, Fedor Korotkiy <prime@yandex-team.ru>
// Copyright (C) 2012, YANDEX LLC.
// This code may be distributed under the terms of the GNU GPL v3.
// See ‘http://www.gnu.org/licenses/gpl.html’.
// vim: set tabstop=4 expandtab:
#include "acceptor_store.H"

#include <pd/base/op.H>
#include <pd/base/assert.H>

#include <phantom/module.H>

namespace phantom {

MODULE(acceptor_store);

bool acceptor_store_t::config_t::check(const in_t::ptr_t& /* ptr */) const {
    // TODO(prime@): you know what to do :)
    return true;
}

acceptor_store_t::acceptor_store_t(const string_t& name,
                                   const config_t& config)
    : obj_t(name),
      ring_buffer_(config.size),
      begin_(0),
      last_snapshot_(0) {}

acceptor_store_t::err_t acceptor_store_t::lookup(
        instance_id_t iid,
        ref_t<acceptor_instance_t>* instance) {
    thr::spinlock_guard_t guard(lock_);

    if(iid < begin_) {
        return err_t::IID_TOO_LOW;
    }

    if(!try_expand_to(iid)) {
        return err_t::IID_TOO_HIGH;
    }

    init_and_fetch(iid, instance);

    assert(begin_ <= last_snapshot_); // just in case
    return err_t::OK;
}

void acceptor_store_t::set_last_snapshot(instance_id_t last_snapshot) {
    thr::spinlock_guard_t guard(lock_);

    last_snapshot_ = max(last_snapshot_, last_snapshot);
}

size_t acceptor_store_t::size() const {
    return ring_buffer_.size();
}

bool acceptor_store_t::try_expand_to(instance_id_t iid) {
    // TODO(prime@): maybe we shouldn't drop uncommited instances
    if(iid >= last_snapshot_ + ring_buffer_.size()) {
        return false;
    } else if(iid >= begin_ + ring_buffer_.size()) {
        // since condition is true there is no int overflow in
        // subtracton
        begin_ += iid - ring_buffer_.size() + 1;
        return true;
    } else {
        return true;
    }
}

void acceptor_store_t::init_and_fetch(instance_id_t iid,
                                      ref_t<acceptor_instance_t>* instance) {
    size_t index = iid % ring_buffer_.size();

    ref_t<acceptor_instance_t>& stored_instance = ring_buffer_[index];
    if(!stored_instance || stored_instance->instance_id() != iid) {
        stored_instance = new acceptor_instance_t(iid);
        *instance = stored_instance;
    }
}

namespace acceptor_store {
config_binding_sname(acceptor_store_t);
config_binding_ctor_(acceptor_store_t);
config_binding_value(acceptor_store_t, size);
} // namespace acceptor_store

} // namespace phantom
