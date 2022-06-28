#include <coldforce/core/co_std.h>

#include <coldforce/http/co_sha1.h>

//---------------------------------------------------------------------------//
// sha1
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
// private
//---------------------------------------------------------------------------//

typedef struct
{
    uint32_t state[5];
    uint32_t count[2];
    uint8_t buffer[64];

} co_sha1_context_t;

#define CO_SHA1_ROL(v, b) \
    (((v) << (b)) | ((v) >> (32 - (b))))

#ifdef CO_LITTLE_ENDIAN
#define CO_SHA1_BLOCK_0(b, i) \
    (b.l[i] = \
        (CO_SHA1_ROL(b.l[i], 24) & 0xff00ff00) | \
        (CO_SHA1_ROL(b.l[i], 8) & 0x00ff00ff))
#else
#define CO_SHA1_BLOCK_0(b, i) b.l[i]
#endif

#define CO_SHA1_BLOCK(b, i) \
    (b.l[i & 15] = \
        CO_SHA1_ROL(b.l[(i + 13) & 15] ^ b.l[(i + 8) & 15] ^ \
        b.l[(i + 2) & 15] ^ b.l[i & 15], 1))

#define CO_SHA1_ROUND_0(b, v, w, x, y, z, i) \
    z += ((w & (x^y))^y) + CO_SHA1_BLOCK_0(b, i) + \
        0x5a827999 + CO_SHA1_ROL(v, 5); \
    w = CO_SHA1_ROL(w, 30);
#define CO_SHA1_ROUND_1(b, v, w, x, y, z, i) \
    z += ((w & (x^y))^y) + CO_SHA1_BLOCK(b, i) + \
        0x5a827999 + CO_SHA1_ROL(v, 5); \
    w = CO_SHA1_ROL(w, 30);
#define CO_SHA1_ROUND_2(b, v, w, x, y, z, i) \
    z += (w^x^y) + CO_SHA1_BLOCK(b, i) + \
        0x6ed9eba1 + CO_SHA1_ROL(v, 5); \
    w = CO_SHA1_ROL(w, 30);
#define CO_SHA1_ROUND_3(b, v, w, x, y, z, i) \
    z += (((w | x) & y) | (w & x)) + CO_SHA1_BLOCK(b, i) + \
        0x8f1bbcdc + CO_SHA1_ROL(v, 5); \
    w = CO_SHA1_ROL(w, 30);
#define CO_SHA1_ROUND_4(b, v, w, x, y, z, i) \
    z += (w^x^y) + CO_SHA1_BLOCK(b, i) + \
        0xca62c1d6 + CO_SHA1_ROL(v, 5); \
    w = CO_SHA1_ROL(w, 30);

