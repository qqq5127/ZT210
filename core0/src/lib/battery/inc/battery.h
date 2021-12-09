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

#ifndef LIB_BATTERY_H
#define LIB_BATTERY_H

#include "battery_charger.h"
#define DBGLOG_LIB_BATTERY_INFO(fmt, arg...)     DBGLOG_LIB_INFO(fmt, ##arg)

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief This function is used to get battery voltage.
 *
 * @return uint16_t is valtage.
 */
uint16_t battery_get_voltage_mv(void);

/**
 * @brief This function is used to set battery bat bad or not.
 *
 * @param bad is to set battery status.
 */
void battery_set_bat_bad(bool_t bad);

/**
 * @brief This function is battery bat bad or not.
 *
 * @return bool_t is show battery status.
 */
bool_t battery_bat_is_bad(void);

/**
 * @brief This function is used to init battery.
 *
 */
void battery_init(void);


#ifdef __cplusplus
}
#endif

#endif /* LIB_BATTERY_H */
