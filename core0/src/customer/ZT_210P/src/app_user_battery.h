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
 * @addtogroup _APP_USER_SPP_H_
 * @{
 * This section introduces the APP _APP_USER_SPP_H_ module's enum, structure, functions and how to use this module.
 */

#ifndef _APP_USER_BATTERY_H_
#define _APP_USER_BATTERY_H_
#include "types.h"
#include "userapp_dbglog.h"
#include "iot_gpio.h"

#define BATTERY_CTRL_GPIO 	IOT_GPIO_72


#define DBGLOG_USER_BATTERY_DBG(fmt, ...) DBGLOG_USER_APP_INFO("[user battery] " fmt, ##__VA_ARGS__)
#define DBGLOG_USER_BATTERY_ERR(fmt, ...) DBGLOG_USER_APP_ERROR("[user battery] " fmt, ##__VA_ARGS__)

void app_user_battery_init(void);
void app_user_battery_off(void);
void app_user_battery_ntc_check(int8_t value);


#endif
