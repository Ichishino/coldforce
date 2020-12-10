#ifndef CO_HTTP_H_INCLUDED
#define CO_HTTP_H_INCLUDED

#include <coldforce/core/co.h>

//---------------------------------------------------------------------------//
// platform
//---------------------------------------------------------------------------//

#ifdef _MSC_VER
#   ifdef CO_HTTP_EXPORTS
#       define CO_HTTP_API  __declspec(dllexport)
#   else
#       define CO_HTTP_API  __declspec(dllimport)
#   endif
#else
#   define CO_HTTP_API
#endif

CO_EXTERN_C_BEGIN

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

#if (SIZE_MAX == UINT64_MAX)
#   define CO_SIZE_T_DEC_DIGIT_MAX     20
#elif (SIZE_MAX == UINT32_MAX)
#   define CO_SIZE_T_DEC_DIGIT_MAX     10
#endif

#define CO_SIZE_T_HEX_DIGIT_MAX (sizeof(size_t) * 2)

#define CO_HTTP_SP                  " "
#define CO_HTTP_CRLF                "\r\n"
#define CO_HTTP_COLON               ":"

#define CO_HTTP_CRLF_LENGTH         2
#define CO_HTTP_COLON_LENGTH        1

#define CO_HTTP_PARSE_COMPLETE      0
#define CO_HTTP_PARSE_ERROR         -1
#define CO_HTTP_PARSE_MORE_DATA     1

#define CO_HTTP_ERROR_CONNECT_FAILED        -9001
#define CO_HTTP_ERROR_TLS_HANDSHAKE_FAILED  -9002
#define CO_HTTP_ERROR_CONNECTION_CLOSED     -9003
#define CO_HTTP_ERROR_OUT_OF_MEMORY         -9004
#define CO_HTTP_ERROR_PARSE_HEADER          -9005
#define CO_HTTP_ERROR_PARSE_CONTENT         -9006
#define CO_HTTP_ERROR_CANCEL                -9007

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

#define CO_HTTP_HEADER_CONTENT_LENGTH       "Content-Length"
#define CO_HTTP_HEADER_HOST                 "Host"
#define CO_HTTP_HEADER_TRANSFER_ENCODING    "Transfer-Encoding"
#define CO_HTTP_HEADER_SET_COOKIE           "Set-Cookie"
#define CO_HTTP_HEADER_COOKIE               "Cookie"

CO_EXTERN_C_END

#endif // CO_HTTP_H_INCLUDED
