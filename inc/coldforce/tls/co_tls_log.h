#ifndef CO_TLS_LOG_H_INCLUDED
#define CO_TLS_LOG_H_INCLUDED

#include <coldforce/core/co_log.h>

#include <coldforce/net/co_net_log.h>

#include <coldforce/tls/co_tls.h>

CO_EXTERN_C_BEGIN

//---------------------------------------------------------------------------//
// tls log
//---------------------------------------------------------------------------//

#define CO_TLS_LOG_CATEGORY_NAME        "TLS"

#define CO_TLS_LOG_CATEGORY             (CO_LOG_CATEGORY_USER_MAX + 3)

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

#define co_tls_log_write(level, addr1, text, addr2, format, ...) \
    co_net_log_write(level, CO_TLS_LOG_CATEGORY, \
        addr1, text, addr2, format, ##__VA_ARGS__)

#define co_tls_log_error(addr1, text, addr2, format, ...) \
    co_tls_log_write(CO_LOG_LEVEL_ERROR, \
        addr1, text, addr2, format, ##__VA_ARGS__)

#define co_tls_log_warning(addr1, text, addr2, format, ...) \
    co_tls_log_write(CO_LOG_LEVEL_WARNING, \
        addr1, text, addr2, format, ##__VA_ARGS__)

#define co_tls_log_info(addr1, text, addr2, format, ...) \
    co_tls_log_write(CO_LOG_LEVEL_INFO, \
        addr1, text, addr2, format, ##__VA_ARGS__)

#define co_tls_log_debug(addr1, text, addr2, format, ...) \
    co_tls_log_write(CO_LOG_LEVEL_DEBUG, \
        addr1, text, addr2, format, ##__VA_ARGS__)

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_TLS_API void co_tls_log_enable(bool enable);

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_EXTERN_C_END

#endif // CO_TLS_LOG_H_INCLUDED
