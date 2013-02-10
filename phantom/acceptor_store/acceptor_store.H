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
#include <phantom/obj.H>

#pragma GCC visibility push(default)
namespace phantom {

class acceptor_store_t : public obj_t {
public:
    struct config_t {
        size_t size;

        config_t() : size(1 << 20) {}

        bool check(const in_t::ptr_t& p) const;
    };

    acceptor_store_t(const string_t& name,
                     const config_t& config);

    enum err_t {
        OK,
        IID_TOO_LOW,
        IID_TOO_HIGH
    };

    err_t lookup(instance_id_t iid,
                 ref_t<acceptor_instance_t>* instance);

    void set_last_snapshot(instance_id_t last_snapshot);

    size_t size() const;

    virtual void init() {};
    virtual void exec() {};
    virtual void fini() {};
	virtual void stat(out_t &, bool ) { /* TODO(prime@) */ };
private:
    std::vector<ref_t<acceptor_instance_t>> ring_buffer_;

    instance_id_t begin_, last_snapshot_;

    thr::spinlock_t lock_;

    bool try_expand_to(instance_id_t iid);
    void init_and_fetch(instance_id_t iid, ref_t<acceptor_instance_t>* instance);

};

}  // namespace phantom
#pragma GCC visibility pop
