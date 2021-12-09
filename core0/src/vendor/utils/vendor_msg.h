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

#ifndef _VENDOR_H__
#define _VENDOR_H__
/**
 * @addtogroup LIB
 * @{
 */

/**
 * @addtogroup VENDOR_MSG
 * @{
 * This section introduces the LIB VENDOR_MSG module's enum, structure, functions and how to use this module.
 */
#include "types.h"
#include "storage_controller.h"

#define DBGLOG_VENDOR_MESSAGE_ERROR(fmt, arg...) \
    DBGLOG_STREAM_ERROR(IOT_VENDOR_MESSAGE_MID, fmt, ##arg)
/** @defgroup lib_vendor_msg_enum Enum
 * @{
 */
typedef enum {
    VENDOR_CFG_INEAR_TOUCH_THRS = VENDOR_BASE_ID,
    VENDOR_CFG_MAX = VENDOR_END_ID
} VENDOR_CFG_ID;

typedef enum {
    VENDOR_MSG_TYPE_INEAR,
    VENDOR_MSG_TYPE_KEY_WQ_TOUCH,
    VENDOR_MSG_TYPE_KEY_DEBOUNCE_IO,
    VENDOR_MSG_TYPE_GSENSOR,
    VENDOR_MSG_TYPE_FORCE_TOUCH,
    VENDOR_MSG_TYPE_KEY_EXT_TOUCH,
    VENDOR_MSG_TYPE_MAX
} vendor_msg_type_t;
/**
 * @}
 */

/**
 * @brief to handle the vendor message
 *
 * @param msg_id id of the message
 * @param msg_value value of the message
 */
typedef void (*vendor_msg_handler_t)(uint8_t msg_id, uint16_t msg_value);

/**
 * @brief register a handler to handle the vendor message
 *
 * @param type type of the message
 * @param handler handler to handle the message
 */
void vendor_register_msg_handler(vendor_msg_type_t type, vendor_msg_handler_t handler);

/**
 * @brief send a vendor message
 *
 * @param type type of the message
 * @param msg_id id of the message
 * @param msg_value value of the msssage
 * @return msssage sent successfully or not
 */
bool_t vendor_send_msg(vendor_msg_type_t type, uint8_t msg_id, uint16_t msg_value);

/**
 * @brief send a vendor message from isr
 *
 * @param type type of the message
 * @param msg_id id of the message
 * @param msg_value value of the message
 * @return msssage sent successfully or not
 */
bool_t vendor_send_msg_from_isr(vendor_msg_type_t type, uint8_t msg_id, uint16_t msg_value);

/**
 * @}
 * addtogroup VENDOR_MSG
 */

/**
 * @}
 * addtogroup LIB
 */
#endif
