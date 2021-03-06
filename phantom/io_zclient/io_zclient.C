// Copyright (C) 2012, Alexander Kharitonov <alexander.kharitonov@gmail.com>
// Copyright (C) 2012, YANDEX LLC.
// This code may be distributed under the terms of the GNU GPL v3.
// See ‘http://www.gnu.org/licenses/gpl.html’.
// vim: set tabstop=4 expandtab:
#include <phantom/io_zclient/io_zclient.H>

#include <pd/base/exception.H>
#include <pd/base/log.H>

#include <phantom/module.H>

namespace phantom {

MODULE(io_zclient);

void io_zclient_t::config_t::check(const in_t::ptr_t& p) const {
    io_t::config_t::check(p);
    if(!zhandle) {
        config::error(p, "zhandle must be set");
    }
}

io_zclient_t::io_zclient_t(const string_t& name, const config_t& config)
    : io_t(name, config),
      io_zhandle_(*config.zhandle),
      next_(NULL),
      me_(NULL),
      todo_list_(NULL),
      todo_last_(&todo_list_)
{}

io_zclient_t::~io_zclient_t() {
    bq_cond_guard_t guard(todo_cond_);
    while(todo_list_) {
        delete todo_list_;
    }
}

void io_zclient_t::init() {
    io_zhandle_.register_client(this);
}

void io_zclient_t::fini() {
    io_zhandle_.deregister_client(this);
}

io_zclient_t::todo_item_t* io_zclient_t::next_todo() {
    bq_cond_guard_t guard(todo_cond_);
    log_debug("io_zclient(%p) waiting on todo_cond", this);

    if(!todo_list_) {
        if(!bq_success(todo_cond_.wait(NULL))) {
            throw exception_sys_t(log::error, errno, "todo_cond_.wait: %m");
        }
    }

    todo_item_t* todo = todo_list_;
    todo->detach();
    return todo;
}

void io_zclient_t::run() {
    while(true) {
        todo_item_t* todo = next_todo();
        todo->apply(io_zhandle_);
        log_debug(
            "io_zclient(%p) released todo_cond, got todo %p",
            this,
            todo);
        delete todo;
    }
}

void io_zclient_t::schedule(todo_item_t* todo_item) {
    bq_cond_guard_t guard(todo_cond_);
    todo_item->attach(this);
}

void io_zclient_t::stat(out_t&, bool) {
}

io_zclient_t::todo_item_t::todo_item_t()
    : zclient_(NULL),
      next_(NULL),
      me_(NULL)
{
    log_debug("todo_item_t ctor(%p)", this);
}

io_zclient_t::todo_item_t::~todo_item_t() {
    if(zclient_) {
        detach();
    }
}

void io_zclient_t::todo_item_t::attach(io_zclient_t* zclient) {
    assert(!zclient_);
    zclient_ = zclient;
    *(me_ = zclient_->todo_last_) = this;
    *(zclient_->todo_last_ = &next_) = NULL;
    zclient_->todo_cond_.send();
}

void io_zclient_t::todo_item_t::detach() {
    assert(zclient_);
    if((*me_ = next_)) {
        next_->me_ = me_;
    }
    if(zclient_->todo_last_ == &next_) {
        zclient_->todo_last_ = me_;
    }
    zclient_ = NULL;
    next_ = NULL;
    me_ = NULL;
}

namespace io_zclient {
config_binding_sname(io_zclient_t);
config_binding_value(io_zclient_t, zhandle);
config_binding_parent(io_zclient_t, io_t, 1);
}  // namespace io_zclient

}  // namespace phantom
