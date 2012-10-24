#pragma once

#include <pd/base/string.H>

#include <zookeeper/zookeeper.h>

#include <list>

#pragma GCC visibility push(default)

namespace pd {

int bq_zoo_create(zhandle_t* zh,
                  const string_t& path,
                  const string_t& value,
                  const struct ACL_vector* acl,
                  int flags,
                  string_t* result);

int bq_zoo_delete(zhandle_t* zh, const string_t& path, int version);

int bq_zoo_wexists(zhandle_t* zh,
                   const string_t& path,
                   watcher_fn watcher,
                   void* watcherCtx,
                   struct Stat* stat);

int bq_zoo_wget(zhandle_t* zh,
                const string_t& path,
                watcher_fn watcher,
                void* watcherCtx,
                string_t* value,
                struct Stat* stat);

int bq_zoo_set(zhandle_t* zh,
               const string_t& path,
               const string_t& value,
               int version,
               struct Stat* stat);

int bq_zoo_wget_children(zhandle_t* zh,
                         const string_t& path,
                         watcher_fn watcher,
                         void* watcherCtx,
                         std::list<string_t>* strings);

}  // namespace pd

#pragma GCC visibility pop
