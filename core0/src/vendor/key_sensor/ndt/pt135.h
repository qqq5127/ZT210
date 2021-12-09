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
#ifndef __PT135_H__
#define __PT135_H__

/**
 * @addtogroup LIB
 * @{
 */

/**
 * @addtogroup PRESSURE_SENSOR
 * @{
 * This section introduces the LIB NTC_SENSOR module's enum, structure, functions and how to use this module.
 */
#include "types.h"
#include "key_sensor.h"

#if KEY_DRIVER_SELECTION == KEY_DRIVER_NDT_PT135

/**
 * @brief init the pressure sensor
 *
 */
void pt135_init(key_callback_t callback);

/**
 * @brief dinit the pressure sensor
 *
 */
void pt135_deinit(bool_t wakeup_enable);

#endif /* KEY_DRIVER_SELECTION == KEY_DRIVER_NDT_PT135 */

/**
 * @}
 * addtogroup PT135
 */

/**
 * @}
 * addtogroup KEY_SENSOR
 */

/**
 * @}
 * addtogroup LIB
 */
#endif

