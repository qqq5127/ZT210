/****************************************************************************

Copyright(c) 2020 by WuQi Technologies. ALL RIGHTS RESERVED.

This Information is proprietary to WuQi Technologies and MAY NOT
be copied by any method or incorporated into another program without
the express written consent of WuQi. This Information or any portion
thereof remains the property of WuQi. The Information contained herein
is believed to be accurate and WuQi assumes no responsibility or
liability for its use in any way and conveys no license or title under
any patent or copyright Land makes no representation or warranty that this
Information is free from patent or copyright infringement.

****************************************************************************/
#ifndef _DRIVER_HAL_MIC_ADC_H
#define _DRIVER_HAL_MIC_ADC_H

#include "types.h"

/*IOT_VPOWER = 1:Vpwr = 1.8v
IOT_VPOWER = 0:Vpwr = 2.8v
*/
#define IOT_VPOWER 1

typedef enum {
    IOT_MIC_ADC_0,
    IOT_MIC_ADC_1,
    IOT_MIC_ADC_2,
    IOT_MIC_ADC_MAX,
} IOT_MIC_ADC;

typedef enum {
    IOT_MIC_ADC_GAIN_N_6DB,
    IOT_MIC_ADC_GAIN_N_3DB,
    IOT_MIC_ADC_GAIN_0DB,
    IOT_MIC_ADC_GAIN_P_3DB,
    IOT_MIC_ADC_GAIN_P_6DB,
    IOT_MIC_ADC_GAIN_P_9DB,
    IOT_MIC_ADC_GAIN_P_12DB,
    IOT_MIC_ADC_GAIN_P_15DB,
    IOT_MIC_ADC_GAIN_FORBID,
} IOT_MIC_ADC_GAIN;

/* 0:micbias 0 and micbias 1 both off,
   1:micbias 0 switch on, micbias 1 switch off,
   2:micbias 0 switch off, micbias 1 switch on,invalid operations,
   3:micbias 0 and micbias 1 both on.*/
typedef enum {
    IOT_MIC_ADC_MICBIAS_OFF_OFF,
    IOT_MIC_ADC_MICBIAS_ON_OFF,
    IOT_MIC_ADC_MICBIAS_OFF_ON,
    IOT_MIC_ADC_MICBIAS_ON_ON,
} IOT_MIC_ADC_MICBIAS_CTRL;

#if IOT_VPOWER
typedef enum {
    IOT_MIC_ADC_MICBIAS_1P45,
    IOT_MIC_ADC_MICBIAS_1P5,
    IOT_MIC_ADC_MICBIAS_1P55,
    IOT_MIC_ADC_MICBIAS_1P6,
    IOT_MIC_ADC_MICBIAS_1P65,
    IOT_MIC_ADC_MICBIAS_1P7,
} IOT_MIC_ADC_MICBIAS_OUT;
#else
typedef enum {
    IOT_MIC_ADC_MICBIAS_1P45,
    IOT_MIC_ADC_MICBIAS_1P5,
    IOT_MIC_ADC_MICBIAS_1P55,
    IOT_MIC_ADC_MICBIAS_1P6,
    IOT_MIC_ADC_MICBIAS_1P65,
    IOT_MIC_ADC_MICBIAS_1P7,
    IOT_MIC_ADC_MICBIAS_1P75,
    IOT_MIC_ADC_MICBIAS_1P8,
    IOT_MIC_ADC_MICBIAS_1P85,
    IOT_MIC_ADC_MICBIAS_1P9,
    IOT_MIC_ADC_MICBIAS_2P05,
    IOT_MIC_ADC_MICBIAS_2P15,
    IOT_MIC_ADC_MICBIAS_2P25,
    IOT_MIC_ADC_MICBIAS_2P35,
    IOT_MIC_ADC_MICBIAS_2P45,
    IOT_MIC_ADC_MICBIAS_2P55,
} IOT_MIC_ADC_MICBIAS_OUT;
#endif

typedef enum {
    IOT_MIC_ADC_MICBIAS_LP_MODE,
    IOT_MIC_ADC_MICBIAS_NORMAL,
} IOT_MIC_ADC_MICBIAS_MODE;

typedef enum {
    IOT_MIC_ADC_NORMAL_NORMAL,
    IOT_MIC_ADC_NORMAL_HALF,
    IOT_MIC_ADC_HALF_NORMAL,
    IOT_MIC_ADC_HALF_HALF,
} IOT_MIC_ADC_MIC_CURRENT;

