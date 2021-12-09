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
 * @addtogroup APP_CLI
 * @{
 * This section introduces the APP CLI module's enum, structure, functions and how to use this module.
 */

#ifndef _APP_CLI_H_
#define _APP_CLI_H_
#include "types.h"
#include "userapp_dbglog.h"

#define DBGLOG_CLI_DBG(fmt, ...) DBGLOG_USER_APP_INFO("[cli] " fmt, ##__VA_ARGS__)
#define DBGLOG_CLI_ERR(fmt, ...) DBGLOG_USER_APP_ERROR("[cli] " fmt, ##__VA_ARGS__)

/** @defgroup app_cli_enum Enum
 * @{
 */

typedef enum {
    APP_CLI_MSGID_SEND_USER_EVENT,
    APP_CLI_MSGID_SEND_APP_MSG,
    APP_CLI_MSGID_SEND_BT_RPC_CMD,
    APP_CLI_MSGID_SEND_HCI_CMD,
    APP_CLI_MSGID_GET_BDADDR,
    APP_CLI_MSGID_TWS_PAIR,
    APP_CLI_MSGID_SEND_HCI_DATA,
    APP_CLI_MSGID_ENABLE_HCI_PASSTHROUGH,
    APP_CLI_MSGID_GET_TWS_STATE,
    APP_CLI_MSGID_GET_BATTERY_STATE,
    APP_CLI_MSGID_AT_TEST,
    APP_CLI_MSGID_GET_KEY_STATE,
    APP_CLI_MSGID_CONTROL_LED,
    APP_CLI_MSGID_SET_PEER_ADDR,
    APP_CLI_MSGID_GET_OEM_BDADDR,
    APP_CLI_MSGID_EN_BATTERY_REPORT,
    APP_CLI_MSG_ID_GET_TOUCH_KEY_INFO,
    APP_CLI_MSG_ID_GET_TOUCH_INEAR_CDC,
    APP_CLI_MSGID_EN_TONE,
    APP_CLI_MSGID_GET_NTC_VOLT,
    APP_CLI_MSGID_GET_OEM_PPM,
    APP_CLI_MSGID_GET_BTN_PRESSED,
    APP_CLI_MSGID_GET_BT_NAME,
    APP_CLI_MSGID_GET_CUSTOM_VER,
    APP_CLI_MSGID_GET_BAT,
    APP_CLI_MSGID_GET_INEAR,
    APP_CLI_MSGID_GET_ANC,
} app_cli_msg_id_e;
/**
 * @}
 */

/**
 * @brief private function to handle hci event
 * @param evt the event code
 * @param param_len length of event param
 * @param param the param
 */
void app_cli_handle_hci_evt(uint8_t evt, uint8_t param_len, uint8_t *param);

/**
 * @brief private function to handle hci data
 *
 * @param data_len length of the hci data
 * @param data the hci data
 */
void app_cli_handle_hci_data(uint16_t data_len, uint8_t *data);

/**
 * @brief init the app_cli module
 */
void app_cli_init(void);

/**
 * @brief deinit the app_cli module
 */
void app_cli_deinit(void);

/**
 * @brief private function to handle system state changed event
 */
void app_cli_handle_sys_state(uint32_t state);

/**
 * @}
 * addtogroup APP_CLI
 */

/**
 * @}
 * addtogroup APP
 */

#endif
