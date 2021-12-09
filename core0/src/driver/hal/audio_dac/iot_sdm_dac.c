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

/* common includes */
#include "types.h"
#include "math.h"
#include "os_utils.h"

/* hw includes */
#include "aud_glb.h"
#include "aud_if.h"
#include "sdm_dem.h"
#include "sdm_dac.h"

/* hal includes */
#include "iot_irq.h"
#include "iot_timer.h"
#include "iot_sdm_dac.h"

#include "driver_dbglog.h"

#define IOT_TX_DFE_NG_DUR_MS                1
#define IOT_TX_DFE_HPF_COEFF                50
#define IOT_TX_DFE_NG_THRESHOLD0            128
#define IOT_TX_DFE_NG_THRESHOLD1            256
#define IOT_TX_DFE_DEM_BARREL_SHIFT_OPERATE 0
#define IOT_SDM_DAC_TX_DFE_SINC_STG         2
#define IOT_SDM_DAC_TX_DFE_SINC_STG_800K    3
#define IOT_SDM_DAC_GAIN_MIN                -511
#define IOT_SDM_DAC_GAIN_MAX                511
#define IOT_TX_DFE_POWER_BALANCE_STEPS      8
#define IOT_TX_DFE_ZERO_CROSS_THRESHOLD     0x800
#define IOT_TX_DFE_DCC_DELAY_DEFAULT        55
//(576 gain steps) * 20 us(ramp down time, sw_gain_ramp_cnt*16/0.8 us)/1000us + 1 = 12.52ms
#define IOT_TX_DFE_ADJUST_GAIN_WAIT_CNT_MAX           13
#define IOT_TX_DFE_ADJUST_GAIN_CONTROL_DIRECTLY_DELAY 35
#define IOT_TX_DFE_POWER_BALANCE_TIMEOUT_CNT          10
#define IOT_TX_DFE_ADJUST_ANA_GAIN_TIMEOUT_CNT        3

typedef struct iot_tx_dfe_freq_config {
    uint8_t sinc_num;
    uint8_t hbf_en;
} iot_tx_dfe_freq_config_t;

typedef struct iot_tx_dfe_state {
    bool_t power_balance_on;
    int16_t power_balance_off_gain;
    bool_t adjust_gain_in_use;
    bool_t power_balance_off_gain_valid;
    iot_tx_dfe_power_balance_callback power_balance_cb[IOT_TX_DFE_CHN_MAX];
} iot_tx_dfe_state_t;

static iot_tx_dfe_state_t tx_dfe;
/*
 * table be used by config the sdm dac frequence
 * 1:sinc num, 2:hbf enable
 */
static const iot_tx_dfe_freq_config_t tx_dfe_param_table[IOT_TX_DFE_FREQ_MAX] = {
    {15, 3}, {9, 3}, {7, 3}, {4, 3}, {4, 1}, {7, 0}, {4, 0}};

static uint32_t iot_tx_dfe_isr_handler(uint32_t vector, uint32_t data)
    IRAM_TEXT(iot_tx_dfe_isr_handler);
static uint8_t iot_tx_dfe_switch_mode_wait_gain_done(IOT_TX_DFE_CHN_ID chn);
static uint32_t iot_dac_log2(uint32_t value, uint8_t mode);

void iot_tx_dfe_init(void)
{
    tx_dfe.adjust_gain_in_use = false;
    tx_dfe.power_balance_cb[0] = NULL;
    tx_dfe.power_balance_cb[1] = NULL;
    tx_dfe.power_balance_on = false;
    tx_dfe.power_balance_off_gain_valid = false;
    tx_dfe.power_balance_off_gain = IOT_SDM_DAC_GAIN_MIN;
}

void iot_tx_dfe_irq_config(void)
{
    iot_irq_t tx_dfe_isr;
    tx_dfe_isr = iot_irq_create(SDM_DAC_REG_INT, 0, iot_tx_dfe_isr_handler);
    sdm_dac_adjust_gain_it_clear(true);
    iot_irq_unmask(tx_dfe_isr);
}

void iot_sdm_dac_mclk_enable(void)
{
    audio_enable_module(AUDIO_MODULE_MCLK_DAC);
}

