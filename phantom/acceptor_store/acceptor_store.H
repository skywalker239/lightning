// Copyright (C) 2012, Fedor Korotkiy <prime@yandex-team.ru>
// Copyright (C) 2012, YANDEX LLC.
// This code may be distributed under the terms of the GNU GPL v3.
// See ‘http://www.gnu.org/licenses/gpl.html’.
// vim: set tabstop=4 expandtab:
#pragma once

#include <pd/lightning/acceptor_instance.H>
#include <pd/lightning/defs.H>

namespace pd {

class acceptor_store_t {
public:
    virtual ~acceptor_store_t() {}

    enum class err_t {
        OK,
        IID_TOO_LOW,
        IID_TOO_HIGH
    };

    virtual err_t lookup(instance_id_t iid,
                         ref_t<acceptor_instance_t>* instance) = 0;
};

}  // namespace pd
