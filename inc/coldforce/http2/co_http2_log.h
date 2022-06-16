#ifndef CO_HTTP2_LOG_H_INCLUDED
#define CO_HTTP2_LOG_H_INCLUDED

#include <coldforce/core/co_log.h>

#include <coldforce/net/co_net_log.h>

#include <coldforce/http2/co_http2.h>
#include <coldforce/http2/co_http2_frame.h>
#include <coldforce/http2/co_http2_header.h>

CO_EXTERN_C_BEGIN

//---------------------------------------------------------------------------//
// http2 log
//---------------------------------------------------------------------------//

#define CO_HTTP2_LOG_CATEGORY_NAME        "HTTP2"

#define CO_HTTP2_LOG_CATEGORY             (CO_LOG_CATEGORY_USER_MAX + 6)

void co_http2_log_write_frame(
    int level,
    const co_net_addr_t* addr1,
    const char* text,
    const co_net_addr_t* addr2,
    const co_http2_frame_t* frame,
    const char* format, ...);

void co_http2_log_write_header(
    int level,
    const co_net_addr_t* addr1,
    const char* text,
    const co_net_addr_t* addr2,
    const co_http2_header_t* header,
    const char* format, ...);

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

#define co_http2_log_write(level, addr1, text, addr2, format, ...) \
    co_net_log_write(level, CO_HTTP2_LOG_CATEGORY, \
        addr1, text, addr2, format, ##__VA_ARGS__)

#define co_http2_log_error(addr1, text, addr2, format, ...) \
    co_http2_log_write(CO_LOG_LEVEL_ERROR, \
        addr1, text, addr2, format, ##__VA_ARGS__)

#define co_http2_log_warning(addr1, text, addr2, format, ...) \
    co_http2_log_write(CO_LOG_LEVEL_WARNING, \
        addr1, text, addr2, format, ##__VA_ARGS__)

#define co_http2_log_info(addr1, text, addr2, format, ...) \
    co_http2_log_write(CO_LOG_LEVEL_INFO, \
        addr1, text, addr2, format, ##__VA_ARGS__)

#define co_http2_log_debug(addr1, text, addr2, format, ...) \
    co_http2_log_write(CO_LOG_LEVEL_DEBUG, \
        addr1, text, addr2, format, ##__VA_ARGS__)

#define co_http2_log_debug_header(addr1, text, addr2, header, format, ...) \
    co_http2_log_write_header(CO_LOG_LEVEL_DEBUG, \
        addr1, text, addr2, header, format, ##__VA_ARGS__)

#define co_http2_log_debug_frame(addr1, text, addr2, frame, format, ...) \
    co_http2_log_write_frame(CO_LOG_LEVEL_DEBUG, \
        addr1, text, addr2, frame, format, ##__VA_ARGS__)

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_HTTP2_API void co_http2_log_set_level(int level);

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_EXTERN_C_END

#endif // CO_HTTP2_LOG_H_INCLUDED
