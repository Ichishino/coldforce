#include <coldforce/core/co_std.h>

#include <coldforce/http/co_base64.h>

//---------------------------------------------------------------------------//
// base64
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
// private
//---------------------------------------------------------------------------//

#define co_ceil(n) \
    ((double)((long)((((double)n) < 0.0) ? n : (((double)n) + 0.9))))

static void
co_base64_common_encode(
    const char* encoding_table,
    const uint8_t* src,
    size_t src_length,
    char** dest,
    size_t* dest_length,
    bool padding
)
{
    (*dest_length) = (size_t)(co_ceil(((double)src_length) / 3) * 4);
    (*dest) = co_mem_alloc((*dest_length) + 1);

    size_t dest_index = 0;
    size_t max_count = (*dest_length) / 4;

    for (size_t index = 0; index < max_count; ++index)
    {
        size_t src_index = index * 3;

        const unsigned char orig[3] =
        {
            (src_index < src_length) ? src[src_index] : (unsigned char)0,
            ((src_index + 1) < src_length) ? src[src_index + 1] : (unsigned char)0,
            ((src_index + 2) < src_length) ? src[src_index + 2] : (unsigned char)0
        };

        (*dest)[dest_index++] =
            encoding_table[((orig[0] & 0xFC) >> 2)];
        (*dest)[dest_index++] =
            encoding_table[((orig[0] & 0x03) << 4) | ((orig[1] & 0xF0) >> 4)];
        (*dest)[dest_index++] =
            encoding_table[((orig[1] & 0x0F) << 2) | ((orig[2] & 0xC0) >> 6)];
        (*dest)[dest_index++] =
            encoding_table[(orig[2] & 0x3F)];
    }

    dest_index -= (3 - src_length % 3) % 3;

    if (padding)
    {
        while (dest_index < (*dest_length))
        {
            (*dest)[dest_index++] = '=';
        }
    }

    (*dest)[dest_index] = '\0';
    (*dest_length) = dest_index;
}

//---------------------------------------------------------------------------//
// public
//---------------------------------------------------------------------------//

void
co_base64_encode(
    const void* src,
    size_t src_length,
    char** dest,
    size_t* dest_length,
    bool padding
)
{
    const char* encoding_table =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789"
        "+/";

    co_base64_common_encode(
        encoding_table, (uint8_t*)src, src_length, dest, dest_length, padding);
}

bool
co_base64_decode(
    const char* src,
    size_t src_length,
    uint8_t** dest,
    size_t* dest_length
)
{
    const uint8_t decoding_table[256] =
    {
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0x80, 0xff, 0xff, 0x80, 0xff, 0xff, // \n, \r
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff,   62, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, // !
        0xff, 0xff, 0xff,   62, 0xff,   62, 0xff,   63, // +, -, /
          52,   53,   54,   55,   56,   57,   58,   59, // 0-7
          60,   61,   63, 0xff, 0xff, 0x80, 0xff, 0xff, // 8, 9, :, =
        0xff,    0,    1,    2,    3,    4,    5,    6, // A-G
           7,    8,    9,   10,   11,   12,   13,   14, // H-O
          15,   16,   17,   18,   19,   20,   21,   22, // P-W
          23,   24,   25, 0xff, 0xff, 0xff, 0xff,   63, // X-Z, _
        0xff,   26,   27,   28,   29,   30,   31,   32, // a-g
          33,   34,   35,   36,   37,   38,   39,   40, // h-s
          41,   42,   43,   44,   45,   46,   47,   48, // p-w
          49,   50,   51, 0xff, 0xff, 0xff, 0xff, 0xff  // x-z
    };

    (*dest_length) = (size_t)(((double)src_length / 4) * 3);
    (*dest) = co_mem_alloc((*dest_length) + 1);

    unsigned char b64[4];
    size_t count = 0;
    size_t dest_index = 0;

    for (size_t src_index = 0; src_index < src_length; ++src_index)
    {
        unsigned char b = decoding_table[((uint8_t*)src)[src_index]];

        if (b == 0xff)
        {
            return false;
        }
        else if (b != 0x80)
        {
            b64[count++] = b;

            if (count == 2)
            {
                (*dest)[dest_index++] =
                    ((b64[0] & 0x3f) << 2) | ((b64[1] & 0x30) >> 4);
            }
            else if (count == 3)
            {
                (*dest)[dest_index++] =
                    ((b64[1] & 0x0f) << 4) | ((b64[2] & 0x3c) >> 2);
            }
            else if (count == 4)
            {
                (*dest)[dest_index++] =
                    ((b64[2] & 0x03) << 6) | (b64[3] & 0x3f);

                count = 0;
            }
        }
        else
        {
            --(*dest_length);
        }
    }

    (*dest)[dest_index] = '\0';

    return true;
}

void
co_base64url_encode(
    const void* src,
    size_t src_length,
    char** dest,
    size_t* dest_length,
    bool padding
)
{
    const char* encoding_table =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789"
        "-_";

    co_base64_common_encode(
        encoding_table, (uint8_t*)src, src_length, dest, dest_length, padding);
}

bool
co_base64url_decode(
    const char* src,
    size_t src_length,
    uint8_t** dest,
    size_t* dest_length
)
{
    return co_base64_decode(src, src_length, dest, dest_length);
}
