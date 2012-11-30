// Copyright (C) 2012, Alexander Kharitonov <alexander.kharitonov@gmail.com>
// Copyright (C) 2012, YANDEX LLC.
// This code may be distributed under the terms of the GNU GPL v3.
// See ‘http://www.gnu.org/licenses/gpl.html’.
// vim: set tabstop=4 expandtab:
#include <phantom/io_zcluster_status/host_status.H>
#include <pd/base/exception.H>

namespace phantom {

host_status_t::host_status_t(int host_id,
                             const string_t& status,
                             ref_t<host_status_t> next)
    : host_id_(host_id),
      status_(status),
      next_(next)
{}

int host_status_t::host_id() const {
    return host_id_;
}

const string_t& host_status_t::status() const {
    return status_;
}

const ref_t<host_status_t>& host_status_t::next() const {
    return next_;
}

ref_t<host_status_t> host_status_t::amend_host(int host_id, const string_t& status) const {
    if(host_id_ == host_id) {
        return new host_status_t(host_id, status, next_);
    } else {
        if(next_ != NULL) {
            return new host_status_t(host_id_, status_, next_->amend_host(host_id, status));
        } else {
            return new host_status_t(host_id_, status_, new host_status_t(host_id, status));
        }
    }
}

host_status_list_t::host_status_list_t(ref_t<host_status_t> head)
    : head_(head)
{}

const ref_t<host_status_t>& host_status_list_t::head() const {
    return head_;
}

ref_t<host_status_list_t> host_status_list_t::amend_host(int host_id,
                                                         const string_t& status) const
{
    if(head_ == NULL) {
        return new host_status_list_t(new host_status_t(host_id, status));
    } else {
        return new host_status_list_t(head_->amend_host(host_id, status));
    }
}

}  // namespace phantom
