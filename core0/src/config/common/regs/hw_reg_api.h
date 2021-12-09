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

#ifndef HW_REG_API_H
#define HW_REG_API_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

//#define MSVC_CHECK_REG
#ifdef __GNUC__
#define SOC_READ_REG(addr)         ((volatile uint32_t) * ((volatile uint32_t *)(addr)))
#define SOC_WRITE_REG(addr, value) (*((volatile uint32_t *)(addr)) = (value))
#else
#ifdef MSVC_CHECK_REG
#define SOC_READ_REG(addr)         printf("r 0x%x\n", (addr)) ? 0 : 0
#define SOC_WRITE_REG(addr, value) printf("w 0x%x = 0x%x\n", (addr), (value))
#else
#define SOC_READ_REG(addr)         (0)
#define SOC_WRITE_REG(addr, value) (void)(0)
#endif
#endif

/*lint -emacro(506, SETMASK) width 'Constant value Boolean'*/
#define SETMASK(width, shift) ((width ? ((0xFFFFFFFF) >> (32 - width)) : 0) << (shift))
#define CLRMASK(width, shift) (~(SETMASK(width, shift)))

#define GET_BITS(reg, shift, width)      (((reg)&SETMASK(width, shift)) >> (shift))
#define SET_BITS(reg, shift, width, val) (((reg)&CLRMASK(width, shift)) | (val << (shift)))

