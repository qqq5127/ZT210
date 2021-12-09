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

#ifndef _SC7A20_H__
#define _SC7A20_H__

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
 * @addtogroup SC7A20
 * @{
 * This section introduces the LIB KEY_SENSOR SC7A20 module's enum, structure, functions and how to use this module.
 */

#include "key_sensor.h"

/**
 * @brief init the sc7a20 driver
 * @param callback callback to handle key press
 */
void sc7a20_init(key_callback_t callback);

/**
 * @brief deinit the sc7a20 driver
 */
void sc7a20_deinit(bool_t wakeup_enable);

/**
 * @brief This function is to set key parameters
 * @param id_cfg key id config
 * @param time_cfg key time config
 */
void sc7a20_set_param(const key_id_cfg_t *id_cfg, const key_time_cfg_t *time_cfg);

/**
 * @}
 * addtogroup SC7A20
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
