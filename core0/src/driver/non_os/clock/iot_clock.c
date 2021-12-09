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

#include "types.h"

#include "clock.h"
#include "pmm.h"
#include "apb.h"

#include "iot_clock.h"

#define CM_TO_ICM(mode)     ((IOT_CLOCK_MODE)mode)

#define CC_TO_ICC(clock)    ((IOT_CLOCK_CORE)clock)
#define ICC_TO_CC(clock)    ((CLOCK_CORE)clock)

#define CLOCK_MODE_SUPPORT_MASK         ((1U<<IOT_CLOCK_MODE_0)   |   \
                                         (1U<<IOT_CLOCK_MODE_6)   |   \
                                         (1U<<IOT_CLOCK_MODE_10)  |   \
                                         (1U<<IOT_CLOCK_MODE_11)  |   \
                                         (1U<<IOT_CLOCK_MODE_12)  |   \
                                         (1U<<IOT_CLOCK_MODE_13)  |   \
                                         (1U<<IOT_CLOCK_MODE_14)  |   \
                                         (1U<<IOT_CLOCK_MODE_18)  |   \
                                         (1U<<IOT_CLOCK_MODE_19)  |   \
                                         (1U<<IOT_CLOCK_MODE_20)  |   \
                                         (1U<<IOT_CLOCK_MODE_21)  |   \
                                         (1U<<IOT_CLOCK_MODE_22)  |   \
                                         (1U<<IOT_CLOCK_MODE_23))
typedef struct {
    PMM_DLDO_VOL dldo;
    CLOCK_MODE clk_mode;
} iot_clock_cfg_t;

const iot_clock_cfg_t iot_clock_cfg_table[] = {
    // FIXME: always set dldo to 0.7 for call flow, need fix.
    [IOT_CLOCK_MODE_0] = {PMM_DLDO_VOL_0_6, CLOCK_MODE_0},

    // clock mode 1-5 not support now, by zhaofei 2021/06/26
    [IOT_CLOCK_MODE_1] = {PMM_DLDO_VOL_MAX, CLOCK_MODE_MAX},
    [IOT_CLOCK_MODE_2] = {PMM_DLDO_VOL_MAX, CLOCK_MODE_MAX},
    [IOT_CLOCK_MODE_3] = {PMM_DLDO_VOL_MAX, CLOCK_MODE_MAX},
    [IOT_CLOCK_MODE_4] = {PMM_DLDO_VOL_MAX, CLOCK_MODE_MAX},
    [IOT_CLOCK_MODE_5] = {PMM_DLDO_VOL_MAX, CLOCK_MODE_MAX},

    [IOT_CLOCK_MODE_6] = {PMM_DLDO_VOL_0_6, CLOCK_MODE_6},

    // clock mode 7-9 not support now, by zhaofei 2021/06/26
    [IOT_CLOCK_MODE_7] = {PMM_DLDO_VOL_MAX, CLOCK_MODE_MAX},
    [IOT_CLOCK_MODE_8] = {PMM_DLDO_VOL_MAX, CLOCK_MODE_MAX},
    [IOT_CLOCK_MODE_9] = {PMM_DLDO_VOL_MAX, CLOCK_MODE_MAX},

    [IOT_CLOCK_MODE_10] = {PMM_DLDO_VOL_0_7, CLOCK_MODE_10},
    [IOT_CLOCK_MODE_11] = {PMM_DLDO_VOL_0_7, CLOCK_MODE_11},
    [IOT_CLOCK_MODE_12] = {PMM_DLDO_VOL_0_7, CLOCK_MODE_12},

    [IOT_CLOCK_MODE_13] = {PMM_DLDO_VOL_0_7, CLOCK_MODE_13},
    [IOT_CLOCK_MODE_14] = {PMM_DLDO_VOL_0_7, CLOCK_MODE_14},

    // clock mode 15-17 not support now, by zhaofei 2021/06/25
    [IOT_CLOCK_MODE_15] = {PMM_DLDO_VOL_MAX, CLOCK_MODE_MAX},
    [IOT_CLOCK_MODE_16] = {PMM_DLDO_VOL_MAX, CLOCK_MODE_MAX},
    [IOT_CLOCK_MODE_17] = {PMM_DLDO_VOL_MAX, CLOCK_MODE_MAX},

    [IOT_CLOCK_MODE_18] = {PMM_DLDO_VOL_0_8, CLOCK_MODE_18},
    [IOT_CLOCK_MODE_19] = {PMM_DLDO_VOL_0_8, CLOCK_MODE_19},
    [IOT_CLOCK_MODE_20] = {PMM_DLDO_VOL_0_8, CLOCK_MODE_20},

    [IOT_CLOCK_MODE_21] = {PMM_DLDO_VOL_0_8, CLOCK_MODE_21},
    [IOT_CLOCK_MODE_22] = {PMM_DLDO_VOL_0_8, CLOCK_MODE_22},
    [IOT_CLOCK_MODE_23] = {PMM_DLDO_VOL_0_8, CLOCK_MODE_23},
};

