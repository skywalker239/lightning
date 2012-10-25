#include <phantom/io_blob_sender/io_blob_sender.H>
#include <phantom/io_blob_sender/mem_heap.H>
#include <phantom/io_blob_sender/out_udp.H>

#include <pd/base/assert.H>
#include <pd/base/exception.H>
#include <pd/base/config.H>

#include <pd/pi/pi_pro.H>

#include <pd/bq/bq_util.H>

#include <phantom/module.H>

#include <unistd.h>

namespace phantom {

MODULE(io_blob_sender);

io_blob_sender_t::config_t::config_t()
    : address(),
      port(0),
      max_datagram_size(0),
      multicast(false)
{}

void io_blob_sender_t::config_t::check(const in_t::ptr_t& p) const {
    io_t::config_t::check(p);

    if(multicast) {
        if(!address) {
            config::error(p, "with multicast, io_blob_sender_t.address must be set");
        }
        if(!port) {
            config::error(p, "with multicast, io_blob_sender_t.port must be set");
        }
    } else {
        if(address) {
            config::error(p, "with multicast, io_blob_sender_t.address must be unset");
        }
        if(port) {
            config::error(p, "with multicast, io_blob_sender_t.port must be unset");
        }
    }
    if(!max_datagram_size) {
        config::error(p, "max_datagram_size must be set");
    }
    //! 20 is IP header, 8 is UDP header, (uint64, 3 * uint32) is chunk header.
    if(max_datagram_size > 8950 - 20 - 8 - 3 * sizeof(uint32_t) - sizeof(uint64_t)) {
        config::error(p, "max_datagram_size is too large (won't fit into a jumbo frame");
    }
}

io_blob_sender_t::io_blob_sender_t(const string_t& name, const config_t& config)
    : io_t(name, config),
      address_(config.address, config.port),
      multicast_(config.multicast),
      max_datagram_size_(config.max_datagram_size),
      fd_(-1),
      blobs_sent_(0),
      bytes_sent_(0),
      packets_sent_(0),
      dups_(0)
{}

io_blob_sender_t::~io_blob_sender_t()
{
    assert(fd_ == -1);
}

void io_blob_sender_t::init() {
    fd_ = ::socket(AF_INET, SOCK_DGRAM, 0);

    if(fd_ < 0) {
        throw exception_sys_t(log::error, errno, "socket: %m");
    }

    bq_fd_setup(fd_);

    if(multicast_) {
        unsigned char ttl = 255;
        if(::setsockopt(fd_, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof(ttl)) < 0) {
            ::close(fd_);
            fd_ = -1;
            throw exception_sys_t(log::error, errno, "setsockopt, IP_MULTICAST_TTL, %d: %m", ttl);
        }
    }
}

void io_blob_sender_t::fini() {
    if(fd_ >= 0) {
        if(::close(fd_) < 0) {
            log_error("io_blob_sender_t::fini: close: %m");
        }
        fd_ = -1;
    }
}

void io_blob_sender_t::run() {
}

void io_blob_sender_t::stat(out_t& out, bool /* clear */) {
    uint32_t blobs_sent = __sync_fetch_and_add(&blobs_sent_, 0);
    uint32_t bytes_sent = __sync_fetch_and_add(&bytes_sent_, 0);
    uint32_t packets_sent = __sync_fetch_and_add(&packets_sent_, 0);
    uint32_t dups = __sync_fetch_and_add(&dups_, 0);

    pi_t::pro_t::map_t::item_t items[4];

    str_t keys[4] = { CSTR("blobs_sent"), CSTR("bytes_sent"), CSTR("packets_sent"), CSTR("dups") };

    items[0].key = pi_t::pro_t(keys[0]);
    items[0].value = pi_t::pro_t::int_t(blobs_sent);
    items[1].key = pi_t::pro_t(keys[1]);
    items[1].value = pi_t::pro_t::int_t(bytes_sent);
    items[2].key = pi_t::pro_t(keys[2]);
    items[2].value = pi_t::pro_t::int_t(packets_sent);
    items[3].key = pi_t::pro_t(keys[3]);
    items[3].value = pi_t::pro_t::int_t(dups);

    pi_t::pro_t::map_t map = { 4, items };
    pi_t::pro_t pro(map);

    mem_heap_t mem;
    pi_t::root_t* root = pi_t::build(pro, mem);
    pi_t::print_text(out, root);
    mem.free(root);
}

void io_blob_sender_t::send(uint64_t guid,
                            const pi_t::root_t& value,
                            const netaddr_ipv4_t& dst)
{
    char buffer[max_datagram_size_];
    out_udp_t out(buffer,
                  sizeof(buffer),
                  value.size * sizeof(value.size),
                  fd_,
                  multicast_ ? address_ : dst,
                  guid,
                  &bytes_sent_,
                  &packets_sent_,
                  &dups_);
    __sync_fetch_and_add(&blobs_sent_, 1);
    pi_t::print_app(out, &value);
}

namespace io_blob_sender {
config_binding_sname(io_blob_sender_t);
config_binding_value(io_blob_sender_t, address);
config_binding_value(io_blob_sender_t, port);
config_binding_value(io_blob_sender_t, max_datagram_size);
config_binding_value(io_blob_sender_t, multicast);
config_binding_parent(io_blob_sender_t, io_t, 1);
config_binding_ctor(io_t, io_blob_sender_t);
}  // namespace io_blob_sender

}  // namespace phantom