void iot_sdm_dac_mclk_disable(void)
{
    audio_disable_module(AUDIO_MODULE_MCLK_DAC);
}

void iot_tx_dfe_gain_config(IOT_TX_DFE_CHN_ID chn, int16_t digital_gain, int8_t analog_gain)
{
    sdm_dac_gain_cfg_t cfg;
    cfg.dig_gain = digital_gain;
    cfg.ana_gain = analog_gain;
    cfg.dig_step = 0;
    cfg.ana_step = 0;

    sdm_dac_gain_config((DAC_CHN_ID)chn, &cfg);
}

void iot_tx_dfe_gain_init(uint16_t dcc_delay)
{
    sdm_dac_gain_adjust_t adjust;

    adjust.ramp_cnt = 0x01; /* the dac ramp counter * 2^4 * data_valid */
    adjust.zc_cnt = 0x01;   /* zero cross counter * 2^4 * detect_pulse */
    /* the amplitude threshold for zero-cross detect */
    adjust.zc_threshold = IOT_TX_DFE_ZERO_CROSS_THRESHOLD;
    adjust.zc_check_amplitude = true;
    adjust.dcc_sync_cnt = dcc_delay;
    /* the analog gain delay from gain control to dcc.
    the default value is based on 800KHz input sample
    rate and 5xsinc4 output to 4MHz */
    adjust.adi_sync_cnt = dcc_delay + IOT_TX_DFE_DCC_DELAY_DEFAULT;

    sdm_dac_gain_init(&adjust);
}

int16_t iot_sdm_dac_get_dig_gain(void)
{
    return sdm_dac_get_dig_gain((DAC_CHN_ID)IOT_TX_DFE_CHN_1);
}

uint8_t iot_tx_dfe_adjust_digital_gain(IOT_TX_DFE_CHN_ID chn, IOT_TX_DFE_GAIN_CONTROL_MODE mode,
                                       bool_t up, int16_t dig_steps, bool_t need_done)
{
    uint32_t adjust_gain_timeout_cnt = 0;
    if (tx_dfe.adjust_gain_in_use) {
        return RET_BUSY;
    }
    tx_dfe.adjust_gain_in_use = true;
    sdm_dac_set_gain_step((DAC_CHN_ID)chn, dig_steps, 0);
    if (!need_done) {
        sdm_dac_dig_gain_start((DAC_CHN_ID)chn, up, (DAC_GAIN_CONTROL_MODE)mode);
        if (mode == IOT_TX_DFE_GAIN_CONTROL_DIRECTLY) {
            iot_timer_delay_us(IOT_TX_DFE_ADJUST_GAIN_CONTROL_DIRECTLY_DELAY);
            if (!sdm_dac_get_adjust_gain_it_done()) {
                tx_dfe.adjust_gain_in_use = false;
                return RET_BUSY;
            } else {
                sdm_dac_adjust_gain_it_clear(true);
            }
        } else {
            while (!sdm_dac_get_adjust_gain_it_done()) {
                os_delay(1);
                adjust_gain_timeout_cnt++;
                /* if you do not wait again, it is likely not to be successful,
                resulting in switching mode to produce pop sound. If you wait too much,
                there is too much delay in tuning the gain */
                if (adjust_gain_timeout_cnt >= IOT_TX_DFE_ADJUST_GAIN_WAIT_CNT_MAX) {
                    tx_dfe.adjust_gain_in_use = false;
                    return RET_FAIL;
                }
            }
            sdm_dac_adjust_gain_it_clear(true);
        }
    } else {
        sdm_dac_dig_gain_start((DAC_CHN_ID)chn, up, (DAC_GAIN_CONTROL_MODE)mode);
        while (!sdm_dac_get_adjust_gain_it_done()) {
            os_delay(1);
        }
        sdm_dac_adjust_gain_it_clear(true);
    }
    tx_dfe.adjust_gain_in_use = false;
    DBGLOG_DRIVER_INFO("[GAIN] tx dfe gain success,chn:%d, cur reg gain: %d\n", chn,
                       sdm_dac_get_dig_gain((DAC_CHN_ID)chn));
    return RET_OK;
}

