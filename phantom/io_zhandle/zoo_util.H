// vim: set tabstop=4 expandtab:
#pragma once

#pragma GCC visibility push(default)

namespace phantom {
namespace zoo_util {

//! Converts a numeric Zookeeper session state to a string
//  (e.g. ZOO_CONNECTED_STATE)
const char* state_string(int state);
//! Converts a numeric Zookeeper event type to a string
//  (e.g. ZOO_CREATED_EVENT)
const char* event_string(int type);

}  // namespace zoo_util
}  // namespace phantom

#pragma GCC visibility pop
