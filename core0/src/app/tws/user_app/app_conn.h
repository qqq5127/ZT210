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
 * @addtogroup APP_CONN
 * @{
 * This section introduces the APP CONN module's enum, structure, functions and how to use this module.
 */

#ifndef _APP_CONN_H__
#define _APP_CONN_H__
#include "types.h"
#include "userapp_dbglog.h"
#include "app_bt.h"

#define DBGLOG_CONN_DBG(fmt, ...) DBGLOG_USER_APP_INFO("[conn] " fmt, ##__VA_ARGS__)
#define DBGLOG_CONN_ERR(fmt, ...) DBGLOG_USER_APP_ERROR("[conn] " fmt, ##__VA_ARGS__)

typedef enum {
    APP_CONN_POWER_ON_ACTION_RECONENCT,
    APP_CONN_POWER_ON_ACTION_PAIRING,
} app_conn_power_on_action_e;

typedef enum {
    APP_CONN_ROLE_CHANGE_ACTION_RECONNECT,
    APP_CONN_ROLE_CHANGE_ACTION_PAIRING,
} app_conn_role_change_action_e;

/**
 * @brief init the connection manager
 */
void app_conn_init(void);

/**
 * @brief deinit the connection manager
 */
void app_conn_deinit(void);

/**
 * @brief connect to the last device
 */
void app_conn_connect_last(void);

/**
 * @brief disconnect the current connection
 */
void app_conn_disconnect(void);

/**
 * @brief handler for bt power on event
 */
void app_conn_handle_bt_power_on(void);

/**
 * @brief handler for bt power off event
 */
void app_conn_handle_bt_power_off(void);

/**
 * @brief handler for bt connection state changed event
 * @param connected connected or not
 * @param reason the reason for the disconnection
 */
void app_conn_handle_bt_conn_state(bool_t connected, uint8_t reason);

/**
 * @brief handler for wws role changed event
 *
 * @param is_master true if new role is master, false if not
 * @param reason role changed reason
 */
void app_conn_handle_wws_role_changed(bool_t is_master, tws_role_changed_reason_t reason);

/**
 * @brief get disconnect reason
 *
 * @return @see BT_DISCONNECT_REASON_XXX
 */
uint8_t app_conn_get_disconnect_reason(void);

/**
 * @brief set connection action after power on
 *
 * @param action the action after power on
 */
void app_conn_set_power_on_action(app_conn_power_on_action_e action);

/**
 * @brief set connection action after role change
 *
 * @param action the action after role change
 */
void app_conn_set_role_change_action(app_conn_role_change_action_e action);
/**
 * @}
 * addtogroup APP_CONN
 */

/**
 * @}
 * addtogroup APP
 */

#endif