static uint8_t iot_tx_dfe_switch_mode_wait_gain_done(IOT_TX_DFE_CHN_ID chn)
{
    DBGLOG_DRIVER_INFO("[GAIN] iot_tx_dfe_switch_mode_wait_gain_done, chn:%d\n", chn);
    sdm_dac_gain_mode((DAC_CHN_ID)chn, DAC_GAIN_CONTROL_DIRECTLY);
    iot_timer_delay_us(IOT_TX_DFE_ADJUST_GAIN_CONTROL_DIRECTLY_DELAY);
    if (!sdm_dac_get_adjust_gain_it_done()) {
        tx_dfe.adjust_gain_in_use = false;
        sdm_dac_adjust_gain_it_clear(true);
        DBGLOG_DRIVER_INFO("[GAIN] adjust gain fail!!!\n");
        return RET_BUSY;
    } else {
        tx_dfe.adjust_gain_in_use = false;
        sdm_dac_adjust_gain_it_clear(true);
    }
    return RET_OK;
}

uint8_t iot_tx_dfe_adjust_analog_gain(IOT_TX_DFE_CHN_ID chn, IOT_TX_DFE_GAIN_CONTROL_MODE mode,
                                      bool_t up)
{
    int adjust_gain_timeout_cnt = 0;
    if (tx_dfe.adjust_gain_in_use) {
        return RET_BUSY;
    }
    tx_dfe.adjust_gain_in_use = true;
    sdm_dac_ana_gain_start((DAC_CHN_ID)chn, up, (DAC_GAIN_CONTROL_MODE)mode);
    while (!sdm_dac_get_adjust_gain_it_done()) {
        os_delay(1);
        adjust_gain_timeout_cnt++;
        if (adjust_gain_timeout_cnt >= IOT_TX_DFE_ADJUST_ANA_GAIN_TIMEOUT_CNT) {
            tx_dfe.adjust_gain_in_use = false;
            sdm_dac_gain_mode((DAC_CHN_ID)chn, DAC_GAIN_CONTROL_DIRECTLY);
            iot_timer_delay_us(IOT_TX_DFE_ADJUST_GAIN_CONTROL_DIRECTLY_DELAY);
            sdm_dac_adjust_gain_it_clear(true);
            return RET_FAIL;
        }
    }
    sdm_dac_adjust_gain_it_clear(true);
    return RET_OK;
}

int16_t iot_tx_dfe_get_dig_gain(IOT_TX_DFE_CHN_ID chn)
{
    if (chn == IOT_TX_DFE_CHN_DOUBLE) {
        assert(sdm_dac_get_dig_gain(DAC_CHN0) == sdm_dac_get_dig_gain(DAC_CHN1));
        return sdm_dac_get_dig_gain(DAC_CHN0);
    } else {
        return sdm_dac_get_dig_gain((DAC_CHN_ID)chn);
    }
}

void iot_tx_dfe_set_gain_step(IOT_TX_DFE_CHN_ID chn, int16_t dig_step, uint8_t ana_step)
{
    sdm_dac_set_gain_step((DAC_CHN_ID)chn, dig_step, ana_step);
}

void iot_tx_dfe_ana_gain_step_set(IOT_TX_DFE_CHN_ID chn, uint8_t ana_gain_step)
{
    sdm_dac_set_ana_gain_step((DAC_CHN_ID)chn, ana_gain_step);
}

void iot_tx_dfe_freq_config(IOT_TX_DFE_FREQ_ID freq)
{
    uint32_t temp;
    sdm_dac_sinc_param_t sinc_param;

    if (freq == IOT_TX_DFE_FREQ_800K) {
        sinc_param.sinc_stg = IOT_SDM_DAC_TX_DFE_SINC_STG_800K;
    } else {
        sinc_param.sinc_stg = IOT_SDM_DAC_TX_DFE_SINC_STG;
    }
    sinc_param.sinc_num = tx_dfe_param_table[freq].sinc_num;
    temp = pow((sinc_param.sinc_num + 1), (sinc_param.sinc_stg));
    sinc_param.lsh = (uint8_t)(25 - iot_dac_log2(temp, 0));
    sdm_dac_config_sinc(&sinc_param);
}

