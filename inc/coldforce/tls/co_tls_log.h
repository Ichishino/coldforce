#ifndef CO_TLS_LOG_H_INCLUDED
#define CO_TLS_LOG_H_INCLUDED

#include <coldforce/core/co_log.h>

#include <coldforce/net/co_net_log.h>

#include <coldforce/tls/co_tls.h>

CO_EXTERN_C_BEGIN

//---------------------------------------------------------------------------//
// tls log
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

#define CO_LOG_CATEGORY_NAME_TLS        "TLS"

#define CO_LOG_CATEGORY_TLS             (CO_LOG_CATEGORY_USER_MAX + 4)

//---------------------------------------------------------------------------//
// private
//---------------------------------------------------------------------------//

void
co_tls_log_write_certificate(
    int level,
    X509* x509
);

#define co_tls_log_write(level, addr1, text, addr2, format, ...) \
    co_net_log_write(level, CO_LOG_CATEGORY_TLS, \
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

#define co_tls_log_debug_hex_dump(addr1, text, addr2, data, size, format, ...) \
    co_net_log_write_hex_dump(CO_LOG_LEVEL_DEBUG, CO_LOG_CATEGORY_TLS, \
        addr1, text, addr2, data, size, format, ##__VA_ARGS__)

#define co_tls_log_debug_certificate(x509) \
    co_tls_log_write_certificate(CO_LOG_LEVEL_DEBUG, x509)

//---------------------------------------------------------------------------//
// public
//---------------------------------------------------------------------------//

CO_TLS_API
void
co_tls_log_set_level(
    int level
);

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_EXTERN_C_END

#endif // CO_TLS_LOG_H_INCLUDED
