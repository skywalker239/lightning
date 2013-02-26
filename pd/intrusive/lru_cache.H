#pragma once
// Copyright (C) 2013, Korotkiy Fedor <prime@yandex-team.ru>
// Copyright (C) 2013, YANDEX LLC.
// This code may be distributed under the terms of the GNU GPL v3.
// See ‘http://www.gnu.org/licenses/gpl.html’.
// vim: set tabstop=4 expandtab:
#include <pd/base/assert.H>

#include <pd/intrusive/list.H>

namespace pd {
namespace intrusive {

struct lru_cache_tag_t {};

template<class x_t>
class lru_cache_hook_t : public list_hook_t<x_t, lru_cache_tag_t> {};

template<class x_t>
class lru_cache_t {
public:
    typedef lru_cache_hook_t<x_t> hook_t;

    lru_cache_t(size_t max_size) : size_(0), max_size_(max_size) {
        assert(max_size >= 1);
    }

    //! push or update element, return evicted if any
    x_t* push(hook_t* element) {
        if(element->is_linked()) {
            --size_;
            element->unlink();
        }

        hook_t* evicted = NULL;
        if(size_ >= max_size_) {
            --size_;
            evicted = elements_.pop_front();
        }

        elements_.push_back(element);
        ++size_;

        return static_cast<x_t*>(evicted);
    }

    void pop(hook_t* element) {
        --size_;
        element->unlink();
    }
private:
    size_t size_;
    const size_t max_size_;
    list_t<x_t, lru_cache_tag_t> elements_;
};

} // namespace intrusive
} // namespace pd
