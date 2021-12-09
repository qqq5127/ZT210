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

#ifndef OS_SHIM_TIMER_H
#define OS_SHIM_TIMER_H

/**
 * @addtogroup OS_SHIM
 * @{
 * @addtogroup OS_TIMER
 * @{
 * This section introduces the TIMER module's enum, structure, functions and how to use this driver.
 */

#include "types.h"
#include "modules.h"


#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup os_shim_timer_typedef Typedef
 * @{
 */
typedef uint32_t timer_id_t;

/**
 * @brief This function is used to define timer callback function pointer type.
 *
 * @param timer_id is timer_id of the timer related with this callback method.
 * @param arg is arg for the callback passed in os_timer_create.
 */
typedef void(*os_timer_func_t)(timer_id_t timer_id, void * arg);
/**
 * @}
 */

/**
 * @brief This function is used to create a timer.
 *
 * @param module_id is the id of module that the timer belongs to.
 * @param auto_reload is parameter passed to the function call.
 * @param cb is callback method of the timer.
 * @param arg is argument for the callback method.
 *
 * @return  0(failure), otherwise(timer id)
 */
timer_id_t os_create_timer(module_id_t module_id,
                           bool_t auto_reload,
                           os_timer_func_t cb,
                           void* arg);

/**
 * @brief os_start_timer() - start a timer. This method should not be called in ISR.
 *
 * @param id is the id of the timer to be started.
 * @param period is time period of the timer in milliseconds.
 */
void os_start_timer(timer_id_t id, uint32_t period);

/**
 * @brief This function is used to stop a timer.
 *        This method should not be called in ISR.
 *
 * @param id is the id of the timer to be stopped.
 */
void os_stop_timer(timer_id_t id);

/**
 * @brief This function is used to stop a timer from ISR.
 *        This method can be called in ISR.
 *
 * @param id is the id of the timer to be stopped.
 */
void os_stop_timer_from_isr(timer_id_t id);

/**
 * @brief This function is used to reset a timer.
 *        This method should not be called in ISR.
 *
 * @param id is the id of the timer to be checked.
 */
void os_reset_timer(timer_id_t id);

/**
 * @brief This function is used to delete a timer.
 *
 * @param id is the id of the timer to be deleted.
 */
void os_delete_timer(timer_id_t id);

/**
 * @brief This function is used to check if timer is active. Active means if
 *        started AND callback will be called later.
 *
 * @param id is the id of the timer to be reset.
 * @return 1(active), 0(not active).
 */

uint32_t os_is_timer_active(timer_id_t id);

#ifdef __cplusplus
}
#endif

/**
 * @}
 * @}
 */
#endif /* OS_SHIM_TIMER_H */
