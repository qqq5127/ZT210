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
#ifndef __HY2751_H_
#define __HY2751_H_

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
 * @addtogroup HY_2751
 * @{
 * This section introduces the LIB INEAR_SENSOR HY_2751 module's enum, structure, functions and how to use this module.
 */
#include "inear_sensor.h"

#ifdef __cplusplus
extern "C" {
#endif

#if INEAR_DRIVER_SELECTION == INEAR_DRIVER_LIGHT_HY2751
/**
 * @brief init the inear sensor using light sensor
 *
 * @param callback the callback to handle in_ear/out_of_ear events.
 */
void inear_hy2751_init(inear_callback_t callback);

/**
 * @brief dinit the inear sensor
 *
 */
void inear_hy2751_deinit(void);

#endif   //IN_EAR_DRIVER_SELECTION == IN_EAR_LIGHT_HY2751

#ifdef __cplusplus
}
#endif

/**
 * @}
 * addtogroup HY_2751
 */

/**
 * @}
 * addtogroup INEAR_SENSOR
 */

/**
 * @}
 * addtogroup LIB
 */
#endif /* __HY2751_H_ */
