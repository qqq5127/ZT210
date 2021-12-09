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
#ifndef KEY_SIMPLE_IO_H_
#define KEY_SIMPLE_IO_H_

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
 * @addtogroup SIMPLE_IO
 * @{
 * This section introduces the LIB KEY_SENSOR SIMPLE_IO module's enum, structure, functions and how to use this module.
 */
#include "types.h"
#include "key_sensor.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief This function is init key simple io module.
 */
void key_simple_io_init(void);

/**
 * @brief deinit the key simple io module.
 */
void key_simple_io_deinit(void);

/**
 * @brief This function is used to open simple io.
 *
 * @param id_cfg key id config
 * @param time_cfg key time config
 */
void key_simple_io_open(const key_id_cfg_t *id_cfg, const key_time_cfg_t *time_cfg);

#ifdef __cplusplus
}
#endif

/**
 * @}
 * addtogroup SIMPLE_IO
 */

/**
 * @}
 * addtogroup KEY_SENSOR
 */

/**
 * @}
 * addtogroup LIB
 */
#endif /* KEY_SIMPLE_IO_H_ */
