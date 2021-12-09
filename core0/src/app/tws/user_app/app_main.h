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

/**
 * @addtogroup APP
 * @{
 */

/**
 * @addtogroup APP_MAIN
 * @{
 * This section introduces the APP MAIN module's enum, structure, functions and how to use this module.
 */

#ifndef APP_MAIN__H_
#define APP_MAIN__H_
#include "types.h"
#include "userapp_dbglog.h"

#define DBGLOG_MAIN_DBG(fmt, ...) DBGLOG_USER_APP_INFO("[main] " fmt, ##__VA_ARGS__)
#define DBGLOG_MAIN_ERR(fmt, ...) DBGLOG_USER_APP_ERROR("[main] " fmt, ##__VA_ARGS__)

/** @defgroup app_main_enum Enum
 * @{
 */

typedef enum {
    MSG_TYPE_BT,
    MSG_TYPE_CONN,
    MSG_TYPE_WWS,
    MSG_TYPE_EVT,
    MSG_TYPE_AUDIO,
    MSG_TYPE_TONE,
    MSG_TYPE_CHARGER,
    MSG_TYPE_BAT,
    MSG_TYPE_BTN,
    MSG_TYPE_LED,
    MSG_TYPE_PM,
    MSG_TYPE_INEAR,
    MSG_TYPE_RO_CFG,
    MSG_TYPE_USR_CFG,
    MSG_TYPE_ECONN,
    MSG_TYPE_CLI,
    MSG_TYPE_OTA,
    MSG_TYPE_SYNC,
    MSG_TYPE_MAIN,
    MSG_TYPE_MAX,
} app_msg_type_t;
/**
 * @}
 */

/**
 * @brief function to handle app message
 *
 * @param msg_id message id
 * @param param message parameters
 */

typedef void (*app_msg_handler_t)(uint16_t msg_id, void *param);

/**
 * @brief main entry for app code
 *
 * @return 0 for success, else for the error code
 */
uint32_t app_main_entry(void);

/**
 * @brief deinit the app modules
 */
void app_deinit(void);

/**
 * @brief register handler for messages specified by the type
 *
 * @param type type of the messages
 * @param handler for the messages
 *
 * @return int 0 for success, else for the error reason
 */
int app_register_msg_handler(app_msg_type_t type, app_msg_handler_t handler);

/**
 * @brief send message to app module
 *
 * @param type type of the message
 * @param id id of the message
 * @param param param of the message
 * @param param_len length of the message param, 0-APP_MAX_PARAM_LEN
 *
 * @return int 0 for success, else for the error reason
 */
int app_send_msg(app_msg_type_t type, uint16_t id, const void *param, uint16_t param_len);

/**
 * @brief send message to app module after a delay
 *
 * @param type type of the message
 * @param id id of the message
 * @param param param of the message
 * @param param_len length of the message param, 0-APP_MAX_PARAM_LEN
 * @param delay_ms the delay in millisecond
 *
 * @return int 0 for success, else for the error reason
 */
int app_send_msg_delay(app_msg_type_t type, uint16_t id, const void *param, uint16_t param_len,
                       uint32_t delay_ms);

/**
 * @brief cancel a message send by app_send_msg_delay
 *
 * @param type type of the message
 * @param id id of the message
 *
 * @return int 0 for success, else for the error reason
 */
int app_cancel_msg(app_msg_type_t type, uint16_t id);

/**
 * @brief private function to handle pending messages
 */
void app_handle_pending_message(void);

/**
 * @}
 * addtogroup APP_MAIN
 */

/**
 * @}
 * addtogroup APP
 */

#endif /* APP_MAIN__H_ */
