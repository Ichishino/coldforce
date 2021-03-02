#ifndef CO_WS_H_INCLUDED
#define CO_WS_H_INCLUDED

#include <coldforce/core/co.h>

//---------------------------------------------------------------------------//
// platform
//---------------------------------------------------------------------------//

#ifdef _MSC_VER
#   ifdef CO_WS_EXPORTS
#       define CO_WS_API  __declspec(dllexport)
#   else
#       define CO_WS_API  __declspec(dllimport)
#   endif
#else
#   define CO_WS_API
#endif

CO_EXTERN_C_BEGIN

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

#define CO_WS_ERROR_INVALID_FRAME          -7001
#define CO_WS_ERROR_INVALID_RESPONSE       -7002
#define CO_WS_ERROR_DATA_TOO_BIG           -7003
#define CO_WS_ERROR_OUT_OF_MEMORY          -7004

#define CO_HTTP_HEADER_SEC_WS_KEY          "Sec-WebSocket-Key"
#define CO_HTTP_HEADER_SEC_WS_EXTENSIONS   "Sec-WebSocket-Extensions"
#define CO_HTTP_HEADER_SEC_WS_ACCEPT       "Sec-WebSocket-Accept"
#define CO_HTTP_HEADER_SEC_WS_PROTOCOL     "Sec-WebSocket-Protocol"
#define CO_HTTP_HEADER_SEC_WS_VERSION      "Sec-WebSocket-Version"

#define CO_WS_PARSE_COMPLETE      0
#define CO_WS_PARSE_MORE_DATA     1

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_EXTERN_C_END

#endif // CO_WS_H_INCLUDED
