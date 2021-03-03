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

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

#define CO_HTTP_VERSION_1_1                 "HTTP/1.1"
#define CO_HTTP_PROTOCOL                    "http/1.1"

#define CO_HTTP_HEADER_CONTENT_LENGTH       "Content-Length"
#define CO_HTTP_HEADER_HOST                 "Host"
#define CO_HTTP_HEADER_TRANSFER_ENCODING    "Transfer-Encoding"
#define CO_HTTP_HEADER_SET_COOKIE           "Set-Cookie"
#define CO_HTTP_HEADER_COOKIE               "Cookie"
#define CO_HTTP_HEADER_CONNECTION           "Connection"
#define CO_HTTP_HEADER_UPGRADE              "Upgrade"

#define CO_HTTP_TRANSFER_ENCODING_CHUNKED   "chunked"

#define CO_HTTP_UPGRADE_CONNECTION_PREFACE  "cp"

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

#define CO_HTTP_ERROR_CONNECT_FAILED              -5001
#define CO_HTTP_ERROR_TLS_HANDSHAKE_FAILED        -5002
#define CO_HTTP_ERROR_CONNECTION_CLOSED           -5003
#define CO_HTTP_ERROR_OUT_OF_MEMORY               -5004
#define CO_HTTP_ERROR_PARSE_HEADER                -5005
#define CO_HTTP_ERROR_PARSE_CONTENT               -5006
#define CO_HTTP_ERROR_CONTENT_TOO_BIG             -5007
#define CO_HTTP_ERROR_HEADER_LINE_TOO_LONG        -5008
#define CO_HTTP_ERROR_HEADER_FIELDS_TOO_MANY      -5009
#define CO_HTTP_ERROR_CANCEL                      -5010
#define CO_HTTP_ERROR_PROTOCOL_ERROR              -5011

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_EXTERN_C_END

#endif // CO_HTTP_H_INCLUDED
