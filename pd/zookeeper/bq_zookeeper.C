#include <pd/zookeeper/bq_zookeeper.H>

#include <pd/base/exception.H>
#include <pd/base/string.H>

#include <pd/bq/bq_cond.H>

#include <string.h>

namespace pd {

using std::list;

namespace {

template<typename T>
class delete_guard_t {
public:
    delete_guard_t(T* t) : t_(t) {}
    ~delete_guard_t() {
        if(t_ != NULL) {
            delete t_;
            t_ = NULL;
        }
    }

private:
    T* t_;
};

struct string_callback_data_t {
    int rc;
    string_t *result;
    bq_cond_t cond;
};

void string_completion(int rc, const char* value, const void* data) {
    string_callback_data_t* cbdata = (string_callback_data_t*) data;
    size_t value_length = strlen(value);

    bq_cond_guard_t guard(cbdata->cond);
    cbdata->rc = rc;
    if(rc == ZOK) {
        *(cbdata->result) = string_t::ctor_t(value_length)(str_t(value, value_length));
    }
    cbdata->cond.send();
}

struct void_callback_data_t {
    int rc;
    bq_cond_t cond;
};

void void_completion(int rc, const void* data) {
    void_callback_data_t* cbdata = (void_callback_data_t*) data;

    bq_cond_guard_t guard(cbdata->cond);
    cbdata->rc = rc;
    cbdata->cond.send();
}

struct stat_callback_data_t {
    int rc;
    struct Stat* stat;
    bq_cond_t cond;
};

void stat_completion(int rc, const struct Stat* stat, const void* data) {
    stat_callback_data_t* cbdata = (stat_callback_data_t*) data;

    bq_cond_guard_t guard(cbdata->cond);
    cbdata->rc = rc;
    if(rc == ZOK) {
        *(cbdata->stat) = *stat;
    }
    cbdata->cond.send();
}

struct data_callback_data_t {
    int rc;
    string_t* value;
    struct Stat* stat;
    bq_cond_t cond;
};

void data_completion(int rc,
                     const char* value,
                     int value_len,
                     const struct Stat* stat,
                     const void* data)
{
    data_callback_data_t* cbdata = (data_callback_data_t*) data;

    bq_cond_guard_t guard(cbdata->cond);
    cbdata->rc = rc;
    if(rc == ZOK) {
        *(cbdata->value) = string_t::ctor_t(value_len)(str_t(value, value_len));
        *(cbdata->stat) = *stat;
    }
    cbdata->cond.send();
}

struct strings_callback_data_t {
    int rc;
    list<string_t>* strings;
    bq_cond_t cond;
};

void strings_completion(int rc,
                        const struct String_vector* strings,
                        const void* data)
{
    strings_callback_data_t* cbdata = (strings_callback_data_t*) data;

    list<string_t> result;
    if(rc == ZOK) {
        for(int i = 0; i < strings->count; ++i) {
            char* value = strings->data[i];
            size_t len = strlen(value);
            result.push_back(string_t::ctor_t(len)(str_t(value, len)));
        }
    }

    bq_cond_guard_t guard(cbdata->cond);
    cbdata->rc = rc;
    if(rc == ZOK) {
        cbdata->strings->swap(result);
    }
    cbdata->cond.send();
}

}  // anonymous namespace

int bq_zoo_create(zhandle_t* zh,
                  const string_t& path,
                  const string_t& value,
                  const struct ACL_vector* acl,
                  int flags,
                  string_t* result)
{
    string_t path_z = string_t::ctor_t(path.size() + 1)(path)('\0');

    string_callback_data_t* data = new string_callback_data_t;
    delete_guard_t<string_callback_data_t> guard(data);

    data->result = result;

    bq_cond_guard_t cbguard(data->cond);
    int rc = zoo_acreate(zh,
                         path_z.ptr(),
                         value.ptr(),
                         value.size(),
                         acl,
                         flags,
                         &string_completion,
                         (const void*) data);
    if(rc != ZOK) {
        return rc;
    } else {
        if(!bq_success(data->cond.wait(NULL))) {
            zookeeper_close(zh);
            throw exception_sys_t(log::error, errno, "bq_zoo_create.wait: %m");
        }
        return data->rc;
    }
}

int bq_zoo_delete(zhandle_t* zh, const string_t& path, int version) {
    string_t path_z = string_t::ctor_t(path.size() + 1)(path)('\0');

    void_callback_data_t* data = new void_callback_data_t;
    delete_guard_t<void_callback_data_t> guard(data);

    bq_cond_guard_t cbguard(data->cond);
    int rc = zoo_adelete(zh, path_z.ptr(), version, &void_completion, (const void*) data);
    if(rc != ZOK) {
        return rc;
    } else {
        if(!bq_success(data->cond.wait(NULL))) {
            zookeeper_close(zh);
            throw exception_sys_t(log::error, errno, "bq_zoo_delete.wait: %m");
        }
        return data->rc;
    }
}

int bq_zoo_wexists(zhandle_t* zh,
                   const string_t& path,
                   watcher_fn watcher,
                   void* watcherCtx,
                   struct Stat* stat)
{
    string_t path_z = string_t::ctor_t(path.size() + 1)(path)('\0');

