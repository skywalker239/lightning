// vim: set tabstop=4 expandtab:
#include <phantom/io_blob_receiver/lru_cache.H>

namespace phantom {

lru_cache_t::lru_cache_t(size_t max_size)
    : max_size_(max_size),
      size_(0),
      evictions_(0),
      items_(NULL),
      last_(&items_)
{}

lru_cache_t::~lru_cache_t() {
    thr::spinlock_guard_t guard(cache_lock_);
    while(items_) {
        delete items_;
    }
}

lru_cache_t::item_t::item_t()
    : cache_(NULL),
      next_(NULL),
      me_(NULL)
{}

lru_cache_t::item_t::~item_t() throw() {
    if(cache_) {
        thr::spinlock_guard_t(cache_->cache_lock_);
        detach();
    }
}

void lru_cache_t::item_t::attach(lru_cache_t* cache) {
    assert(!cache_);
    cache_ = cache;
    *(me_ = cache_->last_) = this;
    *(cache_->last_ = &next_) = NULL;
    ++cache->size_;
}

void lru_cache_t::item_t::detach() {
    assert(cache_);
    if((*me_ = next_)) {
        next_->me_ = me_;
    }
    if(cache_->last_ = &next_) {
        cache_->last_ = me_;
    }
    --cache_->size_;
    cache_ = NULL;
    next_ = NULL;
    me_ = NULL;
}

void lru_cache_t::access(item_t* item) {
    thr::spinlock_guard_t guard(cache_lock_);
    assert(item->cache_ == this);
    item->detach();
    item->attach(this);
}

void lru_cache_t::insert(item_t* item) {
    thr::spinlock_guard_t guard(cache_lock_);
    item_t* evicted_item = NULL;
    if(size_ == max_size_) {
        evicted_item = evict();
    }
    item->attach(this);
    guard.relax();
    if(evicted_item) {
        delete evicted_item;
    }
}

lru_cache_t::item_t* lru_cache_t::evict() {
    assert(items_);
    item_t* item = items_;
    item->detach();
    ++evictions_;
    return item;
}

size_t lru_cache_t::evictions() const {
    thr::spinlock_guard_t guard(cache_lock_);
    return evictions_;
}

}  // namespace phantom
