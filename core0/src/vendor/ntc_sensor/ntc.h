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
#ifndef __NTC_H__
#define __NTC_H__

/**
 * @addtogroup LIB
 * @{
 */

/**
 * @addtogroup NTC_SENSOR
 * @{
 * This section introduces the LIB NTC_SENSOR module's enum, structure, functions and how to use this module.
 */
#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef NTC_INVALID
#define NTC_INVALID 0x7f
#endif

/**
 * @brief init the ntc sensor
 *
 */
void ntc_init(void);

/**
 * @brief read the ntc sensor temperature
 *
 * @return temperature in degrees centigrade if succeed, NTC_INVALID if failed
 */
int8_t ntc_read(void);

/**
 * @brief dinit the ntc sensor
 *
 */
void ntc_deinit(void);

#ifdef __cplusplus
}
#endif

/**
 * @}
 * addtogroup NTC_SENSOR
 */

/**
 * @}
 * addtogroup LIB
 */
#endif /* __NTC_H__ */
