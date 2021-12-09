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

/* Free RTOS includes */
#include "FreeRTOS.h"

/* os shim includes */
#include "os_event.h"
#include "os_mem.h"
#include "os_task.h"
#include "event_groups.h"

#if defined(OS_MALLOC_DEBUG_LEVEL) && OS_MALLOC_DEBUG_LEVEL >= 2
#include "riscv_cpu.h"
#endif

typedef struct _os_event {
    EventGroupHandle_t event;
} os_event_t;

os_event_h os_create_event(module_id_t module_id)
{
    os_event_t *tmp;
#if defined(OS_MALLOC_DEBUG_LEVEL) && OS_MALLOC_DEBUG_LEVEL >= 2
    uint32_t ret_addr;

    ret_addr = CPU_GET_RET_ADDRESS();

    tmp = os_mem_malloc_dbg(module_id, sizeof(*tmp), ret_addr);
#else
    tmp = os_mem_malloc(module_id, sizeof(*tmp));
#endif
    if (tmp) {
        tmp->event = xEventGroupCreate();
        if (tmp->event == NULL) {
            os_mem_free(tmp);
            tmp = NULL;
        }
    }

    return tmp;
}

bool_t os_wait_event(os_event_h event_id, uint32_t timeout, uint32_t *recv)
{
    EventBits_t ret;

    if (timeout != portMAX_DELAY) {
        timeout = pdMS_TO_TICKS(timeout);
        if (timeout == portMAX_DELAY) {
            timeout--;
        }
    }

    ret = xEventGroupWaitBits(((os_event_t *)event_id)->event, ~0xFF000000UL, pdTRUE, pdFALSE, timeout);
    if (recv)
        *recv = ret;
    return true;
} /*lint !e818 Can't be pointer to const due to the typedef. */

/* timeout: ms */
bool_t os_wait_event_with_v(os_event_h event_id, uint32_t set, uint8_t option, uint32_t timeout,
                            uint32_t *recv)
{
    EventBits_t ret;

    if (timeout != portMAX_DELAY) {
        timeout = pdMS_TO_TICKS(timeout);
        if (timeout == portMAX_DELAY) {
            timeout--;
        }
    }

    ret = xEventGroupWaitBits(((os_event_t *)event_id)->event, set,
                              option & EVENT_FLAG_CLEAR ? pdTRUE : pdFALSE,
                              option & EVENT_FLAG_AND ? pdTRUE : pdFALSE, timeout);
    if (recv)
        *recv = ret;
    return true;
} /*lint !e818 Can't be pointer to const due to the typedef. */

bool_t os_set_event(os_event_h event_id, uint32_t set)
{
    xEventGroupSetBits(((os_event_t *)event_id)->event, set);
    return true;
} /*lint !e818 Can't be pointer to const due to the typedef. */

bool_t os_set_event_isr(os_event_h event_id, uint32_t set) IRAM_TEXT(os_set_event_isr);
bool_t os_set_event_isr(os_event_h event_id, uint32_t set)
{
    BaseType_t need_task_switch = pdFALSE;
    xEventGroupSetBitsFromISR(((os_event_t *)event_id)->event, set, &need_task_switch);

    if (need_task_switch) {
        os_task_switch_context();
    }

    return true;
} /*lint !e818 Can't be pointer to const due to the typedef. */

void os_clear_event(os_event_h event_id, uint32_t set)
{
    xEventGroupClearBits(((os_event_t *)event_id)->event, set);
} /*lint !e818 Can't be pointer to const due to the typedef. */

void os_clear_event_isr(os_event_h event_id, uint32_t set)
{
    xEventGroupClearBitsFromISR(((os_event_t *)event_id)->event, set);
} /*lint !e818 Can't be pointer to const due to the typedef. */

void os_delete_event(os_event_h event_id)
{
    vEventGroupDelete(((os_event_t *)event_id)->event);
    os_mem_free(event_id);
} /*lint !e818 Can't be pointer to const due to the typedef. */
