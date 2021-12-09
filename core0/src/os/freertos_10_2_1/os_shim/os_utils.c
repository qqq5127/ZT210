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

/* FreeRTOS includes */
#include "FreeRTOS.h"
#include "task.h"

/* os shim includes */
#include "os_mem.h"
#include "os_utils.h"
#include "os_lock.h"
#include "os_event.h"

#define TICK_PER_MS (configTICK_RATE_HZ / 1000)

static uint64_t boot_time_ms = 0;
static uint32_t last_tick = 0;
static os_sem_h time_sem = NULL;

void os_utils_init(uint32_t stack_top, const os_heap_region_t *heap_region)
{
    os_set_stack(stack_top);
    os_heap_init(heap_region);

    if (time_sem == NULL) {
        // create a event to protect boot time and last tick count
        time_sem = os_create_semaphore(OS_UTILS_MID, MAX_TIME, 1);
    }
}

uint64_t os_boot_time64(void)
{
    uint64_t result;
    uint32_t passed_tick_count = 0;
    uint32_t now_ticks;

    assert(time_sem);
    os_pend_semaphore(time_sem, MAX_TIME);   // start of protection
    now_ticks = xTaskGetTickCount();
    if (now_ticks >= last_tick) {
        passed_tick_count = now_ticks - last_tick;
    } else {
        passed_tick_count = ((TickType_t)(-1) - (last_tick - now_ticks));
    }

    last_tick = now_ticks;
    boot_time_ms += passed_tick_count / TICK_PER_MS;
    result = boot_time_ms;
    os_post_semaphore(time_sem);   // end of protection

    return result;
}

uint32_t os_boot_time32(void)
{
    uint32_t ticks = xTaskGetTickCount();
    return ticks / TICK_PER_MS;
}

uint32_t os_rand(void)
{
    static uint32_t ulNextRand = 0;
    const uint32_t ulMultiplier = 0x015a4e35UL;
    uint32_t ticks = xTaskGetTickCount();
    uint32_t ulIncrement = ticks % 39989;

    if (ulNextRand == 0) {
        ulNextRand = (ticks + ((uint32_t)&ticks & 0xFFFF)) % 49891;
    }

    ulNextRand = (ulMultiplier * ulNextRand) + ulIncrement;
    return (uint32_t)(ulNextRand);
}

uint32_t os_delay(uint32_t millisec)
{
    TickType_t ticks = millisec / portTICK_PERIOD_MS;
    vTaskDelay(ticks ? ticks : 1); /* Minimum delay = 1 tick */
    return 0;
}

uint32_t os_get_ticks(void)
{
    return xTaskGetTickCount();
}
