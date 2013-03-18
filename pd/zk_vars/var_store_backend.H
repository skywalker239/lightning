// vim: set tabstop=4 expandtab:
#pragma once

#include <pd/base/string.H>

namespace pd {

class var_handle_t;

//! An abstract backend for a config variable store.
//
//  A config variable store at this level is just a
//  string -> string map.
class var_store_backend_t {
public:
    virtual ~var_store_backend_t()
    {}

    //! Adds a key for watching. handle will be used to
    //  update the canonical copy of the variable in the
    //  variable store.
    //  Preconditions:
    //    * add_key has not yet been called for this key.
    virtual void add_key(const string_t& key, var_handle_t* handle) = 0;

    //! Removes the key from the watched set.
    //  Preconditions:
    //    * key has been added earlier with add_key.
    virtual void remove_key(const string_t& key) = 0;

    //! Sets the key to a value if the current version is `version'
    //  or unconditionally if `version' is -1.
    //  Returns the success of the operation.
    //  Preconditions:
    //    * key has been added earlier with add_key.
    virtual bool set(const string_t& key,
                     const string_t& value,
                     int version) = 0;
private:
    var_store_backend_t(const var_store_backend_t&) = delete;
    var_store_backend_t& operator=(const var_store_backend_t&) = delete;
};

}  // namespace pd
