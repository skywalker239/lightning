#pragma once

#include <vector>
#include <utility>

#include <pd/base/netaddr_ipv4.H>

#include <pd/lightning/defs.H>

namespace pd { namespace lightning {

class cluster_conf_t {
private:
    std::vector<std::pair<replica_id_t, netaddr_ipv4_t>> alive_replicas_;

public:
    auto begin() -> decltype(alive_replicas_.cbegin());
    auto end() -> decltype(alive_replicas_.cend());
};

}} // namespace pd::lightning
