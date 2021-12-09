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

#ifndef _DRIVER_HAL_SDM_DAC_H
#define _DRIVER_HAL_SDM_DAC_H

/**
 * @addtogroup HAL
 * @{
 * @addtogroup AUDIO_DAC
 * @{
 * This section introduces the DMA module's enum, structure, functions and how to use this driver.
 */

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    IOT_TX_DFE_CHN_0,
    IOT_TX_DFE_CHN_1,
    IOT_TX_DFE_CHN_DOUBLE,
    IOT_TX_DFE_CHN_MAX,
} IOT_TX_DFE_CHN_ID;

typedef enum {
    IOT_DFE_TO_I2S_24_BYTE,
    IOT_DFE_TO_I2S_16_BYTE,
} IOT_TX_DFE_TO_I2S_BIT_SELECT;

typedef enum {
    IOT_DFE_TO_I2S_SRC_ANC,
    IOT_DFE_TO_I2S_SRC_VOLUME,
    IOT_DFE_TO_I2S_SRC_DUMP,
    IOT_DFE_TO_I2S_SRC_DC_CLIP,
} IOT_TX_DFE_TO_I2S_SRC_SELECT;

typedef enum {
    IOT_TX_DFE_DUMP_SELECT_RAW_DATA,
    IOT_TX_DFE_DUMP_SELECT_DOWNSAMPLE,
    IOT_TX_DFE_DUMP_SELECT_FILTER,
    IOT_TX_DFE_DUMP_SELECT_HPF,
} IOT_TX_DFE_DUMP_SELECT;

typedef enum {
    IOT_TX_DFE_GAIN_BYPASS,
    IOT_TX_DFE_GAIN_ZERO_CROSS,
    IOT_TX_DFE_GAIN_SOFT_RAMP,
    IOT_TX_DFE_GAIN_MIX,
} IOT_TX_DFE_GAIN_MODE;

typedef enum {
    IOT_TX_DEF_SRC_DFE_0,
    IOT_TX_DEF_SRC_DFE_1,
    IOT_TX_DEF_SRC_DFE_2,
    IOT_TX_DEF_SRC_DFE_3,
    IOT_TX_DEF_SRC_DFE_4,
    IOT_TX_DEF_SRC_DFE_5,
    IOT_TX_DEF_SRC_DFE_MAX,
    IOT_TX_DEF_SRC_ASRC = IOT_TX_DEF_SRC_DFE_MAX,
    IOT_TX_DEF_SRC_ANC,
    IOT_TX_DEF_SRC_MAX,
} IOT_TX_DEF_SRC_ID;

typedef enum {
    IOT_TX_DFE_FREQ_62K5,
    IOT_TX_DFE_FREQ_100K,
    IOT_TX_DFE_FREQ_125K,
    IOT_TX_DFE_FREQ_200K,
    IOT_TX_DFE_FREQ_400K,
    IOT_TX_DFE_FREQ_500K,
    IOT_TX_DFE_FREQ_800K,
    IOT_TX_DFE_FREQ_MAX,
} IOT_TX_DFE_FREQ_ID;

typedef enum {
    IOT_TX_DFE_NORMAL,
    IOT_TX_DFE_LOOPBACK,
    IOT_TX_DFE_MAX,
} IOT_TX_DFE_MODE;

typedef enum {
    IOT_TX_DFE_GAIN_CONTROL_DIRECTLY,
    IOT_TX_DFE_GAIN_CONTROL_ZERO_CROSS,
    IOT_TX_DFE_GAIN_CONTROL_SOFT_RAMP,
    IOT_TX_DFE_GAIN_CONTROL_DOUBLE,
    IOT_TX_DFE_GAIN_CONTROL_MIX,
} IOT_TX_DFE_GAIN_CONTROL_MODE;

typedef enum {
    IOT_SDM_DAC_DIRECT_FEEDTHROUGH,
    IOT_SDM_DAC_L_R_SWAP,
    IOT_SDM_DAC_L_R_ADD_DIV_2,
    IOT_SDM_DAC_L,
    IOT_SDM_DAC_R,
    IOT_SDM_DAC_L_R_ADD,
    IOT_SDM_DAC_MAX,
} IOT_SDM_DAC_STEREO_MIX_MODE;

typedef enum {
    IOT_TX_DFE_SDM_BIT_OUT,   //sdm output
    IOT_TX_DFE_CLIP_OUT,      //clip output
    IOT_TX_DFE_DCC_OUT,       //dc calibration output
    IOT_TX_DFE_NG_OUT,        //noise gating
    IOT_TX_DFE_SINC_OUT,      //sinc output
    IOT_TX_DFE_HBF_OUT,       //hbf output
    IOT_TX_DFE_HPF_OUT,       //hpf output
    IOT_TX_DFE_HPF_IN,        //hpf input
    IOT_TX_DFE_DUMP_DATA_MAX,
} IOT_TX_DFE_DUMP_DATA_MODULE;

