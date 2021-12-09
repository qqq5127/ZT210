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

#ifndef OS_SHIM_UTILS_H
#define OS_SHIM_UTILS_H

#include "os_mem.h"

/**
 * @addtogroup OS_SHIM
 * @{
 * @addtogroup OS_UTILS
 * @{
 * This section introduces the UTILS module's functions and how to use this driver.
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief This function is used to init the utils module.
 *
 * @param stack_top is os set stack.
 * @param heap_region is os heap init's start.
 */
void os_utils_init(uint32_t stack_top, const os_heap_region_t *heap_region);

/**
 * @brief This function is used to get the time since boot up in ms.
 *        uint32_t time value will overflow in 49.71 days.
 *
 * @return uint32_t duration in milliseconds since the system boot up
 */
uint32_t os_boot_time32(void);

/**
 * @brief This function is used to get the time since boot up in ms
 *
 * @return uint64_t duration in milliseconds since the system boot up
 */
uint64_t os_boot_time64(void);

/**
 * @brief This function is used to get a pseudo random number
 *
 * @return uint32_t a pseudo random number
 */
uint32_t os_rand(void);

/**
 * @brief This function is used to wait for timeout (Time Delay)
 *
 * @param millisec is time delay value
 * @return uint32_t 0.
 */
uint32_t os_delay(uint32_t millisec);

/**
 * @brief This function is used to query ticks counter
 *
 * @return uint32_t return ticks.
 */
uint32_t os_get_ticks(void);

#ifdef __cplusplus
}
#endif

/**
 * @}
 * @}
 */

#endif /* OS_SHIM_UTILS_H */
