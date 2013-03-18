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
    ~var_handle_t();

    //! The key.
    const string_t& key() const;

    //! Used by the backend to reset this var to
    //  the given value and version.
    void reset(const string_t& value, int version);

    //! Used by the backend to set this var to invalid.
    void clear();
private:
    var_handle_t(const var_handle_t&) = delete;
    var_handle_t& operator=(const var_handle_t&) = delete;

    //! -----------------------------------------------------------------------
    //  The following methods must be called with cond_ locked.

    //! Returns false if this var has not yet been fetched from
    //  the backend.
    bool valid() const;

    //! The current value (undefined if not valid())
    const string_t& value() const;

    //! The current version (undefined if not valid()).
    int version() const;

    //! Adds a reference from a var_base_t to this handle.
    void add_ref();
    //! Removes a reference from a var_base_t to this handle.
    //  Returns true if it was the last one.
    bool remove_ref();

    //! End of methods requiring cond_ to be locked.
    //  ----------------------------------------------------------------------
    
    const string_t key_;

    bool valid_;
    string_t value_;
    int version_;

    mutable bq_cond_t cond_;
    int ref_count_;

    friend class var_base_t;
    friend class var_store_t;
};

}  // namespace pd
