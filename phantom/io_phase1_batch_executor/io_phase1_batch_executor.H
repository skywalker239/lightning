#pragma once
// Copyright (C) 2012, Alexey Pervushin <billyevans@yandex-team.ru>
// Copyright (C) 2012, YANDEX LLC.
// This code may be distributed under the terms of the GNU GPL v3.
// See ‘http://www.gnu.org/licenses/gpl.html’.
// vim: set tabstop=4 expandtab:
#include <pd/lightning/defs.H>
#include <pd/lightning/pi_ext.H>

#include <phantom/io.H>
#include <phantom/io_transport_config/io_transport_config.H>

#pragma GCC visibility push(default)
namespace phantom {

/**
 * Executes phase1 batch commands.
 */
class io_phase1_batch_executor {
public:
    struct config_t : public io_t::config_t {
        uint32_t host_id;
        config::objptr_t<io_transport_config_t> transport_config;

        config_t() throw() : host_id(ring_var_t::kInvalidHostId) {}
        ~config_t() throw() {}
        void check(const in_t::ptr_t& p) const;
    };

    io_phase1_batch_executor(const string_t& name,
                             const config_t& config);
    ~io_phase1_batch_executor();

    bool handle_cmd(const ref_t<pi_ext_t>& ring_cmd_raw);

    virtual void init();
    virtual void run();
    virtual void fini();
private:
    struct batch_ring_cmd_t {
        uint32_t sender_host_id;
        uint32_t ring_id;
        instance_id_t start_instance_id;
        instance_id_t end_instance_id;
        request_id_t request_id;
        ballot_id_t ballot_id;

        pi_t::array_t const* reserved_instances;

        ref_t<pi_ext_t> ring_cmd_raw;
    };

    uint32_t host_id_;

    bool parse_cmd(const ref_t<pi_ext_t>& ring_cmd_raw,
                   batch_ring_cmd_t* ring_cmd);
};

} // namespace phantom
#pragma GCC visibility pop
