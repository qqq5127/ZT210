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

#ifndef _IOT_WIC_H_
#define _IOT_WIC_H_

#include "types.h"
#include "os_event.h"
#include "iot_share_task.h"

typedef enum {
#if defined(BUILD_CORE_CORE0)
    EVENT_QUERY_DTOP2BT = (1 << 0),
    EVENT_QUERY_DTOP2AUD = (1 << 1),

    EVENT_WAKEUP_BT2DTOP = (1 << 2),
    EVENT_WAKEUP_AUD2DTOP = (1 << 3),

    EVENT_DMA_DONE_BT2DTOP = (1 << 4),
    EVENT_DMA_DONE_AUD2DTOP = (1 << 5),

#elif defined(BUILD_CORE_CORE1)
    EVENT_QUERY_BT2DTOP = (1 << 0),
    EVENT_QUERY_BT2AUD = (1 << 1),

    EVENT_WAKEUP_DTOP2BT = (1 << 2),
    EVENT_WAKEUP_AUD2BT = (1 << 3),

    EVENT_DMA_DONE_DTOP2BT = (1 << 4),
    EVENT_DMA_DONE_AUD2BT = (1 << 5),

#elif defined(BUILD_CORE_DSP)
    EVENT_QUERY_AUD2DTOP = (1 << 0),
    EVENT_QUERY_AUD2BT = (1 << 1),

    EVENT_WAKEUP_DTOP2AUD = (1 << 2),
    EVENT_WAKEUP_BT2AUD = (1 << 3),

    EVENT_DMA_DONE_DTOP2AUD = (1 << 4),
    EVENT_DMA_DONE_BT2AUD = (1 << 5),

#endif
} WIC_EVENT;

typedef enum {
#if defined(BUILD_CORE_CORE0)
    IOT_WIC_DTOP_CORE,
    IOT_WIC_SELF = IOT_WIC_DTOP_CORE,
    IOT_WIC_BT_CORE,
    IOT_WIC_AUDIO_CORE,
#elif defined(BUILD_CORE_CORE1)
    IOT_WIC_DTOP_CORE,
    IOT_WIC_BT_CORE,
    IOT_WIC_SELF = IOT_WIC_BT_CORE,
    IOT_WIC_AUDIO_CORE,
#elif defined(BUILD_CORE_DSP)
    IOT_WIC_DTOP_CORE,
    IOT_WIC_BT_CORE,
    IOT_WIC_AUDIO_CORE,
    IOT_WIC_SELF = IOT_WIC_AUDIO_CORE,
#endif
    IOT_WIC_CORE_MAX,
    IOT_WIC_ERR_CORE = IOT_WIC_CORE_MAX,
} IOT_WIC_CORE;

enum {
    EVENT_QUERY_R1,
    EVENT_QUERY_R2,

    EVENT_WAKEUP_R1,
    EVENT_WAKEUP_R2,

    EVENT_WIC_MAX,
};

// for src
uint32_t iot_wic_query(IOT_WIC_CORE core, bool_t hold);
void iot_wic_poll(IOT_WIC_CORE core);
void iot_wic_finish(IOT_WIC_CORE core);

// especially for dma
uint32_t iot_wic_dma_query(IOT_WIC_CORE core);
void iot_wic_dma_finish(IOT_WIC_CORE core);

typedef void (*wic_event_callback)(WIC_EVENT event);

uint32_t iot_wic_init(void);
uint32_t iot_wic_event_register(WIC_EVENT event, wic_event_callback cb);
uint32_t iot_wic_wait_event(WIC_EVENT event, WIC_EVENT *ret_event);
uint32_t iot_wic_restore(uint32_t data);

bool_t iot_wic_if_be_hold(void);
#endif
