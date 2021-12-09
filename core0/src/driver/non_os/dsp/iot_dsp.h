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

#ifndef _DRIVER_NON_OS_DSP_H
#define _DRIVER_NON_OS_DSP_H
/**
 * @addtogroup HAL
 * @{
 * @addtogroup DSP
 * @{
 * This section introduces the DSP module's enum, structure, functions and how to use this driver.
 */

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Power on dsp
 */
void iot_dsp_power_on(void);

/**
 * @brief Power off dsp
 */
void iot_dsp_power_off(void);

/**
 * @brief This function is to load dsp image.
 *
 * @param pbin is dsp's image
 */
void iot_dsp_load_image(uint8_t *pbin);

/**
 * @brief This function is to stall dsp core and enable memory.
 *
 */
void iot_dsp_stall_and_enable(void);

/**
 * @brief This function is to start dsp.
 *
 */
void iot_dsp_start(void);

/**
 * @brief This function is to load pimage and start dsp.
 *
 * @param pimage is dsp's pimage
 */
void iot_dsp_load_and_start(uint8_t *pimage);

#ifdef __cplusplus
}
#endif
/**
* @}
* @}
*/
#endif /* _DRIVER_NON_OS_DSP_H */
