#pragma once

#include <pd/paxos/configuration_var_base.H>

#include <pd/base/ref.H>

namespace pd {

struct configuration_var_list_t : public ref_count_atomic_t {
    configuration_var_base_t *head;

    configuration_var_list_t() throw()
        : head(NULL)
    {}

    configuration_var_list_t(configuration_var_base_t* vars)
        : head(vars)
    {}

    ~configuration_var_list_t() {
        while(head) {
            delete head;
        }
    }

    friend class ref_t<configuration_var_list_t>;
};

}  // namespace pd
