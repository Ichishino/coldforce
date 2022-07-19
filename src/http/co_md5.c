#include <coldforce/core/co_std.h>

#include <coldforce/http/co_md5.h>

//---------------------------------------------------------------------------//
// md5
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
// private
//---------------------------------------------------------------------------//

typedef struct
{
    uint32_t state[4];
    uint32_t count[2];
    uint8_t buffer[64];

} co_md5_context_t;

#define F(x, y, z)		((z) ^ ((x) & ((y) ^ (z))))
#define G(x, y, z)		((y) ^ ((z) & ((x) ^ (y))))
#define H(x, y, z)		(((x) ^ (y)) ^ (z))
#define H2(x, y, z)		((x) ^ ((y) ^ (z)))
#define I(x, y, z)		((y) ^ ((x) | ~(z)))

#define CO_MD5_ROUND(f, a, b, c, d, x, t, s) \
    (a) += f((b), (c), (d)) + (x) + (t); \
    (a) = (((a) << (s)) | (((a) & 0xffffffff) >> (32 - (s)))); \
    (a) += (b);

#ifdef CO_LITTLE_ENDIAN
#define SET(block, ptr, n) \
    (block[(n)] = \
        (uint32_t)ptr[(n) * 4] | \
        ((uint32_t)ptr[(n) * 4 + 1] << 8) | \
        ((uint32_t)ptr[(n) * 4 + 2] << 16) | \
        ((uint32_t)ptr[(n) * 4 + 3] << 24))
#define GET(block, ptr, n) (block[(n)])
#else
#define SET(block, ptr, n) (*(uint32_t*)&ptr[(n) * 4])
#define GET(block, ptr, n) SET(block, ptr, n)
#endif