void iot_tx_dfe_config(IOT_TX_DFE_CHN_ID chn, IOT_TX_DFE_FREQ_ID freq)
{
    sdm_dac_config_channel_sinc((DAC_CHN_ID)chn, false);
    iot_tx_dfe_freq_config(freq);
    sdm_dac_config_hbf_en((DAC_CHN_ID)chn, tx_dfe_param_table[freq].hbf_en);
    sdm_dac_config_channel_hpf((DAC_CHN_ID)chn, true, IOT_TX_DFE_HPF_COEFF);
    sdm_dem_barrel_shift_operate((SDM_DEM_CHN_ID)chn, true, IOT_TX_DFE_DEM_BARREL_SHIFT_OPERATE);
    sdm_dac_threshold_config(IOT_TX_DFE_NG_DUR_MS, IOT_TX_DFE_NG_THRESHOLD0,
                             IOT_TX_DFE_NG_THRESHOLD1);
    /*The default tuning gain module compensates the 0 db,power balance module maximum
    96 (18 db / 0.1875 steps), so the initial threshold is set to 96 stpes, but the digital
    gain because the upper level requires greater than 0 or even more than 64 stpes,power balance.
    To prevent this, change the threshold for digital gain to hardware maximum supported 511 */
    sdm_dac_dig_gain_threshold(IOT_SDM_DAC_GAIN_MAX);
}

void iot_tx_dfe_hpf_enable(IOT_TX_DFE_CHN_ID chn, bool_t enable)
{
    sdm_dac_config_channel_hpf((DAC_CHN_ID)chn, !enable, IOT_TX_DFE_HPF_COEFF);
}

void iot_tx_dfe_link_i2s(IOT_TX_DFE_TO_I2S_SRC_SELECT src, IOT_TX_DFE_TO_I2S_BIT_SELECT sel)
{
    sdm_dac_to_i2s_select((DAC_TO_I2S_SRC_SELECT)src, (DAC_TO_I2S_BIT_SELECT)sel);
}

void iot_tx_dfe_reset(IOT_TX_DFE_CHN_ID chn)
{
    if (chn == IOT_TX_DFE_CHN_DOUBLE) {
        audio_reset_module(AUDIO_MODULE_DAC_0);
        audio_reset_module(AUDIO_MODULE_DAC_1);
    } else {
        audio_reset_module(AUDIO_MODULE_DAC_0 + (AUDIO_MODULE_ID)chn);
    }
}

void iot_tx_dfe_dem_data_force(IOT_TX_DFE_CHN_ID chn, bool_t enable)
{
    sdm_dem_overwrite_enable((SDM_DEM_CHN_ID)chn, enable);
}

void iot_tx_dfe_start(IOT_TX_DFE_CHN_ID chn)
{
    sdm_dac_enable_channel((DAC_CHN_ID)chn, true);
}

void iot_tx_dfe_stop(IOT_TX_DFE_CHN_ID chn)
{
    sdm_dac_enable_channel((DAC_CHN_ID)chn, false);
}

void iot_tx_dfe_dem_start(IOT_TX_DFE_CHN_ID chn)
{
    sdm_dem_enable((SDM_DEM_CHN_ID)chn, true);
}

void iot_tx_dfe_dem_stop(IOT_TX_DFE_CHN_ID chn)
{
    sdm_dem_enable((SDM_DEM_CHN_ID)chn, true);
}

void iot_tx_dfe_soft_reset(void)
{
    sdm_dac_reset(true);
    sdm_dac_reset(false);
}

void iot_tx_dfe_enable(IOT_TX_DFE_CHN_ID chn, bool_t sync)
{
    if (chn == IOT_TX_DFE_CHN_DOUBLE) {
        if (sync) {
            audio_cfg_multipath(AUDIO_MODULE_DAC_0);
            audio_cfg_multipath(AUDIO_MODULE_DAC_1);
        } else {
            audio_enable_module(AUDIO_MODULE_DAC_0);
            audio_enable_module(AUDIO_MODULE_DAC_1);
        }
    } else {
        if (sync) {
            audio_cfg_multipath(AUDIO_MODULE_DAC_0 + (AUDIO_MODULE_ID)chn);
        } else {
            audio_enable_module(AUDIO_MODULE_DAC_0 + (AUDIO_MODULE_ID)chn);
        }
    }
}

