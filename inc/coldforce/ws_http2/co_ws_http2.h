#ifndef CO_WS_HTTP2_H_INCLUDED
#define CO_WS_HTTP2_H_INCLUDED

#include <coldforce/core/co.h>

//---------------------------------------------------------------------------//
// platform
//---------------------------------------------------------------------------//

#ifdef _MSC_VER
#   ifdef CO_WS_HTTP2_EXPORTS
#       define CO_WS_HTTP2_API  __declspec(dllexport)
#   else
#       define CO_WS_HTTP2_API
#   endif
#else
#   define CO_WS_HTTP2_API
#endif

CO_EXTERN_C_BEGIN

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_EXTERN_C_END

#endif // CO_WS_HTTP2_H_INCLUDED
