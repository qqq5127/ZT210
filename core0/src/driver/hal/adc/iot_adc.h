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

#ifndef _DRIVER_HAL_ADC_H
#define _DRIVER_HAL_ADC_H

/**
 * @addtogroup HAL
 * @{
 * @addtogroup ADC
 * @{
 * This section introduces the ADC module's enum, structure, functions and how to use this driver.
 */

#include "types.h"
#include "iot_dma.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @defgroup hal_adc_enum Enum
  * @{
  */
#define IOT_ADC_PIN_MAX 4

/*
 * IOT_ADC_PORT_0          pin 0
 * IOT_ADC_PORT_1          pin 1
 * IOT_ADC_PORT_2          pin 2
 * IOT_ADC_PORT_3          pin 3
 * IOT_ADC_PORT_DIFF_0_1   pin 0 pin 1
 * IOT_ADC_PORT_DIFF_2_3   pin 2 pin 3
 */
typedef enum {
    IOT_ADC_PORT_0,
    IOT_ADC_PORT_1,
    IOT_ADC_PORT_2,
    IOT_ADC_PORT_3,
    IOT_ADC_PORT_4,
    IOT_ADC_PORT_5,
    IOT_ADC_PORT_6,
    IOT_ADC_PORT_7,
    IOT_ADC_PORT_MAX,
} IOT_ADC_PORT;

typedef enum {
    IOT_ADC_AVERAGE_DATA,
    IOT_ADC_SUMMATION_DATA,
    IOT_ADC_DATA_MAX,
} IOT_ADC_DATA_MODE;

typedef enum {
    IOT_ADC_METER_ADC_IN,    //sdm_adc raw data and noise cancel output
    IOT_ADC_NC_OUT,          //nc output
    IOT_ADC_SINC_OUT,        //sinc4 downsample output
    IOT_ADC_HPF_OUT,         //through hpf filter output
    IOT_ADC_PWR_SCALE_OUT,   //power scaler output
    IOT_ADC_METER_ADC_OUT,   //adc sdm dfe data output
    IOT_ADC_DUMP_DATA_MAX,
} IOT_ADC_DUMP_DATA_MODULE;

typedef enum {
    IOT_ADC_ALL_OFF = 0,
    IOT_ADC_VAD_SNDR_MEASURE = 1,
    IOT_ADC_EXT_SIG_CH0 = 3,
    IOT_ADC_EXT_SIG_CH1 = 4,
    IOT_ADC_EXT_SIG_CH2 = 5,
    IOT_ADC_EXT_SIG_CH3 = 6,
    IOT_ADC_EXT_DIFF_CH01 = 9,
    IOT_ADC_EXT_DIFF_CH23 = 10,
    IOT_ADC_CHARGER_IN_VOLTAGE = 20,
    IOT_ADC_CHARGER_OUT_VOLTAGE = 21,
    IOT_ADC_VTEMP_SENSER = 49,
} IOT_ADC_SIG_SRC;

typedef enum {
    IOT_ADC_SAMPLE_RATE_2K,
    IOT_ADC_SAMPLE_RATE_4K,
    IOT_ADC_SAMPLE_RATE_8K,
    IOT_ADC_SAMPLE_RATE_16K,
    IOT_ADC_SAMPLE_RATE_32K,
    IOT_ADC_SAMPLE_RATE_64K,
    IOT_ADC_SAMPLE_RATE_128K,
    IOT_ADC_SAMPLE_RATE_250K,
    IOT_ADC_SAMPLE_RATE_500K,
} IOT_ADC_SAMPLE_RATE;

typedef enum {
    IOT_ADC_WORK_MODE_SINGLE,
    IOT_ADC_WORK_MODE_MULTI_CONTINUOUS,
} IOT_ADC_WORK_MODE;

/**
  * @}
  */

/** @defgroup hal_adc_typedef Typedef
  * @{
  */
typedef void (*iot_adc_dump_callback)(uint8_t *buffer, uint32_t length);
/**
  * @}
  */
/**
 * @brief This function is to init ADC module.
 *
 */
void iot_adc_init(void);

/**
 * @brief This function is to deinit ADC module.
 *
 */
void iot_adc_deinit(void);

/**
 * @brief This function is to start ADC module.
 *
 */
void iot_adc_start(void);

/**
 * @brief This function is to stop ADC module.
 *
 */
void iot_adc_stop(void);

/**
 * @brief poll data after adc open and started in IOT_ADC_WORK_MODE_MULTI_CONTINUOUS mode
 *
 * @param port
 * @param mode
 * @return uint32_t
 */
uint32_t iot_adc_single_channel_receive_poll(IOT_ADC_PORT port, IOT_ADC_DATA_MODE mode);

/**
 * @brief This function is to bind adc ch for dma.
 *
 *@param port is adc port IOT_ADC_PORT
 */
void iot_adc_dma_channel_link(IOT_ADC_PORT port);

/**
 * @brief dma recieve adc continous data
 *
 * @param buffer
 * @param length
 * @param cb
 * @return uint8_t
 */
uint8_t iot_adc_receive_dma(uint32_t *buffer, uint32_t length, dma_peri_mem_done_callback cb);

/**
 * @brief enable low power function
 *
 * @param port is adc port IOT_ADC_PORT
 * @param enable
 */
void iot_adc_ana_lp_cfg(uint8_t port, bool_t enable);

/**
 * @brief This function is to poll data only when adc working in IOT_ADC_WORK_MODE_SINGLE mode
 *
 */
uint32_t iot_adc_dfe_poll(void);

/**
 * @brief Open the analog io for adc
 *
 * @param adc_gpio Analog gpio to be open
 */
void iot_adc_open_external_port(uint8_t adc_gpio);

/**
 * @brief bind a signal src to a adc ch
 *
 * @param port is ADC port IOT_ADC_PORT
 * @param sig_type is for ADC dump buffer
 */
void iot_adc_ana_chn_sel(uint8_t port, IOT_ADC_SIG_SRC sig_type);

/**
 * @brief This function is to poll a adc channel result
 *
 * @param ch is ADC signal src type
 * @param gain is for ADC pga gain
 * @param sum_average is the num of average
 *
 * @return int32_t is signed adc code
 */
int32_t iot_adc_poll_data(uint8_t ch, uint8_t gain, uint8_t sum_average);

/**
 * @brief This function is to open and start ADC
 *
 * @param ch is ADC signal src type for port0
 * @param sample_rate is adc sample rate
 * @param gain is for ADC pga gain  for port0
 * @param mode is adc work mode
 *
 * @return int32_t is signed adc code
 */
void iot_adc_open(uint8_t ch, IOT_ADC_SAMPLE_RATE sample_rate, uint8_t gain,
                  IOT_ADC_WORK_MODE mode);

/**
 * @brief This function is to dynamic cfg port1 after open ADC which work in
 *        IOT_ADC_WORK_MODE_MULTI_CONTINUOUS mode
 *
 * @param port is ADC port IOT_ADC_PORT
 * @param ch is ADC signal src type for port1
 * @param gain is for ADC pga gain  for port1
 *
 * @return int32_t is signed adc code
 */
void iot_adc_dynamic_cfg_port(uint8_t port, uint8_t ch, uint8_t gain);

/**
 * @brief stop and close all adc ch
 *
 */
void iot_adc_close(void);

/**
 * @brief This function is to assaign port 0 for vad using
 *
 */
void iot_adc_out_vad_link(void);

/**
 * @brief This function is config adc data dma chansmit
 *
 */
uint8_t iot_adc_dma_config(void);

/**
 * @brief This function is config ada dump src
 *
 * @param sel is ada dump src IOT_ADC_DUMP_DATA_MODULE
 */
uint8_t iot_adc_ada_dump_module(IOT_ADC_DUMP_DATA_MODULE sel);

/**
 * @brief This function is config ada dump src
 *
 * @param trim_code is adc ldo trim code
 */
void iot_adc_ana_meter_ldo_trim(uint8_t trim_code);

/**
 * @brief This function is trim params for a adc port
 *
 * @param port is ADC port IOT_ADC_PORT
 * @param vref_ctrl is param vref trim code
 * @param vcm_ctrl is param vcm_ctrl trim code
 */
void iot_adc_ana_sdm_vref_trim(uint8_t port, uint8_t vref_ctrl, uint8_t vcm_ctrl);

/**
 * @brief This function is to poll a adc chanel result
 *
 * @param isdiff input is diff mode or single ended
 * @param adc_code signed adc result
 */
float iot_adc_2_mv(uint8_t isdiff, int32_t adc_code);

#ifdef __cplusplus
}
#endif

/**
* @}
* @}
*/
#endif   //_DRIVER_HAL_ADC_H