void iot_tx_dfe_disable(IOT_TX_DFE_CHN_ID chn, bool_t sync)
{
    if (chn == IOT_TX_DFE_CHN_DOUBLE) {
        if (sync) {
            audio_release_multipath(AUDIO_MODULE_DAC_0);
            audio_release_multipath(AUDIO_MODULE_DAC_1);
        } else {
            audio_disable_module(AUDIO_MODULE_DAC_0);
            audio_disable_module(AUDIO_MODULE_DAC_1);
        }
    } else {
        if (sync) {
            audio_release_multipath(AUDIO_MODULE_DAC_0 + (AUDIO_MODULE_ID)chn);
        } else {
            audio_disable_module(AUDIO_MODULE_DAC_0 + (AUDIO_MODULE_ID)chn);
        }
    }
}

uint8_t iot_tx_dfe_link_rx_dfe(IOT_TX_DFE_CHN_ID chn, IOT_TX_DEF_SRC_ID id)
{
    if (id > IOT_TX_DEF_SRC_DFE_MAX) {
        return RET_INVAL;
    }
    audio_set_tx_dfe_src((AUDIO_TX_DFE_ID)chn, (AUDIO_TX_DEF_SRC_ID)id);
    return RET_OK;
}

void iot_tx_dfe_link_anc(IOT_TX_DFE_CHN_ID chn)
{
    audio_set_tx_dfe_src((AUDIO_TX_DFE_ID)chn, AUDIO_TX_DEF_SRC_ANC);
}

void iot_tx_dfe_link_asrc(IOT_TX_DFE_CHN_ID chn)
{
    audio_set_tx_dfe_src((AUDIO_TX_DFE_ID)chn, AUDIO_TX_DEF_SRC_ASRC);
}

uint8_t iot_tx_dfe_dump_module(IOT_TX_DFE_DUMP_DATA_MODULE sel)
{
    if (sel >= IOT_TX_DFE_DUMP_DATA_MAX) {
        return RET_INVAL;
    }
    sdm_dac_dump_sel(sel);
    return RET_OK;
}

void iot_tx_dfe_dump_channel(IOT_TX_DFE_CHN_ID chn)
{
    if (chn == IOT_TX_DFE_CHN_DOUBLE) {
        audio_dump_dac_select(AUDIO_DUMP_DAC_0);
        audio_dump_dac_select(AUDIO_DUMP_DAC_1);
    } else {
        audio_dump_dac_select(AUDIO_DUMP_DAC_0 + (AUDIO_DUMP_SRC)chn);
    }
}

void iot_tx_dfe_threshold_config(uint8_t ms, uint32_t threshold0, uint32_t threshold1)
{
    sdm_dac_threshold_config(ms, threshold0, threshold1);
}

void iot_tx_dfe_clip_rate(uint8_t rate, uint32_t threshold)
{
    sdm_dac_clip_rate(rate, threshold);
}

uint8_t iot_tx_dfe_sw_digital_gain_down(IOT_TX_DFE_CHN_ID chn, int16_t steps)
{
    int16_t gain_difference;
    int16_t current_dig_gain;

    if (chn == IOT_TX_DFE_CHN_DOUBLE) {
        assert(sdm_dac_get_dig_gain(DAC_CHN0) == sdm_dac_get_dig_gain(DAC_CHN1));
        current_dig_gain = sdm_dac_get_dig_gain(DAC_CHN0);
    } else {
        current_dig_gain = sdm_dac_get_dig_gain((DAC_CHN_ID)chn);
    }
    gain_difference = steps - current_dig_gain;

    if (gain_difference > IOT_SDM_DAC_GAIN_MAX) {
        return RET_INVAL;
    }
    return iot_tx_dfe_adjust_digital_gain((IOT_TX_DFE_CHN_ID)chn, IOT_TX_DFE_GAIN_CONTROL_DOUBLE,
                                          false, steps, false);
}

