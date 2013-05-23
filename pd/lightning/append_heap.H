#pragma once

#include <utility>

#include <pd/base/ref.H>
#include <pd/lightning/pi_ext.H>
#include <pd/lightning/defs.H>

namespace pd { namespace lightning {

class append_heap_t {
public:
    append_heap_t(size_t limit = 64 * 1024);

    void push(ref_t<pi_ext_t> append_cmd);

    ref_t<pi_ext_t> pop(iid_t prev_iid, term_t prev_term);

    std::pair<bool, std::pair<iid_t, term_t>> first_pending();

    void notify(iid_t latest_iid, term_t latest_term);

    void activate();

    void deactivate();
private:

};

}} // namespace pd::lightning
