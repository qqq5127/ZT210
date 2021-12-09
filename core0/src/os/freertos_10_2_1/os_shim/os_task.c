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
#include "types.h"
#include "FreeRTOS.h"
#include "task.h"

/* os shim includes */
#include "os_task.h"
#include "os_mem.h"

/* common includes */
#include "string.h"
#include "riscv_cpu.h"

#define OS_TASK_DEFAULT_STACK_SZIE 512
#define OS_TASK_NAME_MAX_SIZE      16

typedef struct _os_task {
    TaskHandle_t h;
    os_task_func_t fn;
    void *arg;
} os_task_t;

static const task_priority_t *task_priorirty_list_tbl = NULL;
static uint32_t task_priority_list_len = 0;

static void wrapper_task_func(void *arg)
{
    os_task_t *task = (os_task_t *)arg;
    (task->fn)(task->arg);
    /* free RTOS requires to call this function before exit the task */
    os_mem_free(task);
    vTaskDelete(NULL);
}

void os_task_init(const task_priority_t *list, uint32_t len)
{
    task_priorirty_list_tbl = (task_priority_t *)list;
    task_priority_list_len = len;
}

static inline uint8_t os_map_priority(uint8_t prio)
{
#if (OS_TASK_PRIO_HIGHEST > (configTIMER_TASK_PRIORITY - 1))
    uint8_t hp = configTIMER_TASK_PRIORITY - 1;
    uint8_t gap = 0;

    gap = OS_TASK_PRIO_HIGHEST - hp;
    if (prio > gap)
        prio -= gap;
    else
        prio = 0;
#endif

    return prio;
}

static uint8_t ICACHE_ATTR os_get_priority(const char *name, uint8_t prio)
{
    for (uint32_t i = 0; i < task_priority_list_len; i++) {
        if (!strncmp(task_priorirty_list_tbl[i].name, name, OS_TASK_NAME_MAX_SIZE)) {
            return task_priorirty_list_tbl[i].priority;
        }
    }

    return os_map_priority(prio);
}

os_task_h ICACHE_ATTR os_create_task_ext(os_task_func_t fn, void *arg, uint8_t prio, uint32_t stack_size,
                             const char *name)
{
    os_task_t *handle = NULL;
    TaskHandle_t h = NULL;
#if defined(OS_MALLOC_DEBUG_LEVEL) && OS_MALLOC_DEBUG_LEVEL >= 2
    uint32_t ret_addr;

    ret_addr = CPU_GET_RET_ADDRESS();
#endif

    /* parameter check */
    if (fn == NULL) {
        return handle;
    }

    if (prio > OS_TASK_PRIO_HIGHEST || prio < OS_TASK_PRIO_LOWEST) {
        return handle;
    }
#if defined(OS_MALLOC_DEBUG_LEVEL) && OS_MALLOC_DEBUG_LEVEL >= 2
    handle = os_mem_malloc_dbg(IOT_OS_SHIM_MID, sizeof(*handle), ret_addr);
#else
    handle = os_mem_malloc(IOT_OS_SHIM_MID, sizeof(*handle));
#endif
    if (handle == NULL) {
        return handle;
    }

    prio = os_get_priority(name, prio);

    handle->arg = arg;
    handle->fn = fn;

    if (stack_size == 0) {
        stack_size = OS_TASK_DEFAULT_STACK_SZIE;
    }

    if (pdPASS != xTaskCreate(wrapper_task_func, name, (uint16_t)stack_size, handle, prio, &h)) {
        os_mem_free(handle);
        handle = NULL;
    } else {
        handle->h = h;
        vTaskSetTaskNumber(h, (const uint32_t)handle);
    }
    return handle;
}

os_task_h ICACHE_ATTR os_create_task_smp_ext(os_task_func_t fn, void *arg, uint8_t prio, uint32_t stack_size,
                                 const char *name, uint8_t core_id)
{
    (void)core_id;
    return os_create_task_ext(fn, arg, prio, stack_size, name);
}

void ICACHE_ATTR os_delete_task(os_task_h handle)
{
    if (!handle) {
        os_task_t * cur_handle = os_get_current_task_handle();
        assert(cur_handle);
        os_mem_free(cur_handle);
        vTaskDelete(NULL);
    } else {
        TaskHandle_t h = ((os_task_t *)handle)->h;
        os_mem_free(handle);
        vTaskDelete(h);
    }
}

void ICACHE_ATTR os_set_task_prio(os_task_h handle, uint8_t prio)
{
    if (prio > OS_TASK_PRIO_HIGHEST || prio < OS_TASK_PRIO_LOWEST)
        return;
    prio = os_map_priority(prio);

    vTaskPrioritySet(((os_task_t *)handle)->h, prio);
} /*lint !e818 Can't be pointer to const due to the typedef. */