uint8_t iot_tx_dfe_sw_digital_gain_up(IOT_TX_DFE_CHN_ID chn, int16_t steps)
{
    int16_t gain_sum;
    int16_t current_dig_gain;

    if (chn == IOT_TX_DFE_CHN_DOUBLE) {
        assert(sdm_dac_get_dig_gain(DAC_CHN0) == sdm_dac_get_dig_gain(DAC_CHN1));
        current_dig_gain = sdm_dac_get_dig_gain(DAC_CHN0);
    } else {
        current_dig_gain = sdm_dac_get_dig_gain((DAC_CHN_ID)chn);
    }
    gain_sum = steps + current_dig_gain;

    if (gain_sum > IOT_SDM_DAC_GAIN_MAX) {
        return RET_INVAL;
    }
    return iot_tx_dfe_adjust_digital_gain((IOT_TX_DFE_CHN_ID)chn, IOT_TX_DFE_GAIN_CONTROL_DOUBLE,
                                          true, steps, false);
}

uint8_t iot_tx_dfe_sw_digital_target_gain_set(IOT_TX_DFE_CHN_ID chn, uint8_t mode,
                                              int16_t target_gain, bool_t need_done)
{
    bool_t up = false;
    int16_t gain_steps = 0;
    int16_t current_dig_gain;
    if (chn == IOT_TX_DFE_CHN_DOUBLE) {
        assert(sdm_dac_get_dig_gain(DAC_CHN0) == sdm_dac_get_dig_gain(DAC_CHN1));
        current_dig_gain = sdm_dac_get_dig_gain(DAC_CHN0);
    } else {
        current_dig_gain = sdm_dac_get_dig_gain((DAC_CHN_ID)chn);
    }

    if (target_gain < IOT_SDM_DAC_GAIN_MIN) {
        target_gain = IOT_SDM_DAC_GAIN_MIN;
    } else if (target_gain > IOT_SDM_DAC_GAIN_MAX) {
        target_gain = IOT_SDM_DAC_GAIN_MAX;
    }

    if (target_gain <= current_dig_gain) {
        up = false;
        gain_steps = current_dig_gain - target_gain;
    } else if (target_gain > current_dig_gain) {
        up = true;
        gain_steps = target_gain - current_dig_gain;
    }

    DBGLOG_DRIVER_INFO(
    "[GAIN] tx dfe gain set steps,chn:%d, target gain:%d, current register gain: %d, up:%d, mode:%d, gain_steps:%d\n",
    chn, target_gain, current_dig_gain, up, mode, gain_steps);

    if (gain_steps != 0) {
        return iot_tx_dfe_adjust_digital_gain(
            (IOT_TX_DFE_CHN_ID)chn, (IOT_TX_DFE_GAIN_CONTROL_MODE)mode, up, gain_steps, need_done);
    } else {
        return RET_OK;
    }
}

uint8_t iot_tx_dfe_digital_gain_set(IOT_TX_DFE_CHN_ID chn, int16_t target_gain,
                                    IOT_TX_DFE_GAIN_CONTROL_MODE mode, bool_t need_done)
{
    int16_t cur_target_gain;
    int16_t last_target_gain;

    if (!tx_dfe.power_balance_on) {
        cur_target_gain = target_gain;
        if (iot_tx_dfe_sw_digital_target_gain_set(chn, mode, target_gain, need_done) != RET_OK) {
            iot_tx_dfe_switch_mode_wait_gain_done(chn);
        }
        if (chn == IOT_TX_DFE_CHN_DOUBLE) {
            assert(sdm_dac_get_dig_gain(DAC_CHN0) == sdm_dac_get_dig_gain(DAC_CHN1));
            last_target_gain = sdm_dac_get_dig_gain(DAC_CHN0);
        } else {
            last_target_gain = sdm_dac_get_dig_gain((DAC_CHN_ID)chn);
        }
        if(last_target_gain != cur_target_gain) {
            iot_tx_dfe_sw_digital_target_gain_set(chn, mode, cur_target_gain, need_done);
        }
    } else {
        tx_dfe.power_balance_off_gain = target_gain;
        tx_dfe.power_balance_off_gain_valid = true;
    }

    return RET_OK;
}

void iot_tx_dfe_hw_digital_mute(IOT_TX_DFE_CHN_ID chn, int16_t digital_gain_steps)
{
    sdm_dac_hw_digital_mute((DAC_CHN_ID)chn, digital_gain_steps);
    while (!sdm_dac_get_mute_complete((DAC_CHN_ID)chn)) {
    }
    sdm_dac_mute_balance_clear((DAC_CHN_ID)chn, true);
    sdm_dac_mute_balance_clear((DAC_CHN_ID)chn, false);
}

