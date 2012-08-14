#include <pd/paxos/configuration_var_base.H>

namespace pd {

configuration_var_base_t::configuration_var_base_t(const string_t& name,
                                                   configuration_var_base_t*& list)
    : list_item_t<configuration_var_base_t>(this, list),
      name_(name),
      version_(any)
{}

configuration_var_base_t::configuration_var_base_t(
    const configuration_var_base_t& var,
    configuration_var_base_t*& list)
    : list_item_t<configuration_var_base_t>(this, list),
      value_string_(var.value_string_),
      name_(var.name_),
      version_(var.version_)
{}

}  // namespace pd