static void
co_sha1_transform(
    co_sha1_context_t* ctx,
    const uint8_t* buffer
)
{
    union
    {
        uint8_t c[64];
        uint32_t l[16];

    } block;

    memcpy(&block, buffer, 64);

    uint32_t a = ctx->state[0];
    uint32_t b = ctx->state[1];
    uint32_t c = ctx->state[2];
    uint32_t d = ctx->state[3];
    uint32_t e = ctx->state[4];

    CO_SHA1_ROUND_0(block, a, b, c, d, e, 0);
    CO_SHA1_ROUND_0(block, e, a, b, c, d, 1);
    CO_SHA1_ROUND_0(block, d, e, a, b, c, 2);
    CO_SHA1_ROUND_0(block, c, d, e, a, b, 3);
    CO_SHA1_ROUND_0(block, b, c, d, e, a, 4);
    CO_SHA1_ROUND_0(block, a, b, c, d, e, 5);
    CO_SHA1_ROUND_0(block, e, a, b, c, d, 6);
    CO_SHA1_ROUND_0(block, d, e, a, b, c, 7);
    CO_SHA1_ROUND_0(block, c, d, e, a, b, 8);
    CO_SHA1_ROUND_0(block, b, c, d, e, a, 9);
    CO_SHA1_ROUND_0(block, a, b, c, d, e, 10);
    CO_SHA1_ROUND_0(block, e, a, b, c, d, 11);
    CO_SHA1_ROUND_0(block, d, e, a, b, c, 12);
    CO_SHA1_ROUND_0(block, c, d, e, a, b, 13);
    CO_SHA1_ROUND_0(block, b, c, d, e, a, 14);
    CO_SHA1_ROUND_0(block, a, b, c, d, e, 15);

    CO_SHA1_ROUND_1(block, e, a, b, c, d, 16);
    CO_SHA1_ROUND_1(block, d, e, a, b, c, 17);
    CO_SHA1_ROUND_1(block, c, d, e, a, b, 18);
    CO_SHA1_ROUND_1(block, b, c, d, e, a, 19);

    CO_SHA1_ROUND_2(block, a, b, c, d, e, 20);
    CO_SHA1_ROUND_2(block, e, a, b, c, d, 21);
    CO_SHA1_ROUND_2(block, d, e, a, b, c, 22);
    CO_SHA1_ROUND_2(block, c, d, e, a, b, 23);
    CO_SHA1_ROUND_2(block, b, c, d, e, a, 24);
    CO_SHA1_ROUND_2(block, a, b, c, d, e, 25);
    CO_SHA1_ROUND_2(block, e, a, b, c, d, 26);
    CO_SHA1_ROUND_2(block, d, e, a, b, c, 27);
    CO_SHA1_ROUND_2(block, c, d, e, a, b, 28);
    CO_SHA1_ROUND_2(block, b, c, d, e, a, 29);
    CO_SHA1_ROUND_2(block, a, b, c, d, e, 30);
    CO_SHA1_ROUND_2(block, e, a, b, c, d, 31);
    CO_SHA1_ROUND_2(block, d, e, a, b, c, 32);
    CO_SHA1_ROUND_2(block, c, d, e, a, b, 33);
    CO_SHA1_ROUND_2(block, b, c, d, e, a, 34);
    CO_SHA1_ROUND_2(block, a, b, c, d, e, 35);
    CO_SHA1_ROUND_2(block, e, a, b, c, d, 36);
    CO_SHA1_ROUND_2(block, d, e, a, b, c, 37);
    CO_SHA1_ROUND_2(block, c, d, e, a, b, 38);
    CO_SHA1_ROUND_2(block, b, c, d, e, a, 39);

    CO_SHA1_ROUND_3(block, a, b, c, d, e, 40);
    CO_SHA1_ROUND_3(block, e, a, b, c, d, 41);
    CO_SHA1_ROUND_3(block, d, e, a, b, c, 42);
    CO_SHA1_ROUND_3(block, c, d, e, a, b, 43);
    CO_SHA1_ROUND_3(block, b, c, d, e, a, 44);
    CO_SHA1_ROUND_3(block, a, b, c, d, e, 45);
    CO_SHA1_ROUND_3(block, e, a, b, c, d, 46);
    CO_SHA1_ROUND_3(block, d, e, a, b, c, 47);
    CO_SHA1_ROUND_3(block, c, d, e, a, b, 48);
    CO_SHA1_ROUND_3(block, b, c, d, e, a, 49);
    CO_SHA1_ROUND_3(block, a, b, c, d, e, 50);
    CO_SHA1_ROUND_3(block, e, a, b, c, d, 51);
    CO_SHA1_ROUND_3(block, d, e, a, b, c, 52);
    CO_SHA1_ROUND_3(block, c, d, e, a, b, 53);
    CO_SHA1_ROUND_3(block, b, c, d, e, a, 54);
    CO_SHA1_ROUND_3(block, a, b, c, d, e, 55);
    CO_SHA1_ROUND_3(block, e, a, b, c, d, 56);
    CO_SHA1_ROUND_3(block, d, e, a, b, c, 57);
    CO_SHA1_ROUND_3(block, c, d, e, a, b, 58);
    CO_SHA1_ROUND_3(block, b, c, d, e, a, 59);

    CO_SHA1_ROUND_4(block, a, b, c, d, e, 60);
    CO_SHA1_ROUND_4(block, e, a, b, c, d, 61);
    CO_SHA1_ROUND_4(block, d, e, a, b, c, 62);
    CO_SHA1_ROUND_4(block, c, d, e, a, b, 63);
    CO_SHA1_ROUND_4(block, b, c, d, e, a, 64);
    CO_SHA1_ROUND_4(block, a, b, c, d, e, 65);
    CO_SHA1_ROUND_4(block, e, a, b, c, d, 66);
    CO_SHA1_ROUND_4(block, d, e, a, b, c, 67);
    CO_SHA1_ROUND_4(block, c, d, e, a, b, 68);
    CO_SHA1_ROUND_4(block, b, c, d, e, a, 69);
    CO_SHA1_ROUND_4(block, a, b, c, d, e, 70);
    CO_SHA1_ROUND_4(block, e, a, b, c, d, 71);
    CO_SHA1_ROUND_4(block, d, e, a, b, c, 72);
    CO_SHA1_ROUND_4(block, c, d, e, a, b, 73);
    CO_SHA1_ROUND_4(block, b, c, d, e, a, 74);
    CO_SHA1_ROUND_4(block, a, b, c, d, e, 75);
    CO_SHA1_ROUND_4(block, e, a, b, c, d, 76);
    CO_SHA1_ROUND_4(block, d, e, a, b, c, 77);
    CO_SHA1_ROUND_4(block, c, d, e, a, b, 78);
    CO_SHA1_ROUND_4(block, b, c, d, e, a, 79);

    ctx->state[0] += a;
    ctx->state[1] += b;
    ctx->state[2] += c;
    ctx->state[3] += d;
    ctx->state[4] += e;
}