static IOT_CLOCK_MODE iot_clock_state = IOT_CLOCK_MODE_0;

void iot_clock_reset_state(void)
{
    iot_clock_state = IOT_CLOCK_MODE_0;
    clock_reset_state();
}

void iot_clock_set_mode(IOT_CLOCK_MODE mode)
{
    // Check if need switch
    if (mode == iot_clock_state) {
        return;
    }

    // clk mode 1-9, 15-17 not support now, assert
    if (((1U<<mode) & CLOCK_MODE_SUPPORT_MASK) == 0) {
        assert(0);
    }

    if (iot_clock_cfg_table[iot_clock_state].dldo > iot_clock_cfg_table[mode].dldo) {
        // Need reduce voltage
        // 1. switch clk
        clock_set_core_clock_mode(iot_clock_cfg_table[mode].clk_mode);
        // 2. set memory
        apb_memory_dfs_config((APB_MEM_DVS)iot_clock_cfg_table[mode].dldo);
        pmm_memory_dfs_config(iot_clock_cfg_table[mode].dldo);
        // 3. set voltage
        pmm_dldo_config(iot_clock_cfg_table[mode].dldo);
    } else if (iot_clock_cfg_table[iot_clock_state].dldo < iot_clock_cfg_table[mode].dldo) {
        // Need boost voltage
        // 1. set voltage
        pmm_dldo_config(iot_clock_cfg_table[mode].dldo);
        // 2. set memory
        apb_memory_dfs_config((APB_MEM_DVS)iot_clock_cfg_table[mode].dldo);
        pmm_memory_dfs_config(iot_clock_cfg_table[mode].dldo);
        // 3. switch clk
        clock_set_core_clock_mode(iot_clock_cfg_table[mode].clk_mode);
    } else {
        clock_set_core_clock_mode(iot_clock_cfg_table[mode].clk_mode);
    }

    iot_clock_state = mode;
}

IOT_CLOCK_CORE iot_clock_get_core_clock_by_mode(IOT_CLOCK_MODE mode, uint32_t core_id)
{
    CLOCK_CORE clock;

    clock = clock_get_core_clock_by_mode(iot_clock_cfg_table[mode].clk_mode, core_id);

    return CC_TO_ICC(clock);
}

IOT_CLOCK_MODE iot_clock_get_mode_by_core_clock(IOT_CLOCK_CORE dtop_core_clock,
                                                IOT_CLOCK_CORE bt_core_clock,
                                                IOT_CLOCK_CORE dsp_core_clock)
{
    CLOCK_MODE mode;

    mode = clock_get_mode_by_core_clock(ICC_TO_CC(dtop_core_clock),
                                        ICC_TO_CC(bt_core_clock),
                                        ICC_TO_CC(dsp_core_clock));

    return CM_TO_ICM(mode);
}
