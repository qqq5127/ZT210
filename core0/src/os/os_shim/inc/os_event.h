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

#ifndef OS_SHIM_EVENT_H
#define OS_SHIM_EVENT_H

/**
 * @addtogroup OS
 * @{
 * @addtogroup OS_SHIM
 * @{
 * @addtogroup OS_EVENT
 * @{
 * This section introduces OS event reference api.
 */

#include "modules.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void *os_event_h;

#define MAX_TIME 0xffffffffUL

/**
 * @brief This function is to create a event.
 *
 * @param module_id is the module id that creates the event
 * @return os_event_h 0 -- for failure case otherwise -- event id
 */
os_event_h os_create_event(module_id_t module_id);

/**
 * @brief This function is to wait to take a event. This method should NOT be called in ISR.
 *
 * @param event_id is id of the event being waiting to be taken.
 * @param timeout is time to wait in millisecond for the event to be available.
 *        when timeout is set to MAX_TIME, this method return only if the event become avaliable.
 * @param recv is eventbits
 * @return bool_t TRUE -- Take the event successfully, FALSE -- Failed to take the event within ms milliseconds
 */
bool_t os_wait_event(os_event_h event_id, uint32_t timeout, uint32_t *recv);

#define EVENT_FLAG_AND               0x01            /**< logic and */
#define EVENT_FLAG_OR                0x02            /**< logic or */
#define EVENT_FLAG_CLEAR             0x04

bool_t os_wait_event_with_v(os_event_h event_id, uint32_t set, uint8_t option,
                    uint32_t timeout, uint32_t *recv);

/**
 * @brief This function is to give a event. This method should NOT be called in ISR.
 *
 * @param event_id is id of the event to be given.
 * @param set is setted event
 * @return bool_t  TRUE - Give the event successfully, FALSE - Failed to take the event
 */
bool_t os_set_event(os_event_h event_id, uint32_t set);

/**
 * @brief This function is to  give a event, only called in ISR.
 *
 * @param event_id is id of the event to be given
 * @param set is setted event
 * @return bool_t TRUE - Give the event successfully, FALSE - Failed to take the event
 */
bool_t os_set_event_isr(os_event_h event_id, uint32_t set);

/**
 * @brief This function is to delete a event.
 *
 * @param event_id is id of the event to be deleted
 */
void os_delete_event(os_event_h event_id);

/**
 * @brief to clear event
 *
 * @param event_id is id of the event to be cleared
 * @param set is cleared event
 */
void os_clear_event(os_event_h event_id, uint32_t set);

/**
 * @brief called from ISR to clear event
 *
 * @param event_id is id of the event to be cleared
 * @param set is cleared event
 */
void os_clear_event_isr(os_event_h event_id, uint32_t set);

#ifdef __cplusplus
}
#endif
/**
 * @}
 * addtogroup OS_EVENT
 * @}
 * addtogroup OS_SHIM
 * @}
 * addtogroup OS
 */
#endif /* OS_SHIM_EVENT_H */
