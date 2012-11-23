#include "pd/lightning/instance_pool.H"

namespace pd {

void instance_pool_t::push_open_instance(instance_id_t iid) {
    if (open_instances_.size() >= open_instances_limit_) {
        open_instances_not_full_.wait(NULL);    
    } 
    open_instances_.push(iid);
    open_instances_not_empty_.send();
}

instance_id_t instance_pool_t::pop_open_instance() {
    if (open_instances_.size() == 0) {
        open_instances_not_empty_.wait(NULL);
    }
    instance_id_t iid = open_instances_.top();
    open_instances_.pop();
    open_instances_not_full_.send();
    return iid;
}

void instance_pool_t::push_reserved_instance(instance_id_t iid) {
    if (reserved_instances_.size() >= reserved_instances_limit_) {
        reserved_instances_not_full_.wait(NULL);
    }
    reserved_instances_.push(iid);
    reserved_instances_not_empty_.send();
}

instance_id_t instance_pool_t::pop_reserved_instance() {
    if (reserved_instances_.size() == 0) {
        reserved_instances_not_empty_.wait(NULL);
    }
    instance_id_t iid = reserved_instances_.top();
    reserved_instances_.pop();
    open_instances_not_full_.send();
    return iid;
}
} // nmaespace pd
