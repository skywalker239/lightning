#include "../handler.H"

#include "rpc_messages.pb.h"
#include <google/protobuf/stubs/common.h>

#include <phantom/module.H>

#include <phantom/io_learner/io_learner.H>

#include <pd/base/config.H>
#include <pd/base/log.H>
#include <pd/base/string.H>

namespace phantom {
namespace io_datagram {

MODULE(io_datagram_handler_learner);

class handler_learner_t : public handler_t {
public:
    struct config_t {
        config::objptr_t<io_learner_t> learner;

        inline config_t() throw() {}
        inline ~config_t() throw() {}
        inline void check(const in_t::ptr_t& ptr) const {
            if(!learner) {
                config::error(ptr, "learner is required");
            }
        }
    };

    inline handler_learner_t(const string_t&, const config_t& config) throw()
        : handler_t(),
          learner_(*config.learner)
    {}

    inline ~handler_learner_t() throw()
    {}
private:
    virtual void handle_datagram(const char* data,
                                 size_t length,
                                 const netaddr_t& local_addr,
                                 const netaddr_t& remote_addr);
    virtual void stat(out_t& out, bool clear);

    void handle_phase2_request(const PaxosPhase2RequestData& request);

    void handle_commit(const CommitData& commit);

    io_learner_t& learner_;
};

void handler_learner_t::handle_datagram(const char* data,
                                        size_t length,
                                        const netaddr_t&,
                                        const netaddr_t& remote_addr)
{
    ::google::protobuf::LogSilencer log_silencer;

    RpcMessageData message;
    if(!message.ParseFromArray(data, length)) {
        string_t::ctor_t entry(remote_addr.print_len() + 1);
        entry.print(remote_addr)('\0');
        log_debug("failed to parse message from %s", string_t(entry).ptr());
    }
    if(message.type() != RpcMessageData::PAXOS_PHASE2) {
        return;
    }
    if(!message.has_phase2_request()) {
        return;
    }

    handle_phase2_request(message.phase2_request());
}

void handler_learner_t::handle_phase2_request(const PaxosPhase2RequestData& request) {
    const uint64_t instance_id = request.instance();
    string_t value_id = string(str_t(request.value().id().c_str(), request.value().id().length()));
    string_t value = string(str_t(request.value().data().c_str(), request.value().data().length()));
    learner_.apply_phase2(instance_id, value_id, value);

    for(int i = 0; i < request.commits_size(); ++i) {
        handle_commit(request.commits(i));
    }
}

void handler_learner_t::handle_commit(const CommitData& commit) {
    const uint64_t instance_id = commit.instance();
    string_t value_id = string(str_t(commit.value_id().c_str(), commit.value_id().length()));
    learner_.apply_commit(instance_id, value_id);
}

void handler_learner_t::stat(out_t&, bool) {
    // TODO(skywalker)
}

namespace handler_learner {
config_binding_sname(handler_learner_t);
config_binding_value(handler_learner_t, learner);
config_binding_ctor(handler_t, handler_learner_t);
}  // namespace handler_learner

}  // namespace io_datagram
}  // namespace phantom
