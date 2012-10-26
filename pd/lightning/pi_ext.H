#pragma once

#include <pd/base/ref.H>
#include <pd/base/out.H>

#include <pd/pi/pi.H>

namespace pd {

class pi_ext_t : ref_count_atomic_t {
public:
    pi_t::root_t _root[0];

    inline void* operator new(size_t size, size_t esize) throw() {
        return ::operator new(size + esize);
    }

    inline void operator delete(void* ptr) throw() {
        return ::operator delete(ptr);
    }

    class mem_t : public pi_t::mem_t {
        virtual void* alloc(size_t size) { return new(size) pi_ext_t + 1; }
        virtual void free(void* ptr) { delete ((pi_ext_t*) ptr - 1); }
    public:
        inline mem_t() throw() {}
        inline ~mem_t() throw() {}
    };

    static inline pi_ext_t *parse(
        in_t::ptr_t &ptr, pi_t::parse_fn_t parse_fn
    ) {
        mem_t mem;
        pi_t::root_t *_pi_ext_root = (*parse_fn)(ptr, mem, pi_t::enum_table_def);
        return ((pi_ext_t *)_pi_ext_root) - 1;
    }

    static inline void print(
        out_t &out, ref_t<pi_ext_t> const &pi_ext, pi_t::print_fn_t print_fn
    ) {
        (*print_fn)(out, pi_ext->_root, pi_t::enum_table_def);
    }

    static inline pi_ext_t *__build(pi_t::pro_t const &pro) {
        mem_t mem;
        pi_t::root_t *_pi_ext_root = pi_t::build(pro, mem);
        return ((pi_ext_t *)_pi_ext_root) - 1;
    }

    inline pi_t const &pi() { return _root[0].value; }

    inline pi_t::root_t const &root() { return _root[0]; }

    friend class ref_t<pi_ext_t>;
};

}  // namespace pd
