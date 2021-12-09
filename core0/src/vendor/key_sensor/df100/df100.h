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

#ifndef _DF_100_H__
#define _DF_100_H__

/**
 * @addtogroup LIB
 * @{
 */

/**
 * @addtogroup KEY_SENSOR
 * @{
 * This section introduces the LIB KEY_SENSOR module's enum, structure, functions and how to use this module.
 */

/**
 * @addtogroup DF100
 * @{
 * This section introduces the LIB KEY_SENSOR DF100 module's enum, structure, functions and how to use this module.
 */

#include "types.h"
#include "key_sensor.h"

/**
 * @brief This function is init df100 driver.
 * @param callback to handle key press
 */
void df100_init(key_callback_t callback);
/**
 * @brief This function is deinit df100 driver.
 *
 */
void df100_deinit(bool_t wakeup_enable);

/**
 * @}
 * addtogroup DF100
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
