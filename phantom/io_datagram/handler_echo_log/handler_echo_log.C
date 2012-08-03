#include "../handler.H"

#include <phantom/module.H>

#include <pd/base/config.H>
#include <pd/base/log.H>
#include <pd/base/string.H>

namespace phantom {
namespace io_datagram {

MODULE(io_datagram_handler_echo_log);

class handler_echo_log_t : public handler_t {
public:
    struct config_t {
        inline config_t() throw() { }
        inline ~config_t() throw() { }
        inline void check(const in_t::ptr_t&) const { }
    };
    
    inline handler_echo_log_t(const string_t&, const config_t&) throw()
        : handler_t()
    { }

    inline ~handler_echo_log_t() throw() { }
private:
    virtual void handle_datagram(const char* data,
                                 size_t length,
                                 const netaddr_t& local_addr,
                                 const netaddr_t& remote_addr);
    virtual void stat(out_t& out, bool clear);
};

void handler_echo_log_t::handle_datagram(const char* data,
                                         size_t length,
                                         const netaddr_t& /* local_addr */,
                                         const netaddr_t& remote_addr)
{
    // remote_addr: 'message'
    string_t::ctor_t entry(length + remote_addr.print_len() + 5);
    entry.print(remote_addr)(':')(' ')('\'')(str_t(data, length))('\'')('\0');
    log_info("%s", string_t(entry).ptr());
}

void handler_echo_log_t::stat(out_t&, bool)
{}

namespace handler_echo_log {
config_binding_sname(handler_echo_log_t);
config_binding_ctor(handler_t, handler_echo_log_t);
}  // namespace handler_echo_log

}  // namespace io_datagram
}  // namespace phantom
