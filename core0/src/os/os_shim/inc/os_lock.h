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

#ifndef OS_SHIM_LOCK_H
#define OS_SHIM_LOCK_H
/**
 * @addtogroup OS
 * @{
 * @addtogroup OS_SHIM
 * @{
 * @addtogroup OS_LOCK
 * @{
 * This section introduces OS lock reference api.
 */

/* common includes */
#include "modules.h"

#ifdef __cplusplus
extern "C" {
#endif

/* mutex handle definition */
typedef void *os_mutex_h;

/* sempahore handle definition */
typedef void *os_sem_h;

/**
 * @brief This function is to create a mutex.
 *
 * @param module_id is the module that creates the event.
 * @return os_mutex_h NULL -- for failure case otherwise -- mutex handle
 */
os_mutex_h os_create_mutex(module_id_t module_id);

/**
 * @brief  This function is to acquire a mutex. This function can be called recursively from one task.
 * For each call, a corresponding release call is required to release the mutex.
 * This function can NOT be called in ISR context.
 *
 * @param mutex is mutex to be acquired
 */
void os_acquire_mutex(os_mutex_h mutex);

/**
 * @brief This function is to try to acquire a mutex. This function is
 * not blocking when called. This function can NOT be called in ISR context.
 *
 * @param mutex is mutex to be acquired
 * @return true --  acquire mutex successfully,false -- failed.
 */
bool_t os_try_acquire_mutex(os_mutex_h mutex);

/**
 * @brief This function is to release a mutex This function can NOT be called
 * in ISR context.
 *
 * @param mutex is mutex to be released
 */
void os_release_mutex(os_mutex_h mutex);

/**
 * @brief This function is to delete a mutex
 *
 * @param mutex is mutex to be deleted
 */
void os_delete_mutex(os_mutex_h mutex);

/**
 * @brief This function is to create a count semaphore.
 *
 * @param module_id is the module that creates the semaphore.
 * @param max_count is max count for this semaphore.
 * @param init_count is init count for this semaphore.
 *
 * @return NULL -- for failure case otherwise -- semaphore handle
 */
os_sem_h os_create_semaphore(module_id_t module_id, uint32_t max_count,
    uint32_t init_count);

/**
 * @brief This function is to pend to take a semaphore.
 *  This function can NOT be called in ISR context.
 *
 * @param semaphore is semaphore to be take.
 * @param timeout is timeout time(ms).
 * @return bool_t true -- take semaphore successfully, false - timeout.
 */
bool_t os_pend_semaphore(os_sem_h semaphore, uint32_t timeout);

/**
 * @brief This function is to  post a semaphore.
 * This function can NOT be called in ISR context.
 *
 * @param semaphore is semaphore to be posted
 * @return bool_t true -- post semaphore successfully, false - failed.
 */
bool_t os_post_semaphore(os_sem_h semaphore);

/**
 * @brief This function is to  post a semaphore.
 * This function can only be called in ISR context.
 *
 * @param semaphore is semaphore to be posted
 * @return bool_t true -- post semaphore successfully, false - failed.
 */
bool_t os_post_semaphore_from_isr(os_sem_h semaphore);

/**
 * @brief This function is to  post a semaphore.
 * This function can only be called in critical context.
 *
 * @param semaphore is semaphore to be posted
 * @return bool_t true -- post semaphore successfully, false - failed.
 */
bool_t os_post_semaphore_from_critical(os_sem_h semaphore);

/**
 * @brief This function is to delete a semaphore.
 *
 * @param semaphore is semaphore to be deleted
 */
void os_delete_semaphore(os_sem_h semaphore);

/**
 * @brief This function is turn off interrupt function
 */
void os_critical_enter(void);

/**
 * @brief  This function is turn on interrupt function
 */
void os_critical_exit(void);

#ifdef __cplusplus
}
#endif

/**
 * @}
 * addtogroup OS_LOCK
 * @}
 * addtogroup OS_SHIM
 * @}
 * addtogroup OS
 */

#endif /* OS_SHIM_LOCK_H */
