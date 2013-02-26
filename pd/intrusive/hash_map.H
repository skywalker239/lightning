#pragma once
// Copyright (C) 2013, Korotkiy Fedor <prime@yandex-team.ru>
// Copyright (C) 2013, YANDEX LLC.
// This code may be distributed under the terms of the GNU GPL v3.
// See ‘http://www.gnu.org/licenses/gpl.html’.
// vim: set tabstop=4 expandtab:
#include <functional>
#include <vector>

#include <pd/base/assert.H>
#include <pd/intrusive/list.H>

namespace pd {
namespace intrusive {

struct hash_map_tag_t {};

template<class x_t>
class hash_map_hook_t : public list_hook_t<x_t, hash_map_tag_t> {};

template<class x_t,
         class key_fn_t,
         class hash_fn_t=std::hash<typename x_t::key_t>>
class hash_map_t {
public:
    typedef hash_map_hook_t<x_t> hook_t;

    hash_map_t(size_t size) : buckets_(size), key_(), hash_() {}

    x_t* lookup(typename x_t::key_t key) {
        bucket_t& bucket = key_bucket(key);

        x_t* element = NULL;
        for(auto ptr = bucket.ptr(); ptr; ++ptr) {
            if(key_(*ptr) == key) {
                element = ptr.real_ptr();
            }
        }

        return element;
    }

    void insert(x_t* element) {
        bucket_t& bucket = key_bucket(key_(*element));

        bucket.push_front(element);
    }

    void checkrep() {
        for(int i = 0; i < buckets_.size(); ++i) {
            for(auto ptr = buckets_[i].ptr(); ptr; ++ptr) {
                assert(i == (hash_(key_(*ptr)) % buckets_.size()));
            }
        }
    }
private:
    typedef simple_list_t<x_t, hash_map_tag_t> bucket_t;

    std::vector<bucket_t> buckets_;
    const hash_fn_t hash_;
    const key_fn_t key_;

    bucket_t& key_bucket(typename x_t::key_t key) {
        return buckets_[hash_(key) % buckets_.size()];
    }
};

} // namespace intrusive
} // namespace pd
