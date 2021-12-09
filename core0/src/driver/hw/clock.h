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
#ifndef _DRIVER_HW_CLOCK_H
#define _DRIVER_HW_CLOCK_H

#ifdef __cplusplus
extern "C" {
#endif

#include "types.h"

#define CLOCK_32K 32768
#define CLOCK_TICK 1000000
#define CLOCK_XTAL 16000000

#define CLOCK_CORE_TO_MHZ(clock)    ((clock) << 4)

typedef enum {
    CLOCK_CORE_NONE,
    CLOCK_CORE_16M = 1,
    CLOCK_CORE_32M,
    CLOCK_CORE_48M,
    CLOCK_CORE_64M,
    CLOCK_CORE_80M,
    CLOCK_CORE_96M,
    CLOCK_CORE_128M = 8,
    CLOCK_CORE_160M = 10,
    CLOCK_CORE_MAX,
} CLOCK_CORE;

typedef enum {
    CLOCK_MODE_0,
    CLOCK_MODE_1,
    CLOCK_MODE_2,
    CLOCK_MODE_3,
    CLOCK_MODE_4,
    CLOCK_MODE_5,
    CLOCK_MODE_6,
    CLOCK_MODE_7,
    CLOCK_MODE_8,
    CLOCK_MODE_9,
    CLOCK_MODE_10,
    CLOCK_MODE_11,
    CLOCK_MODE_12,
    CLOCK_MODE_13,
    CLOCK_MODE_14,
    CLOCK_MODE_15,
    CLOCK_MODE_16,
    CLOCK_MODE_17,
    CLOCK_MODE_18,
    CLOCK_MODE_19,
    CLOCK_MODE_20,
    CLOCK_MODE_21,
    CLOCK_MODE_22,
    CLOCK_MODE_23,

    CLOCK_MODE_MAX,
} CLOCK_MODE;

uint32_t clock_get_core_div(uint32_t cpu);
CLOCK_CORE clock_get_core_clock(uint32_t cpu);
uint32_t clock_get_apb_clock(void);
uint32_t clock_get_bt_clock(void);
void clock_set_core_clock_mode(CLOCK_MODE mode);
void clock_reset_state(void);
uint32_t clock_get_tick_clock(void);

CLOCK_CORE clock_get_core_clock_by_mode(CLOCK_MODE mode, uint32_t core_id);

CLOCK_MODE clock_get_mode_by_core_clock(CLOCK_CORE dtop_core_clock,
                                        CLOCK_CORE bt_core_clock,
                                        CLOCK_CORE dsp_core_clock);
#ifdef __cplusplus
}
#endif

#endif /* _DRIVER_HW_CLOCK_H */
