// vim: set tabstop=4 expandtab:
#pragma once

#include <pd/base/string.H>
#include <pd/bq/bq_cond.H>

namespace pd {

//! A handle for a config variable in the var store.
//
//  For a given key it contains the value and version of this var.
//
//
class var_handle_t {
public:
    var_handle_t(const string_t& key);

    //! Returns false if this var has not yet been fetched from
    //  the backend.
    bool valid() const;

    //! The key.
    const string_t& key() const;

    //! The current value (undefined if not valid())
    const string_t& value() const;

    //! The current version (undefined if not valid()).
    int version() const;

    //! Used by the backend to reset this var to
    //  the given value and version.
    void reset(const string_t& value, int version);

    //! Used by the backend to set this var to invalid.
    void clear();
private:
    //! Adds a reference from a var_base_t to this handle.
    void add_ref();
    //! Removes a reference from a var_base_t to this handle.
    //  Returns true if it was the last one.
    bool remove_ref();
    
    const string_t key_;

    bool valid_;
    string_t value_;
    int version_;

    bq_cond_t cond_;
    int refcount_;

    friend class var_base_t;
};

}  // namespace pd
