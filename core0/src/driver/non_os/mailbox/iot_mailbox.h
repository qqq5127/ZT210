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

#ifndef _DRIVER_HAL_MAILBOX_H_
#define _DRIVER_HAL_MAILBOX_H_

/**
 * @addtogroup HAL
 * @{
 * @addtogroup MAILBOX
 * @{
 * This section introduces the MAILBOX module's enum, functions and how to use this driver.
 */

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup hal_mailbox_enum Enum
 * @{
 */

/** @brief MAILBOX channel. */
typedef enum {
    IOT_MAILBOX_CH_0 = 0,
    IOT_MAILBOX_CH_1 = 1,
    IOT_MAILBOX_CH_2 = 2,
    IOT_MAILBOX_CH_3 = 3,
    IOT_MAILBOX_CH_4 = 4,
    IOT_MAILBOX_CH_5 = 5,
    IOT_MAILBOX_CH_6 = 6,
    IOT_MAILBOX_CH_7 = 7,
    IOT_MAILBOX_CH_MAX = 8,
} IOT_MAILBOX_CH;

/** @brief MAILBOX core. */
typedef enum {
    IOT_MAILBOX_CORE0 = 0, /*core0*/
    IOT_MAILBOX_CORE1 = 1, /*core1*/
    IOT_MAILBOX_CORE2 = 2, /*core2*/
    IOT_MAILBOX_CORE3 = 3, /*core3*/
    IOT_MAILBOX_CORE_MAX = 4,
} IOT_MAILBOX_CORE;
/**
 * @}
 */

/**
 * @defgroup hal_mailbox_typedef Typedef
 * @{
 */
typedef void (*iot_mailbox_msg_cb)(void);
/**
 * @}
 */

/**
 * @brief This function is to init mailbox module.
 *
 * @param iot_mailbox_id is mailbox id.
 */
void iot_mailbox_init(uint8_t iot_mailbox_id);

/**
 * @brief This function is to open mailbox.
 *
 * @param target is the target mailbox to open.
 * @param cb is mailbox's message callback.
 * @return uint32_t RET_OK for success else error.
 */
uint32_t iot_mailbox_open(uint32_t target, iot_mailbox_msg_cb cb);

/**
 * @brief This function is to close mailbox.
 *
 * @param target is the target mailbox to close.
 * @return uint32_t RET_OK for success else error.
 */
uint32_t iot_mailbox_close(uint32_t target);

/**
 * @brief This function is to send data to destination.
 *
 * @param target is the destination.
 * @param data is the data to send.
 * @return uint32_t 0 for success else false.
 */
uint32_t iot_mailbox_send(uint32_t target, uint32_t data);

/**
 * @brief This function is to read data from target.
 *
 * @param target is the destination.
 * @param p_data is the address of the data to read.
 * @return uint32_t is the num of the readed data.
 */
uint32_t iot_mailbox_read(uint32_t target, uint32_t *p_data);

/**
 * @brief This function is to get the num of the message from the destination.
 *
 * @param target is the destination.
 * @return uint32_t is the num of message.
 */
uint32_t iot_mailbox_get_msg_num(uint32_t target);

/**
 * @brief This function is to disable mailbox rev interrupt from other core.
 *
 * @param target is the destination.
 * @return uint32_t RET_OK for success else error.
 */
uint32_t iot_mailbox_mask(uint32_t target);

/**
 * @brief This function is to enable mailbox rev interrupt from other core.
 *
 * @param target is the destination.
 * @return uint32_t RET_OK for success else error.
 */
uint32_t iot_mailbox_unmask(uint32_t target);

/**
 * @brief This function is to clean mailbox recently.
 *
 * @param source is the destination.
 * @return uint32_t 0 for success else false.
 */
uint32_t iot_mailbox_recint_clean(uint32_t source);

#ifdef __cplusplus
}
#endif

/**
 * @}
 * @}
 */
#endif /*_DRIVER_HAL_MAILBOX_H_ */
