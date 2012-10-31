#pragma once

#include <phantom/io_zconf/io_zconf.H>

#include <pd/base/string.H>

#include <pd/bq/bq_cond.H>

namespace pd {

//! A base class for config variables stored in Zookeeper.
class var_base_t {
public:
    //! Attaches to io_zconf at key 'key'.
    var_base_t(const string_t& key, phantom::io_zconf_t* io_zconf);
    var_base_t(const var_base_t& other);
    var_base_t& operator=(const var_base_t& other);

    virtual ~var_base_t();

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

    //! The current version (doesn't block).
    int version() const;

    //! The current value as a string (simply the contents of a node in ZK).
    const string_t& value_string() const;

    //! The key.
    const string_t& key() const;
protected:
    //! Resets the variable to a new value. Blocks until ZK call completes.
    //  Does not reset the local copy (i.e. call set() and then update).
    //
    //  If version is -1, then resets the variable unconditionally.
    //  Otherwise, only succeeds if the ZK node version
    //  is equal to version.
    //
    //  Returns true iff the set in ZK was successful.
    bool do_set(const string_t& value, int version);

    int version_;
    string_t value_string_;

    //! Called by update() to parse the underlying value string into
    //  data the descendant class is interested in.
    virtual void update_impl() {}
private:
    void update_internal();

    string_t key_;

    phantom::io_zconf_t* io_zconf_;
    phantom::io_zconf_t::stat_t* stat_;
};

}  // namespace pd
