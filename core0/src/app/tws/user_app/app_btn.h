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
 * @addtogroup APP_BTN
 * @{
 * This section introduces the APP BTN module's enum, structure, functions and how to use this module.
 */

#ifndef _APP_BTN_H_
#define _APP_BTN_H_

#include "types.h"
#include "userapp_dbglog.h"
#include "key_sensor.h"

#define DBGLOG_BTN_DBG(fmt, ...) DBGLOG_USER_APP_INFO("[btn] " fmt, ##__VA_ARGS__)
#define DBGLOG_BTN_ERR(fmt, ...) DBGLOG_USER_APP_ERROR("[btn] " fmt, ##__VA_ARGS__)

/**
 * @brief This function is init app btn module.
 */
void app_btn_init(void);

/**
 * @brief This function is deinit app btn module.
 */
void app_btn_deinit(void);

/**
 * @brief This function is for checking if all keys released.
 *
 * @return bool_t true or false for checking all released.
 */
bool_t app_btn_all_released(void);

/**
 * @brief This function is for register custom key event.
 *
 * @param id the key id
 * @param src the key src
 * @param type the key type
 * @param state_mask system state mask
 * @param event 0 for clear the setting, else for the event to trigger
 *
 * @return see enum RET_TYPE.
 */
int app_btn_cus_key_add_with_id(uint8_t id, uint8_t src, key_pressed_type_t type,
                                uint16_t state_mask, uint16_t event);

/**
 * @brief This function is for register custom key event.
 *
 * @param type the key type
 * @param state_mask system state mask
 * @param event 0 for clear the setting, else for the event to trigger
 *
 * @return see enum RET_TYPE.
 */
int app_btn_cus_key_add(key_pressed_type_t type, uint16_t state_mask, uint16_t event);

/**
 * @brief This function is for reading custom key event.
 *
 * @param id the key id
 * @param src the key src
 * @param type the key type
 * @param state_mask system state mask
 *
 * @return event id.
 */
uint16_t app_btn_cus_key_read_with_id(uint8_t id, uint8_t src, key_pressed_type_t type,
                                      uint16_t state_mask);

/**
 * @brief This function is for reading custom key event.
 *
 * @param type the key type
 * @param state_mask system state mask
 *
 * @return event id.
 */
uint16_t app_btn_cus_key_read(key_pressed_type_t type, uint16_t state_mask);

/**
 * @brief This function is for clear all customized user key event entries.
 *
 * @return see enum RET_TYPE.
 */
int app_btn_cus_key_reset(void);

/**
 * @brief send an virtual btn evt
 *
 * @param key_id key id
 * @param key_src @see KEY_SRC_XXX
 * @param key_type @see key_pressed_type_t
 *
 */
void app_btn_send_virtual_btn(uint8_t key_id, uint8_t key_src, key_pressed_type_t key_type);

/**
 * @brief private function to open key sensor after init
 */
void app_btn_open_sensor(void);

/**
 * @brief check if button has power on function
 *
 * @return true if button power on function is enabled, false if not
 */
bool_t app_btn_has_btn_power_on(void);

/**
 * @brief get the last button type which triggered a user event
 *
 * @return last button type
 */
key_pressed_type_t app_btn_get_last_triggered_type(void);

/**
 * @}
 * addtogroup APP_BTN
 */

/**
 * @}
 * addtogroup APP
 */

#endif
