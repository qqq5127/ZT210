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

#ifndef _DRIVER_NON_OS_ANC_H
#define _DRIVER_NON_OS_ANC_H

/**
 * @addtogroup HAL
 * @{
 * @addtogroup ANC
 * @{
 * This section introduces the ANC module's enum, structure, functions and how to use this driver.
 */
#include "types.h"

/** @defgroup hal_anc_enum Enum
  * @{
  */

/** @brief ANC input src id.*/
typedef enum {
    IOT_ANC_INPUT_SRC_DFE_0,
    IOT_ANC_INPUT_SRC_DFE_1,
    IOT_ANC_INPUT_SRC_DFE_2,
    IOT_ANC_INPUT_SRC_DFE_3,
    IOT_ANC_INPUT_SRC_DFE_4,
    IOT_ANC_INPUT_SRC_DFE_5,
    IOT_ANC_INPUT_SRC_ASRC_0,
    IOT_ANC_INPUT_SRC_ASRC_1,
    IOT_ANC_INPUT_SRC_MAX,
} IOT_ANC_INPUT_SRC_ID;

/** @brief ANC rx src value.*/
typedef enum {
    IOT_ANC_RX_SRC_NONE,
    IOT_ANC_RX_SRC_DFE = 0x01,
    IOT_ANC_RX_SRC_I2S = 0x02,
    IOT_ANC_RX_SRC_METER_ADC = 0x03,
    IOT_ANC_RX_SRC_MAX = 0x04,
    IOT_ANC_RX_SRC_ASRC = IOT_ANC_RX_SRC_DFE,
} IOT_ANC_RX_SRC_VALUE;

/** @brief ANC in src id.*/
typedef enum {
    IOT_ANC_IN_SRC_MIC_0,
    IOT_ANC_IN_SRC_MIC_1,
    IOT_ANC_IN_SRC_MIC_2,
    IOT_ANC_IN_SRC_MIC_3,
    IOT_ANC_IN_SRC_MIC_4,
    IOT_ANC_IN_SRC_MIC_5,
    IOT_ANC_IN_SRC_BIQUAD_0,
    IOT_ANC_IN_SRC_BIQUAD_1,
    IOT_ANC_IN_SRC_BIQUAD_2,
    IOT_ANC_IN_SRC_BIQUAD_3,
    IOT_ANC_IN_SRC_BIQUAD_4,
    IOT_ANC_IN_SRC_BIQUAD_5,
    IOT_ANC_IN_SRC_BIQUAD_6,
    IOT_ANC_IN_SRC_BIQUAD_7,
    IOT_ANC_IN_SRC_CLIPPING_0,
    IOT_ANC_IN_SRC_CLIPPING_1,
    IOT_ANC_IN_SRC_CLIPPING_2,
    IOT_ANC_IN_SRC_CLIPPING_3,
    IOT_ANC_IN_SRC_GATING_0,
    IOT_ANC_IN_SRC_GATING_1,
    IOT_ANC_IN_SRC_GATING_2,
    IOT_ANC_IN_SRC_GATING_3,
    IOT_ANC_IN_SRC_ASRC_0,
    IOT_ANC_IN_SRC_ASRC_1,
    IOT_ANC_IN_SRC_MAX,
    IOT_ANC_IN_SRC_NONE = IOT_ANC_IN_SRC_MAX,
} IOT_ANC_IN_SRC_ID;

/** @brief ANC commponent id.*/
typedef enum {
    IOT_ANC_BIQUAD_0,
    IOT_ANC_BIQUAD_1,
    IOT_ANC_BIQUAD_2,
    IOT_ANC_BIQUAD_3,
    IOT_ANC_BIQUAD_4,
    IOT_ANC_BIQUAD_5,
    IOT_ANC_BIQUAD_6,
    IOT_ANC_BIQUAD_7,
    IOT_ANC_CLIPPING_0,
    IOT_ANC_CLIPPING_1,
    IOT_ANC_CLIPPING_2,
    IOT_ANC_CLIPPING_3,
    IOT_ANC_GATING_0,
    IOT_ANC_GATING_1,
    IOT_ANC_GATING_2,
    IOT_ANC_GATING_3,
    IOT_ANC_BIQUAD_MAX,
} IOT_ANC_COMPONENT_ID;

