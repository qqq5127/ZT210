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
#ifndef LIB_IOT_RESOURCE_H
#define LIB_IOT_RESOURCE_H

#include "gpio_id.h"
#include "kv_id.h"
#include "iot_gpio.h"

/**
 * @addtogroup LIB
 * @{
 */

/**
 * @addtogroup RESOURCE
 * @{
 * This section introduces the resource module's functions and how to use this module.
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief This function is to deinit resource.
 */
void iot_resource_deinit(void);

/**
 * @brief This function is to lookup gpio.
 *
 * @param id is the resource gpio id.
 * @return uint8_t is the gpio.
 */
uint8_t iot_resource_lookup_gpio(RESOURCE_GPIO_ID id);

/**
 * @brief This function is to lookup pull mode.
 *
 * @param gpio is the gpio.
 * @return IOT_GPIO_PULL_MODE is gpio's pull mode.
 */
IOT_GPIO_PULL_MODE iot_resource_lookup_pull_mode(uint8_t gpio);

/**
 * @brief This function isto check gpio wheter configured in IOMAP.
 *
 * @param gpio is the gpio.
 * @return bool_t is configured or not.
 */
bool_t iot_resource_check_gpio_configured(uint8_t gpio);

/**
 * @brief This function is to lookup adc port
 *
 * @param id is the resource audio path id.
 * @return uint8_t is the adc port.
 */
uint8_t iot_resource_lookup_adc(RESOURCE_AUDIO_PATH_ID id);

/**
 * @brief This function is to lookup micbias id
 *
 * @param adc is the resource audio adc id
 * @return uint8_t is the micbias id.
 */
uint8_t iot_resource_lookup_bias(uint8_t adc);

/**
 * @brief This function is to lookup audio adc channel
 *
 * @param id is the resource audio path id.
 * @return uint8_t is the adc channel.
 */
uint8_t iot_resource_lookup_channel(RESOURCE_AUDIO_PATH_ID id);

/**
 * @brief This function is to lookup asrc id
 *
 * @param id is the resource audio path id.
 * @return uint8_t is the asrc id.
 */
uint8_t iot_resource_lookup_asrc(RESOURCE_AUDIO_PATH_ID id);

/**
 * @brief This function is to lookup rx fifo id
 *
 * @param id is the resource audio path id.
 * @return uint8_t is the rx fifo id.
 */
uint8_t iot_resource_lookup_rx_fifo(RESOURCE_AUDIO_PATH_ID id);

/**
 * @brief This function is to lookup audio dac id
 *
 * @param id is the resource audio path id.
 * @return uint8_t is the dac id.
 */
uint8_t iot_resource_lookup_dac(RESOURCE_AUDIO_PATH_ID id);

/**
 * @brief This funciton is to lookup Touch ID.
 *
 * @param id is the resource gpio id.
 * @return uint8_t is Touch ID. 0xff is invalid id.
 */
uint8_t iot_resource_lookup_touch_id(RESOURCE_GPIO_ID id);

#ifdef __cplusplus
}
#endif

/**
 * @}
 * addtogroup RESOURCE
 */

/**
* @}
 * addtogroup LIB
*/

#endif//LIB_IOT_RESOURCE_H
