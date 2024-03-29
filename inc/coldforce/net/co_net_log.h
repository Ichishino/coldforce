#ifndef CO_NET_LOG_H_INCLUDED
#define CO_NET_LOG_H_INCLUDED

#include <coldforce/core/co_log.h>

#include <coldforce/net/co_net.h>
#include <coldforce/net/co_net_addr.h>

CO_EXTERN_C_BEGIN

//---------------------------------------------------------------------------//
// net log
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

#define CO_LOG_CATEGORY_NAME_TCP        "TCP"
#define CO_LOG_CATEGORY_NAME_UDP        "UDP"

#define CO_LOG_CATEGORY_TCP             (CO_LOG_CATEGORY_USER_MAX + 2)
#define CO_LOG_CATEGORY_UDP             (CO_LOG_CATEGORY_USER_MAX + 3)

//---------------------------------------------------------------------------//
// private
//---------------------------------------------------------------------------//

CO_NET_API
void
co_net_log_write_addresses(
    co_log_t* log,
    int category,
    const co_net_addr_t* addr1,
    const char* text,
    const co_net_addr_t* addr2
);

CO_NET_API
void
co_net_log_write(
    int level,
    int category,
    const co_net_addr_t* addr1,
    const char* text,
    const co_net_addr_t* addr2,
    const char* format,
    ...
);

CO_NET_API
void
co_net_log_write_hex_dump(
    int level,
    int category,
    const co_net_addr_t* addr1,
    const char* text,
    const co_net_addr_t* addr2,
    const void* data, size_t size,
    const char* format,
    ...
);

#define co_tcp_log_write(level, addr1, text, addr2, format, ...) \
    co_net_log_write(level, CO_LOG_CATEGORY_TCP, \
        addr1, text, addr2, format, ##__VA_ARGS__)

#define co_tcp_log_error(addr1, text, addr2, format, ...) \
    co_tcp_log_write(CO_LOG_LEVEL_ERROR, \
        addr1, text, addr2, format, ##__VA_ARGS__)

#define co_tcp_log_warning(addr1, text, addr2, format, ...) \
    co_tcp_log_write(CO_LOG_LEVEL_WARNING, \
        addr1, text, addr2, format, ##__VA_ARGS__)

#define co_tcp_log_info(addr1, text, addr2, format, ...) \
    co_tcp_log_write(CO_LOG_LEVEL_INFO, \
        addr1, text, addr2, format, ##__VA_ARGS__)

#define co_tcp_log_debug(addr1, text, addr2, format, ...) \
    co_tcp_log_write(CO_LOG_LEVEL_DEBUG, \
        addr1, text, addr2, format, ##__VA_ARGS__)

#define co_tcp_log_debug_hex_dump(addr1, text, addr2, data, size, format, ...) \
    co_net_log_write_hex_dump(CO_LOG_LEVEL_DEBUG, CO_LOG_CATEGORY_TCP, \
        addr1, text, addr2, data, size, format, ##__VA_ARGS__)

#define co_udp_log_write(level, addr1, text, addr2, format, ...) \
    co_net_log_write(level, CO_LOG_CATEGORY_UDP, \
        addr1, text, addr2, format, ##__VA_ARGS__)

#define co_udp_log_error(addr1, text, addr2, format, ...) \
    co_udp_log_write(CO_LOG_LEVEL_ERROR, \
        addr1, text, addr2, format, ##__VA_ARGS__)

#define co_udp_log_warning(addr1, text, addr2, format, ...) \
    co_udp_log_write(CO_LOG_LEVEL_WARNING, \
        addr1, text, addr2, format, ##__VA_ARGS__)

#define co_udp_log_info(addr1, text, addr2, format, ...) \
    co_udp_log_write(CO_LOG_LEVEL_INFO, \
        addr1, text, addr2, format, ##__VA_ARGS__)

#define co_udp_log_debug(addr1, text, addr2, format, ...) \
    co_udp_log_write(CO_LOG_LEVEL_DEBUG, \
        addr1, text, addr2, format, ##__VA_ARGS__)

#define co_udp_log_debug_hex_dump(addr1, text, addr2, data, size, format, ...) \
    co_net_log_write_hex_dump(CO_LOG_LEVEL_DEBUG, CO_LOG_CATEGORY_UDP, \
        addr1, text, addr2, data, size, format, ##__VA_ARGS__)

//---------------------------------------------------------------------------//
// public
//---------------------------------------------------------------------------//

CO_NET_API
void
co_tcp_log_set_level(
    int level
);

CO_NET_API
void
co_udp_log_set_level(
    int level
);

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_EXTERN_C_END

#endif // CO_NET_LOG_H_INCLUDED
