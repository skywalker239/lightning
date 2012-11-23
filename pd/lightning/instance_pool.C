#include "pd/lightning/instance_pool.H"

namespace pd {
void instance_pool_t::push_open_instance(instance_id_t iid) {
	bq_cond_guard_t guard(open_instances_cond_);

    while (open_instances_.size() >= open_instances_limit_) {
        open_instances_cond_.wait(NULL);    
    }

    open_instances_.push(iid);
    open_instances_cond_.send();
}

instance_id_t instance_pool_t::pop_open_instance() {
	bq_cond_guard_t guard(open_instances_cond_);

	while (open_instances_.size() == 0) {
        open_instances_cond_.wait(NULL);
    }

    instance_id_t iid = open_instances_.top();
    open_instances_.pop();
    open_instances_cond_.send();
    return iid;
}

void instance_pool_t::push_reserved_instance(instance_id_t iid) {
	bq_cond_guard_t guard(reserved_instances_cond_);

	while (reserved_instances_.size() >= reserved_instances_limit_) {
        reserved_instances_cond_.wait(NULL);
    }

    reserved_instances_.push(iid);
    reserved_instances_cond_.send();
}

instance_id_t instance_pool_t::pop_reserved_instance() {
	bq_cond_guard_t guard(reserved_instances_cond_);

	while (reserved_instances_.size() == 0) {
        reserved_instances_not_empty_.wait(NULL);
    }

    instance_id_t iid = reserved_instances_.top();
    reserved_instances_.pop();
    open_instances_cond_.send();
    return iid;
}

} // nmaespace pd
