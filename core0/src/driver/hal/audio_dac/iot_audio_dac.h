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

#ifndef _DRIVER_HAL_AUDIO_DAC_H
#define _DRIVER_HAL_AUDIO_DAC_H
/**
 * @addtogroup HAL
 * @{
 * @addtogroup AUDIO_DAC
 * @{
 * This section introduces the AUDIO_DAC module's enum, structure, functions and how to use this driver.
 */

#include "types.h"
#include "iot_sdm_dac.h"
#include "iot_spk_dac.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @defgroup hal_audio_dac_enum Enum
  * @{
  */

/** @brief AUDIO DAC port id.*/
typedef enum {
    IOT_AUDIO_DAC_PORT_0,
    IOT_AUDIO_DAC_PORT_1,
    IOT_AUDIO_DAC_PORT_DOUBLE,
    IOT_AUDIO_DAC_PORT_MAX,
} IOT_AUDIO_DAC_PORT_ID;

/** @brief AUDIO DAC channel.*/
typedef enum {
    IOT_AUDIO_DAC_CHN_MONO_L, /* (ASRC0) -> (L) */
    IOT_AUDIO_DAC_CHN_MONO_R, /* (ASRC1) -> (R) */
    IOT_AUDIO_DAC_CHN_STEREO, /* (ASRC0, ASRC1) -> (L, R) */
    IOT_AUDIO_DAC_CHN_MAX,
    ITO_AUDIO_DAC_CHN_NONE,
} IOT_AUDIO_DAC_CHANNEL;

typedef enum {
    IOT_AUDIO_DAC_FS_62K5,
    IOT_AUDIO_DAC_FS_100K,
    IOT_AUDIO_DAC_FS_125K,
    IOT_AUDIO_DAC_FS_200K,
    IOT_AUDIO_DAC_FS_400K,
    IOT_AUDIO_DAC_FS_500K,
    IOT_AUDIO_DAC_FS_800K,
    IOT_AUDIO_DAC_FS_MAX,
} IOT_AUDIO_DAC_SAMPLING_RATE;

/** @brief AUDIO DAC mix mode.*/
typedef enum {
    IOT_AUDIO_DAC_STEREO_DIRECT,  /* (ASRC0, ASRC1) -> (L, R) */
    IOT_AUDIO_DAC_STEREO_SWAP,    /* (ASRC0, ASRC1) -> (R, L) */
    IOT_AUDIO_DAC_STEREO_AVERAGE, /* (((ASRC0 + ASRC1) / 2), (((ASRC0 + ASRC1) / 2))) -> (L, R) */
    IOT_AUDIO_DAC_MONO_0,         /* (ASRC0, ASRC0) -> (L, R) */
    IOT_AUDIO_DAC_MONO_1,         /* (ASRC1, ASRC1) -> (L, R) */
    IOT_AUDIO_DAC_STEREO_SUM,     /* ((ASRC0 + ASRC1), (ASRC0 + ASRC1)) -> (L, R)*/
    IOT_AUDIO_DAC_MAX,
} IOT_AUDIO_DAC_MIX_MODE;

typedef enum {
    IOT_AUDIO_DAC_GAIN_DIRECTLY,
    IOT_AUDIO_DAC_GAIN_ZERO_CROSS,
    IOT_AUDIO_DAC_GAIN_SOFT_RAMP,
    IOT_AUDIO_DAC_GAIN_DOUBLE,
    IOT_AUDIO_DAC_GAIN_MAX,
} IOT_AUDIO_DAC_GAIN_MODE;

/** @brief AUDIO DAC src id.*/
typedef enum {
    IOT_AUDIO_DAC_SRC_ADC_0,
    IOT_AUDIO_DAC_SRC_ADC_1,
    IOT_AUDIO_DAC_SRC_ADC_2,
    IOT_AUDIO_DAC_SRC_ADC_3,
    IOT_AUDIO_DAC_SRC_ADC_4,
    IOT_AUDIO_DAC_SRC_ADC_5,
    IOT_AUDIO_DAC_SRC_ASRC,
    IOT_AUDIO_DAC_SRC_ANC,
    IOT_AUDIO_DAC_SRC_MAX,
} IOT_AUDIO_DAC_SRC_ID;
/**
 * @}
 */

/** @defgroup hal_audio_dac_struct Struct
  * @{
  */
typedef struct iot_audio_dac_config {
    IOT_AUDIO_DAC_SRC_ID src;
    IOT_AUDIO_DAC_SAMPLING_RATE fs;
    int full_scale_limit;
    uint16_t dc_offset_dig_calibration;
} iot_audio_dac_config_t;
/**
  * @}
  */

/** @defgroup hal_audio_dac_typedef Typedef
  * @{
 */
typedef void (*iot_audio_dac_dump_callback)(uint8_t *buffer, uint32_t length);
/**
  * @}
  */

/**
 * @brief audio dac init digital and analog part
 * init clk, gain,clip threshold
 */
void iot_audio_dac_init(void);

/**
 * @brief audio dac deinit digital and analog part
 */
void iot_audio_dac_deinit(void);

/**
 * @brief This function is to open audio dac's channel
 *
 * @param chn is to select left channel or right channel
 * @param cfg is audio dac's config
 */
void iot_audio_dac_open(IOT_AUDIO_DAC_CHANNEL chn, const iot_audio_dac_config_t *cfg);

/**
 * @brief This function is to close audio dac's channel
 *
 * @param chn is to select left channel or right channel
 */
void iot_audio_dac_close(IOT_AUDIO_DAC_CHANNEL chn);


/**
 * @brief audio dac stop digital part, audio dac config digital and analog part,
 * open the clk, the gain,the clip threshold for the first time,
 * at the end of, unmute the audio adc is needed
 *
 * @param chn is to select left channel or right channel
 */
void iot_audio_dac_start(IOT_AUDIO_DAC_CHANNEL chn);

/**
 * @brief This function is to stop audio dac.
 *  first of all, mute the audio adc is needed
 *
 * @param chn is to select left channel or right channel
 */
void iot_audio_dac_stop(IOT_AUDIO_DAC_CHANNEL chn);

/**
 * @brief This function is to select audio dac stereo mixer mode and mix dac sound.
 *
 * @param mode is Audio dac mix data mode from asrc
 * 0:direct feedthrough;
 * 1:L&R swap;
 * 2:(L+R)/2 mix;
 * 3:Mono replication(0);
 * 4:Mono replication(1);
 * 5:L+R mix
 */
void iot_audio_dac_stereo_mixer(IOT_AUDIO_DAC_MIX_MODE mode);

/**
 * @brief enable audio multipath sync
 *
 */
void iot_audio_dac_multipath_sync(void);

/**
 * @brief get the state of whether the audio dac is open
 *
 * @param chn  is to select left channel or right channel
 * @return bool_t audio dac open status
 */
bool_t iot_audio_dac_get_open_status(IOT_AUDIO_DAC_CHANNEL chn);

/**
 * @brief This function is to mute audio dac.
 *
 * @param chn is to select left channel or right channel
 * @param mode audio dac gain mode
 *
 * @return uint8_t RET_OK for success else error.
 */
uint8_t iot_audio_dac_mute(IOT_AUDIO_DAC_CHANNEL chn, IOT_AUDIO_DAC_GAIN_MODE mode);

/**
 * @brief This function is to unmute audio dac.
 *
 * @param chn is to select left channel or right channel
 * @param mode audio dac gain mode
 *
 * @return uint8_t RET_OK for success else error.
 */
uint8_t iot_audio_dac_unmute(IOT_AUDIO_DAC_CHANNEL chn, IOT_AUDIO_DAC_GAIN_MODE mode);

/**
 * @brief This function is to adjust audio dac gain step
 *
 * @param port is to select left channel or right channel
 * @param mode is audio dac gain control mode
 * 00:bypass gain control module, the gain is directly transferred to next module;
 * 01:zero-cross method;
 * 10:soft-ramp method;
 * 11:zero-cross+soft-ramp, first comes first use.
 * @param up true-add gain by step, false-reduce gain by step
 * @param digit_gain_steps adjusting gain steps, per step is 0.1875dB,
 * which is also used as the total power down for mute
 * @param need_complete need to be done
 */
void iot_audio_dac_gain_adjust(IOT_AUDIO_DAC_PORT_ID port, IOT_AUDIO_DAC_GAIN_MODE mode, bool_t up, int16_t digit_gain_steps,
                               bool_t need_complete);
/**
 * @brief set audio dac target gain
 *
 * @param chn is to select left channel or right channel
 * @param target_gain gain range -511 to 96
 * @return uint8_t RET_OK for success else error.
 */
uint8_t iot_audio_dac_gain_target(IOT_AUDIO_DAC_CHANNEL chn, int16_t target_gain);

/**
 * @brief downward the gain by step
 *
 * @param port is to select left channel or right channel
 * @param steps adjusting gain steps, per step is 0.1875dB
 * @return uint8_t RET_OK for success else error.
 */
uint8_t iot_audio_dac_gain_down(IOT_AUDIO_DAC_PORT_ID port, int16_t steps);

/**
 * @brief increase the gain by step
 *
 * @param port is to select left channel or right channel
 * @param steps adjusting gain steps, per step is 0.1875dB
 * @return uint8_t RET_OK for success else error.
 */
uint8_t iot_audio_dac_gain_up(IOT_AUDIO_DAC_PORT_ID port, int16_t steps);

/**
 * @brief
 *
 * @param chn  is to select left channel or right channel
 * @param balance_on true is open and false is close power balance
 * @param close_dac does the audio dac need to be closed
 * @return uint8_t RET_OK for success else error.
 */
uint8_t iot_audio_dac_power_balance(IOT_AUDIO_DAC_CHANNEL chn, bool_t balance_on, bool_t close_dac);

#ifdef __cplusplus
}
#endif
/**
* @}
* @}
*/
#endif
