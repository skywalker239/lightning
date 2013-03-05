#pragma once
// Copyright (C) 2013, Korotkiy Fedor <prime@yandex-team.ru>
// Copyright (C) 2013, YANDEX LLC.
// This code may be distributed under the terms of the GNU GPL v3.
// See ‘http://www.gnu.org/licenses/gpl.html’.
// vim: set tabstop=4 expandtab:
#include <stdint.h>
#include <memory>
#include <vector>

#include <pd/base/ref.H>
#include <pd/base/thr.H>
#include <pd/base/str.H>
#include <pd/intrusive/lru_cache.H>
#include <pd/intrusive/hash_map.H>

#include <phantom/pd.H>

#pragma GCC visibility push(default)

namespace phantom {

class blob_fragment_pool_t {
public:
    class blob_t : public ref_count_atomic_t {
    public:
        blob_t(uint64_t guid, uint32_t size)
            : guid_(guid),
              size_(size),
              received_(0),
              data_(new char[size]) {
            assert(size > 0);
        };

        uint64_t guid() { return guid_; }

        bool update(uint32_t part_begin, str_t data);

        str_t get_data();
    private:
        const uint64_t guid_;
        const uint32_t size_;

        uint32_t received_;
        std::unique_ptr<char[]> data_;

        thr::spinlock_t lock_;

        bool completed();

        friend class ref_t<blob_t>;
    };

    blob_fragment_pool_t(size_t cache_size, size_t hash_size)
        : lru_(cache_size), map_(hash_size) {}

    ~blob_fragment_pool_t();

    ref_t<blob_t> lookup(uint64_t guid, uint32_t size);
    void remove(uint64_t guid);
private:
    struct pool_item_t : public intrusive::lru_cache_hook_t<pool_item_t>,
                         public intrusive::hash_map_hook_t<pool_item_t> {
        typedef uint64_t key_t;

        pool_item_t(const ref_t<blob_t>& blob) : blob(blob) {}

        ref_t<blob_t> blob;
    };

    struct pool_item_key_fn_t {
        uint64_t operator() (const pool_item_t& item) const {
            return item.blob->guid();
        }
    };

    typedef intrusive::lru_cache_t<pool_item_t> lru_cache_t;
    typedef intrusive::hash_map_t<pool_item_t, pool_item_key_fn_t> hash_map_t;

    typedef lru_cache_t::hook_t lru_hook_t;
    typedef hash_map_t::hook_t hash_hook_t;

    lru_cache_t lru_;
    hash_map_t map_;
    thr::spinlock_t lock_;

    void insert(const ref_t<blob_t>& blob);
};

}  // namespace phantom

#pragma GCC visibility pop
