#pragma once

#include <pd/base/list.H>
#include <pd/base/ref.H>
#include <pd/base/string.H>

namespace pd {

class configuration_var_base_t : public list_item_t<configuration_var_base_t>
{
public:
    using list_item_t<configuration_var_base_t>::next;

    enum version {
        any = ~0ULL            // for unconditional updates
    };

    configuration_var_base_t(const string_t& name,
                             configuration_var_base_t*& list) throw();

    //! copies name, value and version from var
    configuration_var_base_t(const configuration_var_base_t& var,
                             configuration_var_base_t*& list) throw();

    virtual ~configuration_var_base_t() throw() {}

    const string_t& name() const { return name_; }

    uint64_t version() const { return version_; }

    const string_t& value_string() const { return value_string_; }

    void set(const string_t& value_string, uint64_t version = any);
protected:
    string_t value_string_;

private:
    //! Override to parse value_string_ into something subclass-dependent.
    virtual void parse_value() {}

    string_t name_;
    uint64_t version_;

    friend class configuration_accessor_t;
};

}  // namespace pd
