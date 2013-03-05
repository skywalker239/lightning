// Copyright (C) 2012, Alexander Kharitonov <alexander.kharitonov@gmail.com>
// Copyright (C) 2012, YANDEX LLC.
// This code may be distributed under the terms of the GNU GPL v3.
// See ‘http://www.gnu.org/licenses/gpl.html’.
// vim: set tabstop=4 expandtab:
#include <phantom/io_blob_receiver/blob_fragment_pool.H>

#include <cstring>

#include <pd/base/log.H>

namespace phantom {

bool blob_fragment_pool_t::blob_t::update(uint32_t part_begin,
                                          str_t data) {
    thr::spinlock_guard_t guard(lock_);

    if(completed() ||
       part_begin + data.size() > size_ ||
       data.size() + received_ > size_)
    {
        return false;
    }

    std::memcpy(data_.get() + part_begin, data.ptr(), data.size());

    received_ += data.size();

    return completed();
}

str_t blob_fragment_pool_t::blob_t::get_data() {
    thr::spinlock_guard_t guard(lock_);

    assert(completed());

    return str_t(data_.get(), size_);
}

bool blob_fragment_pool_t::blob_t::completed() {
    return received_ == size_;
}

blob_fragment_pool_t::~blob_fragment_pool_t() {
    // TODO(prime@): delete pool items
}

ref_t<blob_fragment_pool_t::blob_t> blob_fragment_pool_t::lookup(uint64_t guid,
                                                                 uint32_t size) {
    thr::spinlock_guard_t guard(lock_);

    pool_item_t* item = map_.lookup(guid);

    if(item) {
        lru_.push(item);
        return item->blob;
    } else {
        ref_t<blob_t> blob(new blob_t(guid, size));
        insert(blob);
        return blob;
    }
}

void blob_fragment_pool_t::insert(const ref_t<blob_t>& blob) {
    pool_item_t* item = new pool_item_t(blob);

    map_.insert(item);

    pool_item_t* evicted = lru_.push(item);
    if(evicted) {
        evicted->hash_hook_t::unlink();
        delete evicted;
    }
}

void blob_fragment_pool_t::remove(uint64_t guid) {
    thr::spinlock_guard_t guard(lock_);

    pool_item_t* item = map_.lookup(guid);

    if(item) {
        lru_.pop(item);
        item->hash_hook_t::unlink();
        delete item;
    }
}

}  // namespace phantom
