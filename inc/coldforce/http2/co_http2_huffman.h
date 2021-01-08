#ifndef CO_HTTP2_HUFFMAN_H_INCLUDED
#define CO_HTTP2_HUFFMAN_H_INCLUDED

#include <coldforce/http2/co_http2.h>

CO_EXTERN_C_BEGIN

//---------------------------------------------------------------------------//
// http2 huffman encoding
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

void co_http2_huffman_encode(
    const char* str, size_t str_length, uint8_t** dest, size_t* dest_length);

bool co_http2_huffman_decode(
    const uint8_t* src, size_t src_length, char** dest, size_t* dest_length);

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_EXTERN_C_END

#endif // CO_HTTP2_HUFFMAN_H_INCLUDED
