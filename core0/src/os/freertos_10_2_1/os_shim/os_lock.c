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
#include "semphr.h"

/* os shim includes */
#include "os_mem.h"
#include "os_lock.h"
#include "os_task.h"
#include "riscv_cpu.h"

#define MUTEX_TIMEOUT_TIME  (60 * 1000)     // 1min

typedef struct _os_mutex {
    SemaphoreHandle_t mutex;
} os_mutex_t;

typedef struct _os_sem {
    SemaphoreHandle_t sem;
} os_sem_t;

os_mutex_h os_create_mutex(module_id_t module_id)
{
    os_mutex_t *tmp;
#if defined(OS_MALLOC_DEBUG_LEVEL) && OS_MALLOC_DEBUG_LEVEL >= 2
    uint32_t ret_addr;

    ret_addr = CPU_GET_RET_ADDRESS();

    tmp = os_mem_malloc_dbg(module_id, sizeof(*tmp), ret_addr);
#else
    tmp = os_mem_malloc(module_id, sizeof(*tmp));
#endif
    if (tmp) {
        tmp->mutex = xSemaphoreCreateRecursiveMutex();
        if (tmp->mutex == NULL) {
            os_mem_free(tmp);
            tmp = NULL;
        }
    }

    return tmp;
}

void os_acquire_mutex(os_mutex_h mutex)
{
    /* it's a WAR for flash log/dump actually, We should bypass these OS-relate APIs
       in flash P/E interface when system in panic status
       */
    if (in_irq()) {
        return;
    }
    BaseType_t result = xSemaphoreTakeRecursive(((os_mutex_t *)mutex)->mutex, MUTEX_TIMEOUT_TIME);
    assert(result == pdTRUE);
} /*lint !e818 Can't be pointer to const due to the typedef. */

void os_release_mutex(os_mutex_h mutex)
{
     /* it's a WAR for flash log/dump actually, We should bypass these OS-relate APIs
       in flash P/E interface when system in panic status
       */
    if (in_irq()) {
        return;
    }
    xSemaphoreGiveRecursive(((os_mutex_t *)mutex)->mutex);
} /*lint !e818 Can't be pointer to const due to the typedef. */

bool_t os_try_acquire_mutex(os_mutex_h mutex)
{
     /* it's a WAR for flash log/dump actually, We should bypass these OS-relate APIs
       in flash P/E interface when system in panic status
       */
    if (in_irq()) {
        return true;
    }
    BaseType_t result;
    result = xSemaphoreTakeRecursive(((os_mutex_t *)mutex)->mutex, 0);
    return result == pdPASS ? true : false;
} /*lint !e818 Can't be pointer to const due to the typedef. */

void os_delete_mutex(os_mutex_h mutex)
{
    vSemaphoreDelete(((os_mutex_t *)mutex)->mutex);
    os_mem_free(mutex);
} /*lint !e818 Can't be pointer to const due to the typedef. */

os_sem_h os_create_semaphore(module_id_t module_id, uint32_t max_count,
                       uint32_t init_count)
{
    os_sem_t *tmp;

#if defined(OS_MALLOC_DEBUG_LEVEL) && OS_MALLOC_DEBUG_LEVEL >= 2
    uint32_t ret_addr;

    ret_addr = CPU_GET_RET_ADDRESS();

    tmp = os_mem_malloc_dbg(module_id, sizeof(*tmp), ret_addr);
#else
    tmp = os_mem_malloc(module_id, sizeof(*tmp));
#endif
    if (tmp) {
        tmp->sem = xSemaphoreCreateCounting(max_count, init_count);
        if (tmp->sem == NULL) {
            os_mem_free(tmp);
            tmp = NULL;
        }
    }
    return tmp;
}

bool_t os_pend_semaphore(os_sem_h semaphore, uint32_t timeout)
{
    if (timeout != portMAX_DELAY) {
        timeout = pdMS_TO_TICKS(timeout);
        if (timeout == portMAX_DELAY) {
            timeout--;
        }
    }
    BaseType_t result = xSemaphoreTake(((os_sem_t *)semaphore)->sem, timeout);
    return result == pdTRUE ? true : false;
} /*lint !e818 Can't be pointer to const due to the typedef. */

bool_t os_post_semaphore(os_sem_h semaphore)
{
    BaseType_t result = xSemaphoreGive(((os_sem_t *)semaphore)->sem);
    return result == pdTRUE ? true : false;
} /*lint !e818 Can't be pointer to const due to the typedef. */

bool_t os_post_semaphore_from_isr(os_sem_h semaphore)
    IRAM_TEXT(os_post_semaphore_from_isr);
bool_t os_post_semaphore_from_isr(os_sem_h semaphore)
{
    BaseType_t need_task_switch = pdFALSE;
    BaseType_t result = xSemaphoreGiveFromISR(((os_sem_t *)semaphore)->sem, &need_task_switch);

    if (need_task_switch) {
        os_task_switch_context();
    }

    return result == pdTRUE ? true : false;
} /*lint !e818 Can't be pointer to const due to the typedef. */

bool_t os_post_semaphore_from_critical(os_sem_h semaphore)
{
    BaseType_t result = xSemaphoreGiveFromISR(((os_sem_t *)semaphore)->sem, NULL);

    return result == pdTRUE ? true : false;
} /*lint !e818 Can't be pointer to const due to the typedef. */

void os_delete_semaphore(os_sem_h semaphore)
{
    vSemaphoreDelete(((os_sem_t *)semaphore)->sem);
    os_mem_free(semaphore);
} /*lint !e818 Can't be pointer to const due to the typedef. */

void os_critical_enter(void)
{
}

void os_critical_exit(void)
{
}