static void
co_sha1_update(
    co_sha1_context_t* ctx,
    const uint8_t* data,
    uint32_t data_size
)
{
    uint32_t j = ctx->count[0];

    ctx->count[0] += (data_size << 3);

    if (ctx->count[0] < j)
    {
        ++ctx->count[1];
    }

    ctx->count[1] += (data_size >> 29);

    j = (j >> 3) & 63;

    uint32_t i = 0;

    if ((j + data_size) > 63)
    {
        i = 64 - j;

        memcpy(&ctx->buffer[j], data, i);

        co_sha1_transform(ctx, ctx->buffer);

        for (; i + 63 < data_size; i += 64)
        {
            co_sha1_transform(ctx, &data[i]);
        }

        j = 0;
    }

    memcpy(&ctx->buffer[j], &data[i], data_size - i);
}

static void
co_sha1_final(
    co_sha1_context_t* ctx,
    uint8_t* hash
)
{
    uint8_t fcount[8];

    for (uint32_t i = 0; i < 8; ++i)
    {
        fcount[i] =
            (uint8_t)((ctx->count[((i >= 4) ? 0 : 1)]
                >> ((3 - (i & 3)) * 8)) & 0xff);
    }

    uint8_t c = 0x80;

    co_sha1_update(ctx, &c, 1);

    while ((ctx->count[0] & 0x000001f8) != 0x000001c0)
    {
        c = 0x00;
        co_sha1_update(ctx, &c, 1);
    }

    co_sha1_update(ctx, fcount, 8);

    for (uint32_t i = 0; i < CO_SHA1_HASH_SIZE; ++i)
    {
        hash[i] =
            (uint8_t)((ctx->state[i >> 2] >>
                ((3 - (i & 3)) * 8)) & 0xff);
    }
}

//---------------------------------------------------------------------------//
// public
//---------------------------------------------------------------------------//

void
co_sha1(
    const void* data,
    uint32_t data_size,
    uint8_t* hash
)
{
    co_sha1_context_t ctx =
        { { 0x67452301, 0xefcdab89, 0x98badcfe, 0x10325476, 0xc3d2e1f0  },
          { 0 },{ 0 } };

    co_sha1_update(&ctx, data, data_size);
    co_sha1_final(&ctx, hash);
}