static const void*
co_md5_transform(
    co_md5_context_t* ctx,
    const uint8_t* buffer,
    uint32_t size
)
{
    uint32_t a = ctx->state[0];
    uint32_t b = ctx->state[1];
    uint32_t c = ctx->state[2];
    uint32_t d = ctx->state[3];

    do
    {
        uint32_t saved_a = a;
        uint32_t saved_b = b;
        uint32_t saved_c = c;
        uint32_t saved_d = d;

        uint32_t block[16];

        CO_MD5_ROUND(F, a, b, c, d, SET(block, buffer, 0), 0xd76aa478, 7);
        CO_MD5_ROUND(F, d, a, b, c, SET(block, buffer, 1), 0xe8c7b756, 12);
        CO_MD5_ROUND(F, c, d, a, b, SET(block, buffer, 2), 0x242070db, 17);
        CO_MD5_ROUND(F, b, c, d, a, SET(block, buffer, 3), 0xc1bdceee, 22);
        CO_MD5_ROUND(F, a, b, c, d, SET(block, buffer, 4), 0xf57c0faf, 7);
        CO_MD5_ROUND(F, d, a, b, c, SET(block, buffer, 5), 0x4787c62a, 12);
        CO_MD5_ROUND(F, c, d, a, b, SET(block, buffer, 6), 0xa8304613, 17);
        CO_MD5_ROUND(F, b, c, d, a, SET(block, buffer, 7), 0xfd469501, 22);
        CO_MD5_ROUND(F, a, b, c, d, SET(block, buffer, 8), 0x698098d8, 7);
        CO_MD5_ROUND(F, d, a, b, c, SET(block, buffer, 9), 0x8b44f7af, 12);
        CO_MD5_ROUND(F, c, d, a, b, SET(block, buffer, 10), 0xffff5bb1, 17);
        CO_MD5_ROUND(F, b, c, d, a, SET(block, buffer, 11), 0x895cd7be, 22);
        CO_MD5_ROUND(F, a, b, c, d, SET(block, buffer, 12), 0x6b901122, 7);
        CO_MD5_ROUND(F, d, a, b, c, SET(block, buffer, 13), 0xfd987193, 12);
        CO_MD5_ROUND(F, c, d, a, b, SET(block, buffer, 14), 0xa679438e, 17);
        CO_MD5_ROUND(F, b, c, d, a, SET(block, buffer, 15), 0x49b40821, 22);

        CO_MD5_ROUND(G, a, b, c, d, GET(block, buffer, 1), 0xf61e2562, 5);
        CO_MD5_ROUND(G, d, a, b, c, GET(block, buffer, 6), 0xc040b340, 9);
        CO_MD5_ROUND(G, c, d, a, b, GET(block, buffer, 11), 0x265e5a51, 14);
        CO_MD5_ROUND(G, b, c, d, a, GET(block, buffer, 0), 0xe9b6c7aa, 20);
        CO_MD5_ROUND(G, a, b, c, d, GET(block, buffer, 5), 0xd62f105d, 5);
        CO_MD5_ROUND(G, d, a, b, c, GET(block, buffer, 10), 0x02441453, 9);
        CO_MD5_ROUND(G, c, d, a, b, GET(block, buffer, 15), 0xd8a1e681, 14);
        CO_MD5_ROUND(G, b, c, d, a, GET(block, buffer, 4), 0xe7d3fbc8, 20);
        CO_MD5_ROUND(G, a, b, c, d, GET(block, buffer, 9), 0x21e1cde6, 5);
        CO_MD5_ROUND(G, d, a, b, c, GET(block, buffer, 14), 0xc33707d6, 9);
        CO_MD5_ROUND(G, c, d, a, b, GET(block, buffer, 3), 0xf4d50d87, 14);
        CO_MD5_ROUND(G, b, c, d, a, GET(block, buffer, 8), 0x455a14ed, 20);
        CO_MD5_ROUND(G, a, b, c, d, GET(block, buffer, 13), 0xa9e3e905, 5);
        CO_MD5_ROUND(G, d, a, b, c, GET(block, buffer, 2), 0xfcefa3f8, 9);
        CO_MD5_ROUND(G, c, d, a, b, GET(block, buffer, 7), 0x676f02d9, 14);
        CO_MD5_ROUND(G, b, c, d, a, GET(block, buffer, 12), 0x8d2a4c8a, 20);

        CO_MD5_ROUND(H, a, b, c, d, GET(block, buffer, 5), 0xfffa3942, 4);
        CO_MD5_ROUND(H2, d, a, b, c, GET(block, buffer, 8), 0x8771f681, 11);
        CO_MD5_ROUND(H, c, d, a, b, GET(block, buffer, 11), 0x6d9d6122, 16);
        CO_MD5_ROUND(H2, b, c, d, a, GET(block, buffer, 14), 0xfde5380c, 23);
        CO_MD5_ROUND(H, a, b, c, d, GET(block, buffer, 1), 0xa4beea44, 4);
        CO_MD5_ROUND(H2, d, a, b, c, GET(block, buffer, 4), 0x4bdecfa9, 11);
        CO_MD5_ROUND(H, c, d, a, b, GET(block, buffer, 7), 0xf6bb4b60, 16);
        CO_MD5_ROUND(H2, b, c, d, a, GET(block, buffer, 10), 0xbebfbc70, 23);
        CO_MD5_ROUND(H, a, b, c, d, GET(block, buffer, 13), 0x289b7ec6, 4);
        CO_MD5_ROUND(H2, d, a, b, c, GET(block, buffer, 0), 0xeaa127fa, 11);
        CO_MD5_ROUND(H, c, d, a, b, GET(block, buffer, 3), 0xd4ef3085, 16);
        CO_MD5_ROUND(H2, b, c, d, a, GET(block, buffer, 6), 0x04881d05, 23);
        CO_MD5_ROUND(H, a, b, c, d, GET(block, buffer, 9), 0xd9d4d039, 4);
        CO_MD5_ROUND(H2, d, a, b, c, GET(block, buffer, 12), 0xe6db99e5, 11);
        CO_MD5_ROUND(H, c, d, a, b, GET(block, buffer, 15), 0x1fa27cf8, 16);
        CO_MD5_ROUND(H2, b, c, d, a, GET(block, buffer, 2), 0xc4ac5665, 23);

        CO_MD5_ROUND(I, a, b, c, d, GET(block, buffer, 0), 0xf4292244, 6);
        CO_MD5_ROUND(I, d, a, b, c, GET(block, buffer, 7), 0x432aff97, 10);
        CO_MD5_ROUND(I, c, d, a, b, GET(block, buffer, 14), 0xab9423a7, 15);
        CO_MD5_ROUND(I, b, c, d, a, GET(block, buffer, 5), 0xfc93a039, 21);
        CO_MD5_ROUND(I, a, b, c, d, GET(block, buffer, 12), 0x655b59c3, 6);
        CO_MD5_ROUND(I, d, a, b, c, GET(block, buffer, 3), 0x8f0ccc92, 10);
        CO_MD5_ROUND(I, c, d, a, b, GET(block, buffer, 10), 0xffeff47d, 15);
        CO_MD5_ROUND(I, b, c, d, a, GET(block, buffer, 1), 0x85845dd1, 21);
        CO_MD5_ROUND(I, a, b, c, d, GET(block, buffer, 8), 0x6fa87e4f, 6);
        CO_MD5_ROUND(I, d, a, b, c, GET(block, buffer, 15), 0xfe2ce6e0, 10);
        CO_MD5_ROUND(I, c, d, a, b, GET(block, buffer, 6), 0xa3014314, 15);
        CO_MD5_ROUND(I, b, c, d, a, GET(block, buffer, 13), 0x4e0811a1, 21);
        CO_MD5_ROUND(I, a, b, c, d, GET(block, buffer, 4), 0xf7537e82, 6);
        CO_MD5_ROUND(I, d, a, b, c, GET(block, buffer, 11), 0xbd3af235, 10);
        CO_MD5_ROUND(I, c, d, a, b, GET(block, buffer, 2), 0x2ad7d2bb, 15);
        CO_MD5_ROUND(I, b, c, d, a, GET(block, buffer, 9), 0xeb86d391, 21);

        a += saved_a;
        b += saved_b;
        c += saved_c;
        d += saved_d;

        buffer += 64;

    } while (size -= 64);

    ctx->state[0] = a;
    ctx->state[1] = b;
    ctx->state[2] = c;
    ctx->state[3] = d;

    return buffer;
}

