// vim: set tabstop=4 expandtab:
#pragma once

#include <pd/base/string.H>

#include <pd/bq/bq_mutex.H>

#include <unordered_map>

namespace pd {

class var_handle_t;
class var_store_backend_t;

//! A key -> value store of versioned configuration variables.
class var_store_t {
public:
    var_store_t(var_store_backend_t& backend);
    ~var_store_t();

    //! If the current version of var @ key is version,
    //  overwrite it with value.
    //  If version is -1, overwrite unconditionally.
    //
    //  Returns true on success.
    bool set(const string_t& key, const string_t& value, int version);

    //! Used by var_base_t constructor to register the var in the store.
    var_handle_t* add_var_ref(const string_t& key);

    //! Used by var_base_t destructor to deregister the var in the store.
    void remove_var_ref(const string_t& key);
private:
    var_store_t(const var_store_t&) = delete;
    var_store_t& operator=(const var_store_t&) = delete;

    struct string_cmp_t {
        bool operator()(const string_t& a, const string_t& b) const {
            return string_t::cmp_eq<ident_t>(a, b);
        }
    };

    struct string_hash_t {
        size_t operator()(const string_t& s) const {
            return s.str().fnv<ident_t>();
        }
    };

    typedef std::unordered_map<string_t,
                               var_handle_t*,
                               string_hash_t,
                               string_cmp_t>
            var_handle_map_t;

    var_store_backend_t& backend_;

    var_handle_map_t var_handle_map_;
    bq_mutex_t       var_handle_map_lock_;

    friend class var_base_t;
};

}  // namespace pd
