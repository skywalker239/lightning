#pragma once

#include <pd/paxos/configuration_var_list.H>

namespace pd {

class configuration_store_t {
public:
    virtual ~configuration_store_t() {}


    //! If the current store version is greater than version or
    //  if version is configuration_var_base_t::any,
    //  resets snapshot with the current contents of the store,
    //  sets new_version to the current version and returns true.
    //  Else does nothing and returns false.
    virtual bool update_snapshot(uint64_t version,
                                 uint64_t *new_version,
                                 ref_t<configuration_var_list_t>*
                                    snapshot) const = 0;

    //! If the current store version is version or
    //  if version is configuration_var_base_t::any,
    //  writes the vars from list into the store and returns true,
    //  setting new_version to the current store version.
    //  Otherwise returns false and does nothing.
    virtual bool apply_update(uint64_t version,
                              uint64_t* new_version,
                              configuration_var_base_t* list) = 0;
};

}  // namespace pd
