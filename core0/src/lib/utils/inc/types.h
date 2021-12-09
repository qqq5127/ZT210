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

#ifndef _LIB_UTILS_TYPES_H
#define _LIB_UTILS_TYPES_H

#include "core_def.h"
#include "utils.h"
#include "errno.h"
#include "assert.h"

/* Get bool, true and false.
 * If using GCC then get stdbool, otherwise create definitions here,
 */
#if (defined(__GNUC__) || defined(C99)) && !defined(_lint)
#include <stdbool.h>
typedef bool bool_t;
#elif defined(__cplusplus)
/* bool is already defined in C++ */
#else
typedef _Bool bool_t;
#define true ((bool_t) 1)
#define false ((bool_t) 0)

typedef bool_t bool;
#endif

// Use meaningful names when bool used for success/failure, e.g. for function returns.
#define SUCCESS true
#define FAILURE false

#if defined(__GNUC__) || defined(C99)
#include <stdint.h>
#include <stddef.h>
#else
typedef signed char int8_t;
typedef short       int16_t;
typedef int         int32_t;
typedef long long   int64_t;

typedef unsigned char      uint8_t;
typedef unsigned short     uint16_t;
typedef unsigned int       uint32_t;
typedef unsigned long long uint64_t;

#define NULL ((void*)0)
#endif

#define MAX_UINT8  0xFF
#define MAX_UINT16 0xFFFF
#define MAX_INT32  0x7FFFFFFF
#define MAX_UINT32 0xFFFFFFFF
#define MAX_UINT64 0xFFFFFFFFFFFFFFFFULL
#define MIN_INT16  (-0x8000)
#define MAX_INT16  (0x7FFF)

typedef uint32_t iot_addrword_t;
typedef uint32_t iot_addr_t;

#endif /* _LIB_UTILS_TYPES_H */
