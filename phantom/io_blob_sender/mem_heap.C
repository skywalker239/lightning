#include <phantom/io_blob_sender/mem_heap.H>

#include <stdlib.h>

namespace phantom {

void* mem_heap_t::alloc(size_t size) {
    return ::malloc(size);
}

void mem_heap_t::free(void* ptr) {
    return ::free(ptr);
}

}  // namespace phantom
