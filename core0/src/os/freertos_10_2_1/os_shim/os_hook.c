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
/* common includes */
#include "types.h"
#include "stdio.h"
#include "critical_sec.h"

/* os shim includes */
#include "os_hook.h"

#ifdef LIB_DBGLOG_ENABLE
#include "dbglog.h"
#endif

#define OS_HOOK_CALLBACK_MAX_COUNT 5
#define OS_HOOK_MAX_LOG_SIZE 64
static os_hook_idle_callback idle_cb[OS_HOOK_CALLBACK_MAX_COUNT];
static os_hook_tick_callback tick_cb[OS_HOOK_CALLBACK_MAX_COUNT];
static os_hook_malloc_fail_callback malloc_fail_cb;
static os_hook_sleep_process_callback sleep_cb;
static os_hook_time_stamp_callback time_stamp_cb;
static os_hook_save_callback sys_save_cb;
static os_hook_restore_callback sys_restore_cb;
static os_hook_failed_dump_cb sys_failed_dump_cb;
static os_hook_sleep_fail_callback sys_sleep_failed_cb;

// make sure the sleep and the wakeup is pairing
static uint8_t sleep_flags = 0;

void os_hook_sleep_fail_regisiter_callback(os_hook_sleep_fail_callback cb)
{
    sys_sleep_failed_cb = cb;
}

void os_hook_sleep_fail_handler(void)
{
    if (sys_sleep_failed_cb) {
        sys_sleep_failed_cb();
    }
}

void os_hook_set_sleep_flags(uint8_t val) IRAM_TEXT(os_hook_set_sleep_flags);
void os_hook_set_sleep_flags(uint8_t val)
{
    sleep_flags = val;
}

uint8_t os_hook_get_sleep_flags(void)
{
    return sleep_flags;
}

static void os_hook_send(uint8_t *buffer, uint8_t length)
{
#ifdef LIB_DBGLOG_ENABLE
    dbglog_crash_log_write(buffer, length);
#else
    UNUSED(buffer);
    UNUSED(length);
#endif
} /*lint !e818 Can't be pointer to const due to the api's flexibility. */

static void os_hook_log_print(char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
#ifdef LIB_DBGLOG_ENABLE
    char str[OS_HOOK_MAX_LOG_SIZE];
    uint32_t length = (uint32_t)vsnprintf(str, OS_HOOK_MAX_LOG_SIZE, fmt, ap);
    os_hook_send((uint8_t *)str, (uint8_t)length);
#else
    UNUSED(os_hook_send);
    vprintf(fmt, ap);
#endif
    va_end(ap);
} /*lint !e818 Can't be pointer to const due to the api's flexibility. */

void vApplicationIdleHook(void)
{
    for (uint32_t i = 0; i < OS_HOOK_CALLBACK_MAX_COUNT; i++) {
        if (idle_cb[i]) {
            idle_cb[i]();
        }
    }
}

void vApplicationTickHook(void)
{
    for (uint32_t i = 0; i < OS_HOOK_CALLBACK_MAX_COUNT; i++) {
        if (tick_cb[i]) {
            tick_cb[i]();
        }
    }
}

void vApplicationMallocFailedHook(void)
{
    if (malloc_fail_cb != NULL) {
        malloc_fail_cb();
    }
}

void vApplicationStackOverflowHook(TaskHandle_t pxTask, char *pcTaskName)
{
    os_hook_log_print("task %s:%p overflow\n", pcTaskName, pxTask);
    assert(0);
}

void vPortProcessSleep(uint32_t expected_idle_time_ms)
{
    if (sleep_cb != NULL) {
        sleep_cb(expected_idle_time_ms);
    }
}

uint32_t vPortSaveHook(void) IRAM_TEXT(vPortSaveHook);
uint32_t vPortSaveHook(void)
{
    if (sys_save_cb != NULL) {
        if (sys_save_cb() != RET_OK) {
            return RET_BUSY;
        }
    }

    return RET_OK;
}

void vPortRestoreHook(void)
{
    if (sys_restore_cb != NULL) {
        sys_restore_cb();
    }
}

uint32_t vPortGetCurrentTime(void)
{
    if (time_stamp_cb != NULL) {
        return time_stamp_cb();
    }

    return 0;
}

void vPortFailedDumpHook(void)
{
    if (sys_failed_dump_cb != NULL) {
        sys_failed_dump_cb();
    }
}

uint8_t os_hook_failed_dump_register_callback(os_hook_failed_dump_cb cb)
{
    sys_failed_dump_cb = cb;

    return RET_OK;
}

uint8_t os_hook_idle_register_callback(os_hook_idle_callback cb)
{
    uint32_t i;

    cpu_critical_enter();

    for (i = 0; i < OS_HOOK_CALLBACK_MAX_COUNT; i++) {
        if (idle_cb[i] == NULL) {
            idle_cb[i] = cb;
            break;
        }
    }

    cpu_critical_exit();

    if (i == OS_HOOK_CALLBACK_MAX_COUNT) {
        return RET_FAIL;
    } else {
        return RET_OK;
    }
}

uint8_t os_hook_tick_register_callback(os_hook_tick_callback cb)
{
    uint32_t i;

    cpu_critical_enter();

    for (i = 0; i < OS_HOOK_CALLBACK_MAX_COUNT; i++) {
        if (tick_cb[i] == NULL) {
            tick_cb[i] = cb;
            break;
        }
    }

    cpu_critical_exit();

    if (i == OS_HOOK_CALLBACK_MAX_COUNT) {
        return RET_FAIL;
    } else {
        return RET_OK;
    }
}

uint8_t os_hook_malloc_fail_register_callback(os_hook_malloc_fail_callback cb)
{
    malloc_fail_cb = cb;

    return RET_OK;
}

uint8_t os_hook_sleep_register_callback(os_hook_sleep_process_callback cb)
{
    sleep_cb = cb;

    return RET_OK;
}

uint8_t os_hook_time_stamp_register_callback(os_hook_time_stamp_callback cb)
{
    time_stamp_cb = cb;

    return RET_OK;
}

uint8_t os_hook_idle_unregister_callback(os_hook_idle_callback cb)
{
    uint32_t i;

    cpu_critical_enter();

    for (i = 0; i < OS_HOOK_CALLBACK_MAX_COUNT; i++) {
        if (idle_cb[i] == cb) {
            idle_cb[i] = NULL;
            break;
        }
    }

    cpu_critical_exit();

    if (i == OS_HOOK_CALLBACK_MAX_COUNT) {
        return RET_FAIL;
    } else {
        return RET_OK;
    }
}

uint8_t os_hook_tick_unregister_callback(os_hook_tick_callback cb)
{
    uint32_t i;

    cpu_critical_enter();

    for (i = 0; i < OS_HOOK_CALLBACK_MAX_COUNT; i++) {
        if (tick_cb[i] == cb) {
            tick_cb[i] = NULL;
            break;
        }
    }

    cpu_critical_exit();

    if (i == OS_HOOK_CALLBACK_MAX_COUNT) {
        return RET_FAIL;
    } else {
        return RET_OK;
    }
}

uint8_t os_hook_malloc_fail_unregister_callback(void)
{
    malloc_fail_cb = NULL;

    return RET_OK;
}

uint8_t os_hook_sys_save_register_callback(os_hook_save_callback cb)
{
    sys_save_cb = cb;
    sleep_flags = 0;
    return RET_OK;
}

uint8_t os_hook_sys_restore_register_callback(os_hook_restore_callback cb)
{
    sys_restore_cb = cb;

    return RET_OK;
}
