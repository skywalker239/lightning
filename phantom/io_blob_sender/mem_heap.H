#pragma once

#include <pd/pi/pi.H>
#include <phantom/pd.H>

namespace phantom {

class mem_heap_t : public pi_t::mem_t {
public:
    virtual void* alloc(size_t size);
    virtual void free(void* ptr);
};

}  // namespace phantom
