// Copyright (C) 2012, Fedor Korotkiy <prime@yandex-team.ru>
// Copyright (C) 2012, YANDEX LLC.
// This code may be distributed under the terms of the GNU GPL v3.
// See ‘http://www.gnu.org/licenses/gpl.html’.
// vim: set tabstop=4 expandtab:
#pragma once

#include <vector>

#include <pd/base/config.H>
#include <pd/bq/bq_thr.H>
#include <pd/lightning/acceptor_instance.H>
#include <pd/lightning/defs.H>

#include <phantom/pd.H>
#include <phantom/io.H>

#pragma GCC visibility push(default)
namespace phantom {

class io_acceptor_store_t : public io_t {
public:
    struct config_t : io_t::config_t {
        size_t size;

        config_t() : size(1 << 20) {}

        bool check(const in_t::ptr_t& p) const;
    };

    io_acceptor_store_t(const string_t& name,
                        const config_t& config);

    enum err_t {
        // Acceptor participated in this instance in his previous
        // life(before failure). Now he can't participate in it, or he
        // may break a promise.
        DEAD,
        // Instance id is lower than last snapshot, instance was
        // evicted from store.
        FORGOTTEN,
        // Instance found in store.
        OK,
        // Instance can't be created because store is full.
        // E.g. instance is to far ahead from last snapshot.
        UNREACHABLE,
        // Acceptor is not allowed to participate in this instance yet.
        BEHIND_WALL
    };

    err_t lookup(instance_id_t iid,
                 ref_t<acceptor_instance_t>* instance);
    void nofity_commit();

    void set_birth(instance_id_t birth);
    void move_last_snapshot_to(instance_id_t last_snapshot);
    void move_wall_to(instance_id_t wall);

    instance_id_t next_to_max_touched_iid();
    instance_id_t min_not_committed_iid();
    size_t size();

    virtual void init() {};
    virtual void run() {};
    virtual void fini() {};
	virtual void stat(out_t &, bool ) { /* TODO(prime@) */ };
private:
    std::vector<ref_t<acceptor_instance_t>> ring_buffer_;
    instance_id_t begin_;

    instance_id_t birth_, last_snapshot_, wall_;

    instance_id_t min_not_committed_iid_, next_to_max_touched_iid_;

    thr::spinlock_t lock_;

    void try_expand_to(instance_id_t iid);
    ref_t<acceptor_instance_t>& init_and_fetch(instance_id_t iid);

    bool check_rep();
};

}  // namespace phantom
#pragma GCC visibility pop
