/****************************************************************************

Copyright(c) 2021 by WuQi Technologies. ALL RIGHTS RESERVED.

This Information is proprietary to WuQi Technologies and MAY NOT
be copied by any method or incorporated into another program without
the express written consent of WuQi. This Information or any portion
thereof remains the property of WuQi. The Information contained herein
is believed to be accurate and WuQi assumes no responsibility or
liability for its use in any way and conveys no license or title under
any patent or copyright and makes no representation or warranty that this
Information is free from patent or copyright infringement.

****************************************************************************/
#ifndef __DFS_H__
#define __DFS_H__

#include "assert.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DFS_TIMEOUT_MS_MAX          0X7FFFFFFFUL

/* for dfs_start/dfs_stop 1st argument
 * Warning: don't modifiy the DFS_OBJ_xxx_START value, which looks
 * like 32, 64, 96, 128.
 */
typedef enum {
    //put shared and dsp as first to reduce modification counter

    //fixed frequency
    DFS_OBJ_SHARED_FIXED_START = 0,

    DFS_OBJ_SHARED_FIXED_16_16_16M = DFS_OBJ_SHARED_FIXED_START,
    DFS_OBJ_SHARED_FIXED_32_32_16M,
    DFS_OBJ_SHARED_FIXED_48_48_16M,
    DFS_OBJ_SHARED_FIXED_64_64_16M,
    DFS_OBJ_SHARED_FIXED_80_80_16M,

    DFS_OBJ_SHARED_FIXED_16_16_32M,
    DFS_OBJ_SHARED_FIXED_32_32_32M,
    DFS_OBJ_SHARED_FIXED_64_64_32M,

    DFS_OBJ_SHARED_FIXED_16_16_48M,
    DFS_OBJ_SHARED_FIXED_48_48_48M,

    DFS_OBJ_SHARED_FIXED_16_16_64M,
    DFS_OBJ_SHARED_FIXED_32_32_64M,
    DFS_OBJ_SHARED_FIXED_64_64_64M,

    DFS_OBJ_SHARED_FIXED_16_16_80M,
    DFS_OBJ_SHARED_FIXED_80_80_80M,

    DFS_OBJ_SHARED_FIXED_16_16_96M,
    DFS_OBJ_SHARED_FIXED_32_32_96M,
    DFS_OBJ_SHARED_FIXED_48_48_96M,

    DFS_OBJ_SHARED_FIXED_16_16_128M,
    DFS_OBJ_SHARED_FIXED_32_32_128M,
    DFS_OBJ_SHARED_FIXED_64_64_128M,

    DFS_OBJ_SHARED_FIXED_16_16_160M,
    DFS_OBJ_SHARED_FIXED_32_32_160M,
    DFS_OBJ_SHARED_FIXED_80_80_160M,

    DFS_OBJ_SHARED_FIXED_END,

    //at least frequency
    DFS_OBJ_SHARED_ATLEAST_START = 32,

    DFS_OBJ_SHARED_ATLEAST_32_32_16M = DFS_OBJ_SHARED_ATLEAST_START,
    DFS_OBJ_SHARED_ATLEAST_48_48_16M,
    DFS_OBJ_SHARED_ATLEAST_64_64_16M,
    DFS_OBJ_SHARED_ATLEAST_80_80_16M,

    DFS_OBJ_SHARED_ATLEAST_16_16_32M,
    DFS_OBJ_SHARED_ATLEAST_32_32_32M,
    DFS_OBJ_SHARED_ATLEAST_64_64_32M,

    DFS_OBJ_SHARED_ATLEAST_16_16_48M,
    DFS_OBJ_SHARED_ATLEAST_48_48_48M,

    DFS_OBJ_SHARED_ATLEAST_16_16_64M,
    DFS_OBJ_SHARED_ATLEAST_32_32_64M,
    DFS_OBJ_SHARED_ATLEAST_64_64_64M,

    DFS_OBJ_SHARED_ATLEAST_16_16_80M,
    DFS_OBJ_SHARED_ATLEAST_80_80_80M,

    DFS_OBJ_SHARED_ATLEAST_16_16_96M,
    DFS_OBJ_SHARED_ATLEAST_32_32_96M,
    DFS_OBJ_SHARED_ATLEAST_48_48_96M,

    DFS_OBJ_SHARED_ATLEAST_16_16_128M,
    DFS_OBJ_SHARED_ATLEAST_32_32_128M,
    DFS_OBJ_SHARED_ATLEAST_64_64_128M,

    DFS_OBJ_SHARED_ATLEAST_16_16_160M,
    DFS_OBJ_SHARED_ATLEAST_32_32_160M,

    DFS_OBJ_SHARED_ATLEAST_END,

    //dsp caller
    DFS_OBJ_DSP_START = 64,

    DFS_OBJ_DSP_MIC_STREAM = DFS_OBJ_DSP_START,
    DFS_OBJ_DSP_MUSIC_STREAM,
    DFS_OBJ_DSP_VOICE_STREAM,
    DFS_OBJ_DSP_TONE_STREAM,

    DFS_OBJ_DSP_END,

    //bt caller
    DFS_OBJ_BT_START = 96,

    DFS_OBJ_BT_TDS = DFS_OBJ_BT_START,
    DFS_OBJ_BT_SCO_LINK,
    DFS_OBJ_BT_MUSIC_STREAM,
    DFS_OBJ_BT_BLE_CON,

    DFS_OBJ_BT_END,

    //dtop caller
    DFS_OBJ_DTOP_START = 128,

    DFS_OBJ_DTOP_OTA = DFS_OBJ_DTOP_START,
    DFS_OBJ_DTOP_TONE_STREAM,
    DFS_OBJ_DTOP_ANC,
    DFS_OBJ_DTOP_VAD,
    DFS_OBJ_DTOP_PLAYER_SPK,

    DFS_OBJ_DTOP_END,

    DFS_OBJ_MAX = 160,
} DFS_OBJ;

#ifdef static_assert
static_assert(DFS_OBJ_SHARED_FIXED_END <= DFS_OBJ_SHARED_ATLEAST_START, dfs_c);
static_assert(DFS_OBJ_SHARED_ATLEAST_END <= DFS_OBJ_DSP_START, dfs_c);
static_assert(DFS_OBJ_DSP_END <= DFS_OBJ_BT_START, dfs_c);
static_assert(DFS_OBJ_BT_END <= DFS_OBJ_DTOP_START, dfs_c);
static_assert(DFS_OBJ_DTOP_END <= DFS_OBJ_MAX, dfs_c);
#endif

typedef enum {
    DFS_ACT_STOP = 0,
    DFS_ACT_START,
    DFS_ACT_NUM,
} DFS_ACT;

/* Separating dfs_caller and dfs_callee header file to make hierarchy
 * looks clear.
 * Here, to make dfs caller use it convenience, make a dfs.h, incluede
 * dfs_caller.h and dfs_callee.h. Then dfs caller can just include dfs.h
 * to use, don't need to recoginize dfs_caller.h or dfs_callee.h.
 */
#include "dfs_caller.h"
#include "dfs_callee.h"

#endif /* __DFS_H__ */
