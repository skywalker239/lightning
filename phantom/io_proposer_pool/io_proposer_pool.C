// Copyright (C) 2012, Philipp Sinitsyn <prime@yandex-team.ru>
// Copyright (C) 2012, YANDEX LLC.
// This code may be distributed under the terms of the GNU GPL v3.
// See ‘http://www.gnu.org/licenses/gpl.html’.
// vim: set tabstop=4 expandtab:
#include "io_proposer_pool.H"

#include <phantom/module.H>

namespace phantom {

MODULE(io_proposer_pool);

size_t  io_proposer_pool_t::size() {
    return open_instances_.size() +
           reserved_instances_.size() +
           failed_instances_.size();
}

size_t io_proposer_pool_t::open_size() {
    return open_instances_.size();
}

size_t io_proposer_pool_t::failed_size() {
    return failed_instances_.size();
}

size_t io_proposer_pool_t::reserved_size() {
    return reserved_instances_.size();
}

bool io_proposer_pool_t::empty() {
    return open_instances_.empty() &&
           failed_instances_.empty() &&
           reserved_instances_.empty();
}

void io_proposer_pool_t::clear() {
    open_instances_.clear();
    failed_instances_.clear();
    reserved_instances_.clear();
    return;
}

void io_proposer_pool_t::activate() {
    open_instances_.activate();
    reserved_instances_.activate();
    failed_instances_.activate();
}

void io_proposer_pool_t::deactivate() {
    open_instances_.deactivate();
    reserved_instances_.deactivate();
    failed_instances_.deactivate();
}

void io_proposer_pool_t::push_failed(instance_id_t instance_id, ballot_id_t ballot_hint) {
    failed_instances_.push(std::make_tuple(instance_id, ballot_hint));
}

bool io_proposer_pool_t::pop_failed(instance_id_t* instance_id, ballot_id_t* ballot_hint) {
    std::tuple<instance_id_t, ballot_id_t> result;

    if (!failed_instances_.pop(&result))
        return false;

    std::tie(*instance_id, *ballot_hint) = result;
    return true;
}

bool io_proposer_pool_t::failed_empty() {
    return failed_instances_.empty();
}

void io_proposer_pool_t::push_open(instance_id_t instance_id, ballot_id_t ballot_id) {
    open_instances_.push(std::make_tuple(instance_id, ballot_id));
}

bool io_proposer_pool_t::pop_open(instance_id_t* instance_id, ballot_id_t* ballot_id) {
    std::tuple<instance_id_t, ballot_id_t> result;

    if (!open_instances_.pop(&result))
        return false;

    std::tie(*instance_id, *ballot_id) = result;
    return true;
}

bool io_proposer_pool_t::open_empty() {
    return open_instances_.empty();
}

void io_proposer_pool_t::push_reserved(instance_id_t instance_id,
                       ballot_id_t ballot_id,
                       value_t value) {
    reserved_instances_.push(std::make_tuple(instance_id, ballot_id, value));
}

bool io_proposer_pool_t::pop_reserved(instance_id_t* instance_id,
                      ballot_id_t* ballot_id,
                      value_t* value) {
    std::tuple<instance_id_t, ballot_id_t, value_t> result;

    if (!reserved_instances_.pop(&result))
        return false;

    std::tie(*instance_id, *ballot_id, *value) = result;
    return true;
}

bool io_proposer_pool_t::reserved_empty() {
    return reserved_instances_.empty();
}

namespace io_proposer_pool {
config_binding_sname(io_proposer_pool_t);
config_binding_parent(io_proposer_pool_t, io_t, 1);
config_binding_ctor(io_t, io_proposer_pool_t);
}  // namespace io_proposer_pool

}
