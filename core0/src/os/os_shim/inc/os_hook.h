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

#ifndef OS_SHIM_HOOK_H
#define OS_SHIM_HOOK_H

/**
 * @addtogroup OS
 * @{
 * @addtogroup OS_SHIM
 * @{
 * @addtogroup OS_HOOK
 * @{
 * This section introduces OS hook reference api.
 */

#include "FreeRTOS.h"
#include "task.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*os_hook_idle_callback)(void);
typedef void (*os_hook_tick_callback)(void);
typedef void (*os_hook_malloc_fail_callback)(void);
typedef void (*os_hook_sleep_process_callback)(uint32_t expected_idle_time_ms);
typedef uint32_t (*os_hook_time_stamp_callback)(void);
typedef uint32_t (*os_hook_save_callback)(void);
typedef uint32_t (*os_hook_restore_callback)(void);
typedef void (*os_hook_failed_dump_cb)(void);
typedef void (*os_hook_sleep_fail_callback)(void);

/**
 * @brief This function is OS's flag set.
 */
void os_hook_set_sleep_flags(uint8_t val);

/**
 * @brief This function is OS's  flag get.
 */
uint8_t os_hook_get_sleep_flags(void);

/**
 * @brief This function is OS's idle task hook.
 */
void vApplicationIdleHook(void);

/**
 * @brief This function is OS's systick hook.
 */
void vApplicationTickHook(void);

/**
 * @brief This function is malloc failed hook.
 */
void vApplicationMallocFailedHook(void);

/**
 * @brief This function is OS's task stack overflow hook.
 *
 * @param pxTask is stack overflow task handle.
 * @param pcTaskName is stack overflow task name.
 */
void vApplicationStackOverflowHook(TaskHandle_t pxTask, char *pcTaskName);

/**
 * @brief This function is port to process sleep task.
 *
 * @param expected_idle_time_ms is the expected idle time in ms.
 */
void vPortProcessSleep(uint32_t expected_idle_time_ms);

/**
 * @brief This function is port to save hook.
 */
uint32_t vPortSaveHook(void);

/**
 * @brief This function is port to restore hook.
 */
void vPortRestoreHook(void);

/**
 * @brief This function is port to get current time.
 *
 * @return uint32_t is the current time.
 */
uint32_t vPortGetCurrentTime(void);

/**
 * @brief This function is port to failed dump hook.
 */
void vPortFailedDumpHook(void);

/**
 * @brief This function is failed dump hook to register callback function.
 *
 * @param cb is the failed dump hook callback.
 * @return uint8_t RET_OK for success else error.
 */
uint8_t os_hook_failed_dump_register_callback(os_hook_failed_dump_cb cb);

/**
 * @brief This function is idle hook to register callback function.
 *
 * @param cb is the idle hook callback.
 * @return uint8_t RET_OK for success else error.
 */
uint8_t os_hook_idle_register_callback(os_hook_idle_callback cb);

/**
 * @brief This function is tick hook to register callback function.
 *
 * @param cb is the tick hook callback.
 * @return uint8_t RET_OK for success else error.
 */
uint8_t os_hook_tick_register_callback(os_hook_tick_callback cb);

/**
 * @brief This function is hook to malloc failed register callback function.
 *
 * @param cb is the malloc failed callback.
 * @return uint8_t RET_OK for success else error.
 */
uint8_t os_hook_malloc_fail_register_callback(os_hook_malloc_fail_callback cb);

/**
 * @brief This function is idle hook to unregister callback function.
 *
 * @param cb is the idle hook callback.
 * @return uint8_t RET_OK for success else error.
 */
uint8_t os_hook_idle_unregister_callback(os_hook_idle_callback cb);

/**
 * @brief This function is tick hook to unregister callback function.
 *
 * @param cb is the tick hook callback.
 * @return uint8_t RET_OK for success else error.
 */
uint8_t os_hook_tick_unregister_callback(os_hook_tick_callback cb);

/**
 * @brief This function is hook to malloc failed unregister callback function.
 *
 * @return uint8_t RET_OK for success else error.
 */
uint8_t os_hook_malloc_fail_unregister_callback(void);

/**
 * @brief This function is sleep register callback.
 *
 * @param cb is the sleep process callback.
 * @return uint8_t RET_OK for success else error.
 */
uint8_t os_hook_sleep_register_callback(os_hook_sleep_process_callback cb);

/**
 * @brief This function is time stamp register callback.
 *
 * @param cb is the hook time stamp callback.
 * @return uint8_t RET_OK for success else error.
 */
uint8_t os_hook_time_stamp_register_callback(os_hook_time_stamp_callback cb);

uint8_t os_hook_sys_save_register_callback(os_hook_save_callback cb);

uint8_t os_hook_sys_restore_register_callback(os_hook_restore_callback cb);

void os_hook_sleep_fail_regisiter_callback(os_hook_sleep_fail_callback cb);

void os_hook_sleep_fail_handler(void);

#ifdef __cplusplus
}
#endif
/**
 * @}
 * addtogroup OS_HOOK
 * @}
 * addtogroup OS_SHIM
 * @}
 * addtogroup OS
 */
#endif /* OS_SHIM_HOOK_H */
