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

#ifndef _DRIVER_HAL_SPK_DAC_H
#define _DRIVER_HAL_SPK_DAC_H

#include "types.h"

#define IOT_ENABLE 1
#define IOT_DISABLE 0
#define IOT_RDAC_VCM_DEFAULT 0b1100
#define IOT_VREF_1P2_1P4_DELTA_CODE 0x5

typedef enum {
    IOT_SPK_DAC_PORT_0,
    IOT_SPK_DAC_PORT_1,
    IOT_SPK_DAC_PORT_MAX,
} IOT_SPK_DAC_PORT_ID;

typedef enum{
    IOT_SPK_DAC_SPK_OUT_NONE,
    IOT_SPK_DAC_SPK_OUT_ATB,
}IOT_SPK_DAC_SPK_OUT;

typedef enum{
    IOT_SPK_DAC_RDAC_VREF_0P96,
    IOT_SPK_DAC_RDAC_VREF_0P98,
    IOT_SPK_DAC_RDAC_VREF_1P0,
    IOT_SPK_DAC_RDAC_VREF_1P02,
    IOT_SPK_DAC_RDAC_VREF_1P04,
    IOT_SPK_DAC_RDAC_VREF_1P08,
    IOT_SPK_DAC_RDAC_VREF_1P12,
    IOT_SPK_DAC_RDAC_VREF_1P16,
    IOT_SPK_DAC_RDAC_VREF_1P2,
    IOT_SPK_DAC_RDAC_VREF_1P24,
    IOT_SPK_DAC_RDAC_VREF_1P28,
    IOT_SPK_DAC_RDAC_VREF_1P32,
    IOT_SPK_DAC_RDAC_VREF_1P36,
    IOT_SPK_DAC_RDAC_VREF_1P40,
    IOT_SPK_DAC_RDAC_VREF_1P42,
    IOT_SPK_DAC_RDAC_VREF_1P44,
}IOT_SPK_DAC_RDAC_VREF;

/**
 * @brief This function is to do DAC VREF test
 *
 * @param port is to select left channel or right channel
 * @param vref_out is vcm value
 */
void iot_spk_dac_vref_trim(uint8_t port, IOT_SPK_DAC_RDAC_VREF vref_out);

/**
 * @brief This function is to do DAC/SPK VCM test
 *
 * @param port is to select left channel or right channel
 * @param vcm_out is vcm value
 */
void iot_spk_dac_vcm_trim(uint8_t port, uint8_t vcm_out);

/**
 * @brief This function is to trim speaker compensation.
 *
 * @param port is to select left channel or right channel
 * @param is_pos is to select left channel or right channel
 * @param comp_pos is compensation value
 */
void iot_spk_dac_dc_offset_trim(uint8_t port, bool_t is_pos, uint16_t comp_pos);

/**
 * @brief This function is to init speaker.
 *
 * @param port is to select left channel or right channel
 */
void iot_spk_dac_init(uint8_t port);

/**
 * @brief This function is to control Speaker gain
 *
 * @param port is to select left channel or right channel
 * @param gain is speaker gain value
 */
void iot_spk_dac_gain(uint8_t port, uint8_t gain);

/**
 * @brief This function is to deinit speaker.
 *
 */
void iot_spk_dac_deinit(void);

/**
 * @brief This function is to enable dac channel.
 *
 * @param port is to select left channel or right channel
 */
void iot_spk_dac_enable(uint8_t port);

/**
 * @brief start spk
 *
 * @param port is to select left channel or right channel
 */
void iot_spk_dac_start(uint8_t port);

/**
 * @brief This function is to disable dac channel.
 *
 * @param port is to select left channel or right channel
 */
void iot_spk_dac_disable(uint8_t port);

#endif
