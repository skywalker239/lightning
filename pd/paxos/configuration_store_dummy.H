#pragma once

#include <pd/paxos/configuration_store.H>

#include <pd/base/thr.H>

namespace pd {

//! Manages a static set of vars (does not support adding or removing vars).
class configuration_store_dummy_t : public configuration_store_t
{
public:
    configuration_store_dummy_t(uint64_t version,
                                configuration_var_base_t* vars);

    virtual bool update_snapshot(uint64_t version,
                                 uint64_t* new_version,
                                 ref_t<configuration_var_list_t>*
                                    snapshot) const;

    virtual bool apply_update(uint64_t version,
                              uint64_t* new_version,
                              configuration_var_base_t* list);
private:
    uint64_t version_;
    ref_t<configuration_var_list_t> configuration_;

    mutable thr::spinlock_t lock_;
};

}  // namespace pd