/** @brief ANC out id.*/
typedef enum {
    IOT_ANC_OUT_DFE_0,
    IOT_ANC_OUT_DFE_1,
    IOT_ANC_OUT_MAX,
} IOT_ANC_OUT_ID;

/** @brief ANC coff param set.*/
typedef enum {
    IOT_ANC_COEFF_PARAM_0,
    IOT_ANC_COEFF_PARAM_1,
    IOT_ANC_COEFF_PARAM_MAX,
} IOT_ANC_COEFF_PARAM_SET;

/**
 * @}
 */

typedef void (*iot_anc_switch_mode_completed_cb)(void);

/**
 * @brief stash anc enable register
 *
 * @param sync If sync is true, just temporarily save the enable bits
 * and set them enable together when all of moduler configuration is complete
 */
void iot_anc_enable_sync(bool_t sync);

/**
 * @brief stash anc disable register
 *
 * @param sync If sync is true, just temporarily save the disable bits
 * and set them disable together when all of moduler configuration is complete
 */
void iot_anc_disable_sync(bool_t sync);

/**
 * @brief enable audio multipath sync
 *
 */
void iot_anc_multipath_sync(void);

/**
 * @brief This function is to start ANC module.
 *
 *
 */
void iot_anc_start(void);

/**
 * @brief This function is to stop ANC module.
 *
 */
void iot_anc_stop(void);

/**
 * @brief if asrc passes through anc module and anc_out_dfe_1 output,
 * need to configure clipping, limit data too large
 *
 * @param full_scale_limit anc spk full scale dB limit
 * @param enable if don't want to use drc, enable is false
 */
void iot_anc_drc_set(int full_scale_limit, bool_t enable);

/**
 * @brief This function is to config ANC module.
 *
 * @param rate is set rate for anc module
 * @param anc_coeff0 is unsigned integer setted to param_base
 * @param anc_coeff1 is unsigned integer setted to param_base
 */
void iot_anc_config(uint32_t rate, const uint32_t *anc_coeff0, const uint32_t *anc_coeff1);

/**
 * @brief anc interrupt config
 */
void iot_anc_interrupt_open(void);

/**
 * @brief Coeff switch time = sample_rate/frame_cnt_div * 4096, range 1 to 255
 *
 * @param frame_div_cnt
 */
void iot_anc_frame_division_counter(uint8_t frame_div_cnt);

/**
 * @brief to do
 *
 * @param id is ANC module's component id
 * @param in is ANC module's in src id
 */
void iot_anc_in_component_link(IOT_ANC_COMPONENT_ID id, IOT_ANC_IN_SRC_ID in);
/**
 * @brief to do
 *
 * @param id is ANC module component id
 * @param out is ANC module out id
 */
void iot_anc_out_component_link(IOT_ANC_COMPONENT_ID id, IOT_ANC_OUT_ID out);

/**
 * @brief anc delink out module
 *
 * @param id is ANC module component id
 * @param out is ANC module out id
 */
void iot_anc_out_component_delink(IOT_ANC_COMPONENT_ID id, IOT_ANC_OUT_ID out);

/**
 * @brief to do
 *
 * @param id
 * @param value
 */
void iot_anc_src_select(IOT_ANC_INPUT_SRC_ID id, IOT_ANC_RX_SRC_VALUE value);

/**
 * @brief to do
 *
 * @param anc_coeff
 * @param cb
 */
void iot_anc_write_coeff_table(const uint32_t *anc_coeff, iot_anc_switch_mode_completed_cb cb);

/**
 * @brief to do
 *
 * @param set
 * @param offset
 * @param param
 * @param len
 * @return uint8_t
 */
uint8_t iot_anc_write_coeff_param(IOT_ANC_COEFF_PARAM_SET set, uint32_t offset, const uint32_t *param,
                                  uint32_t len);

/**
 * @brief get switch mode complete flag
 *
 */
bool_t iot_anc_get_switch_mode_done(void);

/**
 * @brief clear switch mode complete flag
 *
 */
void iot_anc_switch_mode_done_clear(void);

/**
 * @brief get anc clk open
 * @return 0 diable, 1 enable
 *
 */
uint8_t iot_get_anc_clk_enable(void);
/**
* @}
* @}
*/

#endif
