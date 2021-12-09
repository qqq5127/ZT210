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

#ifndef _APP_BAT_H__
#define _APP_BAT_H__

/**
 * @addtogroup APP
 * @{
 */

/**
 * @addtogroup APP_BAT
 * @{
 * This section introduces the APP BAT module's enum, structure, functions and how to use this module.
 */

#include "types.h"
#include "userapp_dbglog.h"

#define DBGLOG_BAT_DBG(fmt, ...) DBGLOG_USER_APP_INFO("[bat] " fmt, ##__VA_ARGS__)
#define DBGLOG_BAT_ERR(fmt, ...) DBGLOG_USER_APP_ERROR("[bat] " fmt, ##__VA_ARGS__)

#define UVP_VOLT_MV       3100
#define BAT_VOLT_INVALID  0xFFFF
#define BAT_LEVEL_INVALID 255

/**
 * @brief init the app battery module
 */
void app_bat_init(void);

/**
 * @brief deinit the app battery module
 */
void app_bat_deinit(void);

/**
 * @brief get current battery level
 *
 * @return current battery level, 0-100
 */
uint8_t app_bat_get_level(void);

/**
 * @brief check if battery low or not
 *
 * @return true if battery is low, false if not.
 */
bool_t app_bat_is_low(void);

/**
 * @brief check if battery is critical low
 *
 * @return true if battery is critical low, false if not.
 */
bool_t app_bat_is_critical_low(void);

/**
 * @brief check if battery is full
 *
 * @return true if battery is full, false if not.
 */
bool_t app_bat_is_full(void);

/**
 * @brief set virtual volt, BAT_VOLT_INVALID is used to clear the virtual volt
 *
 * @param volt virtual volt in mV, default is BAT_VOLT_INVALID
 */
void app_bat_set_virtual_volt(uint16_t volt);

/**
 * @brief get virtual volt
 *
 * @return volt virtual volt in mV
 */
uint16_t app_bat_get_virtual_volt(void);

/**
 * @brief enable or disable device reporting battery level to peer and phone
 *
 * @param en enable or not.
 */
void app_bat_enable_report(bool_t en);

/**
 * @brief private function to handle change changed event
 *
 * @param charging true if charging, false if not
 */
void app_bat_handle_charge_changed(bool_t charging);

/**
 * @}
 * addtogroup APP_BAT
 */

/**
 * @}
 * addtogroup APP
 */

#endif   //_APP_BAT_H__
