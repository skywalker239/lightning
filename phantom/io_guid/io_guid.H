// Copyright (C) 2012, Alexey Pervushin <billyevans@yandex-team.ru>
// Copyright (C) 2012, YANDEX LLC.
// This code may be distributed under the terms of the GNU GPL v3.
// See ‘http://www.gnu.org/licenses/gpl.html’.
// vim: set tabstop=4 expandtab:
#pragma once

#include <phantom/io.H>
#include <pd/lightning/guid.H>

#pragma GCC visibility push(default)

namespace phantom {

class io_guid_t : public io_t {
public:
    struct config_t : public io_t::config_t {
        host_id_t host_id;

        config_t() throw();
        ~config_t() throw() {};
        void check(const in_t::ptr_t& p) const;
    };

    io_guid_t(const string_t& name, const config_t& config);
    ~io_guid_t() throw() {}

    guid_generator_t& generator();
private:
    guid_generator_t guid_;
    virtual void init();
    virtual void run();
    virtual void fini();

    virtual void stat(out_t& out, bool clear);
};

} // namespace phantom

#pragma GCC visibility pop
