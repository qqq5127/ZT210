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
#ifndef KEY_EXT_TOUCH_H_
#define KEY_EXT_TOUCH_H_

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
 * @addtogroup EXT_TOUCH
 * @{
 * This section introduces the LIB KEY_SENSOR EXT_TOUCH module's enum, structure, functions and how to use this module.
 */

#include "types.h"
#include "key_sensor.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief This function is init key external touch.
 *
 */
void key_ext_touch_init(void);

/**
 * @brief deinit the key debounce io.
 */
void key_ext_touch_deinit(bool_t wakeup_enable);

/**
 * @brief This function is used to open external touch.
 *
 * @param id_cfg key id config
 * @param time_cfg key time config
 */
void key_ext_touch_open(const key_id_cfg_t *id_cfg, const key_time_cfg_t *time_cfg);

/**
 * @brief This function is used to enable/disable external touch
 *
 * @param enable false -- disable, true -- enable
 */
void key_ext_touch_set_enabled(bool_t enable);

#ifdef __cplusplus
}
#endif

/**
 * @}
 * addtogroup EXT_TOUCH
 */

/**
 * @}
 * addtogroup KEY_SENSOR
 */

/**
 * @}
 * addtogroup LIB
 */
#endif /* KEY_EXT_TOUCH_H_ */
