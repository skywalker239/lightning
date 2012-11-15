// vim: set tabstop=4 expandtab:
#pragma once

#include <pd/base/string.H>

namespace pd {

class var_store_t;
class var_handle_t;

//! A base class for config variables stored in Zookeeper.
class var_base_t {
public:
    //! Attaches to io_zconf at key 'key'.
    var_base_t(const string_t& key, var_store_t& store);
    var_base_t(const var_base_t& other);
    var_base_t& operator=(const var_base_t& other);

    virtual ~var_base_t();

    //! Used to override version check on set().
    static const int kAnyVersion = -1;

    //! Update the local copy from the store.
    //  Returns the current version of the variable.
    //  If the variable has not yet been fetched from the store backend,
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
    //! Resets the variable to a new value. Blocks until the backend call
    //  completes.
    //  Does not reset the local copy (i.e. call set() and then update).
    //
    //  If version is -1, then resets the variable unconditionally.
    //  Otherwise, only succeeds if the current version in the backend
    //  is equal to version.
    //
    //  Returns true iff the set was successful.
    bool do_set(const string_t& value, int version);

    int version_;
    string_t value_string_;

    //! Called by update() to parse the underlying value string into
    //  data the descendant class is interested in.
    virtual void update_impl() {}
private:
    void update_internal();

    string_t key_;

    var_store_t*  store_;
    var_handle_t* handle_;
};

}  // namespace pd
