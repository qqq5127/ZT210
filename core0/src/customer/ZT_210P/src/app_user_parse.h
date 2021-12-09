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

#ifndef _APP_USER_PARSE_H_
#define _APP_USER_PARSE_H_
#include "types.h"
#include "userapp_dbglog.h"

#define DBGLOG_USER_PARSE_DBG(fmt, ...) DBGLOG_USER_APP_INFO("[user parse] " fmt, ##__VA_ARGS__)
#define DBGLOG_USER_PARSE_ERR(fmt, ...) DBGLOG_USER_APP_ERROR("[user parse] " fmt, ##__VA_ARGS__)


typedef enum
{
		CHARGE_BOX		 = 0xA1,
    EARPHONE_LEFT  = 0xA3,		// left earphone
    EARPHONE_RIGHT = 0xA2,		// right earphone
    EARPHONE_UNKNOWN = 0xFF,	// earphone status unknown
} vbus_earphone_type_enum;


typedef enum {
    APP_PARSE_TYPE_SPP,
		APP_PARSE_TYPE_BLE,
		APP_PARSE_TYPE_UART,


} app_parse_type_e;


typedef enum {
    APP_PARSE_ENTRY_PAIR = 0x01,
		APP_PARSE_GET_PAIR_STATUS = 0x02,
		APP_PARSE_ENTRY_OTA = 0x03,
		APP_PARSE_FACTORY_RESET = 0x04,
		APP_PARSE_POWER_OFF = 0x05,
		APP_PARSE_GET_BATTERY = 0x06,
		APP_PARSE_HARDWARE_RESET = 0x07,
		
		APP_PARSE_ENTRY_FREEMAN = 0x09,
		APP_PARSE_OPEN_BOX			= 0x0A,
		APP_PARSE_CLOSE_BOX			= 0x0B,
		APP_PARSE_SOFT_RESET 		= 0x0C,

		APP_PARSE_FORCE_TOUCH_READ_ID = 0x58,		
		APP_PARSE_FORCE_TOUCH_CURVE_DATA = 0x59,

		APP_PARSE_ENTRY_DUT = 0x62,
		APP_PARSE_SET_EARSIDE = 0x77,

} app_parse_command_e;


/** @defgroup app_cli_enum Enum
 * @{
 */

void app_user_parse_data(uint8_t *data,uint8_t len,app_parse_type_e type);

#endif
