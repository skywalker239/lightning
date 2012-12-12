// Copyright (C) 2012, Fedor Korotkiy <prime@yandex-team.ru>
// Copyright (C) 2012, YANDEX LLC.
// This code may be distributed under the terms of the GNU GPL v3.
// See ‘http://www.gnu.org/licenses/gpl.html’.
// vim: set tabstop=4 expandtab:
#include <phantom/io_phase1_batch_executor/io_phase1_batch_executor.H>
#include <phantom/module.H>

namespace phantom {

MODULE(io_phase1_batch_executor_t);

io_phase1_batch_executor_t::io_phase1_batch_executor_t(const string_t& name,
                                                       const config_t& config)
    : io_phase_executor_base_t(name, config) {}

void io_phase1_batch_executor_t::run_acceptor() {
    while(true) {
        wait_becoming_master();

        
    }
}

void io_phase1_batch_executor_t::run_proposer() {

}

} // namespace phantom
