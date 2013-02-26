// Copyright (C) 2012, Korotkiy Fedor <prime@yandex-team.ru>
// Copyright (C) 2012, YANDEX LLC.
// This code may be distributed under the terms of the GNU GPL v3.
// See ‘http://www.gnu.org/licenses/gpl.html’.
// vim: set tabstop=4 expandtab:
#pragma once

#include <pd/lightning/defs.H>

#include <phantom/io.H>

namespace phantom {

class io_throttle_t : public io_t {
public:
    void start_batch(size_t batch_size);

    void bind_value_to_open_instance();

    void failed_instance_recovered();

    void instance_failed();

    void commit();

    void start();
    void stop();
private:
};

} // namespace phantom