void iot_tx_dfe_hw_digital_unmute(IOT_TX_DFE_CHN_ID chn, int16_t digital_gain_steps)
{
    sdm_dac_hw_digital_unmute((DAC_CHN_ID)chn, digital_gain_steps);
    while (!sdm_dac_get_mute_complete((DAC_CHN_ID)chn)) {
    }
    sdm_dac_mute_balance_clear((DAC_CHN_ID)chn, true);
    sdm_dac_mute_balance_clear((DAC_CHN_ID)chn, false);
}

/* power balance */
uint8_t iot_tx_dfe_power_balance(const iot_tx_dfe_power_balance_cfg_t *cfg)
{
    uint8_t ret = RET_OK;
    if (tx_dfe.adjust_gain_in_use) {
        return RET_BUSY;
    }

    if ((tx_dfe.power_balance_on && cfg->balance_on)
        || (!tx_dfe.power_balance_on && !cfg->balance_on)) {
        return RET_INVAL;
    }
    int timeout_cnt = 0;
    tx_dfe.adjust_gain_in_use = true;

    //set the flag when it to set PB, then it can not adj vol
    if (cfg->balance_on) {
        tx_dfe.power_balance_on = cfg->balance_on;
    }

    sdm_dac_adjust_gain_mode((DAC_CHN_ID)cfg->chn, DAC_GAIN_CONTROL_SOFT_RAMP);
    iot_tx_dfe_ana_gain_step_set(cfg->chn, IOT_TX_DFE_POWER_BALANCE_STEPS);
    sdm_dac_power_balance((DAC_CHN_ID)cfg->chn, cfg->balance_on);
    if (cfg->mode == IOT_SDM_DAC_POWER_BALANCE_INTERRUT) {
        if (cfg->cb != NULL) {
            tx_dfe.power_balance_cb[cfg->chn] = cfg->cb;
        }
        if((!cfg->balance_on) && (cfg->close_dfe)) {
        } else {
            sdm_dac_adjust_gain_it_enable(true);
        }
    } else {
        do {
            if (timeout_cnt > IOT_TX_DFE_POWER_BALANCE_TIMEOUT_CNT) {
                DBGLOG_DRIVER_INFO("[POWER BALANCE] power balance timeout, on:%d\n",
                                   cfg->balance_on);
                tx_dfe.adjust_gain_in_use = true;

                if((!cfg->balance_on) && (cfg->close_dfe)) {
                    ret = RET_TIMEOVER;
                    break;
                } else {
                    assert(0);
                }
            }
            os_delay(1);
            timeout_cnt++;
        } while (!sdm_dac_get_power_balance_complete((DAC_CHN_ID)cfg->chn));
        sdm_dac_mute_balance_clear((DAC_CHN_ID)cfg->chn, true);
        sdm_dac_mute_balance_clear((DAC_CHN_ID)cfg->chn, false);
        sdm_dac_adjust_gain_it_clear(true);
        tx_dfe.adjust_gain_in_use = false;
    }

    //clear the flag when it to clear PB, then it can adj vol
    if (!cfg->balance_on) {
        tx_dfe.power_balance_on = cfg->balance_on;
    }

    if ((!cfg->balance_on) && tx_dfe.power_balance_off_gain_valid) {
        DBGLOG_DRIVER_INFO("[POWER BALANCE] power balance off gain value, on:%d, target_gain:%d\n",
                        cfg->balance_on, tx_dfe.power_balance_off_gain);
        if(tx_dfe.power_balance_off_gain != sdm_dac_get_dig_gain((DAC_CHN_ID)cfg->chn)) {
            if(!cfg->close_dfe) {
                iot_tx_dfe_digital_gain_set(cfg->chn, tx_dfe.power_balance_off_gain,
                                IOT_TX_DFE_GAIN_CONTROL_DIRECTLY, false);
            }
        }
        tx_dfe.power_balance_off_gain = IOT_SDM_DAC_GAIN_MIN;
        tx_dfe.power_balance_off_gain_valid = false;
    }

    return ret;
}