typedef enum {
    IOT_MIC_ADC_FISRT_INT,
    IOT_MIC_ADC_SECOND_INT,
} IOT_MIC_ADC_MIC_INT_CUR;

typedef enum {
    IOT_MIC_ADC_MICBIAS_CUR_LOW_R = 1,
    IOT_MIC_ADC_MICBIAS_CUR_HIGH_R = 7,
} IOT_MIC_ADC_MICBIAS_CUR_FLT_R;

typedef void (*iot_mic_adc_timer_done_callback)(void);

uint8_t iot_mic_reg_init(void);

void iot_mic_adc_init(void);

bool_t iot_mic_adc_get_complete(void);

uint8_t iot_mic_get_micbias_vout(void);

/**
 * @brief This function is to deinitialize micphone.
 */
void iot_mic_bias_deinit(void);

/**
 * @brief micbias init
 *
 */
void iot_mic_bias_init(void);

/**
 * @brief This function is to config mic_adc.
 *
 * @param mic_map is mic id bit map.
 * @param cb
 */
uint8_t iot_mic_config(uint8_t mic_map, iot_mic_adc_timer_done_callback cb);

/**
 * @brief This function is to release mic_adc.
 *
 * @param mic_map is mic id bit map.
 */
void iot_mic_release(uint8_t mic_map);

/**
 * @brief This function is to switch on mic_adc bias.
 *
 * @param mic_map is mic id bit map.
 * @param micbias_map micbias used by mic0~2 are placed in micbias_map bit0mic0~2.
 *  for example, mic0 's micbias is 1, putting 1 on micbias_map' s bit1
 * @return uint8_t
 */
uint8_t iot_mic_bias_switch_on(uint8_t mic_map, uint8_t micbias_map);

/**
 * @brief This function is to switch off mic_adc bias.
 *
 * @param mic_map is mic id bit map.
 * @return uint8_t
 */
uint8_t iot_mic_bias_switch_off(uint8_t mic_map);

/**
 * @brief This function is to set micphone adc vref voltage.
 *
 * @param id is micphone id.
 * @param vref_code is vref value,range from 0 to 7, default-7 equals 1mv.
 */
void iot_mic_adc_vref_ctrl(IOT_MIC_ADC id, uint8_t vref_code);

/**
 * @brief This function is to set micphone adc vcm voltage.
 *
 * @param chn is micphone channel.
 * @param vcm_code is vcm value,range from 0 to 7, default 0 equals 1mv.
 */
void iot_mic_adc_vcm_ctrl(IOT_MIC_ADC chn, uint8_t vcm_code);

/**
 * @brief This function is to set micbias output voltage.
 *
 * @param micbias_out is micbias output code.
 */
void iot_mic_bias_control(IOT_MIC_ADC_MICBIAS_OUT micbias_out);

/**
 * @brief This function is to select micbias mode.
 *
 * @param mode is the mode signal,1 normal mode, 0 low power mode.
 */
void iot_mic_bias_mode(IOT_MIC_ADC_MICBIAS_MODE mode);

/**
 * @brief This function is to configure mic_adc and mic_pga fast.
 *
 * @param mic_map is mic id bit map.
 * @param cb mic open done cb
 */
void iot_mic_adc_pga_init(uint8_t mic_map, iot_mic_adc_timer_done_callback cb);

/**
 * @brief This function is mic_adc rc tuning, runs when power on each time.
 *
 * @return uint32_t RET_OK for success else error.
 */
uint32_t iot_mic_rc_tuning(void);

/**
 * @brief This function is to select mic pga gain.
 *
 * @param id is micphone channel.
 * @param gain is available pga gain(-6dB, -3dB, 0dB, 3dB, 6dB, 9dB, 12dB, 15dB), defualt 0dB.
 */
void iot_mic_gain_control(IOT_MIC_ADC id, IOT_MIC_ADC_GAIN gain);

/**
 * @brief This function is to run into low power mode.
 *
 * @param id is micphone channel.
 */
void iot_mic_low_power_mode(IOT_MIC_ADC id);

/**
 * @brief rc tune reset
 *
 */
void iot_mic_rc_tune_reset(void);

/**
 * @brief This function is to config mic analog gain.
 * @param ana_gain set the mic analog gain.
 * @return uint8_t RET_OK for success else error.
 */
uint8_t iot_mic_adc_ana_gain_init(int8_t ana_gain);
#endif
