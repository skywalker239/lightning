// Copyright (C) 2012, Alexander Kharitonov <alexander.kharitonov@gmail.com>
// Copyright (C) 2012, YANDEX LLC.
// This code may be distributed under the terms of the GNU GPL v3.
// See ‘http://www.gnu.org/licenses/gpl.html’.
// vim: set tabstop=4 expandtab:
#include <phantom/io_blob_receiver/blob_fragment_pool.H>

#include <pd/base/log.H>

#include <string.h>

namespace phantom {

blob_fragment_pool_t::blob_fragment_pool_t(size_t cache_size)
    : blob_cache_(cache_size)
{}

blob_fragment_pool_t::data_t::data_t(uint64_t guid)
    : guid_(guid),
      size_(-1),
      bytes_known_(0)
{}

bool blob_fragment_pool_t::data_t::ready() const {
    return bytes_known_ == size_;
}

int64_t blob_fragment_pool_t::data_t::size() const {
    return size_;
}

const char* blob_fragment_pool_t::data_t::data() const {
    return &data_[0];
}

uint64_t blob_fragment_pool_t::data_t::guid() const {
    return guid_;
}

bool blob_fragment_pool_t::data_t::update(const char* data,
                                          uint32_t total_size,
                                          uint32_t begin,
                                          uint32_t part_size,
                                          uint32_t part_number)
{
    if(size_ == -1) {
        size_ = total_size;
        data_.assign(total_size, '\0');
    } else {
        if(total_size != size_) {
            log_warning("data_t::update(%ld): total_size mismatch (%d, %ld)", guid_, total_size, size_);
            return false;
        }

        if(part_number >= parts_.size()) {
            for(size_t i = parts_.size(); i <= part_number; ++i) {
                parts_.push_back(false);
            }
        }

        if(!parts_[part_number]) {
            bytes_known_ += part_size;
            memcpy(&data_[begin], data, part_size);
            parts_[part_number] = true;
        }

        return true;
    }
}

blob_fragment_pool_t::slot_t::slot_t() throw()
    : spin_(),
      list_(NULL)
{}

blob_fragment_pool_t::slot_t::~slot_t() throw() {
    assert(list_ == NULL);
}

blob_fragment_pool_t::item_t::item_t(blob_fragment_pool_t& pool, uint64_t guid)
    : ref_t<data_t>(new data_t(guid)),
      slot_(pool.slot(guid))
{
    thr::spinlock_guard_t guard(slot_.spin_);
    if((next_ = slot_.list_)) {
        next_->me_ = &next_;
    }
    *(me_ = &slot_.list_) = this;
    pool.blob_cache_.insert(this);
}

blob_fragment_pool_t::item_t::~item_t() throw() {
    thr::spinlock_guard_t guard(slot_.spin_);
    if((*me_ = next_)) {
        next_->me_ = me_;
    }
}

ref_t<blob_fragment_pool_t::data_t> blob_fragment_pool_t::insert(uint64_t guid)
{
    slot_t& _slot = slot(guid);

    thr::spinlock_guard_t guard(_slot.spin_);
    return *(new item_t(*this, guid));
}

ref_t<blob_fragment_pool_t::data_t> blob_fragment_pool_t::lookup(uint64_t guid) {
    slot_t& _slot = slot(guid);

    thr::spinlock_guard_t guard(_slot.spin_);
    for(item_t* item = _slot.list_; item; item = item->next_) {
        if((*item)->guid() == guid) {
            blob_cache_.access(item);
            return (*item);
        }
    }
    
    return NULL;
}

}  // namespace phantom
