// Copyright (C) 2012, Alexander Kharitonov <alexander.kharitonov@gmail.com>
// Copyright (C) 2012, YANDEX LLC.
// This code may be distributed under the terms of the GNU GPL v3.
// See ‘http://www.gnu.org/licenses/gpl.html’.
// vim: set tabstop=4 expandtab:
#pragma once

#include <pd/lightning/defs.H>
#include <pd/zk_vars/var_base.H>
#include <vector>

namespace pd {

//! Represents a Ring Paxos ring.
//  A ring is characterized by its id and the list of
//  participating hosts. The first host in the ring is its master.
class ring_var_t : public var_base_t {
public:
    ring_var_t(const string_t& key, var_store_t& store);

    //! False iff failed to parse current ring from Zookeeper.
    bool valid() const;

    //! This ring id.
    ring_id_t ring_id() const;

    //! The host id of the master of this ring.
    host_id_t master_host_id() const;

    //! Returns true iff host_id belongs to this ring.
    bool is_in_ring(host_id_t host_id) const;

    //! Returns the host id of the next host in the ring after
    //  host_id.
    //  If host_id is not in ring, returns kInvalidHostId.
    host_id_t next_host_id(host_id_t host_id) const;

    //! Returns the host id of the previous host in the ring before
    //  host_id.
    //  If host_id is not in ring, returns kInvalidHostId.
    host_id_t prev_host_id(host_id_t host_id) const;

    //! Sets the ring variable in Zookeeper.
    void set(uint32_t ring_id, const std::vector<uint32_t>& host_ids);
private:
    virtual void update_impl();
};

}  // namespace pd
