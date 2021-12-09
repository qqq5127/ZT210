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
 * @addtogroup APP_LED
 * @{
 * This section introduces the APP LED module's enum, structure, functions and how to use this module.
 */

#ifndef _APP_LED_H_
#define _APP_LED_H_

#include "types.h"
#include "userapp_dbglog.h"

#define DBGLOG_LED_DBG(fmt, ...) DBGLOG_USER_APP_INFO("[led] " fmt, ##__VA_ARGS__)
#define DBGLOG_LED_ERR(fmt, ...) DBGLOG_USER_APP_ERROR("[led] " fmt, ##__VA_ARGS__)

/**
 * @brief app_led_init() - init led module
 */
void app_led_init(void);

/**
 * @brief app_led_deinit() - deinit led module
 *
 */
void app_led_deinit(void);

/**
 * @brief app_led_indicate_event() -- process led indicate event.
 *
 * @param event the event id to process.

 */
void app_led_indicate_event(uint32_t event);

/**
 * @brief app_led_indicate_state() - set led patten indicate new state
 *
 * @param state system state id to process
 */
void app_led_indicate_state(uint32_t state);

/**
 * @brief check if any event led is playing
 *
 * @return true if there is a led playing, false if not
 */
bool_t app_led_is_event_playing(void);

/**
 * @brief set custom state to replace the state led
 *
 * @param state STATE_CUSTOM_XXX, 0 for disable custom state, default is 0
 */
void app_led_set_custom_state(uint32_t state);

/*
 * @brief enable/disable all led
 *
 * @param enable true for enable, false for disable
 */
void app_led_set_enabled(bool_t enable);

/**
 * @brief enable/disable state led
 *
 * @param enable true for enable, false for disable
 */
void app_led_set_state_led_enabled(bool_t enable);

/**
 * @brief private function to handle tws state
 *
 * @param connected true if connected, false if not
 */
void app_led_handle_wws_state_changed(bool_t connected);

/**
 * @brief private function to handle tws role changed event
 *
 * @param is_master true if current role is master, false if not
 */
void app_led_handle_wws_role_changed(bool_t is_master);
/**
 * @}
 * addtogroup APP_LED
 */

/**
 * @}
 * addtogroup APP
 */

#endif
