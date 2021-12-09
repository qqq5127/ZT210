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
#ifndef INEAR_SENSOR__H_
#define INEAR_SENSOR__H_

/**
 * @addtogroup LIB
 * @{
 */

/**
 * @addtogroup INEAR_SENSOR
 * @{
 * This section introduces the LIB INEAR_SENSOR module's enum, structure, functions and how to use this module.
 */

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define INEAR_DRIVER_NONE         0
#define INEAR_DRIVER_CUSTOMIZE    1
#define INEAR_DRIVER_SIMPLE_IO    2
#define INEAR_DRIVER_DEBOUNCE_IO  3
#define INEAR_DRIVER_WQ_TOUCH     4
#define INEAR_DRIVER_LIGHT_HY2751 5

#ifndef INEAR_DRIVER_SELECTION
#define INEAR_DRIVER_SELECTION INEAR_DRIVER_NONE
#endif
#define DBGLOG_INEAR_SENSOR_INFO(fmt, arg...) \
    DBGLOG_STREAM_INFO(IOT_SENSOR_HUB_MANAGER_MID, fmt, ##arg)
#define DBGLOG_INEAR_SENSOR_ERROR(fmt, arg...) \
    DBGLOG_STREAM_ERROR(IOT_SENSOR_HUB_MANAGER_MID, fmt, ##arg)

/**
 * @brief callback to handle inear
 * @param inear is inear
 */
typedef void (*inear_callback_t)(bool_t inear);

/**
 * @brief init the inear sensor
 *
 * @param callback the callback to handle in_ear/out_of_ear events.
 */
void inear_sensor_init(inear_callback_t callback);

/**
 * @brief deinit the inear sensor
 */
void inear_sensor_deinit(void);

/**
 * @brief set inear sensor open
 */
void inear_sensor_open(void);

/**
 * @brief factory reset the inear sensor cfg
 */
void inear_sensor_cfg_reset(void);

#ifdef __cplusplus
}
#endif

/**
 * @}
 * addtogroup INEAR_SENSOR
 */

/**
 * @}
 * addtogroup LIB
 */
#endif /* INEAR_SENSOR__H_ */
