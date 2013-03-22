// Copyright (C) 2012, Alexey Pervushin <billyevans@yandex-team.ru>
// Copyright (C) 2012, YANDEX LLC.
// This code may be distributed under the terms of the GNU GPL v3.
// See ‘http://www.gnu.org/licenses/gpl.html’.
// vim: set tabstop=4 expandtab:
#include "proto_value_receiver.H"

#include <pd/lightning/pi_ext.H>
#include <pd/lightning/value.H>

namespace phantom { namespace io_stream {

MODULE(io_stream_proto_value_receiver);

void proto_value_receiver_t::config_t::check(const in_t::ptr_t& p) const
{
    if(!proposer_pool) {
        config::error(p, "proposer_pool must be set");
    }

    if(!value_id_generator) {
        config::error(p, "proposer_pool must be set");
    }
}

proto_value_receiver_t::proto_value_receiver_t(const string_t&, const config_t& config) throw()
    : proto_t(),
    received_count_(0),
    last_pushed_id_(0),
    master_(false),
    proposer_pool_(*config.proposer_pool),
    value_id_generator_(*config.value_id_generator)
{ }

void proto_value_receiver_t::set_master(bool master) throw()
{
    master_ = master;
}

bool proto_value_receiver_t::request_proc(
    in_t::ptr_t& ptr, out_t&, const netaddr_t&, const netaddr_t&
) {
    if(!ptr || !master_) {
        return false;
    }

    ref_t<pi_ext_t> parsed;
    try {
        parsed = pi_ext_t::parse(ptr, &pi_t::parse_app);
    } catch(const pi_t::exception_t& ex) {
        ex.log();
    } catch(const exception_t& ex) {
        ex.log();
    }

    if(parsed == NULL) {
        return false;
    }

    {
        instance_id_t iid;
        ballot_id_t ballot;

        if(!proposer_pool_.pop_open(&iid, &ballot))
            return false;
        const str_t value_str((char const *)&(parsed->root()), parsed->root().size * sizeof(pi_t::_size_t));
        value_id_t value_id = value_id_generator_.get_guid();
        proposer_pool_.push_reserved(iid, ballot, value_t(value_id, pd::string(value_str)));

        ++received_count_;
        last_pushed_id_ = value_id;
    }
    return true;
}

void proto_value_receiver_t::stat(out_t &out, bool clear) {
    if(clear) {
        received_count_ = 0;
        last_pushed_id_ = 0;
    }
    out('{').lf();
    out(CSTR("\"received_count\":")).print(get_recv_count())(',').lf();
    out(CSTR("\"lats_pushed_value_id\":")).print(get_last_pushed_value_id())(',').lf();
    out('}').lf();
}

size_t proto_value_receiver_t::get_recv_count() const throw() {
    return received_count_;
}

value_id_t proto_value_receiver_t::get_last_pushed_value_id() const throw() {
    return last_pushed_id_;
}

namespace proto_value_receiver {
config_binding_sname(proto_value_receiver_t);
config_binding_value(proto_value_receiver_t, proposer_pool);
config_binding_value(proto_value_receiver_t, value_id_generator);
config_binding_ctor(proto_t, proto_value_receiver_t);
}

}} // namespace phantom::io_stream
