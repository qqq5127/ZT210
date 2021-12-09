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

#ifndef OS_SHIM_QUEUE_H
#define OS_SHIM_QUEUE_H

/**
 * @addtogroup OS
 * @{
 * @addtogroup OS_SHIM
 * @{
 * @addtogroup OS_QUEUE
 * @{
 * This section introduces OS QUEUE reference api.
 */

/* os shim includes */

/* common includes */
#include "types.h"
#include "modules.h"

#ifdef __cplusplus
extern "C" {
#endif

/* queue handle definition */
typedef void *os_queue_h;

/**
 * @brief This function is used to create a queue.
 *
 * @param module_id is the module that creates the queue.
 * @param queue_lenth is the module that the depth of queue.
 * @param item_size is the module that each item size.
 * @return os_queue_h NULL(failure), otherwise(queue handle).
 */
os_queue_h os_queue_create(module_id_t module_id, uint32_t queue_lenth, uint32_t item_size);

/**
 * @brief This function is used to send a queue.
 *
 * @param queue is queue handle.
 * @param queue_tx_msg is queue msg to be send.
 * @return bool_t true(successfully), false(failed).
 */
bool_t os_queue_send(os_queue_h queue, void *queue_tx_msg);

/**
 * @brief This function is used to send a queue.This function can be
 *        called in ISR context.
 *
 * @param queue is queue handle.
 * @param queue_tx_msg is queue msg to be send.
 * @return bool_t true(successfully), false(failed).
 */
bool_t os_queue_send_from_isr(os_queue_h queue, void *queue_tx_msg);

/**
 * @brief This function is used to receive a queue.This function is
 *        blocking when called.
 *
 * @param queue is queue handle.
 * @param queue_rx_msg is queue msg to be received.
 * @return bool_t true(successfully), false(failed)..
 */
bool_t os_queue_receive(os_queue_h queue, void *queue_rx_msg);

/**
 * @brief This function is used to peek a queue.This function is
 *        non-blocking when called.
 *
 * @param queue is queue handle.
 * @param queue_rx_msg is queue msg to be received.
 * @return bool_t true(successfully), false(failed).
 */
bool_t os_queue_peek(os_queue_h queue, void *queue_rx_msg);

/**
 * @brief This function is used to receive a queue with timeout.This function is
 *        blocking when called.
 *
 * @param queue is queue handle.
 * @param queue_rx_msg is queue msg to be received.
 * @param timeout max wait time.
 *
 * @return bool_t true(successfully), false(failed).
 */
bool_t os_queue_receive_timeout(os_queue_h queue, void *queue_rx_msg, uint32_t timeout);

/**
 * @brief This function is used to receive a queue.This function is
 *        blocking when called.This function can be called in ISR context.
 *
 * @param queue is queue handle.
 * @param queue_rx_msg is queue msg to be received.
 * @return bool_t trues(uccessfully), false(failed).
 */
bool_t os_queue_receive_from_isr(os_queue_h queue, void *queue_rx_msg);

/**
 * @brief This function is used to get a queue item numbers available.
 *
 * @param queue is queue handle.
 * @return uint32_t queue available items numbers.
 */
uint32_t os_queue_items_get(os_queue_h queue);

/**
 * @brief This function is used to delete a queue.
 *
 * @param queue is queue handle.
 */
void os_queue_delete(os_queue_h queue);

#ifdef __cplusplus
}
#endif

/**
 * @}
 * addtogroup OS_QUEUE
 * @}
 * addtogroup OS_SHIM
 * @}
 * addtogroup OS
 */

#endif /* OS_SHIM_QUEUE_H */
