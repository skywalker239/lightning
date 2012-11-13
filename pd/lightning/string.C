// vim: set tabstop=4 expandtab:
#include <pd/base/in.H>
#include <pd/base/string.H>

namespace pd {

template<>
bool in_t::helper_t<string_t>::parse(
    ptr_t& ptr, string_t& val, char const * /*fmt*/, error_handler_t /*handler*/)
{
    ptr_t p(ptr);
    p.seek_end();
    size_t len = p - ptr;
    val = string_t::ctor_t(len)(ptr, len);
    return true;
}

}  // namespace pd
