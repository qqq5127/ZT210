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

#ifndef _HW_WIC_H_
#define _HW_WIC_H_

#define NUM_WIC_PCORE 2

#if defined(BUILD_CORE_CORE0)

typedef enum {
    WIC_DTOP_CORE,
    WIC_SELF = WIC_DTOP_CORE,
    WIC_BT_CORE,
    WIC_AUDIO_CORE,
    WIC_ERR_CORE,
} WIC_CORE;

typedef enum {
    WIC_REG_BT,
    WIC_REG_AUDIO,
    WIC_REG_ERR,
} WIC_REG;

#elif defined(BUILD_CORE_CORE1)

typedef enum {
    WIC_DTOP_CORE,
    WIC_BT_CORE,
    WIC_SELF = WIC_BT_CORE,
    WIC_AUDIO_CORE,
    WIC_ERR_CORE,
} WIC_CORE;

typedef enum {
    WIC_REG_DTOP,
    WIC_REG_AUDIO,
    WIC_REG_ERR,
} WIC_REG;

#elif defined(BUILD_CORE_DSP)

typedef enum {
    WIC_DTOP_CORE,
    WIC_BT_CORE,
    WIC_AUDIO_CORE,
    WIC_SELF = WIC_AUDIO_CORE,
    WIC_ERR_CORE,
} WIC_CORE;

typedef enum {
    WIC_REG_DTOP,
    WIC_REG_BT,
    WIC_REG_ERR,
} WIC_REG;

#endif

WIC_REG wic_core2reg(WIC_CORE core);
WIC_CORE wic_reg2core(WIC_REG reg);

uint32_t wic_get_query_status_raw(WIC_REG target);
void wic_set_query_status(WIC_REG target);
void wic_clear_query_status(WIC_REG target);

void wic_cpu_query_int_enable(WIC_REG target);
uint32_t wic_get_query_int_status(WIC_REG target);
void wic_clear_query_int_status(WIC_REG target);

void wic_cpu_wakeup_int_enable(WIC_REG target);
uint32_t wic_get_wakeup_int_status(WIC_REG target);
void wic_clear_wakeup_int_status(WIC_REG target);

void wic_dma_done_int_enable(WIC_REG target);
uint32_t wic_get_dma_done_int_status(WIC_REG target);
void wic_clear_dma_done_int_status(WIC_REG target);
void wic_set_dma_query_status(WIC_REG target);
void wic_clear_dma_query_status(WIC_REG target);

uint32_t wic_init(void);

#endif
