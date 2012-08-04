#pragma once

#include <phantom/io.H>

#include <pd/paxos/commit_tracker.H>
#include <pd/paxos/instance_buffer.H>

#pragma GCC visibility push(default)

namespace phantom {

class io_learner_t : public io_t {
public:
    struct config_t : io_t::config_t {
        sizeval_t instance_buffer_size;

        config_t() throw();
        inline ~config_t() throw() {}
        void check(const in_t::ptr_t& ptr) const;
    };

    io_learner_t(const string_t& name, const config_t& config);
    ~io_learner_t() throw();

    void apply_phase2(uint64_t instance_id,
                      const string_t& value_id,
                      const string_t& value);

    void apply_commit(uint64_t instance_id,
                      const string_t& value_id);

private:
    virtual void init();
    virtual void run();
    virtual void fini();
    virtual void stat(out_t& out, bool clear);

    commit_tracker_t commit_tracker_;
    instance_buffer_t instance_buffer_;
};

}  // namespace phantom

#pragma GCC visibility pop
