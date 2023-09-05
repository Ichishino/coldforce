#ifndef CO_TLS_DEBUG_H_INCLUDED
#define CO_TLS_DEBUG_H_INCLUDED

#include <coldforce/tls/co_tls.h>

CO_EXTERN_C_BEGIN

//---------------------------------------------------------------------------//
// tls debug tools
//---------------------------------------------------------------------------//

#if defined(CO_USE_TLS) && defined(CO_OS_WIN) && defined(CO_DEBUG)
#define co_tls_debug_mem_check() \
    do {\
        co_win_debug_crt_set_flags(); \
        CRYPTO_set_mem_functions( \
            co_mem_alloc_dbg, co_mem_realloc_dbg, co_mem_free_dbg); \
    } while (0)
#else
#define co_tls_debug_mem_check()  ((void)0)
#endif

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_EXTERN_C_END

#endif // CO_TLS_DEBUG_H_INCLUDED
