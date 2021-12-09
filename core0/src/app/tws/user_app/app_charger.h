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
 * @addtogroup APP_CHARGER
 * @{
 * This section introduces the APP CHARGER module's enum, structure, functions and how to use this module.
 */

#ifndef _APP_CHARGER_H_
#define _APP_CHARGER_H_
#include "types.h"
#include "userapp_dbglog.h"

#define DBGLOG_CHARGER_DBG(fmt, ...) DBGLOG_USER_APP_INFO("[charge] " fmt, ##__VA_ARGS__)
#define DBGLOG_CHARGER_ERR(fmt, ...) DBGLOG_USER_APP_ERROR("[charge] " fmt, ##__VA_ARGS__)

typedef enum {
    BOX_STATE_UNKNOWN,
    BOX_STATE_OPENED,
    BOX_STATE_CLOSED,
} box_state_t;

/**
 * @brief init the app_charger module
 */
void app_charger_init(void);

/**
 * @brief deinit the app_charger module
 */
void app_charger_deinit(void);

/**
 * @brief get current charge state
 *
 * @return true if charging, false if not charging
 */
bool_t app_charger_is_charging(void);

/**
 * @brief check if box is opened
 *
 * @return true if opened, false if closed
 */
bool_t app_charger_is_box_open(void);

/**
 * @brief check if box is charging
 *
 * @return true if charging, false if closed
 */
bool_t app_charger_is_box_charging(void);

/**
 * @brief get the battery level of charger box
 *
 * @return battery level of charger box
 */
uint8_t app_charger_get_box_battery(void);

/**
 * @brief get box state
 *
 * @return current box state, unknown, opened, closed
 */
box_state_t app_charger_get_box_state(void);

/**
 * @brief send an charger event
 *
 * @param evt the event, see charger_evt_t for detail
 */
void app_charger_evt_send(uint8_t evt);

/**
 * @}
 * addtogroup APP_CHARGER
 */

/**
 * @}
 * addtogroup APP
 */

#endif
