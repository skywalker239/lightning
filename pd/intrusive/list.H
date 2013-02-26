#pragma once
// Copyright (C) 2013, Korotkiy Fedor <prime@yandex-team.ru>
// Copyright (C) 2013, YANDEX LLC.
// This code may be distributed under the terms of the GNU GPL v3.
// See ‘http://www.gnu.org/licenses/gpl.html’.
// vim: set tabstop=4 expandtab:

#include <pd/base/assert.H>

namespace pd {
namespace intrusive {

struct default_tag_t {};

template<class x_t, class tag_t>
class list_t;

template<class x_t, class tag_t = default_tag_t>
class list_hook_t {
public:
    list_hook_t() : next_(NULL), me_(NULL) {}

    list_hook_t(list_hook_t&) = delete;
    list_hook_t(list_hook_t&&) = default;

    list_hook_t& operator= (list_hook_t&) = delete;
    list_hook_t& operator= (list_hook_t&&) = default;

    void link(list_hook_t** after) {
        if((next_ = *after))
            next_->me_ = &next_;

        *after = this;
        me_ = after;
    }

    void unlink() {
        if((*me_ = next_))
            next_->me_ = me_;

        me_ = NULL;
        next_ = NULL;
    }

    bool is_linked() {
        return me_ != NULL;
    }

    list_hook_t* next_;
    list_hook_t** me_;
};

template<class x_t, class tag_t = default_tag_t>
class simple_list_t {
public:
    typedef list_hook_t<x_t, tag_t> hook_t;

    class ptr_t {
    public:
        explicit inline ptr_t(hook_t* p) : p_(p) {}

        inline ptr_t& operator ++ () {
            p_ = p_->next_;
            return *this;
        }

        inline ptr_t operator ++ (int) {
            ptr_t p = ptr_t(p_);
            ++(*this);
            return p;
        }

        inline operator bool () const {
            return p_ != NULL;
        }

        inline operator x_t *() const {
            return static_cast<x_t*>(p_);
        }

        inline x_t* operator->() const {
            return static_cast<x_t*>(p_);
        }

        inline x_t* real_ptr() const {
            return static_cast<x_t*>(p_);
        }
    private:
        hook_t* p_;
    };

    simple_list_t() : first_(NULL) {}

    void push_front(hook_t* x) {
        x->link(&first_);
    }

    void pop_front() {
        first_->unlink();
    }

    x_t* front() {
        return static_cast<x_t*>(first_);
    }

    ptr_t ptr() {
        return ptr_t(first_);
    }

    bool empty() {
        return first_ == NULL;
    }

    void checkrep() {
        if(first_) {
            assert(&first_ == first_->me_);
        }

        for(hook_t* hook = first_; hook && hook->next_; hook = hook->next_) {
            assert(&(hook->next_) == hook->next_->me_);
        }
    };
private:
    hook_t* first_;
};

template<class x_t, class tag_t = default_tag_t>
class list_t {
public:
    typedef list_hook_t<x_t, tag_t> hook_t;

    list_t() {
        first_.next_ = &first_;
        first_.me_ = &(first_.next_);
    }

    x_t* front() {
        return static_cast<x_t*>(first_.next_);
    }

    x_t* pop_front() {
        hook_t* front_element = first_.next_;
        front_element->unlink();
        return static_cast<x_t*>(front_element);
    }

    void push_back(hook_t* element) {
        element->link(first_.me_);
    }

    bool empty() {
        return first_.next_ == &first_;
    }
private:
    hook_t first_;
};

} // namespace intrusive
} // namespace