typedef enum {
    IOT_TX_DFE_DCC_BYPASS_HPF_HBF = 104,
    IOT_TX_DFE_DCC_ENABLE_HPF_BYPASS_HBF = 120,
    IOT_TX_DFE_DCC_BYPASS_HPF_ENABLE_HBF = 636,
    IOT_TX_DFE_DCC_ENABLE_HPF_HBF = 716,
} IOT_TX_DFE_DCC_SYNC_CNT;

typedef enum {
    IOT_SDM_DAC_POWER_BALANCE_POLLING,
    IOT_SDM_DAC_POWER_BALANCE_INTERRUT,
    IOT_SDM_DAC_POWER_BALANCE_MODD_MAX,
}IOT_SDM_DAC_POWER_BALANCE_MODE;

typedef void (*iot_tx_dfe_power_balance_callback)(void);
typedef void (*iot_tx_dfe_force_reset_callback)(uint8_t data);

typedef struct iot_sdm_dac_power_balance_cfg {
    IOT_TX_DFE_CHN_ID chn;
    IOT_SDM_DAC_POWER_BALANCE_MODE mode;
    iot_tx_dfe_power_balance_callback cb;
    bool_t balance_on;
    bool_t close_dfe;
}iot_tx_dfe_power_balance_cfg_t;

/**
 * @brief init dfe tx
 *
 */
void iot_tx_dfe_init(void);

/**
 * @brief config the dfe tx interrupt
 *
 */
void iot_tx_dfe_irq_config(void);

/**
 * @brief This function is to enable AUDIO DAC MCLK module.
 *
 */
void iot_sdm_dac_mclk_enable(void);

/**
 * @brief This function is to disable AUDIO DAC MCLK module.
 *
 */
void iot_sdm_dac_mclk_disable(void);

/**
 * @brief This function is to set tx dfe's dump module.
 *
 * @param sel is tx dfe dump data module
 * @return uint8_t RET_OK or RET_INVAL,RET_OK for success else error.
 */
uint8_t iot_tx_dfe_dump_module(IOT_TX_DFE_DUMP_DATA_MODULE sel);

/**
 * @brief This function is to set tx dfe dump channel.
 *
 * @param chn is  tx dfe channel id
 */
void iot_tx_dfe_dump_channel(IOT_TX_DFE_CHN_ID chn);

/**
 * @brief This function is to config tx dfe.
 *
 * @param freq
 */
void iot_tx_dfe_freq_config(IOT_TX_DFE_FREQ_ID freq);

/**
 * @brief This function is to deconfig tx dfe config.
 *
 * @param chn is tx dfe channel id
 * @param freq
 */
void iot_tx_dfe_config(IOT_TX_DFE_CHN_ID chn, IOT_TX_DFE_FREQ_ID freq);

/**
 * @brief This function is to enable tx dfe hpf.
 *
 * @param chn  is tx dfe channel id
 * @param enable is to enable tx dfe hpf.
 */
void iot_tx_dfe_hpf_enable(IOT_TX_DFE_CHN_ID chn, bool_t enable);

/**
 * @brief This function is to enable tx dfe.
 *
 * @param chn is tx dfe channel id
 * @param sync whetcher dfe need to synchronize
 */
void iot_tx_dfe_enable(IOT_TX_DFE_CHN_ID chn, bool_t sync);

/**
 * @brief This function is to disable tx dfe.
 *
 * @param chn is tx dfe channel id
 * @param sync whetcher dfe need to synchronize
 */
void iot_tx_dfe_disable(IOT_TX_DFE_CHN_ID chn, bool_t sync);

/**
 * @brief This function is to reset tx dfe.
 *
 * @param chn is tx dfe channel id
 */
void iot_tx_dfe_reset(IOT_TX_DFE_CHN_ID chn);

/**
 * @brief This function is to start tx dfe.
 *
 * @param chn is tx dfe channel id
 */
void iot_tx_dfe_start(IOT_TX_DFE_CHN_ID chn);

/**
 * @brief This function is to stop tx dfe.
 *
 * @param chn is tx dfe channel id
 */
void iot_tx_dfe_stop(IOT_TX_DFE_CHN_ID chn);

/**
 * @brief This function is to start tx dfe dem.
 *
 * @param chn is tx dfe channel id
 */
void iot_tx_dfe_dem_start(IOT_TX_DFE_CHN_ID chn);

/**
 * @brief This function is to stop tx dfe dem.
 *
 * @param chn is tx dfe channel id
 */
