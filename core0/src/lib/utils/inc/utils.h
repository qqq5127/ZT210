/****************************************************************************

Copyright(c) 2020 by WuQi Technologies. ALL RIGHTS RESERVED.

This Information is proprietary to WuQi Technologies and MAY NOT
be copied by any method or incorporated into another program without
the express written consent of WuQi. This Information or any portion
thereof remains the property of WuQi. The Information contained herein
is believed to be accurate and WuQi assumes no responsibility or
liability for its use in any way and conveys no license or title under
any patent or copyright and makes no representation or warranty that this
Information is free from patent or copyright infringement.

****************************************************************************/

#ifndef _LIB_UTILS_UTILS_H
#define _LIB_UTILS_UTILS_H

#define likely(x)   __builtin_expect((x), 1)
#define unlikely(x) __builtin_expect((x), 0)

// Rounding operations (efficient when n is a power of 2)
// Round down to the nearest multiple of n
#ifndef ROUNDDOWN
#define ROUNDDOWN(a, n)               \
    ({                                \
        uint32_t __a = (uint32_t)(a); \
        (typeof(a))(__a - __a % (n)); \
    })
#endif

// Round up to the nearest multiple of n
#ifndef ROUNDUP
#define ROUNDUP(a, n)                                         \
    ({                                                        \
        uint32_t __n = (uint32_t)(n);                         \
        (typeof(a))(ROUNDDOWN((uint32_t)(a) + __n - 1, __n)); \
    })
#endif

// Efficient min and max operations
#ifndef MIN
#define MIN(_a, _b)             \
    ({                          \
        typeof(_a) __a = (_a);  \
        typeof(_b) __b = (_b);  \
        __a <= __b ? __a : __b; \
    })
#endif

#ifndef MAX
#define MAX(_a, _b)             \
    ({                          \
        typeof(_a) __a = (_a);  \
        typeof(_b) __b = (_b);  \
        __a >= __b ? __a : __b; \
    })
#endif

#ifndef CLAMP
#define CLAMP(a, lo, hi)              \
    ({                                \
        typeof(a) __max = MAX(a, lo); \
        MIN(hi, __max);               \
    })
#endif

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))
#endif

#ifndef EXTRACT_FIELD
#define EXTRACT_FIELD(val, which) (((val) & (which)) / ((which) & ~((which)-1)))
#endif

#ifndef INSERT_FIELD
#define INSERT_FIELD(val, which, fieldval) \
    (((val) & ~(which)) | ((fieldval) * ((which) & ~((which)-1))))
#endif

#ifndef STR
#define STR(x) XSTR(x)
#endif
#ifndef XSTR
#define XSTR(x) #x
#endif

#ifndef UNUSED
#define UNUSED(x) (void)x
#endif

#ifndef BIT
//lint -emacro(835, BIT) '<<0'
#define BIT(x) (1u << (x))
#endif

#ifndef GET_BIT_VALUE
#define GET_BIT_VALUE(x, bit) ((x & (1u << bit)) >> bit)
#endif

#ifndef NTOHL
#define NTOHL(x) \
    (((x & 0xff) << 24) | ((x & 0xff00) << 8) | ((x & 0xff0000) >> 8) | ((x & 0xff000000) >> 24))
#endif

#ifndef NTOHS
#define NTOHS(x) (((x & 0xff) << 8) | ((x & 0xff00) >> 8))
#endif

#ifndef HTONL
#define HTONL NTOHL
#endif

#ifndef HTONS
#define HTONS NTOHS
#endif

/// endian int32/uint32 reverse uint8 point
#ifndef END32_REVS_P8
#define END32_REVS_P8(p8_o, p8_i) \
    do {                          \
        (p8_o)[0] = (p8_i)[3];    \
        (p8_o)[1] = (p8_i)[2];    \
        (p8_o)[2] = (p8_i)[1];    \
        (p8_o)[3] = (p8_i)[0];    \
    } while (0)
#endif

#ifndef END16_REVS_P8
// convert big end to little end
#define END16_REVS_P8(dst, src) \
    do {                        \
        (dst)[0] = (src)[1];    \
        (dst)[1] = (src)[0];    \
    } while (0)

#endif

/**
 * @brief Swap bytes of an array of bytes
 *
 * @param[out] out The output value.
 * @param[in]  in  The input value.
 *
 * @param[in] length      number of bytes to swap
 */
#define bswap(out, in, length)                                            \
    {                                                                     \
        uint8_t *p_out = (void *)out;                                     \
        const uint8_t *p_in = (void *)in;                                 \
        uint16_t len = length;                                            \
        uint16_t diff = (p_in > p_out) ? (p_in - p_out) : (p_out - p_in); \
        assert(diff > length);                                            \
        while (len > 0) {                                                 \
            len--;                                                        \
            *p_out = p_in[len];                                           \
            p_out++;                                                      \
        }                                                                 \
    }

#if (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
//Convert host to network.
#define HTON(out, in, length) bswap(out, in, length)
//Convert network to host.
#define NTOH(out, in, length) bswap(out, in, length)
#else
//Convert host to network.
#define HTON(out, in, length)
//Convert network to host.
#define NTOH(out, in, length)
#endif

/// Macro to get a structure from one of its structure field
/*lint -emacro(826, CONTAINER_OF) Suspicious pointer-to-pointer conversion (area too small) */
#define CONTAINER_OF(ptr, type, member) ((type *)( (char *)ptr - offsetof(type,member) ))

#ifdef USE_IRAM_TEXT
#ifndef IRAM_ATTR
#define IRAM_ATTR __attribute__((section(".iram")))
#endif
#define IRAM_TEXT(x)   __attribute__((section(".iram_text." #x)))
#define IRAM_RODATA(x) __attribute__((section(".iram_rodata." #x)))
#else
#ifndef IRAM_ATTR
#define IRAM_ATTR
#endif
#define IRAM_TEXT(x)
#define IRAM_RODATA(x)
#endif

#ifdef BUILD_ROM_LIB
#ifndef ICACHE_ATTR
#define ICACHE_ATTR
#endif
#else
#ifndef ICACHE_ATTR
#define ICACHE_ATTR __attribute__((section(".icache_text")))
#endif
#endif

#endif /* _LIB_UTILS_UTILS_H */
