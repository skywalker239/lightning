#include <pd/paxos/configuration_store_dummy.H>

namespace pd {

configuration_store_dummy_t::configuration_store_dummy_t(uint64_t version,
                                                         configuration_var_base_t* vars)
    : version_(version),
      configuration_(new configuration_var_list_t(vars))
{}

bool configuration_store_dummy_t::update_snapshot(uint64_t version,
                                                  uint64_t* new_version,
                                                  ref_t<configuration_var_list_t>*
                                                    snapshot) const
{
    thr::spinlock_guard_t guard(lock_);

    if(version == configuration_var_base_t::any || version_ > version) {
        *new_version = version_;
        *snapshot = configuration_;
        return true;
    } else {
        return false;
    }
}

bool configuration_store_dummy_t::apply_update(uint64_t version,
                                               uint64_t* new_version,
                                               configuration_var_base_t* list)
{
    while(true) {
        ref_t<configuration_var_list_t> snapshot;
        uint64_t snapshot_version = version_;

        {
            thr::spinlock_guard_t guard(lock_);
            snapshot_version = version_;
            if(version == configuration_var_base_t::any ||
               version == snapshot_version)
            {
                snapshot = configuration_;
            } else {
                return false;
            }
        }

        ref_t<configuration_var_list_t> new_snapshot(new configuration_var_list_t);
        uint64_t next_version = snapshot_version + 1;

        for(configuration_var_base_t* old_var = snapshot->head; old_var; old_var = old_var->next) {
            configuration_var_base_t* v = new configuration_var_base_t(*old_var, new_snapshot->head);

            for(configuration_var_base_t* new_var = list; new_var; new_var = new_var->next) {
                if(string_t::cmp_eq<ident_t>(old_var->name(), new_var->name())) {
                    v->set(new_var->value_string(), next_version);
                    new_var->set(new_var->value_string(), next_version);
                }
            }
        }

        {
            thr::spinlock_guard_t guard(lock_);
            if(version_ == snapshot_version) {
                configuration_ = new_snapshot;
                version_ = next_version;
                *new_version = next_version;
                return true;
            }
        }
    }
}

}  // namespace pd
