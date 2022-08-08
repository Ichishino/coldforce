#ifndef CO_WS_LOG_H_INCLUDED
#define CO_WS_LOG_H_INCLUDED

#include <coldforce/core/co_log.h>

#include <coldforce/net/co_net_log.h>

#include <coldforce/ws/co_ws.h>

CO_EXTERN_C_BEGIN

//---------------------------------------------------------------------------//
// websocket log
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

#define CO_LOG_CATEGORY_NAME_WS        "WS"

#define CO_LOG_CATEGORY_WS             (CO_LOG_CATEGORY_USER_MAX + 7)

//---------------------------------------------------------------------------//
// private
//---------------------------------------------------------------------------//

CO_WS_API
void
co_ws_log_write_frame(
    int level,
    const co_net_addr_t* addr1,
    const char* text,
    const co_net_addr_t* addr2,
    bool fin,
    uint8_t opcode,
    const void* data,
    size_t data_size,
    const char* format,
    ...
);

#define co_ws_log_write(level, addr1, text, addr2, format, ...) \
    co_net_log_write(level, CO_LOG_CATEGORY_WS, \
        addr1, text, addr2, format, ##__VA_ARGS__)

#define co_ws_log_error(addr1, text, addr2, format, ...) \
    co_ws_log_write(CO_LOG_LEVEL_ERROR, \
        addr1, text, addr2, format, ##__VA_ARGS__)

#define co_ws_log_warning(addr1, text, addr2, format, ...) \
    co_ws_log_write(CO_LOG_LEVEL_WARNING, \
        addr1, text, addr2, format, ##__VA_ARGS__)

#define co_ws_log_info(addr1, text, addr2, format, ...) \
    co_ws_log_write(CO_LOG_LEVEL_INFO, \
        addr1, text, addr2, format, ##__VA_ARGS__)

#define co_ws_log_debug(addr1, text, addr2, format, ...) \
    co_ws_log_write(CO_LOG_LEVEL_DEBUG, \
        addr1, text, addr2, format, ##__VA_ARGS__)

#define co_ws_log_debug_frame(addr1, text, addr2, fin, opcode, data, data_size, format, ...) \
    co_ws_log_write_frame(CO_LOG_LEVEL_DEBUG, \
        addr1, text, addr2, fin, opcode, data, data_size, format, ##__VA_ARGS__)

//---------------------------------------------------------------------------//
// public
//---------------------------------------------------------------------------//

CO_WS_API
void
co_ws_log_set_level(
    int level
);

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_EXTERN_C_END

#endif // CO_WS_LOG_H_INCLUDED
