// Copyright (C) 2013, Philipp Sinitsyn <ph.sinitsyn@gmail.com>
// Copyright (C) 2013, YANDEX LLC.
// This code may be distributed under the terms of the GNU GPL v3.
// See ‘http://www.gnu.org/licenses/gpl.html’.
// vim: set tabstop=4 expandtab:

#include <queue>
#include <vector>
#include <functional> 

#include <phantom/pd.H>
#include <pd/bq/bq_cond.H>
#include <pd/base/exception.H>

#pragma once

namespace phantom {

//! Represents a Heap Queue with synchronious access
//  Has activate() and deactivate() methods so you can (guess it) turn it on and off.
//
//  1) push() on inactive queue return immediately doing nothing.
//  2) pop() on inactive queue return false immediately leaving
//     other arguments unmodified.
template <typename T, typename Comaprator>
class concurrent_heap_t {
 public:
     concurrent_heap_t () : active(true) 
     {}

     size_t size() {
        bq_cond_guard_t guard(cond_);
        return heap_.size();
     }

     bool empty() {
        bq_cond_guard_t guard(cond_);
        return heap_.empty(); 
     }

     void push(T value) {
        bq_cond_guard_t guard(cond_);
		if (! active)
		  return;
        heap_.push(value);
        cond_.send(true);
     }

     bool pop(T* value) {
        bq_cond_guard_t guard(cond_);

        while(heap_.empty() && active) {
            if(!bq_success(cond_.wait(NULL))) {
                throw exception_sys_t(log::error, errno, "concurrent_heap::pop: %m");
			}
		}

        if (!active)
            return false;
    
        T result = heap_.top();
        heap_.pop();
        (*value) = result;
        return true;
     }

     void activate() {
        bq_cond_guard_t guard(cond_);
        active = true;
        cond_.send(true);
     }

     void deactivate() {
        bq_cond_guard_t guard(cond_);
        active = false;
        cond_.send(true);
     }

     bool is_active() {
        bq_cond_guard_t guard(cond_);
        return active;
     }

	 void clear() {
		bq_cond_guard_t guard(cond_);
		while(!heap_.empty()) {
		  heap_.pop();
		}
	 }


 private:
     bool active;

     bq_cond_t cond_;
     std::priority_queue<T, std::vector<T>, Comaprator> heap_;
};
}

