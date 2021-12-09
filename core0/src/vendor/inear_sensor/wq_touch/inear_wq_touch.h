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
#ifndef INEAR_WUQI_TOUCH__H_
#define INEAR_WUQI_TOUCH__H_

/**
 * @addtogroup LIB
 * @{
 */

/**
 * @addtogroup INEAR_SENSOR
 * @{
 * This section introduces the LIB INEAR_SENSOR module's enum, structure, functions and how to use this module.
 */

/**
 * @addtogroup WQ_INEAR
 * @{
 * This section introduces the LIB INEAR_SENSOR WQ_INEAR module's enum, structure, functions and how to use this module.
 */
#include "types.h"
#include "inear_sensor.h"
#include "iot_resource.h"

#ifdef __cplusplus
extern "C" {
#endif

#if INEAR_DRIVER_SELECTION == INEAR_DRIVER_WQ_TOUCH

#define INEAR_TOUCH_PAD_ID GPIO_INEAR_KEY_0
/**
 * @brief init the inear sensor using wuqi touch
 *
 * @param callback the callback to handle in_ear/out_of_ear events.
 */
void inear_wuqi_touch_init(inear_callback_t callback);

/**
 * @brief deinit the inear sensor
 */
void inear_wuqi_touch_deinit(void);

/**
 * @brief wuqi touch open
 */
void inear_wuqi_touch_open(void);

/**
 * @brief factory reset the cfg
 */
void inear_wuqi_touch_cfg_reset(void);

#endif

#ifdef __cplusplus
}
#endif

/**
 * @}
 * addtogroup WQ_INEAR
 */

/**
 * @}
 * addtogroup INEAR_SENSOR
 */

/**
 * @}
 * addtogroup LIB
 */
#endif /* INEAR_WUQI_TOUCH__H_ */
