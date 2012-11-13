// vim: set tabstop=4 expandtab:
#pragma once

#include <pd/lightning/var_base.H>
#include <vector>

namespace pd {

//! Represents a Ring Paxos ring.
//  A ring is characterized by its id and the list of
//  participating hosts. The first host in the ring is its master.
class ring_var_t : public var_base_t {
public:
    ring_var_t(const string_t& key, phantom::io_zconf_t* io_zconf);

    //! False iff failed to parse current ring from Zookeeper.
    bool valid() const;

    //! This ring id.
    uint32_t ring_id() const;

    //! The host id of the master of this ring.
    uint32_t master_host_id() const;

    //! Returns true iff host_id belongs to this ring.
    bool is_in_ring(uint32_t host_id) const;

    //! Returns the host id of the next host in the ring after
    //  host_id.
    //  If host_id is not in ring, returns kInvalidHostId.
    uint32_t next_host_id(uint32_t host_id) const;

    //! Sets the ring variable in Zookeeper.
    void set(uint32_t ring_id, const std::vector<uint32_t>& host_ids);

    static const uint32_t kInvalidHostId = ~0U;
private:
    virtual void update_impl();
};

}  // namespace pd