void iot_tx_dfe_dem_stop(IOT_TX_DFE_CHN_ID chn);

/**
 * @brief tx dfe soft reset
 *
 */
void iot_tx_dfe_soft_reset(void);

/**
 * @brief This function is to active noise ancelling for tx dfe.
 *
 * @param chn is tx dfe channel id
 */
void iot_tx_dfe_link_anc(IOT_TX_DFE_CHN_ID chn);

/**
 * @brief to do.
 *
 * @param chn is tx dfe channel id
 */
void iot_tx_dfe_link_asrc(IOT_TX_DFE_CHN_ID chn);

/**
 * @brief to do
 *
 * @param src
 * @param sel
 */
void iot_tx_dfe_link_i2s(IOT_TX_DFE_TO_I2S_SRC_SELECT src, IOT_TX_DFE_TO_I2S_BIT_SELECT sel);

/**
 * @brief to do
 *
 * @param chn
 * @param id
 * @return uint8_t
 */
uint8_t iot_tx_dfe_link_rx_dfe(IOT_TX_DFE_CHN_ID chn, IOT_TX_DEF_SRC_ID id);

/**
 * @brief This function is to config tx dfe threshold.
 *
 * @param ms
 * @param threshold0
 * @param threshold1
 */
void iot_tx_dfe_threshold_config(uint8_t ms, uint32_t threshold0, uint32_t threshold1);

/**
 * @brief  to do
 *
 * @param rate
 * @param threshold
 */
void iot_tx_dfe_clip_rate(uint8_t rate, uint32_t threshold);

/**
 * @brief This function is to config tx dfe gain.
 *
 */

/* gain */
void iot_tx_dfe_gain_config(IOT_TX_DFE_CHN_ID chn, int16_t digital_gain, int8_t analog_gain);

/**
 * @brief This function is to init tx dfe gain.
 *
 * @param dcc_delay is dcc delay time
 */
void iot_tx_dfe_gain_init(uint16_t dcc_delay);

/**
 * @brief the sampling rate is 800 khz, the sampling points are controlled
 * by the dac gain ramp counter,the delay is: the dac gain ramp counter*16/800ms,
 * 16 is a fixed multiple, a total of steps*32/800ms to be adjusted
 *
 * @param chn is tx dfe channel id
 * @param mode is DAC gain control mode
 * @param up true-add gain by step, false-reduce gain by step
 * @param dig_steps digtial gain steps
 * @param need_done need to be done
 * @return uint8_t RET_OK for success else error.
 */
uint8_t iot_tx_dfe_adjust_digital_gain(IOT_TX_DFE_CHN_ID chn, IOT_TX_DFE_GAIN_CONTROL_MODE mode,
                                       bool_t up, int16_t dig_steps, bool_t need_done);

/**
 * @brief This function is to adjust tx dfe analog gain
 *
 * @param chn is tx dfe channel id
 * @param mode is DAC gain control mode
 * @param up true-add gain by step, false-reduce gain by step
 * @return uint8_t RET_OK for success else error.
 */
uint8_t iot_tx_dfe_adjust_analog_gain(IOT_TX_DFE_CHN_ID chn, IOT_TX_DFE_GAIN_CONTROL_MODE mode,
                                      bool_t up);

/**
 * @brief This function is to set tx dfe gain step.
 *
 * @param chn is tx dfe channel id
 * @param dig_step is digital step
 * @param ana_step is analog step
 */
void iot_tx_dfe_set_gain_step(IOT_TX_DFE_CHN_ID chn, int16_t dig_step, uint8_t ana_step);

/**
 * @brief This function is to set tx dfe ana gain step.
 *
 * @param chn is tx dfe channel id
 * @param ana_gain_step is ana gain step
 */
void iot_tx_dfe_ana_gain_step_set(IOT_TX_DFE_CHN_ID chn, uint8_t ana_gain_step);

/**
 * @brief This function is to mute tx dfe hw digital.
 *
 * @param chn is tx dfe channel id
 * @param digital_gain_steps is
 */

void iot_tx_dfe_hw_digital_mute(IOT_TX_DFE_CHN_ID chn, int16_t digital_gain_steps);
/**
 * @brief This function is to unmute tx dfe hw digital.
 *
 * @param chn is tx dfe channel id
 * @param digital_gain_steps is digital gain steps
 */
void iot_tx_dfe_hw_digital_unmute(IOT_TX_DFE_CHN_ID chn, int16_t digital_gain_steps);
/**
 * @brief to do
 *
 * @param chn
 * @param steps
 * @return uint8_t RET_OK for success else error.
 */
uint8_t iot_tx_dfe_sw_digital_gain_up(IOT_TX_DFE_CHN_ID chn, int16_t steps);

