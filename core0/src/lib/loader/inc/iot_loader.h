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

#ifndef IOT_LOADER_H
#define IOT_LOADER_H

/**
 * @addtogroup LIB
 * @{
 */

/**
 * @addtogroup LOADER
 * @{
 * This section introduces the loader module's enum, structure, functions and how to use this module.
 */

#ifdef __cplusplus
extern "C" {
#endif
#ifdef LIB_DBGLOG_ENABLE
#include "dbglog.h"
#include "modules.h"
#else
#include "stdio.h"
#endif

#ifdef LIB_DBGLOG_ENABLE
#define DBGLOG_LIB_LOADER_INFO(fmt, arg...)     DBGLOG_STREAM_INFO(LIB_LOADER_MID, fmt, ##arg)
#else
#define DBGLOG_LIB_LOADER_INFO(fmt, arg...)     printf(fmt, ##arg)
#endif
/**
 * @brief This function is to laod bt.
 *
 * @return uint32_t is the start pc position.
 */
uint32_t iot_loader_load_bt(void);

/**
 * @brief This function is to load bsp.
 *
 * @return uint32_t is the start pc position.
 */
uint32_t iot_loader_load_dsp(void);

/**
 * @brief This function is to load tone.
 *
 * @return uint32_t is the tone's address in the flash. if return 0, no tone in flash.
 */
uint32_t iot_loader_load_tone(void);

#ifdef __cplusplus
}
#endif

/**
 * @}
 * addtogroup LOADER
 */

/**
* @}
 * addtogroup LIB
*/

#endif /* IOT_LOADER_H */
