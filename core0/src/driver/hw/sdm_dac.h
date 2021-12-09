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

#ifndef __DRIVER_HW_SDM_DAC_H__
#define __DRIVER_HW_SDM_DAC_H__

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    DAC_CHN0 = 0,
    DAC_CHN1,
    DAC_CHN_DOUBUL,
    DAC_CHN_MAX,
    DAC_CHN_NONE,
} DAC_CHN_ID;

typedef enum {
    DAC_GAIN_CONTROL_DIRECTLY,
    DAC_GAIN_CONTROL_ZERO_CROSS,
    DAC_GAIN_CONTROL_SOFT_RAMP,
    DAC_GAIN_CONTROL_DOUBLE,
    DAC_GAIN_CONTROL_MIX,
} DAC_GAIN_CONTROL_MODE;

typedef enum {
    DAC_DUMP_NORMAL_TRIG,
    DAC_DUMP_SOFTWARE_TRIG,
} DAC_DUMP_TRIG;

typedef enum {
    DAC_DUMP_SELECT_RAW_DATA,
    DAC_DUMP_SELECT_DOWNSAMPLE,
    DAC_DUMP_SELECT_FILTER,
    DAC_DUMP_SELECT_HPF,
} DAC_DUMP_SELECT;

typedef enum {
    DAC_TO_I2S_SRC_ANC,
    DAC_TO_I2S_SRC_VOLUME,
    DAC_TO_I2S_SRC_DUMP,
    DAC_TO_I2S_SRC_DC_CLIP,
} DAC_TO_I2S_SRC_SELECT;

typedef enum {
    DAC_TO_I2S_24_BIT,
    DAC_TO_I2S_16_BIT,
} DAC_TO_I2S_BIT_SELECT;

typedef struct sdm_dac_sinc_param {
    uint8_t sinc_num;
    uint8_t sinc_stg;
    uint8_t lsh;
} sdm_dac_sinc_param_t;

typedef struct tx_dfe_config {
    uint8_t num;
    uint8_t stg;
    uint8_t lsh;
    uint8_t hpf_coef;
    bool_t bypass_hpf;
    bool_t bypass_sinc;
    bool_t dump_soft_trig;
    DAC_DUMP_SELECT dump_sel;
} tx_dfe_config_t;

typedef struct dac_gain_cfg {
    int16_t dig_gain;
    uint16_t dig_step;
    int8_t ana_gain;
    uint8_t ana_step;
} sdm_dac_gain_cfg_t;

typedef struct sdm_dac_gain_adjust {
    uint16_t ramp_cnt;
    uint16_t adi_sync_cnt;
    uint16_t zc_cnt;
    uint16_t zc_threshold;
    uint16_t dcc_sync_cnt;
    bool_t zc_check_amplitude;
}sdm_dac_gain_adjust_t;

void sdm_dac_adjust_gain_it_enable(bool_t en);
void sdm_dac_adjust_gain_it_clear(bool_t clr);
bool_t sdm_dac_get_adjust_gain_it_state(void);
bool_t sdm_dac_get_adjust_gain_it_done(void);

bool_t sdm_dac_check_gain_adjust_done(void);
void sdm_dac_reset(bool_t reset);
void sdm_dac_enable_channel(DAC_CHN_ID chn, bool_t enable);
void sdm_dac_anc_sync(bool_t sync);
void sdm_dac_clip_rate(uint8_t rate, uint32_t threshold);
void sdm_dac_clear_gain_adjust_done(void);
void sdm_dac_dump_sel(uint8_t sel);
void sdm_dac_config_sinc(const sdm_dac_sinc_param_t *sinc_param);
void sdm_dac_config_hbf_en(DAC_CHN_ID chn, uint8_t hbf_en);
void sdm_dac_config_channel_sinc(DAC_CHN_ID chn, bool_t bypass_sinc);
void sdm_dac_config_dump(DAC_DUMP_SELECT dump_sel, bool_t dump_soft_trig);
void sdm_dac_config_channel_hpf(DAC_CHN_ID chn, bool_t bypass_hpf,
                                uint8_t hpf_coef);
void sdm_dac_threshold_config(uint8_t ms, uint32_t threshold0,
                              uint32_t threshold1);
void sdm_dac_to_i2s_select(DAC_TO_I2S_SRC_SELECT sel,
                           DAC_TO_I2S_BIT_SELECT bit);
void sdm_dac_power_balance(DAC_CHN_ID chn, bool_t start);
bool_t sdm_dac_get_power_balance_complete(DAC_CHN_ID chn);
void sdm_dac_mixer(uint8_t mixer);

void sdm_dac_gain_init(const sdm_dac_gain_adjust_t *cfg);
void sdm_dac_gain_config(DAC_CHN_ID chn, const sdm_dac_gain_cfg_t *cfg);
void sdm_dac_dig_gain_start(DAC_CHN_ID chn, bool_t up, DAC_GAIN_CONTROL_MODE mode);
void sdm_dac_gain_mode(DAC_CHN_ID chn, DAC_GAIN_CONTROL_MODE mode);
void sdm_dac_ana_gain_start(DAC_CHN_ID chn, bool_t up, DAC_GAIN_CONTROL_MODE mode);
void sdm_dac_gain_start(DAC_CHN_ID chn, bool_t up, DAC_GAIN_CONTROL_MODE mode);
void sdm_dac_adjust_gain_mode(DAC_CHN_ID chn, DAC_GAIN_CONTROL_MODE mode);
void sdm_dac_set_ana_gain_step(DAC_CHN_ID chn, uint8_t ana_step);
void sdm_dac_dig_gain_threshold(uint16_t threshold);
void sdm_dac_set_gain_step(DAC_CHN_ID chn, int16_t dig_step, uint8_t ana_step);
void sdm_dac_gain_overwrite_power_scale(DAC_CHN_ID chn, bool_t overwrite, uint16_t dig_overwrite,
                                        uint8_t ana_overwrite);
void sdm_dac_hw_digital_mute(DAC_CHN_ID chn, int16_t digital_gain_steps);
void sdm_dac_hw_digital_unmute(DAC_CHN_ID chn, int16_t digital_gain_steps);
void sdm_dac_mute(DAC_CHN_ID chn, bool_t on);
void sdm_dac_mute_balance_clear(DAC_CHN_ID chn, bool_t clear);
bool_t sdm_dac_get_mute_complete(DAC_CHN_ID chn);
int16_t sdm_dac_get_dig_gain(DAC_CHN_ID chn);
uint8_t sdm_dac_get_ana_gain(DAC_CHN_ID chn);
void sdm_dac_dcc_compensate(DAC_CHN_ID chn, uint16_t comp_value);

#ifdef __cplusplus
}
#endif

#endif
