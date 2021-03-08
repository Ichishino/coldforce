#ifndef CO_STRING_CONV_H_INCLUDED
#define CO_STRING_CONV_H_INCLUDED

#include <coldforce/core/co_string.h>

#include <coldforce/ws/co_ws.h>

CO_EXTERN_C_BEGIN

//---------------------------------------------------------------------------//
// string conv
//---------------------------------------------------------------------------//

CO_WS_API size_t co_string_conv_utf8_to_sjis(
    const char* src, size_t src_length, char* dest_buffer, size_t dest_buffer_size);

CO_WS_API size_t co_string_conv_sjis_to_utf8(
    const char* src, size_t src_length, char* dest_buffer, size_t dest_buffer_size);

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_EXTERN_C_END

#endif // CO_STRING_CONV_H_INCLUDED
