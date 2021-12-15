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

#ifndef IOT_SHARE_TASK_H
#define IOT_SHARE_TASK_H

/* os shim includes */
#include "types.h"


/**
 * @addtogroup LIB
 * @{
 */

/**
 * @addtogroup SHARE_TASK
 * @{
 * This section introduces the share task module's functions and how to use this module.
 */

#ifdef __cplusplus
extern "C" {
#endif

/* define priorities for message to be handle */
#define IOT_SHARE_TASK_QUEUE_HP         0
#define IOT_SHARE_TASK_QUEUE_LP         1

/* Event ID NOT the Message ID */
typedef enum {
    IOT_SHARE_EVENT_START = 0,
    IOT_SHARE_EVENT_MSG_EVENT = 0,
    IOT_SHARE_EVENT_CPU_USAGE_EVENT = 1,
    IOT_SHARE_EVENT_KV_CACHE_EVENT = 2,
    IOT_SHARE_EVENT_CHARGER_EVENT = 3,
    IOT_SHARE_EVENT_DBGLOG_CACHE_EVENT = 4,
    IOT_SHARE_EVENT_SPK_MIC_DUMP_EVENT,
    IOT_SHARE_EVENT_SPK_SINE_TONE_EVENT,
    IOT_SHARE_EVENT_CHARGER_CMC_EVENT,
    IOT_SHARE_EVENT_CFG_ANC_EVENT,
    IOT_SHARE_EVENT_END = 32,
} iot_share_event_type;

typedef void (*iot_share_event_func)(void *);

/**
 * @brief This function is to init share task and to create the background.
 *
 * @return uint32_t RET_OK for success else error.
 */
uint32_t iot_share_task_init(void);

/**
 * @brief This function is to deinit share task and to delete the background share task.
 *
 */
void iot_share_task_deinit(void);

/**
 * @brief This function is to bind a event with a callback function.
 *
 * @param prio is priority to handle this message.
 * @param type is the event type.
 * @param func is the callback function.
 * @param arg is argument for the callback function.
 * @return uint32_t RET_OK for success else error.
 */
uint32_t iot_share_task_event_register(uint32_t prio,
                                        iot_share_event_type type,
                                        iot_share_event_func func,
                                        void *arg);

/**
 * @brief This function is to unbind a event with a callback function.
 *
 * @param prio is priority to handle this message.
 * @param type is the event type.
 * @return uint32_t RET_OK for success else error.
 */
uint32_t iot_share_task_event_unregister(uint32_t prio,
                                        iot_share_event_type type);

/**
 * @brief This function is to post event to share task from isr,caller is an ISR.
 *
 * @param prio is priority to handle this message.
 * @param type is the event type.
 * @return uint32_t RET_OK for success else error.
 */
uint32_t iot_share_task_post_event_from_isr
    (uint32_t prio, iot_share_event_type type);

/**
 * @brief This function is to post event to share task.
 *
 * @param prio is priority to handle this message.
 * @param type is the event type.
 * @return uint32_t RET_OK for success else error.
 */
uint32_t iot_share_task_post_event
    (uint32_t prio, iot_share_event_type type);

/**
 * @brief This function is to post message to share task.
 *
 * @param prio is priority to handle this message.
 * @param msg_id is the message id.
 * @param data is the address of message data.
 * @return bool_t true for success else false.
 */
bool_t iot_share_task_post_msg(uint32_t prio, int32_t msg_id, void *data);

/**
 * @brief This function is to post message to share task from isr.
 *
 * @param prio is priority to handle this message.
 * @param msg_id is the message id.
 * @param data is the address of message data.
 * @return bool_t true for success else false.
 */
bool_t iot_share_task_post_msg_from_isr(uint32_t prio, int32_t msg_id, void *data);

/**
 * @brief This function is to register message in share task.
 *
 * @param exec_func is share event function.
 * @return int32_t 0 for fail else is message id.
 */
int32_t iot_share_task_msg_register(iot_share_event_func exec_func);

/**
 * @brief This function is to unregister message in share task.
 *
 * @param msg_id is the message id.
 */
void iot_share_task_msg_unregister(int32_t msg_id);

#ifdef __cplusplus
}
#endif

/**
 * @}
 * addtogroup SHARE_TASK
 */

/**
* @}
 * addtogroup LIB
*/


#endif /* IOT_SHARE_TASK_H */
