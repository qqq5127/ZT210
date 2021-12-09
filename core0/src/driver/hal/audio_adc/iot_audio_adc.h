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

#ifndef _DRIVER_HAL_AUDIO_ADC_H
#define _DRIVER_HAL_AUDIO_ADC_H
/**
 * @addtogroup HAL
 * @{
 * @addtogroup AUDIO_ADC
 * @{
 * This section introduces the AUDIO_ADC module's enum, structure, functions and how to use this driver.
 */

#include "types.h"
#include "iot_sdm_adc.h"
#include "iot_mic_adc.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @defgroup hal_audio_dac_enum Enum
  * @{
  */

/** @brief audio adc dfe id */
typedef enum {
    IOT_AUDIO_CHN_0,
    IOT_AUDIO_CHN_1,
    IOT_AUDIO_CHN_2,
    IOT_AUDIO_CHN_3,
    IOT_AUDIO_CHN_4,
    IOT_AUDIO_CHN_5,
    IOT_AUDIO_CHN_MAX,
    IOT_AUDIO_CHN_NONE,
} IOT_AUDIO_CHN_ID;

/** @brief AUDIO ADC port id.*/
typedef enum {
    IOT_AUDIO_ADC_PORT_0,
    IOT_AUDIO_ADC_PORT_1,
    IOT_AUDIO_ADC_PORT_2,
    IOT_AUDIO_ADC_PORT_MAX,
    IOT_AUDIO_ADC_PORT_NONE = 0xFF,
} IOT_AUDIO_ADC_PORT_ID;

typedef enum {
    IOT_AUDIO_ADC_POWER_0,
    IOT_AUDIO_ADC_POWER_1,
    IOT_AUDIO_ADC_POWER_MAX,
    IOT_AUDIO_ADC_POWER_NONE,
}IOT_AUDIO_ADC_POWER_ID;

typedef enum {
    IOT_AUDIO_ADC_FS_8K,
    IOT_AUDIO_ADC_FS_16K,
    IOT_AUDIO_ADC_FS_32K,
    IOT_AUDIO_ADC_FS_48K,
    IOT_AUDIO_ADC_FS_62K5,
    IOT_AUDIO_ADC_FS_96K,
    IOT_AUDIO_ADC_FS_192K,
    IOT_AUDIO_ADC_FS_MAX,
} IOT_AUDIO_ADC_SAMPLING_RATE;

/**
 * @}
 */

typedef void (*iot_audio_adc_timer_done_callback)(void);

/** @defgroup hal_audio_adc_struct Struct
  * @{
  */
typedef struct iot_audio_adc_config {
    iot_rx_dfe_config_t dfe;
} iot_audio_adc_config_t;
/**
  * @}
  */

/** @defgroup hal_audio_adc_typedef Typedef
  * @{
 */
typedef void (*iot_audio_adc_dump_callback)(uint8_t *buffer, uint32_t length);
typedef void (*iot_audio_adc_timer_done_callback)(void);
/**
  * @}
  */

/**
 * @brief This funtion is to init audio_adc module.
 *
 */
void iot_audio_adc_init(void);

/**
 * @brief audio adc deinit
 *
 */
void iot_audio_adc_deinit(void);

/**
 * @brief This funtion is to stop audio_adc module, micbias 0,1 all disable,
 * close micbias 0,1.
 * @warning : micbias 0 control mic 0 1 3 port,micbias 1 control mic 2 port
 *
 * @param port is port id mapped to channel
 * @param chn is port id mapped to channel
 *
 * @return uint8_t RET_OK for success else error.
 */
uint8_t iot_audio_adc_stop(IOT_AUDIO_ADC_PORT_ID port, IOT_AUDIO_CHN_ID chn);

/**
 * @brief This funtion is to start audio_adc module. audio adc init, micbias 0,1 all enable,
 * open micbias 0, close micbias 1.
 * @warning micbias 0 control mic 0 1 3 port,micbias 1 control mic 2 port
 *
 * @param port is port id mapped to channel
 * @param chn dfe channel id
 * @param cfg audio adc param
 *
 * @return uint8_t RET_OK for success else error.
 */
uint8_t iot_audio_adc_start(IOT_AUDIO_ADC_PORT_ID port, IOT_AUDIO_CHN_ID chn,
                                const iot_audio_adc_config_t *cfg);

/**
 * @brief enable audio multipath sync
 */
void iot_audio_adc_multipath_sync(void);

/**
 * @brief audio adc close, if use port2, need to micbias 1 open,micbias 2 open
 *
 * @param adc_bitmap is port id mapping
 * @param power_bitmap mic0~2's micbias id is put in the power_bitmap bit0~2
 * @param cb
 *
 * @return dfe channel id
 */
uint8_t iot_audio_adc_open(uint8_t adc_bitmap, uint8_t power_bitmap, iot_audio_adc_timer_done_callback cb);

/**
 * @brief audio adc close, if use port2, need to micbias 1 open,micbias 2 close
 *
 * @param adc_bitmap bitmap is port id mapping
 * @return uint8_t RET_OK for success else error.
 */
uint8_t iot_audio_adc_close(uint8_t adc_bitmap);

/**
 * @brief the gain pre step is 0.1875dB
 *
 * @param chn : dfe channel id
 * @param gain target gain
 * @return uint8_t RET_OK for success else error.
 */
uint8_t iot_audio_adc_gain_set(IOT_AUDIO_CHN_ID chn, int16_t gain);

#ifdef __cplusplus
}
#endif
/**
* @}
* @}
*/
#endif