#define REG_FIELD_GET(name, dword) ((dword & name##_MASK) >> name##_OFFSET)
#define REG_FIELD_SET(name, dword, value)                      \
    do {                                                       \
        (dword) &= ~name##_MASK;                               \
        (dword) |= (((value) << name##_OFFSET) & name##_MASK); \
    } while (0)

#define __REG_N_FIELD__(...) __REG_N_FIELD_(__VA_ARGS__, __FIELD_RSEQ_N())
#define __REG_N_FIELD_(...)  __REG_FIELD_N(__VA_ARGS__)
#define __REG_FIELD_N(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, \
                      _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, \
                      _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, \
                      _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, _61, \
                      _62, _63, _64, N, ...)                                                     \
    N
#define __FIELD_RSEQ_N()                                                                        \
    32, 32, 31, 31, 30, 30, 29, 29, 28, 28, 27, 27, 26, 26, 25, 25, 24, 24, 23, 23, 22, 22, 21, \
        21, 20, 20, 19, 19, 18, 18, 17, 17, 16, 16, 15, 15, 14, 14, 13, 13, 12, 12, 11, 11, 10, \
        10, 9, 9, 8, 8, 7, 7, 6, 6, 5, 5, 4, 4, 3, 3, 2, 2, 1, 1, 0, 0,

#define CONCATENATE(arg1, arg2) arg1##arg2

#define WR_FIELD_1(r, field, val) (r)._b.field = (val)
#define WR_FIELD_2(r, f, val, ...)  \
    {                               \
        WR_FIELD_1(r, f, val);      \
        WR_FIELD_1(r, __VA_ARGS__); \
    }
#define WR_FIELD_3(r, f, val, ...)  \
    {                               \
        WR_FIELD_1(r, f, val);      \
        WR_FIELD_2(r, __VA_ARGS__); \
    }
#define WR_FIELD_4(r, f, val, ...)  \
    {                               \
        WR_FIELD_1(r, f, val);      \
        WR_FIELD_3(r, __VA_ARGS__); \
    }
#define WR_FIELD_5(r, f, val, ...)  \
    {                               \
        WR_FIELD_1(r, f, val);      \
        WR_FIELD_4(r, __VA_ARGS__); \
    }
#define WR_FIELD_6(r, f, val, ...)  \
    {                               \
        WR_FIELD_1(r, f, val);      \
        WR_FIELD_5(r, __VA_ARGS__); \
    }
#define WR_FIELD_7(r, f, val, ...)  \
    {                               \
        WR_FIELD_1(r, f, val);      \
        WR_FIELD_6(r, __VA_ARGS__); \
    }
#define WR_FIELD_8(r, f, val, ...)  \
    {                               \
        WR_FIELD_1(r, f, val);      \
        WR_FIELD_7(r, __VA_ARGS__); \
    }
#define WR_FIELD_9(r, f, val, ...)  \
    {                               \
        WR_FIELD_1(r, f, val);      \
        WR_FIELD_8(r, __VA_ARGS__); \
    }
#define WR_FIELD_10(r, f, val, ...) \
    {                               \
        WR_FIELD_1(r, f, val);      \
        WR_FIELD_9(r, __VA_ARGS__); \
    }
#define WR_FIELD_11(r, f, val, ...)  \
    {                                \
        WR_FIELD_1(r, f, val);       \
        WR_FIELD_10(r, __VA_ARGS__); \
    }
#define WR_FIELD_12(r, f, val, ...)  \
    {                                \
        WR_FIELD_1(r, f, val);       \
        WR_FIELD_11(r, __VA_ARGS__); \
    }
#define WR_FIELD_13(r, f, val, ...)  \
    {                                \
        WR_FIELD_1(r, f, val);       \
        WR_FIELD_12(r, __VA_ARGS__); \
    }
#define WR_FIELD_14(r, f, val, ...)  \
    {                                \
        WR_FIELD_1(r, f, val);       \
        WR_FIELD_13(r, __VA_ARGS__); \
    }
#define WR_FIELD_15(r, f, val, ...)  \
    {                                \
        WR_FIELD_1(r, f, val);       \
        WR_FIELD_14(r, __VA_ARGS__); \
    }
#define WR_FIELD_16(r, f, val, ...)  \
    {                                \
        WR_FIELD_1(r, f, val);       \
        WR_FIELD_15(r, __VA_ARGS__); \
    }
#define WR_FIELD_17(r, f, val, ...)  \
    {                                \
        WR_FIELD_1(r, f, val);       \
        WR_FIELD_16(r, __VA_ARGS__); \
    }
#define WR_FIELD_18(r, f, val, ...)  \
    {                                \
        WR_FIELD_1(r, f, val);       \
        WR_FIELD_17(r, __VA_ARGS__); \
    }
#define WR_FIELD_19(r, f, val, ...)  \
    {                                \
        WR_FIELD_1(r, f, val);       \
        WR_FIELD_18(r, __VA_ARGS__); \
    }
#define WR_FIELD_20(r, f, val, ...)  \
    {                                \
        WR_FIELD_1(r, f, val);       \
        WR_FIELD_19(r, __VA_ARGS__); \
    }
#define WR_FIELD_21(r, f, val, ...)  \
    {                                \
        WR_FIELD_1(r, f, val);       \
        WR_FIELD_20(r, __VA_ARGS__); \
    }
#define WR_FIELD_22(r, f, val, ...)  \
    {                                \
        WR_FIELD_1(r, f, val);       \
        WR_FIELD_21(r, __VA_ARGS__); \
    }
#define WR_FIELD_23(r, f, val, ...)  \
    {                                \
        WR_FIELD_1(r, f, val);       \
        WR_FIELD_22(r, __VA_ARGS__); \
    }
#define WR_FIELD_24(r, f, val, ...)  \
    {                                \
        WR_FIELD_1(r, f, val);       \
        WR_FIELD_23(r, __VA_ARGS__); \
    }
#define WR_FIELD_25(r, f, val, ...)  \
    {                                \
        WR_FIELD_1(r, f, val);       \
        WR_FIELD_24(r, __VA_ARGS__); \
    }
#define WR_FIELD_26(r, f, val, ...)  \
    {                                \
        WR_FIELD_1(r, f, val);       \
        WR_FIELD_25(r, __VA_ARGS__); \
    }
#define WR_FIELD_27(r, f, val, ...)  \
    {                                \
        WR_FIELD_1(r, f, val);       \
        WR_FIELD_26(r, __VA_ARGS__); \
    }
#define WR_FIELD_28(r, f, val, ...)  \
    {                                \
        WR_FIELD_1(r, f, val);       \
        WR_FIELD_27(r, __VA_ARGS__); \
    }
#define WR_FIELD_29(r, f, val, ...)  \
    {                                \
        WR_FIELD_1(r, f, val);       \
        WR_FIELD_28(r, __VA_ARGS__); \
    }
#define WR_FIELD_30(r, f, val, ...)  \
    {                                \
        WR_FIELD_1(r, f, val);       \
        WR_FIELD_29(r, __VA_ARGS__); \
    }
#define WR_FIELD_31(r, f, val, ...)  \
    {                                \
        WR_FIELD_1(r, f, val);       \
        WR_FIELD_30(r, __VA_ARGS__); \
    }
#define WR_FIELD_32(r, f, val, ...)  \
    {                                \
        WR_FIELD_1(r, f, val);       \
        WR_FIELD_31(r, __VA_ARGS__); \
    }

#ifdef _lint
// Lint cannot correctly recognize typeof(*xxx), replace the macro to make lint happy
#define WR_REG_FIELD_(N, reg, ...) ({ CONCATENATE(WR_FIELD_, N)(reg, __VA_ARGS__); })

#define WR_REG_FIELD(reg, ...) WR_REG_FIELD_(__REG_N_FIELD__(reg, __VA_ARGS__), reg, __VA_ARGS__)
#define WR_REG(reg, value)     (reg.w = (value))
#define SET_REG_BIT(reg, n)    ({ WR_REG(reg, (RE_REG(reg) | BIT(n))); })
#define CLR_REG_BIT(reg, n)    ({ WR_REG(reg, (RE_REG(reg) & ~BIT(n))); })

#define RE_REG_FIELD(reg, field) ({ (uint32_t)(reg)._b.field; })
#define RE_REG(reg)              (reg.w)
#else
#define WR_REG_FIELD_(N, reg, ...)                             \
    ({                                                         \
        typeof(reg) _reg_x_ = {.w = SOC_READ_REG(&((reg).w))}; \
        CONCATENATE(WR_FIELD_, N)(_reg_x_, __VA_ARGS__);       \
        SOC_WRITE_REG(&((reg).w), _reg_x_.w);                  \
    })

#define WR_REG(reg, value)     SOC_WRITE_REG(&((reg).w), (value));

/**
 * @brief Set a bit to a reg
 *
 * @param reg Standard union reg type
 * @param n   The bit to be set, 0 ~ 31
 */
#define SET_REG_BIT(reg, n)    ({ WR_REG(reg, RE_REG(reg) | BIT(n)); })

/**
 * @brief Clear a bit to a reg
 *
 * @param reg Standard union reg type
 * @param n   The bit to be clear, 0 ~ 31
 */
#define CLR_REG_BIT(reg, n)    ({ WR_REG(reg, RE_REG(reg) & ~BIT(n)); })

/**
 * WR_REG_FIELD(reg, field1, val1, field2, val2, field3, val3, ...)
 */
#define WR_REG_FIELD(reg, ...) WR_REG_FIELD_(__REG_N_FIELD__(reg, __VA_ARGS__), reg, __VA_ARGS__)

#define RE_REG_FIELD(reg, field)                           \
    ({                                                     \
        typeof(reg) __x = {.w = SOC_READ_REG(&((reg).w))}; \
        (uint32_t) __x._b.field;                           \
    })

#define RE_REG(reg) SOC_READ_REG(&((reg).w))

#endif

#ifdef __cplusplus
}
#endif

#endif   // !HW_REG_API_H
