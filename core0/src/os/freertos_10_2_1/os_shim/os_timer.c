/****************************************************************************

Copyright(c) 2016 by WuQi Technologies. ALL RIGHTS RESERVED.

This Information is proprietary to WuQi Technologies and MAY NOT
be copied by any method or incorporated into another program without
the express written consent of WuQi. This Information or any portion
thereof remains the property of WuQi. The Information contained herein
is believed to be accurate and WuQi assumes no responsibility or
liability for its use in any way and conveys no license or title under
any patent or copyright and makes no representation or warranty that this
Information is free from patent or copyright infringement.

****************************************************************************/

/* Free RTOS includes */
#include "FreeRTOS.h"
#include "timers.h"
#include "utils.h"

/* os shim includes */
#include "os_timer.h"
#include "os_mem.h"
#include "os_task.h"

/* common includes */
#include "stdio.h"
#include "string.h"
#include "lib_dbglog.h"

#if defined(OS_MALLOC_DEBUG_LEVEL) && OS_MALLOC_DEBUG_LEVEL >= 2
#include "riscv_cpu.h"
#endif

#ifdef CHECK_ISR_USAGE
#include "iot_irq.h"
#include "iot_timer.h"
#if defined(BUILD_CORE_CORE1)
#define IOT_OS_TIMER_HANDLER_TIME_LIMIT_US 1800
#else
#define IOT_OS_TIMER_HANDLER_TIME_LIMIT_US 250
#endif
#endif

#define MAX_TIMER_NAME_LEN 28

typedef struct _wrapper_timer_arg {
    os_timer_func_t fn;
    void *arg;
    uint8_t timer_name[MAX_TIMER_NAME_LEN];
} wrapper_timer_arg_t;

static void wrapper_timer_func(TimerHandle_t timer_id)
{
    wrapper_timer_arg_t *arg = pvTimerGetTimerID(timer_id);

    FUNCTION_STACK_CHECK_START();

#ifdef CHECK_ISR_USAGE
    uint64_t start = iot_timer_get_time();
#endif

    (arg->fn)((timer_id_t)timer_id, arg->arg);   // arg->fn won't be NULL.

#ifdef CHECK_ISR_USAGE
    uint64_t stop = iot_timer_get_time();
    uint32_t duration_us = (stop - start);
    iot_irq_timer_usage_insert(duration_us, (uint32_t)arg->fn);
    //os timer handler time cost limit
    if (duration_us > IOT_OS_TIMER_HANDLER_TIME_LIMIT_US) {
        DBGLOG_LIB_WARNING("[WARNING]func:%x time %d",(uint32_t)arg->fn, duration_us);
    }
#endif

    FUNCTION_STACK_CHECK_END(256);
}

timer_id_t ICACHE_ATTR os_create_timer(module_id_t module_id, bool_t auto_reload, os_timer_func_t cb, void *arg)
{
    timer_id_t timer_id = (timer_id_t)NULL;
    wrapper_timer_arg_t *timer_arg_ptr;
#if defined(OS_MALLOC_DEBUG_LEVEL) && OS_MALLOC_DEBUG_LEVEL >= 2
    uint32_t ret_addr;

    ret_addr = CPU_GET_RET_ADDRESS();
#endif
    if (cb == NULL) {
        return timer_id;
    }
#if defined(OS_MALLOC_DEBUG_LEVEL) && OS_MALLOC_DEBUG_LEVEL >= 2
    timer_arg_ptr = (wrapper_timer_arg_t *)os_mem_malloc_dbg(module_id, sizeof(*timer_arg_ptr), ret_addr);
#else
    timer_arg_ptr = (wrapper_timer_arg_t *)os_mem_malloc(module_id, sizeof(*timer_arg_ptr));
#endif
    if (timer_arg_ptr == NULL) {
        return timer_id;
    }

    memset(timer_arg_ptr, 0, sizeof(*timer_arg_ptr));
    timer_arg_ptr->fn = cb;
    timer_arg_ptr->arg = arg;
    snprintf((char *const)timer_arg_ptr->timer_name, MAX_TIMER_NAME_LEN - 1, "%08X %08X",
             (unsigned int)cb, (unsigned int)arg);

    /* temporarily initialize the timer period to a known good one as the
     * real period must be specified in start timer function.
     */
    TimerHandle_t timer_handle =
        xTimerCreate((char *)timer_arg_ptr->timer_name, 10000 / portTICK_PERIOD_MS,
                     auto_reload ? pdTRUE : pdFALSE, (void *)timer_arg_ptr, wrapper_timer_func);
    if (timer_handle) {
        timer_id = (timer_id_t)timer_handle;
    } else {
        os_mem_free(timer_arg_ptr);
    }

    return timer_id;
}

void os_start_timer(timer_id_t id, uint32_t period)
{
    if (period == 0) {
        /* avoid crash */
        period = 1;
    }
    xTimerChangePeriod((TimerHandle_t)id, period / portTICK_PERIOD_MS, portMAX_DELAY);
}

void os_stop_timer_from_isr(timer_id_t id)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    xTimerStopFromISR((TimerHandle_t)id, &xHigherPriorityTaskWoken);
}

void os_stop_timer(timer_id_t id)
{
    xTimerStop((TimerHandle_t)id, portMAX_DELAY);
}

void os_reset_timer(timer_id_t id)
{
    xTimerReset((TimerHandle_t)id, portMAX_DELAY);
}

void os_delete_timer(timer_id_t id)
{
    if (xTimerIsTimerActive((TimerHandle_t)id)) {
        os_stop_timer(id);
    }

    wrapper_timer_arg_t *arg = (wrapper_timer_arg_t *)pvTimerGetTimerID((TimerHandle_t)id);
    xTimerDelete((TimerHandle_t)id, portMAX_DELAY);
    os_mem_free(arg);
}

uint32_t os_is_timer_active(timer_id_t id)
{
    if (xTimerIsTimerActive((TimerHandle_t)id)) {
        return 1;
    }

    return 0;
}
