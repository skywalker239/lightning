// Copyright (C) 2012, Alexander Kharitonov <alexander.kharitonov@gmail.com>
// Copyright (C) 2012, YANDEX LLC.
// This code may be distributed under the terms of the GNU GPL v3.
// See ‘http://www.gnu.org/licenses/gpl.html’.
// vim: set tabstop=4 expandtab:
#pragma once

#include <pd/base/ref.H>

#include <phantom/io_blob_receiver/lru_cache.H>

#include <stdint.h>
#include <vector>

namespace phantom {

class blob_fragment_pool_t {
public:
    blob_fragment_pool_t(size_t cache_size);
    inline ~blob_fragment_pool_t() throw() {}

    class data_t;
    
    ref_t<data_t> insert(uint64_t guid);
    ref_t<data_t> lookup(uint64_t guid);
    
    class data_t : public ref_count_atomic_t
    {
    public:
        data_t(uint64_t guid);
        ~data_t() throw() {}

        bool ready() const;

        int64_t size() const;

        const char* data() const;

        uint64_t guid() const;

        bool update(const char* data,
                    uint32_t total_size,
                    uint32_t begin,
                    uint32_t part_size,
                    uint32_t part_number);
    private:
        const uint64_t guid_;
        int64_t size_;
        uint64_t bytes_known_;
        std::vector<char> data_;
        std::vector<bool> parts_;

        friend class ref_t<data_t>;
    };

private:
    struct item_t;

    struct slot_t {
        thr::spinlock_t spin_;
        item_t* list_;

        slot_t() throw();
        ~slot_t() throw();
    };

    slot_t slots[31];
    lru_cache_t blob_cache_;

    inline slot_t& slot(uint64_t guid) throw() {
        return slots[guid % 31];
    }

    class item_t : public ref_t<data_t>,
                   public lru_cache_t::item_t
    {
        item_t(blob_fragment_pool_t& pool, uint64_t guid);
        ~item_t() throw();
    private:
        slot_t& slot_;
        item_t* next_;
        item_t** me_;

        friend class blob_fragment_pool_t;
    };
};

}  // namespace phantom