static void
co_md5_update(
    co_md5_context_t* ctx,
    const uint8_t* data,
    uint32_t data_size
)
{
    uint32_t j = ctx->count[0];

    ctx->count[0] = (j + data_size) & 0x1fffffff;

    if (ctx->count[0] < j)
    {
        ++ctx->count[1];
    }

    ctx->count[1] += data_size >> 29;

    j = j & 0x3f;

    if (j > 0)
    {
        uint32_t i = 64 - j;

        if (data_size < i)
        {
            memcpy(&ctx->buffer[j], data, data_size);

            return;
        }

        memcpy(&ctx->buffer[j], data, i);

        data += i;
        data_size -= i;

        co_md5_transform(ctx, ctx->buffer, 64);
    }

    if (data_size >= 64)
    {
        data = co_md5_transform(
            ctx, data, data_size & ~(uint32_t)0x3f);
        data_size &= 0x3f;
    }

    memcpy(ctx->buffer, data, data_size);
}

static void
co_md5_final(
    co_md5_context_t* ctx,
    uint8_t* hash
)
{
    uint32_t j = ctx->count[0] & 0x3f;

    ctx->buffer[j] = 0x80;
    ++j;

    uint32_t i = 64 - j;

    if (i < 8)
    {
        memset(&ctx->buffer[j], 0x00, i);

        co_md5_transform(ctx, ctx->buffer, 64);

        j = 0;
        i = 64;
    }

    memset(&ctx->buffer[j], 0x00, i - 8);

    ctx->count[0] <<= 3;

    #define COPY_UINT32(dst, src) \
        (dst)[0] = (uint8_t)(src); \
        (dst)[1] = (uint8_t)((src) >> 8); \
        (dst)[2] = (uint8_t)((src) >> 16); \
        (dst)[3] = (uint8_t)((src) >> 24);

    COPY_UINT32(&ctx->buffer[56], ctx->count[0]);
    COPY_UINT32(&ctx->buffer[60], ctx->count[1]);

    co_md5_transform(ctx, ctx->buffer, 64);

    COPY_UINT32(&hash[0], ctx->state[0]);
    COPY_UINT32(&hash[4], ctx->state[1]);
    COPY_UINT32(&hash[8], ctx->state[2]);
    COPY_UINT32(&hash[12], ctx->state[3]);
}

//---------------------------------------------------------------------------//
// public
//---------------------------------------------------------------------------//

void
co_md5(
    const void* data,
    uint32_t data_size,
    uint8_t* hash
)
{
    co_md5_context_t ctx =
    { { 0x67452301, 0xefcdab89, 0x98badcfe, 0x10325476 },
      { 0 },{ 0 } };

    co_md5_update(&ctx, data, data_size);
    co_md5_final(&ctx, hash);
}