bool_t iot_tx_dfe_power_balance_get(IOT_TX_DFE_CHN_ID chn)
{
    return sdm_dac_get_power_balance_complete((DAC_CHN_ID)chn);
}

void iot_tx_dfe_power_balance_clr(IOT_TX_DFE_CHN_ID chn)
{
    sdm_dac_mute_balance_clear((DAC_CHN_ID)chn, false);
}

void iot_tx_dfe_mixer(IOT_SDM_DAC_STEREO_MIX_MODE mixer)
{
    audio_set_tx_dfe_mix_mode((AUDIO_MIX_MODE)mixer);
}

void iot_sdm_dac_dcc_compensate(IOT_TX_DFE_CHN_ID chn, uint16_t comp_value)
{
    sdm_dac_dcc_compensate((DAC_CHN_ID)chn, comp_value);
}

void iot_dfe_dac_ana_gain_sync(IOT_TX_DFE_CHN_ID chn)
{
    iot_tx_dfe_enable(chn, false);
    sdm_dac_anc_sync(false);
    sdm_dem_barrel_shift_operate((SDM_DEM_CHN_ID)chn, true, 0);
    sdm_dac_enable_channel((DAC_CHN_ID)chn, true);
    sdm_dem_enable((SDM_DEM_CHN_ID)chn, true);
    sdm_dac_enable_channel((DAC_CHN_ID)chn, false);
    sdm_dac_anc_sync(true);
    iot_tx_dfe_disable(chn, false);
    sdm_dem_enable((SDM_DEM_CHN_ID)chn, false);
}

void iot_sdm_dac_gain_overwrite_gain(IOT_TX_DFE_CHN_ID chn, bool_t overwrite,
                                     uint16_t dig_overwrite, uint8_t ana_overwrite)
{
    sdm_dac_gain_overwrite_power_scale((DAC_CHN_ID)chn, overwrite, dig_overwrite, ana_overwrite);
}

void iot_tx_dfe_power_balance_dcc_sync_cnt(IOT_TX_DFE_DCC_SYNC_CNT cnt)
{
    iot_tx_dfe_gain_init(cnt);
}

void iot_tx_dfe_dem_soft_reset(IOT_TX_DFE_CHN_ID chn)
{
    sdm_dem_soft_reset((SDM_DEM_CHN_ID)chn);
}

static uint32_t iot_tx_dfe_isr_handler(uint32_t vector, uint32_t data)
{
    UNUSED(vector);
    UNUSED(data);
    sdm_dac_adjust_gain_it_enable(false);
    if (sdm_dac_get_power_balance_complete(DAC_CHN_DOUBUL)) {
        if (tx_dfe.power_balance_cb[DAC_CHN_DOUBUL] != NULL) {
            tx_dfe.power_balance_cb[DAC_CHN_DOUBUL]();
        }
        sdm_dac_mute_balance_clear(DAC_CHN_DOUBUL, true);
        sdm_dac_mute_balance_clear(DAC_CHN_DOUBUL, false);
    } else if (sdm_dac_get_power_balance_complete(DAC_CHN0)) {
        if (tx_dfe.power_balance_cb[DAC_CHN0] != NULL) {
            tx_dfe.power_balance_cb[DAC_CHN0]();
        }
        sdm_dac_mute_balance_clear(DAC_CHN0, true);
        sdm_dac_mute_balance_clear(DAC_CHN0, false);
    } else if (sdm_dac_get_power_balance_complete(DAC_CHN1)) {
        if (tx_dfe.power_balance_cb[DAC_CHN1] != NULL) {
            tx_dfe.power_balance_cb[DAC_CHN1]();
        }
        sdm_dac_mute_balance_clear(DAC_CHN1, true);
        sdm_dac_mute_balance_clear(DAC_CHN1, false);
    }
    sdm_dac_adjust_gain_it_clear(true);
    tx_dfe.adjust_gain_in_use = false;
    return RET_OK;
}

/* mode: 0: ceil log2; 1: floor */
static uint32_t iot_dac_log2(uint32_t value, uint8_t mode)
{
    uint32_t ret;
    uint32_t temp = value;

    if (!mode) {
        temp = (temp * 2) - 1;
        ret = log2(temp);
    } else {
        ret = log2(temp);
    }
    return ret;
}
