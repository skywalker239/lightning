#pragma once

#include <phantom/io_zconf/io_zconf.H>

#include <pd/base/string.H>

#include <pd/bq/bq_cond.H>

namespace pd {

class var_base_t {
public:
    var_base_t(const string_t& key, phantom::io_zconf_t* io_zconf);

    ~var_base_t();

    //! Used to override version check on set().
    static const int kAnyVersion = -1;

    //! Update the local copy from io_zconf.
    //  Returns the current version of the variable.
    //  If the variable has not yet been fetched by io_zconf,
    //  blocks until it becomes available.
    int update();

    //! Blocks until the version of the variable becomes different from
    //  old_version.
    int wait(int old_version);

    //! Resets the variable to a new value. Blocks until ZK call completes.
    //  Does not reset the local copy (i.e. call set() and then update).
    //  
    bool set(const string_t& value, int version);

    int version() const;

    const string_t& value_string() const;

    const string_t& key() const;
protected:
    int version_;
    string_t value_string_;

    //! Called by update() to parse the underlying value string into
    //  data the descendant class is interested in.
    virtual void update_impl() {}
private:
    var_base_t(const var_base_t&) = delete;
    var_base_t& operator=(const var_base_t&) = delete;

    void update_internal();

    string_t key_;

    phantom::io_zconf_t* io_zconf_;
    phantom::io_zconf_t::stat_t* stat_;
};

}  // namespace pd