    stat_callback_data_t* data = new stat_callback_data_t;
    delete_guard_t<stat_callback_data_t> guard(data);
    data->stat = stat;

    bq_cond_guard_t cbguard(data->cond);
    int rc = zoo_awexists(zh, path_z.ptr(), watcher, watcherCtx, &stat_completion, (const void*) data);
    if(rc != ZOK) {
        return rc;
    } else {
        if(!bq_success(data->cond.wait(NULL))) {
            zookeeper_close(zh);
            throw exception_sys_t(log::error, errno, "bq_zoo_wexists: %m");
        }
        return data->rc;
    }
}

int bq_zoo_wget(zhandle_t* zh,
                const string_t& path,
                watcher_fn watcher,
                void* watcherCtx,
                string_t* value,
                struct Stat* stat)
{
    string_t path_z = string_t::ctor_t(path.size() + 1)(path)('\0');

    data_callback_data_t* data = new data_callback_data_t;
    delete_guard_t<data_callback_data_t> guard(data);
    data->value = value;
    data->stat = stat;

    bq_cond_guard_t cbguard(data->cond);
    int rc = zoo_awget(zh, path_z.ptr(), watcher, watcherCtx, &data_completion, (const void*) data);

    if(rc != ZOK) {
        return rc;
    } else {
        if(!bq_success(data->cond.wait(NULL))) {
            zookeeper_close(zh);
            throw exception_sys_t(log::error, errno, "bq_zoo_wget.wait: %m");
        }
        return data->rc;
    }
}

int bq_zoo_set(zhandle_t* zh,
               const string_t& path,
               const string_t& value,
               int version,
               struct Stat* stat)
{
    string_t path_z = string_t::ctor_t(path.size() + 1)(path)('\0');

    stat_callback_data_t* data = new stat_callback_data_t;
    delete_guard_t<stat_callback_data_t> guard(data);

    data->stat = stat;

    bq_cond_guard_t cbguard(data->cond);
    int rc = zoo_aset(zh, path_z.ptr(), value.ptr(), value.size(), version, &stat_completion, (const void*) data);
    if(rc != ZOK) {
        return rc;
    } else {
        if(!bq_success(data->cond.wait(NULL))) {
            zookeeper_close(zh);
            throw exception_sys_t(log::error, errno, "bq_zoo_set.wait: %m");
        }
        return data->rc;
    }
}

int bq_zoo_wget_children(zhandle_t* zh,
                         const string_t& path,
                         watcher_fn watcher,
                         void* watcherCtx,
                         list<string_t>* strings)
{
    string_t path_z = string_t::ctor_t(path.size() + 1)(path)('\0');

    strings_callback_data_t* data = new strings_callback_data_t;
    delete_guard_t<strings_callback_data_t> guard(data);
    data->strings = strings;

    bq_cond_guard_t cbguard(data->cond);
    int rc = zoo_awget_children(zh, path_z.ptr(), watcher, watcherCtx, &strings_completion, (const void*) data);
    if(rc != ZOK) {
        return rc;
    } else {
        if(!bq_success(data->cond.wait(NULL))) {
            zookeeper_close(zh);
            throw exception_sys_t(log::error, errno, "bq_zoo_wget_children.wait: %m");
        }
        return data->rc;
    }
}

}  // namespace pd
