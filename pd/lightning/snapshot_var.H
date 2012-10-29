#pragma once

#include <pd/lightning/var_base.H>

namespace pd {

class snapshot_var_t : public var_base_t {
public:
    snapshot_var_t(const string_t& key, phantom::io_zconf_t* io_zconf);

    bool valid() const;

    uint64_t snapshot_version() const;

    const string_t& location() const;
private:
    virtual void update_impl();

    void set_invalid();

    uint64_t snapshot_version_;
    string_t location_;
    bool valid_;
};

}  // namespace pd
