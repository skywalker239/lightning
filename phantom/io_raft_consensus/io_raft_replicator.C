#include <phantom/io_raft_consensus/io_raft_replicator.H>

#include <pd/lightning/raft_cmd.H>

#include <phantom/io_raft_consensus/io_replica_set.H>
#include <phantom/io_raft_consensus/io_raft_state.H>
#include <phantom/io_raft_consensus/io_log_replica.H>

namespace phantom {

void io_raft_replicator_t::run() {
    while(true) {
//        term_t current_term = raft_state_->wait_state(io_raft_state_t::replica_state_t::LEADER);

        lightning::cluster_conf_t cluster_conf = raft_state_->get_cluster_conf();

        for(const std::pair<replica_id_t, netaddr_ipv4_t>& replica : cluster_conf) {
            accept_client(
                network_->connect(replica.second),
                replica_set_->get_replica_info(replica.first)
            );
        }
    }
}

// void io_raft_replicator_t::accept_client(ref_t<lightning_link_t> link,
//                                          ref_t<io_replica_set_t::replica_info_t> replica_info) {

// }

void io_raft_replicator_t::accept_client(ref_t<lightning_link_t> link) {
    accept_client(link, NULL);
}


} // namespace phantom