void os_set_task_event(os_task_h handle)
    IRAM_TEXT(os_set_task_event);
void os_set_task_event(os_task_h handle)
{
    if (!in_irq()) {
        xTaskNotify(((os_task_t *)handle)->h, 0, eNoAction);
        return;
    }

    BaseType_t pxHigherPriorityTaskWoken;
    xTaskNotifyFromISR(((os_task_t *)handle)->h, 0, eNoAction, &pxHigherPriorityTaskWoken);

    if (pxHigherPriorityTaskWoken) {
        os_task_switch_context();
    }
} /*lint !e818 Can't be pointer to const due to the typedef. */

void os_set_task_event_with_v(os_task_h handle, uint32_t v)
    IRAM_TEXT(os_set_task_event_with_v);
void os_set_task_event_with_v(os_task_h handle, uint32_t v)
{
    if (!in_irq()) {
        xTaskNotify(((os_task_t *)handle)->h, v, eSetBits);
        return;
    }

    BaseType_t pxHigherPriorityTaskWoken;
    xTaskNotifyFromISR(((os_task_t *)handle)->h, v, eSetBits, &pxHigherPriorityTaskWoken);

    if (pxHigherPriorityTaskWoken) {
        os_task_switch_context();
    }
} /*lint !e818 Can't be pointer to const due to the typedef. */

void os_set_task_event_with_v_from_isr(os_task_h handle, uint32_t v)
    IRAM_TEXT(os_set_task_event_with_v_from_isr);
void os_set_task_event_with_v_from_isr(os_task_h handle, uint32_t v)
{
    xTaskNotifyFromISR(((os_task_t *)handle)->h, v, eSetBits, NULL);
} /*lint !e818 Can't be pointer to const due to the typedef. */

void os_wait_task_event(void)
{
    xTaskNotifyWait(0, 0, NULL, portMAX_DELAY);
}

uint32_t os_wait_task_event_with_v(uint32_t time_to_wait)
{
    uint32_t value = 0;
    xTaskNotifyWait(0, 0xFFFFFFFF, &value, time_to_wait);
    return value;
}

void os_start_kernel(void)
{
    vTaskStartScheduler();
}

void os_start_scheduler(void)
{
    xPortStartScheduler();
}

void os_disable_irq(void)
{
    taskDISABLE_INTERRUPTS();
}

void os_enable_irq(void)
{
    taskENABLE_INTERRUPTS();
}

int os_disable_irq_from_isr(void)
{
    return taskENTER_CRITICAL_FROM_ISR();
}

void os_enable_irq_from_isr(int mask)
{
    taskEXIT_CRITICAL_FROM_ISR(mask);
}

int os_atomic_check_set(int *tagert, int old_value, int value)
{
    int mask, success = 1;

    mask = taskENTER_CRITICAL_FROM_ISR();
    if (((volatile int *)tagert) && (old_value == *(volatile int *)tagert))
        *(volatile int *)tagert = value;
    else
        success = 0;
    taskEXIT_CRITICAL_FROM_ISR(mask);

    return success;
}

int os_atomic_add(int *tagert, int value)
{
    int mask, success = 1;

    mask = taskENTER_CRITICAL_FROM_ISR();
    if ((volatile int *)tagert) {
        *(volatile int *)tagert += value;
    } else {
        success = 0;
    }
    taskEXIT_CRITICAL_FROM_ISR(mask);

    return success;
}

int os_atomic_get(int *tagert, int *value)
{
    int mask, success = 1;

    mask = taskENTER_CRITICAL_FROM_ISR();
    if (((volatile int *)tagert) && (value)) {
        *value = *(volatile int *)tagert;
    } else {
        success = 0;
    }
    taskEXIT_CRITICAL_FROM_ISR(mask);

    return success;
} /*lint !e818 Can't be pointer to const due to the typedef. */

int os_get_scheduler_state(void)
{
    uint32_t state;
    state = (uint32_t)xTaskGetSchedulerState();

    if (state == taskSCHEDULER_NOT_STARTED)
        return 0;
    else
        return 1;
}

void os_task_switch_context(void)
    IRAM_TEXT(os_task_switch_context);
void os_task_switch_context(void)
{
    vTaskSwitchContext();
}

uint32_t os_get_total_task_cnt(void)
{
    volatile UBaseType_t task_cnt;

    task_cnt = uxTaskGetNumberOfTasks();

    /* sizeof(UBaseType_t) = 4 */

    return (uint32_t)task_cnt;
}

