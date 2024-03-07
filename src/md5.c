#include "md5.h"
#include "ds.h"
#include <endian.h>
#include <stdint.h>

uint32_t s[64] = {7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22,
                  5, 9,  14, 20, 5, 9,  14, 20, 5, 9,  14, 20, 5, 9,  14, 20,
                  4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23,
                  6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21};

uint32_t K[64] = {
    0xd76aa478, 0xe8c7b756, 0x242070db, 0xc1bdceee, 0xf57c0faf, 0x4787c62a,
    0xa8304613, 0xfd469501, 0x698098d8, 0x8b44f7af, 0xffff5bb1, 0x895cd7be,
    0x6b901122, 0xfd987193, 0xa679438e, 0x49b40821, 0xf61e2562, 0xc040b340,
    0x265e5a51, 0xe9b6c7aa, 0xd62f105d, 0x02441453, 0xd8a1e681, 0xe7d3fbc8,
    0x21e1cde6, 0xc33707d6, 0xf4d50d87, 0x455a14ed, 0xa9e3e905, 0xfcefa3f8,
    0x676f02d9, 0x8d2a4c8a, 0xfffa3942, 0x8771f681, 0x6d9d6122, 0xfde5380c,
    0xa4beea44, 0x4bdecfa9, 0xf6bb4b60, 0xbebfbc70, 0x289b7ec6, 0xeaa127fa,
    0xd4ef3085, 0x04881d05, 0xd9d4d039, 0xe6db99e5, 0x1fa27cf8, 0xc4ac5665,
    0xf4292244, 0x432aff97, 0xab9423a7, 0xfc93a039, 0x655b59c3, 0x8f0ccc92,
    0xffeff47d, 0x85845dd1, 0x6fa87e4f, 0xfe2ce6e0, 0xa3014314, 0x4e0811a1,
    0xf7537e82, 0xbd3af235, 0x2ad7d2bb, 0xeb86d391};

uint32_t a0 = 0x67452301; // A
uint32_t b0 = 0xefcdab89; // B
uint32_t c0 = 0x98badcfe; // C
uint32_t d0 = 0x10325476; // D

typedef struct message_array {
        uint8_t *items;
        uint32_t count;
        uint32_t capacity;
} message_array;

static int build_message(const char *input, struct message_array *m) {
    int result = 0;

    uint32_t input_length = strlen(input);
    ds_da_append_many(m, input, input_length);

    ds_da_append(m, 0x80);

    while (m->count % 64 != 56) {
        ds_da_append(m, 0x00);
    }

    uint64_t length = input_length * 8;

    for (int i = 0; i < 8; i++) {
        ds_da_append(m, (length >> (i * 8)) & 0xFF);
    }

defer:
    return result;
}

static void md5_hash(struct message_array *m, uint32_t *a, uint32_t *b,
                     uint32_t *c, uint32_t *d) {
    // Process the message in successive 512-bit chunks:
    for (uint32_t j = 0; j < m->count; j += 64) {
        // chunk into sixteen 32-bit words M[j], 0 ≤ j ≤ 15
        uint32_t *M = (uint32_t *)(m->items + j);

        // Initialize hash value for this chunk:
        uint32_t A = *a;
        uint32_t B = *b;
        uint32_t C = *c;
        uint32_t D = *d;

        // Main loop:
        for (int i = 0; i < 64; i++) {
            uint32_t F = 0;
            uint32_t g = 0;

            if (0 <= i && i <= 15) {
                F = (B & C) | ((~B) & D);
                g = i;
            } else if (16 <= i && i <= 31) {
                F = (D & B) | ((~D) & C);
                g = (5 * i + 1) % 16;
            } else if (32 <= i && i <= 47) {
                F = B ^ C ^ D;
                g = (3 * i + 5) % 16;
            } else if (48 <= i && i <= 63) {
                F = C ^ (B | (~D));
                g = (7 * i) % 16;
            }

            F = F + A + K[i] + M[g];
            A = D;
            D = C;
            C = B;
            B = B + ((F << s[i]) | (F >> (32 - s[i])));
        }

        // Add this chunk's hash to result so far:
        *a += A;
        *b += B;
        *c += C;
        *d += D;
    }
}

int md5_hash_digest(const char *input, char *digest) {
    int result = 0;

    struct message_array m = {0};
    if (build_message(input, &m) != 0) {
        return_defer(1);
    }

    uint32_t a = a0;
    uint32_t b = b0;
    uint32_t c = c0;
    uint32_t d = d0;

    md5_hash(&m, &a, &b, &c, &d);

    snprintf(digest, 33, "%08x%08x%08x%08x", htobe32(a), htobe32(b), htobe32(c),
             htobe32(d));

defer:
    if (m.items != NULL) {
        free(m.items);
    }
    return result;
}
