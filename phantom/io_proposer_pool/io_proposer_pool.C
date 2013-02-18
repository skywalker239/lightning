#include "io_proposer_pool.H"
#include <phantom/module.H>

namespace phantom {

MODULE(io_proposer_pool);

void io_proposer_pool_t::activate() {
    active = true;
    open_instances_.active = true;
    reserved_instances_.active = true;
    failed_instances_.active = true;
}

void io_proposer_pool_t::deactivate() { 
    active = false;
    open_instances_.active = false;
    reserved_instances_.active = false;
    failed_instances_.active = false;
}

void io_proposer_pool_t::push_failed(instance_id_t instance_id, ballot_id_t ballot_hint) {
    if (!active)
        return;
    failed_instances_.push(open_blob(instance_id, ballot_hint));
}
    
bool io_proposer_pool_t::pop_failed(instance_id_t* instance_id, ballot_id_t* ballot_hint) {
    if (!active)
        return false;
    open_blob result = failed_instances_.pop();
    (*instance_id) = result.iid;
    (*ballot_hint) = result.ballot;
    return true;
}
    
bool io_proposer_pool_t::failed_empty() {
    return failed_instances_.empty();
}

void io_proposer_pool_t::push_open(instance_id_t instance_id, ballot_id_t ballot_id) {
    if (!active)
        return;
    open_instances_.push(open_blob(instance_id, ballot_id));
}
 
bool io_proposer_pool_t::pop_open(instance_id_t* instance_id, ballot_id_t* ballot_id) {
    if (!active)
        return false;
    open_blob result = open_instances_.pop();
    (*instance_id) = result.iid;
    (*ballot_id) = result.ballot;
    return true;
}

bool io_proposer_pool_t::open_empty() {
    return open_instances_.empty();
}

void io_proposer_pool_t::push_reserved(instance_id_t instance_id,
                       ballot_id_t ballot_id,
                       value_t value) {
    if (!active)
        return;
    reserved_instances_.push(reserved_blob(instance_id, ballot_id, value));
}

bool io_proposer_pool_t::pop_reserved(instance_id_t* instance_id,
                      ballot_id_t* ballot_id,
                      value_t* value) {
    if (!active)
        return false;
    reserved_blob result = reserved_instances_.pop();
    (*instance_id) = result.iid;
    (*ballot_id)   = result.ballot;
    (*value)       = result.value;
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
