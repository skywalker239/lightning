#pragma once

#include <utility>

#include <pd/lightning/pi_ext.H>
#include <pd/lightning/defs.H>

#pragma GCC visibility push(default)

namespace phantom {

class io_log_replica_t {
public:
    // tries to append entry to log
    bool append(iid_t prev_iid, term_t prev_term, term_t term, ref_t<pi_ext_t> entry);

    // commit all log entries in range [0, last_iid]
    bool commit(term_t term, iid_t last_iid);

    // return iid and term of the latest appended entry
    std::pair<iid_t, term_t> latest_entry();

    void wait_append(iid_t append_iid, interval_t* timeout = NULL);

    iid_t last_commited() const;

    virtual ~io_log_replica_t() {}

protected:
    // try to commit entry
    virtual bool try_commit(iid_t iid, term_t term, ref_t<pi_ext_t> entry) = 0;

    // return pending entry, NULL if there is no entry with such iid
    ref_t<pi_ext_t> get_pending_entry(iid_t iid);
};

class io_follower_log_replica_t : public io_log_replica_t {
public:
    // tell log replica that it can forget entries in range [0, snapshot_iid]
    void move_snapshoot(iid_t snapshot_iid);

    enum class err_t {
        OK,
        FORGOTTEN,
        NOT_APPENDED
    };

    virtual bool try_commit(iid_t iid, term_t term, ref_t<pi_ext_t> entry);

    err_t get_entry(iid_t iid, ref_t<pi_ext_t>* entry, term_t* prev_term);
};

} // namespace phantom

#pragma GCC visibility pop
