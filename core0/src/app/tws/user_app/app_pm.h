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
 * @addtogroup APP_PM
 * @{
 * This section introduces the APP PM module's enum, structure, functions and how to use this module.
 */

#ifndef _APP_PM_H_
#define _APP_PM_H_
#include "types.h"
#include "userapp_dbglog.h"

#define DBGLOG_PM_DBG(fmt, ...) DBGLOG_USER_APP_INFO("[pm] " fmt, ##__VA_ARGS__)
#define DBGLOG_PM_ERR(fmt, ...) DBGLOG_USER_APP_ERROR("[pm] " fmt, ##__VA_ARGS__)

/** @defgroup app_pm_enum Enum
 * @{
 */

typedef enum {
    PM_POWER_ON_REASON_UNKNOWN,
    PM_POWER_ON_REASON_WDT_CRASH,
    PM_POWER_ON_REASON_PWR_RESET,
    PM_POWER_ON_REASON_GPIO,
    PM_POWER_ON_REASON_CHARGER,
    PM_POWER_ON_REASON_REBOOT,
    PM_POWER_ON_REASON_FACTORY_RESET,
    PM_POWER_ON_REASON_OTA,
    PM_POWER_ON_REASON_HW_RESET,
} pm_power_on_reason_t;

typedef enum {
    PM_REBOOT_REASON_USER,
    PM_REBOOT_REASON_CHARGER,
    PM_REBOOT_REASON_FACTORY_RESET,
    PM_REBOOT_REASON_OTA,
} pm_reboot_reason_t;
/**
 * @}
 */

/**
 * @brief init the app power management module
 */
void app_pm_init(void);

/**
 * @brief deinit the app power management module
 */
void app_pm_deinit(void);

/**
 * @brief power on, enable bluetooth
 */
void app_pm_power_on(void);

/**
 * @brief power off, disable bluetooth and shutdown
 */
void app_pm_power_off(void);

/**
 * @brief reboot
 *
 * @param reason reboot reason
 */
void app_pm_reboot(pm_reboot_reason_t reason);

/**
 * @brief get power on reason
 *
 * @return the reason of power on
 */
pm_power_on_reason_t app_pm_get_power_on_reason(void);

/**
 * @brief get app_pm power off if enabled
 *
 * @return false:disable  true:enabled
 */
bool_t app_pm_is_power_off_enabled(void);

/**
 * @brief temporarily set auto power off timeout, the timeout will not be saved
 *
 * @param timeout_s auto power timeout
 */
void app_pm_set_auto_power_off_timeout(uint16_t timeout_s);

/**
 * @brief check if shutdown running
 *
 * @return true if shutding down, false if not
 */
bool_t app_pm_is_shuting_down(void);

/**
 * @brief handler for bt connected event
 */
void app_pm_handle_bt_connected(void);

/**
 * @brief handler for bt disconnected event
 */
void app_pm_handle_bt_disconnected(void);

/**
 * @brief handler for bt power on event
 */
void app_pm_handle_bt_power_on(void);

/**
 * @brief handler for bt power off event
 */
void app_pm_handle_bt_power_off(void);

/**
 * @brief handler for battery full event
 */
void app_pm_handle_battery_full(void);

/**
 * @brief handler for battery critical low event
 */
void app_pm_handle_battery_critical_low(void);

/**
 * @brief handle charge off event
 */
void app_pm_handle_charge_off(void);

/**
 * @brief handle disable bt command by charger
 */
void app_pm_handle_charger_disable_bt(void);

/**
 * @brief enable/disble auto power off
 *
 * @param enabled true if enable auto power off when bluetooth disconnected, false if not
 */
void app_pm_set_auto_power_off_enabled(bool_t enabled);
/**
 * @}
 * addtogroup APP_PM
 */

/**
 * @}
 * addtogroup APP
 */

#endif
