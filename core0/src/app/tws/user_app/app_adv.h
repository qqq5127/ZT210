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
#ifndef _APP_ADV_H__
#define _APP_ADV_H__

/**
 * @addtogroup APP
 * @{
 */

/**
 * @addtogroup APP_ADV
 * @{
 * This section introduces the APP ADV module's enum, structure, functions and how to use this module.
 */

#include "types.h"
#include "userapp_dbglog.h"

#define DBGLOG_ADV_DBG(fmt, ...) DBGLOG_USER_APP_INFO("[adv] " fmt, ##__VA_ARGS__)
#define DBGLOG_ADV_ERR(fmt, ...) DBGLOG_USER_APP_ERROR("[adv] " fmt, ##__VA_ARGS__)

/**
 * @brief enable/disable ble advertisement
 *
 * @param enabled true if enable, false if disable
 */
void app_adv_set_enabled(bool_t enabled);

/**
 * @brief set the interval of ble advertisement
 *
 * @param interval_min advertisement minimum interval.Range: 0x0020 to 0x4000
 * @param interval_max advertisement maxinum interval,interval_max >= interval_min.Range: 0x0020 to 0x4000
 */
void app_adv_set_interval(uint16_t interval_min, uint16_t interval_max);

/**
 * @brief set the data of ble advertisement
 *
 * @param data advertisement data
 * @param data_len length of the data
 */
void app_adv_set_adv_data(const uint8_t *data, uint8_t data_len);

/**
 * @brief set scan response of ble advertisement
 *
 * @param response the scan response data
 * @param reseponse_len length of the scan response
 */
void app_adv_set_scan_response(const uint8_t *response, uint8_t reseponse_len);

/**
 * @brief set local random address
 *
 * @param address local random address
 */
void app_adv_set_random_address(const uint8_t address[6]);

/**
 * @brief private function to handle bt power on event
 */
void app_adv_handle_bt_power_on(void);

/**
 * @brief private function to handle gatts register done event
 */
void app_adv_handle_gatts_register_done(void);

/**
 * @brief private function to handle ble connected event
 */
void app_adv_handle_ble_connected(void);

/**
 * @brief private function to handle ble disconnected event
 */
void app_adv_handle_ble_disconnected(void);

/**
 * @}
 * addtogroup APP_ADV
 */

/**
 * @}
 * addtogroup APP
 */

#endif   //_APP_ADV_H__
