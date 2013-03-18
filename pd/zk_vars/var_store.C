// vim: set tabstop=4 expandtab:
#include <pd/zk_vars/var_store.H>
#include <pd/zk_vars/var_store_backend.H>
#include <pd/zk_vars/var_handle.H>

namespace pd {

var_store_t::var_store_t(var_store_backend_t& backend)
    : backend_(backend)
{}

var_store_t::~var_store_t() {
    assert(var_handle_map_.empty());
}

bool var_store_t::set(const string_t& key,
                      const string_t& value,
                      int version)
{
    return backend_.set(key, value, version);
}

var_handle_t* var_store_t::add_var_ref(const string_t& key) {
    bq_mutex_guard_t guard(var_handle_map_lock_);

    std::pair<var_handle_map_t::iterator, bool> insert_result =
        var_handle_map_.insert(std::make_pair(key, (var_handle_t*) NULL));
    var_handle_t*& handle_ref = insert_result.first->second;
    if(insert_result.second) {
        var_handle_t* handle = new var_handle_t(key);
        handle_ref = handle;
        backend_.add_key(key, handle);
    } else {
        handle_ref->add_ref();
    }
    return handle_ref;
}

void var_store_t::remove_var_ref(const string_t& key) {
    bq_mutex_guard_t guard(var_handle_map_lock_);

    auto handle_iter = var_handle_map_.find(key);
    assert(handle_iter != var_handle_map_.end());
    var_handle_t* var_handle = handle_iter->second;
    if(var_handle->remove_ref()) {
        backend_.remove_key(key);
        var_handle_map_.erase(handle_iter);
        guard.relax();
        delete var_handle;
    }
}

}  // namespace pd
