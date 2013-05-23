#pragma once

#pragma GCC visibility push(default)

#include <pd/bq/bq_cond.H>

#include <pd/lightning/pi_ext.H>
#include <pd/lightning/defs.H>
#include <pd/lightning/cluster_conf.H>

#include <phantom/pd.H>

namespace phantom {

class io_replica_set_t {
public:
    class replica_info_t : public ref_count_atomic_t {
    public:
        replica_id_t replica_id() const;

        void log_appended(iid_t last_iid, term_t last_term);

        void append_failed(iid_t prev_iid, term_t prev_term);

        friend class ref_t<replica_info_t>;
    };

    iid_t wait_safe_commit_iid(term_t leader_term, iid_t last_safe_commit);

    void set_cluster_conf(lightning::cluster_conf_t conf);

    ref_t<replica_info_t> get_replica_info(replica_id_t replica_id);
};

} // namespace phantom
