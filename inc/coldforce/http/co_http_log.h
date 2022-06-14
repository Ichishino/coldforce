#ifndef CO_HTTP_LOG_H_INCLUDED
#define CO_HTTP_LOG_H_INCLUDED

#include <coldforce/core/co_log.h>

#include <coldforce/net/co_net_log.h>

#include <coldforce/http/co_http.h>
#include <coldforce/http/co_http_client.h>

CO_EXTERN_C_BEGIN

//---------------------------------------------------------------------------//
// http log
//---------------------------------------------------------------------------//

#define CO_HTTP_LOG_CATEGORY_NAME        "HTTP"

#define CO_HTTP_LOG_CATEGORY             (CO_LOG_CATEGORY_USER_MAX + 4)

void co_http_log_write_request_header(
    int level,
    const co_http_client_t* client,
    const char* text,
    const co_http_request_t* request,
    const char* format, ...);

void co_http_log_write_response_header(
    int level,
    const co_http_client_t* client,
    const char* text,
    const co_http_response_t* response,
    const char* format, ...);

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

#define co_http_log_write(level, addr1, text, addr2, format, ...) \
    co_net_log_write(level, CO_HTTP_LOG_CATEGORY, \
        addr1, text, addr2, format, ##__VA_ARGS__)

#define co_http_log_error(addr1, text, addr2, format, ...) \
    co_http_log_write(CO_LOG_LEVEL_ERROR, \
        addr1, text, addr2, format, ##__VA_ARGS__)

#define co_http_log_warning(addr1, text, addr2, format, ...) \
    co_http_log_write(CO_LOG_LEVEL_WARNING, \
        addr1, text, addr2, format, ##__VA_ARGS__)

#define co_http_log_info(addr1, text, addr2, format, ...) \
    co_http_log_write(CO_LOG_LEVEL_INFO, \
        addr1, text, addr2, format, ##__VA_ARGS__)

#define co_http_log_debug(addr1, text, addr2, format, ...) \
    co_http_log_write(CO_LOG_LEVEL_DEBUG, \
        addr1, text, addr2, format, ##__VA_ARGS__)

#define co_http_log_debug_request_header(client, text, request, format, ...) \
    co_http_log_write_request_header(CO_LOG_LEVEL_DEBUG, \
        client, text, request, format, ##__VA_ARGS__)

#define co_http_log_debug_response_header(client, text, response, format, ...) \
    co_http_log_write_response_header(CO_LOG_LEVEL_DEBUG, \
        client, text, response, format, ##__VA_ARGS__)

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_HTTP_API void co_http_log_set_level(int level);

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_EXTERN_C_END

#endif // CO_HTTP_LOG_H_INCLUDED
