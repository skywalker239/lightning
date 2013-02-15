// Copyright (C) 2012, Alexander Kharitonov <alexander.kharitonov@gmail.com>
// Copyright (C) 2012, YANDEX LLC.
// This code may be distributed under the terms of the GNU GPL v3.
// See ‘http://www.gnu.org/licenses/gpl.html’.
// vim: set tabstop=4 expandtab:
#pragma once

#include <pd/zk_vars/var_base.H>

namespace pd {

//! Represents information about a snapshot stored somewhere else.
//  This information consists of the
//  - snapshot version (the instance id at which the snapshot was taken).
//  - snapshot location (currently just a string, maybe a URL...)
class snapshot_var_t : public var_base_t {
public:
    snapshot_var_t(const string_t& key, phantom::io_zconf_t* io_zconf);

    //! False iff failed to parse the information from Zookeeper.
    bool valid() const;

    //! The snapshot version.
    uint64_t snapshot_version() const;

    //! The snapshot location.
    const string_t& location() const;
private:
    virtual void update_impl();

    void set_invalid();

    uint64_t snapshot_version_;
    string_t location_;
    bool valid_;
};

}  // namespace pd
