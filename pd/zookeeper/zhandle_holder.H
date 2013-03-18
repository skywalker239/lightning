// vim: set tabstop=4 expandtab:
#pragma once

#include <pd/base/thr.H>

#include <zookeeper/zookeeper.h>

#pragma GCC visibility push(default)

namespace pd {

//! Provides a Zookeeper session handle to other
//  objects.
class zhandle_holder_t {
public:
    virtual ~zhandle_holder_t()
    {}

    //! Blocks until there is a current valid Zookeeper handle to return.
    virtual zhandle_t* wait() = 0;
};

}  // namespace pd

#pragma GCC visibility pop