/**
 * @brief to do
 *
 * @param chn
 * @param steps
 * @return uint8_t RET_OK for success else error.
 */
uint8_t iot_tx_dfe_sw_digital_gain_down(IOT_TX_DFE_CHN_ID chn, int16_t steps);

/**
 * @brief adjust gain for mute unmute
 *
 * @param chn
 * @param mode adjust gain mode
 * @param target_gain target gain value
 * @param need_done need adjust gain completed
 * @return uint8_t RET_OK for success else error.
 */
uint8_t iot_tx_dfe_sw_digital_target_gain_set(IOT_TX_DFE_CHN_ID chn, uint8_t mode,
                                              int16_t target_gain, bool_t need_done);
/**
 * @brief adjust gain for volume
 *
 * @param chn
 * @param target_gain  target gain value
 * @param mode is DAC gain control mode
 * @param need_done need adjust gain completed
 * @return uint8_t uint8_t RET_OK for success else error.
 */
uint8_t iot_tx_dfe_digital_gain_set(IOT_TX_DFE_CHN_ID chn, int16_t target_gain,
                                    IOT_TX_DFE_GAIN_CONTROL_MODE mode, bool_t need_done);

/**
 * @brief to do
 *
 * @param chn
 * @return int16_t
 */
int16_t iot_tx_dfe_get_dig_gain(IOT_TX_DFE_CHN_ID chn);

/**
 * @brief This function is to mix dac sound.
 *
 * @param mixer is select DAC stereo mix mode
 */
void iot_tx_dfe_mixer(IOT_SDM_DAC_STEREO_MIX_MODE mixer);

/**
 * @brief DC calibration compensate
 *
 * @param chn
 * @param comp_value  DC calibration compensate accuracy 600000uV / 2^19 = 1.144uV
 */
void iot_sdm_dac_dcc_compensate(IOT_TX_DFE_CHN_ID chn, uint16_t comp_value);

/**
 * @brief This function is to input data from dac dfe
 *
 * @param chn is tx dfe channel id
 * @param enable is enable input data from tx dfe
 */
void iot_tx_dfe_dem_data_force(IOT_TX_DFE_CHN_ID chn, bool_t enable);

/**
 * @brief This function is to get tx dfe power balance.
 *
 */
/* power balance */
bool_t iot_tx_dfe_power_balance_get(IOT_TX_DFE_CHN_ID chn);

/**
 * @brief This function is to switch on tx dfe power balance.
 *
 * @param cfg if need balance enable is true,otherwise, enable is false, if need interrupt,
 * mode select interrupt and need callback.
 */
uint8_t iot_tx_dfe_power_balance(const iot_tx_dfe_power_balance_cfg_t *cfg);

/**
 * @brief This function is to clear tx dfe power balance.
 *
 * @param chn is tx dfe channel id
 */
void iot_tx_dfe_power_balance_clr(IOT_TX_DFE_CHN_ID chn);

/**
 * @brief
 *
 * @param cnt
 */
void iot_tx_dfe_power_balance_dcc_sync_cnt(IOT_TX_DFE_DCC_SYNC_CNT cnt);

/**
 * @brief to solve the problem that the analog gain configuration can not
 * take effect,it is necessary to send an invalid data to the analog part
 * during initialization,so that the analog gain configuration can take effect
 *
 * @param chn is tx dfe channel id
 */
void iot_dfe_dac_ana_gain_sync(IOT_TX_DFE_CHN_ID chn);
/**
 * @brief
 *
 * @param chn is tx dfe channel id
 * @param overwrite  true is enable overwrite gain,false is disable overwrite gain.
 * @param dig_overwrite overwrite the default mute and power balance value, with the follow steps
 * configuration.0:use the default mute digital step is current gain to -511, analog steps is current
 * gain to 8. The default balance step is current analog gain to 8
 * @param ana_overwrite the original analog gain. 4'd15=-1.5dB, 4'd14=-3dB, ... , 4'd8=-12dB,
 * gain step is 1.5dB, but 4'd0=-19.5dB, 1~7 not used
 */
void iot_sdm_dac_gain_overwrite_gain(IOT_TX_DFE_CHN_ID chn, bool_t overwrite, uint16_t dig_overwrite,
                                        uint8_t ana_overwrite);

 /**
 * @brief
 *
 * @param chn is tx dfe channel id
 */
void iot_tx_dfe_dem_soft_reset(IOT_TX_DFE_CHN_ID chn);

/**
 * @brief get cur dac dig gain
 * @return int16_t cur dig gain in register.
 */
int16_t iot_sdm_dac_get_dig_gain(void);

#ifdef __cplusplus
}
#endif

/**
 * @}
 * addtogroup AUDIO_DAC
 * @}
 * addtogroup HAL
 */

#endif
