#include "io_learner.H"

#include <phantom/module.H>

#include <pd/base/config.H>
#include <pd/base/log.H>

namespace phantom {

MODULE(io_learner);

io_learner_t::config_t::config_t() throw()
    : instance_buffer_size(0)
{ }

void io_learner_t::config_t::check(const in_t::ptr_t& ptr) const {
    io_t::config_t::check(ptr);

    if(!instance_buffer_size) {
        config::error(ptr, "instance_buffer_size must be > 0");
    }
}

io_learner_t::io_learner_t(const string_t& name, const config_t& config)
    : io_t(name, config),
      commit_tracker_(),
      instance_buffer_(commit_tracker_, config.instance_buffer_size)
{}

io_learner_t::~io_learner_t() throw()
{}

void io_learner_t::init()
{}

void io_learner_t::run()
{}

void io_learner_t::fini()
{}

void io_learner_t::stat(out_t&, bool)
{}

void io_learner_t::apply_phase2(uint64_t instance_id,
                                const string_t& value_id,
                                const string_t& value)
{
    instance_buffer_t::result_t r;
    switch((r = instance_buffer_.begin(instance_id, value_id, value))) {
        case instance_buffer_t::too_old:
            log_warning("apply_phase2(%zu, %s, %s): too_old",
                        instance_id,
                        value_id.ptr(),
                        value.ptr());
            break;
        case instance_buffer_t::too_new:
            log_warning("apply_phase2(%zu, %s, %s): too_new",
                        instance_id,
                        value_id.ptr(),
                        value.ptr());
            break;
        case instance_buffer_t::failed:
            log_warning("apply_phase2(%zu, %s, %s): failed",
                        instance_id,
                        value_id.ptr(),
                        value.ptr());
            break;
        case instance_buffer_t::ok:
            log_debug("apply_phase2(%zu, %s, %s): ok",
                        instance_id,
                        value_id.ptr(),
                        value.ptr());
            break;
        default:
            log_warning("apply_phase2(%zu, %s, %s): unknown result %d",
                        instance_id,
                        value_id.ptr(),
                        value.ptr(),
                        r);
            break;
    }
}

void io_learner_t::apply_commit(uint64_t instance_id,
                                const string_t& value_id)
{
    instance_buffer_t::result_t r;
    switch((r = instance_buffer_.commit(instance_id, value_id))) {
        case instance_buffer_t::too_old:
            log_warning("apply_commit(%zu, %s): too_old",
                        instance_id,
                        value_id.ptr());
            break;
        case instance_buffer_t::too_new:
            log_warning("apply_commit(%zu, %s): too_new",
                        instance_id,
                        value_id.ptr());
            break;
        case instance_buffer_t::failed:
            log_warning("apply_commit(%zu, %s): failed",
                        instance_id,
                        value_id.ptr());
            break;
        case instance_buffer_t::ok:
            log_debug("apply_commit(%zu, %s): ok",
                        instance_id,
                        value_id.ptr());
            break;
        default:
            log_warning("apply_commit(%zu, %s): unknown result %d",
                        instance_id,
                        value_id.ptr(),
                        r);
            break;
    }
}

namespace io_learner {
config_binding_sname(io_learner_t);
config_binding_value(io_learner_t, instance_buffer_size);
config_binding_parent(io_learner_t, io_t, 1);
config_binding_ctor(io_t, io_learner_t);
}  // namespace io_learner

}  // namespace phantom
