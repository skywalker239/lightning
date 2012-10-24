#pragma once

#include <pd/base/ref.H>
#include <pd/base/string.H>

#include <phantom/pd.H>

#pragma GCC visibility push(default)

namespace phantom {

class host_status_t : public ref_count_atomic_t {
public:
    host_status_t(int host_id, const string_t& status, ref_t<host_status_t> next = NULL);

    int host_id() const;

    const string_t& status() const;

    const ref_t<host_status_t>& next() const;

    ref_t<host_status_t> amend_host(int host_id,
                                    const string_t& status) const;
    ref_t<host_status_t> remove_host(int host_id) const;
private:
    const int host_id_;
    const string_t status_;
    ref_t<host_status_t> next_;

    friend class ref_t<host_status_t>;
};

class host_status_list_t : public ref_count_atomic_t {
public:
    host_status_list_t(ref_t<host_status_t> head = NULL);

    const ref_t<host_status_t>& head() const;

    ref_t<host_status_list_t> amend_host(int host_id,
                                         const string_t& status) const;
    ref_t<host_status_list_t> remove_host(int host_id) const;
private:
    ref_t<host_status_t> head_;

    friend class ref_t<host_status_list_t>;
};

}  // namespace phantom

#pragma GCC visibility pop