task_info_t* ICACHE_ATTR os_get_task_info(uint32_t *tasks_to_get, uint32_t *task_time)
{
    uint32_t task_cnt, i;
    TaskStatus_t *p_task;
    TaskStatus_t *p_task_array;
    task_info_t *p_task_buf;
#if defined(OS_MALLOC_DEBUG_LEVEL) && OS_MALLOC_DEBUG_LEVEL >= 2
    uint32_t ret_addr;

    ret_addr = CPU_GET_RET_ADDRESS();
#endif

    task_cnt = os_get_total_task_cnt();

#if defined(OS_MALLOC_DEBUG_LEVEL) && OS_MALLOC_DEBUG_LEVEL >= 2
    p_task_buf = os_mem_malloc_dbg(IOT_OS_SHIM_MID, sizeof(task_info_t) * task_cnt, ret_addr);
#else
    p_task_buf = os_mem_malloc(IOT_OS_SHIM_MID, sizeof(task_info_t) * task_cnt);
#endif
    if (NULL == p_task_buf) {
        return NULL;
    }

#if defined(OS_MALLOC_DEBUG_LEVEL) && OS_MALLOC_DEBUG_LEVEL >= 2
    p_task_array = os_mem_malloc_dbg(IOT_OS_SHIM_MID, sizeof(TaskStatus_t) * task_cnt, ret_addr);
#else
    p_task_array = os_mem_malloc(IOT_OS_SHIM_MID, sizeof(TaskStatus_t) * task_cnt);
#endif
    if (NULL == p_task_array) {
        os_mem_free(p_task_buf);
        return NULL;
    }

    task_cnt = uxTaskGetSystemState2(p_task_array, task_cnt, task_time);

    for (i = 0; i < task_cnt; i++) {
        p_task = p_task_array + i;

        (p_task_buf + i)->id = (uint32_t)p_task->xTaskNumber;
        (p_task_buf + i)->name = p_task->pcTaskName;
        (p_task_buf + i)->cpu_ts = p_task->ulRunTimeCounter;
        (p_task_buf + i)->stack_size = p_task->usStackHighWaterMark;
        (p_task_buf + i)->priority = p_task->uxCurrentPriority;
    }

    os_mem_free(p_task_array);

    *tasks_to_get = task_cnt;
    return p_task_buf;
}

uint32_t os_get_cpu_utilization(void)
{
    uint32_t cpu_u;

    // TODO: need work
    cpu_u = 100;

    return cpu_u;
}

uint32_t os_get_mem_utilization(void)
{
    uint32_t mem_u, mem_free;
    uint32_t heap_size = os_get_heap_size();

    mem_free = xPortGetFreeHeapSize();

    mem_u = ((heap_size - mem_free) * 100) / heap_size;

    return mem_u;
}

os_task_h os_get_current_task_handle(void)
{
    os_task_h task_handle;
    TaskHandle_t handle = xTaskGetCurrentTaskHandle();
    task_handle = (os_task_h)(uxTaskGetTaskNumber(handle));
    return task_handle;
}

uint32_t os_get_task_id(os_task_h handle)
{
    uint32_t task_id;
    TaskStatus_t status;
    vTaskGetInfo(((os_task_t *)handle)->h, &status, pdFALSE, eInvalid);
    task_id = status.xTaskNumber;

    return task_id;
} /*lint !e818 Can't be pointer to const due to the typedef. */

void os_set_task_yield(void)
{
    taskYIELD();
}

uint32_t ICACHE_ATTR os_task_snapshot_all(task_snapshot_t *const task_snapshot_array, const uint32_t array_size,
                              uint32_t *tcb_size)
{
    return uxTaskGetSnapshotAll(
        (TaskSnapshot_t *)task_snapshot_array, /*lint !e740 Unusual pointer cast.*/
        array_size, tcb_size);
}

uint32_t *os_task_get_stack_watermark(os_task_h handle)
{
    /*pointer to the highwater mark*/
    uint32_t *phigh_water_mark;
    TaskStatus_t task_status = {0};
    /*vTaskGetInfo API used for query stack base pointer and high water mark of the task given by the handle*/
    if (handle != NULL) {
        vTaskGetInfo(((os_task_t *)handle)->h, &task_status, pdTRUE, eRunning);
    } else {
        vTaskGetInfo(NULL, &task_status, pdTRUE, eRunning);
    }
    phigh_water_mark = (uint32_t *)((task_status.pxStackBase) + (task_status.usStackHighWaterMark));
    return phigh_water_mark;
} /*lint !e818 Can't be pointer to const due to the typedef. */

void os_task_suspend_all(void)
{
    vTaskSuspendAll();
}

void os_task_resume_all(void)
{
    xTaskResumeAll();
}

bool_t in_scheduling(void)
{
    int32_t state;
    state = xTaskGetSchedulerState();

    if (state == taskSCHEDULER_NOT_STARTED || state == taskSCHEDULER_SUSPENDED)
        return false;
    else
        return true;
}
