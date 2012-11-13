// vim: set tabstop=4 expandtab:
#pragma once

#include <pd/base/thr.H>
#include <phantom/pd.H>

namespace phantom {

class lru_cache_t {
public:
    lru_cache_t(size_t max_size);
    ~lru_cache_t();

    class item_t {
    public:
        item_t();
        virtual ~item_t() throw();

        void attach(lru_cache_t* cache);
        void detach();
    private:
        lru_cache_t* cache_;
        item_t* next_;
        item_t** me_;

        friend class lru_cache_t;
    };

    void access(item_t* item);

    void insert(item_t* item);
    
    size_t evictions() const;
private:
    item_t* evict();

    const size_t max_size_;

    size_t size_;
    size_t evictions_;
    item_t* items_;
    item_t** last_;

    mutable thr::spinlock_t cache_lock_;
    
    friend class item_t;
};

}  // namespace phantom
