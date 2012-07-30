#include "instance_buffer.H"

namespace pd {

instance_buffer_t::instance_buffer_t(const commit_tracker_t& commit_tracker,
                                     size_t buffer_size)
    : commit_tracker_(commit_tracker),
      max_size_(buffer_size),
      begin_(instance_t::invalid_instance_id)
{
    instances_.reserve(max_size_);
    for(size_t i = 0; i < max_size_; ++i) {
        instances_.push_back(ref_t<instance_t>(new instance_t));
    }
}

instance_buffer_t::result_t instance_buffer_t::set(uint64_t instance_id,
                                                   const string_t& value_id,
                                                   const string_t& value)
{
    thr::spinlock_guard_t guard(lock_);

    instance_t* instance;
    result_t res;
    if((res = lookup(instance_id, &instance)) != ok) {
        return res;
    }

    instance->reset(instance_id);
    instance->set_value(value_id, value);
    instance->commit(value_id);

    return ok;
}

instance_buffer_t::result_t instance_buffer_t::commit(uint64_t instance_id,
                                                      const string_t& value_id)
{
    thr::spinlock_guard_t guard(lock_);

    instance_t* instance;
    result_t res;
    if((res = lookup(instance_id, &instance)) != ok) {
        return res;
    }

    return instance->commit(value_id) ? ok : failed;
}

instance_buffer_t::result_t instance_buffer_t::begin(uint64_t instance_id,
                                                     const string_t& value_id,
                                                     const string_t& value)
{
    thr::spinlock_guard_t guard(lock_);

    instance_t* instance;
    result_t res;
    if((res = lookup(instance_id, &instance)) != ok) {
        return res;
    }

    instance->set_value(value_id, value);
    return ok;
}

instance_buffer_t::result_t instance_buffer_t::committed(uint64_t instance_id)
{
    thr::spinlock_guard_t guard(lock_);

    instance_t* instance;
    result_t res;
    if((res = lookup(instance_id, &instance)) != ok) {
        return res;
    }

    return instance->committed() ? ok : failed;
}

instance_buffer_t::result_t instance_buffer_t::get(uint64_t instance_id,
                                                   ref_t<instance_t>* _instance) const
{
    thr::spinlock_guard_t guard(lock_);

    const ref_t<instance_t>& instance = instances_[instance_id % max_size_];

    if(instance_id < begin_) {
        return too_old;
    } else if(instance_id < begin_ + max_size_) {
        if(instance->instance_id() != instance_id) {
            return too_new;
        }
        *_instance = instance;
        return ok;
    } else {
        return too_new;
    }
}

instance_buffer_t::result_t instance_buffer_t::lookup(
    uint64_t instance_id,
    instance_t** destination)
{
    ref_t<instance_t>& instance = instances_[instance_id % max_size_];

    if(begin_ == instance_t::invalid_instance_id) {
        begin_ = instance_id;
        instance->reset(instance_id);
        *destination = instance;
        return ok;
    }

    if(instance_id < begin_) {
        return too_old;
    } else if(instance_id < begin_ + max_size_) {
        if(instance->instance_id() != instance_id) {
            instance->reset(instance_id);
        }
        *destination = instance;
        return ok;
    } else {
        // instance_id >= begin_ + max_size_, need to evict smth.
        uint64_t new_begin = instance_id - max_size_ + 1;
        if(commit_tracker_.first_unknown_instance() < new_begin) {
            return too_new;
        } else {
            begin_ = new_begin;
            instance->reset(instance_id);
            *destination = instance;
            return ok;
        }
    }
}

}  // namespace pd
