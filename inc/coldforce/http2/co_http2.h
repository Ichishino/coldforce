#ifndef CO_HTTP2_H_INCLUDED
#define CO_HTTP2_H_INCLUDED

#include <coldforce/core/co.h>

//---------------------------------------------------------------------------//
// platform
//---------------------------------------------------------------------------//

#ifdef _MSC_VER
#   ifdef CO_HTTP2_EXPORTS
#       define CO_HTTP2_API  __declspec(dllexport)
#   else
#       define CO_HTTP2_API
#   endif
#else
#   define CO_HTTP2_API
#endif

CO_EXTERN_C_BEGIN

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

#define CO_HTTP2_ERROR_STREAM_CLOSED        -6000
#define CO_HTTP2_ERROR_FILE_IO              -6101
#define CO_HTTP2_ERROR_PARSE_ERROR          -6102
#define CO_HTTP2_ERROR_MAX_STREAMS          -6103
#define CO_HTTP2_ERROR_UPGRADE_FAILED       -6104

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

#define CO_HTTP2_PROTOCOL            "h2"
#define CO_HTTP2_UPGRADE             "h2c"

#define CO_HTTP2_CONNECTION_PREFACE  "PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n"
#define CO_HTTP2_CONNECTION_PREFACE_LENGTH  24

#define CO_HTTP2_HEADER_SETTINGS     "HTTP2-Settings"

#define CO_HTTP2_HEADER_SET_COOKIE    "set-cookie"
#define CO_HTTP2_HEADER_COOKIE        "cookie"

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

#define CO_HTTP2_SETTING_DEFAULT_HEADER_TABLE_SIZE          4096
#define CO_HTTP2_SETTING_DEFAULT_ENABLE_PUSH                1
#define CO_HTTP2_SETTING_DEFAULT_MAX_CONCURRENT_STREAMS     UINT32_MAX
#define CO_HTTP2_SETTING_DEFAULT_INITIAL_WINDOW_SIZE        65535
#define CO_HTTP2_SETTING_DEFAULT_MAX_FRAME_SIZE             16384
#define CO_HTTP2_SETTING_DEFAULT_MAX_HEADER_LIST_SIZE       UINT32_MAX

#define CO_HTTP2_SETTING_MIN_MAX_FRAME_SIZE                 16384
#define CO_HTTP2_SETTING_MAX_MAX_FRAME_SIZE                 16777215

#define CO_HTTP2_SETTING_MAX_WINDOW_SIZE                    INT32_MAX

// identifier
#define CO_HTTP2_SETTING_ID_HEADER_TABLE_SIZE         1
#define CO_HTTP2_SETTING_ID_ENABLE_PUSH               2
#define CO_HTTP2_SETTING_ID_MAX_CONCURRENT_STREAMS    3
#define CO_HTTP2_SETTING_ID_INITIAL_WINDOW_SIZE       4
#define CO_HTTP2_SETTING_ID_MAX_FRAME_SIZE            5
#define CO_HTTP2_SETTING_ID_MAX_HEADER_LIST_SIZE      6

typedef struct
{
    uint16_t id;
    uint32_t value;

} co_http2_setting_param_st;

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_EXTERN_C_END

#endif // CO_HTTP2_H_INCLUDED
