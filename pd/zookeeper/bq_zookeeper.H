// vim: set tabstop=4 expandtab:
#pragma once

/**
 * These are the wrappers for Zookeeper calls that mimic the
 * synchronous Zookeeper API, but block a single coroutine
 * instead of the whole thread.
 *
 * They also transparently handle Zookeeper session failures and
 * operation timeouts by waiting for a new valid session and retrying
 * on operation timeouts.
 * This behaviour can be turned off by explicitly setting the retry
 * flag to false.
 */

#include <pd/base/string.H>

#include <pd/zookeeper/zhandle_holder.H>

#include <zookeeper/zookeeper.h>

#include <list>

#pragma GCC visibility push(default)

namespace pd {

int bq_zoo_create(zhandle_holder_t& zhandle_holder,
                  const string_t& path,
                  const string_t& value,
                  const struct ACL_vector* acl,
                  int flags,
                  string_t* result,
                  bool retry = false);

int bq_zoo_delete(zhandle_holder_t& zhandle_holder,
                  const string_t& path,
                  int version,
                  bool retry = false);

int bq_zoo_wexists(zhandle_holder_t& zhandle_holder,
                   const string_t& path,
                   watcher_fn watcher,
                   void* watcherCtx,
                   struct Stat* stat,
                   bool retry = false);

int bq_zoo_wget(zhandle_holder_t& zhandle_holder,
                const string_t& path,
                watcher_fn watcher,
                void* watcherCtx,
                string_t* value,
                struct Stat* stat,
                bool retry = false);

int bq_zoo_set(zhandle_holder_t& zhandle_holder,
               const string_t& path,
               const string_t& value,
               int version,
               struct Stat* stat,
               bool retry = false);

int bq_zoo_wget_children(zhandle_holder_t& zhandle_holder,
                         const string_t& path,
                         watcher_fn watcher,
                         void* watcherCtx,
                         std::list<string_t>* strings,
                         bool retry = false);

}  // namespace pd

#pragma GCC visibility pop
