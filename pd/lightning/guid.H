// vim: set tabstop=4 expandtab:
#pragma once

#include <stdint.h>
#include <pd/base/thr.H>

namespace pd {

class guid_generator_t {
public:
    guid_generator_t(uint32_t host_id) throw();
    ~guid_generator_t() throw() { };

    uint64_t get_guid();
private:
    thr::spinlock_t last_musec_lock_;
    uint64_t last_musec_;
    int host_id_;

    guid_generator_t(const guid_generator_t&) = delete;
    guid_generator_t& operator=(const guid_generator_t&) = delete;
};

}
