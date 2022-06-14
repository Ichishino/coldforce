#ifndef CO_WS_LOG_H_INCLUDED
#define CO_WS_LOG_H_INCLUDED

#include <coldforce/core/co_log.h>

#include <coldforce/net/co_net_log.h>

#include <coldforce/ws/co_ws.h>

CO_EXTERN_C_BEGIN

//---------------------------------------------------------------------------//
// websocket log
//---------------------------------------------------------------------------//

#define CO_WS_LOG_CATEGORY_NAME        "WS"

#define CO_WS_LOG_CATEGORY             (CO_LOG_CATEGORY_USER_MAX + 5)

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

#define co_ws_log_write(level, addr1, text, addr2, format, ...) \
    co_net_log_write(level, CO_WS_LOG_CATEGORY, \
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

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_WS_API void co_ws_log_set_level(int level);

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_EXTERN_C_END

#endif // CO_WS_LOG_H_INCLUDED
