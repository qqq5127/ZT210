/****************************************************************************

Copyright(c) 2020 by WuQi Teportologies. ALL RIGHTS RESERVED.

This Information is proprietary to WuQi Teportologies and MAY NOT
be copied by any method or incorporated into another program without
the express written consent of WuQi. This Information or any portion
thereof remains the property of WuQi. The Information contained herein
is believed to be accurate and WuQi assumes no responsibility or
liability for its use in any way and conveys no license or title under
any patent or copyright and makes no representation or warranty that this
Information is free from patent or copyright infringement.

****************************************************************************/
#include "types.h"
#include "string.h"
#include "critical_sec.h"
#include "adi_slave.h"
#include "aud_glb.h"
#include "aud_if.h"

#include "iot_sdm_dac.h"
#include "iot_spk_dac.h"
#include "iot_audio_dac.h"
#include "aud_intf_pwr.h"

#include "driver_dbglog.h"

#define IOT_AUDIO_DAC_GAIN_MAX_THRESHOLD   511
#define IOT_AUDIO_DAC_GAIN_MIN_THRESHOLD   -511
#define IOT_AUDIO_DAC_MUTE_DIG_GAIN        -511
#define IOT_AUDIO_DAC_INITIAL_DCC_DELAY    104
#define IOT_AUDIO_DAC_CLIP_RATE            4
#define IOT_AUDIO_DAC_CLIPPING_DB_MAX      13

/* 0 ~ -12dB full scale clipping limit: 0.90, 0.89, 0.79, 0.71, 0.63, 0.56, 0.50,
0.45, 0.40, 0.35, 0.32, 0.28, 0.25, * 2^23 */
static const uint32_t iot_audio_dac_clipping_threshold[IOT_AUDIO_DAC_CLIPPING_DB_MAX] = {
    7549747, 7465861, 6627000, 1426063, 5284823, 4697620, 4194304,
    3774875, 3355443, 2936013, 2684355, 2348810, 2097152,
};

typedef struct iot_audio_dac_port_info {
    int16_t current_gain;
    bool_t can_adjust_gain;
    bool_t used;
} iot_audio_dac_port_info_t;

typedef struct iot_audio_dac_state {
    uint8_t dac_counter;
    iot_audio_dac_port_info_t port[IOT_AUDIO_DAC_PORT_MAX];
} iot_audio_dac_state_t;

static iot_audio_dac_state_t iot_audio_dac_state;

static uint8_t iot_audio_dac_port_start(IOT_AUDIO_DAC_PORT_ID port);
static uint8_t iot_audio_dac_port_stop(IOT_AUDIO_DAC_PORT_ID port);
static uint8_t iot_audio_dac_release(IOT_AUDIO_DAC_PORT_ID port);
static uint8_t iot_audio_dac_config(IOT_AUDIO_DAC_PORT_ID port, const iot_audio_dac_config_t *cfg);

void iot_audio_dac_init(void)
{
    iot_audio_dac_state.dac_counter = 0;
    for (uint8_t i = 0; i < IOT_AUDIO_DAC_PORT_MAX; i++) {
        iot_audio_dac_state.port[i].used = false;
        iot_audio_dac_state.port[i].can_adjust_gain = false;
    }
    iot_tx_dfe_init();
}

void iot_audio_dac_deinit(void)
{
    memset(&iot_audio_dac_state, 0x00, sizeof(iot_audio_dac_state_t));
}

void iot_audio_dac_open(IOT_AUDIO_DAC_CHANNEL chn, const iot_audio_dac_config_t *cfg)
{
    //audio intf power on vote
    aud_intf_pwr_on(AUDIO_MODULE_MCLK_DAC);

    if (chn == IOT_AUDIO_DAC_CHN_MONO_L) {
        iot_audio_dac_config(IOT_AUDIO_DAC_PORT_0, cfg);
        iot_audio_dac_stereo_mixer(IOT_AUDIO_DAC_MONO_0);
    } else if (chn == IOT_AUDIO_DAC_CHN_MONO_R) {
        iot_audio_dac_config(IOT_AUDIO_DAC_PORT_0, cfg);
        iot_audio_dac_config(IOT_AUDIO_DAC_PORT_1, cfg);
        iot_audio_dac_stereo_mixer(IOT_AUDIO_DAC_MONO_1);
    } else {
        iot_audio_dac_config(IOT_AUDIO_DAC_PORT_DOUBLE, cfg);
        iot_audio_dac_stereo_mixer(IOT_AUDIO_DAC_STEREO_DIRECT);
    }
}

void iot_audio_dac_close(IOT_AUDIO_DAC_CHANNEL chn)
{
    if (chn == IOT_AUDIO_DAC_CHN_MONO_L) {
        iot_audio_dac_release(IOT_AUDIO_DAC_PORT_0);
    } else if (chn == IOT_AUDIO_DAC_CHN_MONO_R) {
        iot_audio_dac_release(IOT_AUDIO_DAC_PORT_1);
        iot_audio_dac_release(IOT_AUDIO_DAC_PORT_0);
    } else {
        iot_audio_dac_release(IOT_AUDIO_DAC_PORT_DOUBLE);
    }

    if (!iot_audio_dac_state.dac_counter) {
        //audio intf power off vote
        aud_intf_pwr_off(AUDIO_MODULE_MCLK_DAC);
    }
}

void iot_audio_dac_start(IOT_AUDIO_DAC_CHANNEL chn)
{
    if (chn != IOT_AUDIO_DAC_CHN_STEREO) {
        iot_audio_dac_state.port[chn].can_adjust_gain = true;
        iot_audio_dac_mute(chn, IOT_AUDIO_DAC_GAIN_DIRECTLY); //TODO overwrite gain to -511
        iot_audio_dac_port_start((IOT_AUDIO_DAC_PORT_ID)chn);
    }  else {
        //to solve the hw bug
        iot_audio_dac_state.port[IOT_AUDIO_DAC_PORT_0].can_adjust_gain = true;
        iot_audio_dac_state.port[IOT_AUDIO_DAC_PORT_1].can_adjust_gain = true;
        iot_audio_dac_mute(chn, IOT_AUDIO_DAC_GAIN_DIRECTLY); //TODO overwrite gain to -511
        iot_audio_dac_port_start(IOT_AUDIO_DAC_PORT_DOUBLE);
    }
    iot_tx_dfe_irq_config();
}

void iot_audio_dac_stop(IOT_AUDIO_DAC_CHANNEL chn)
{
    if (chn != IOT_AUDIO_DAC_CHN_STEREO) {
        iot_spk_dac_disable((uint8_t)chn);
        iot_audio_dac_port_stop((IOT_AUDIO_DAC_PORT_ID)chn);
    } else {
        iot_spk_dac_disable(IOT_AUDIO_DAC_PORT_0);
        iot_spk_dac_disable(IOT_AUDIO_DAC_PORT_1);
        //all the ports must be mute to stop,against pop sounds
        iot_audio_dac_port_stop(IOT_AUDIO_DAC_PORT_DOUBLE);
    }
}

void iot_audio_dac_stereo_mixer(IOT_AUDIO_DAC_MIX_MODE mode)
{
    iot_tx_dfe_mixer((IOT_SDM_DAC_STEREO_MIX_MODE)mode);
}

void iot_audio_dac_multipath_sync(void)
{
    audio_multipath_enable();
}

bool_t iot_audio_dac_get_open_status(IOT_AUDIO_DAC_CHANNEL chn)
{
    if(chn >= IOT_AUDIO_DAC_CHN_STEREO) {
        chn = IOT_AUDIO_DAC_CHN_MONO_L;
    }
    uint32_t audio_clk_eb = audio_get_global_clk_status();
    return (audio_clk_eb & BIT((AUDIO_MODULE_ID)(AUDIO_MODULE_DAC_0 + chn)) ? true : false);
}

uint8_t iot_audio_dac_mute(IOT_AUDIO_DAC_CHANNEL chn, IOT_AUDIO_DAC_GAIN_MODE mode)
{
    if (iot_audio_dac_state.port[chn].can_adjust_gain) {
        DBGLOG_DRIVER_INFO("[GAIN] audio dac mute, chn:%d, mode:%d\n", chn, mode);
        if(IOT_AUDIO_DAC_MUTE_DIG_GAIN == iot_audio_dac_state.port[chn].current_gain) {
            return RET_OK;
        }
        if (iot_tx_dfe_digital_gain_set((IOT_TX_DFE_CHN_ID)chn, IOT_AUDIO_DAC_MUTE_DIG_GAIN,
                                            (IOT_TX_DFE_GAIN_CONTROL_MODE)mode, false)
            != RET_OK) {
            return RET_BUSY;
        }
    } else {
        return RET_INVAL;
    }

    return RET_OK;
}

uint8_t iot_audio_dac_unmute(IOT_AUDIO_DAC_CHANNEL chn, IOT_AUDIO_DAC_GAIN_MODE mode)
{
    if (iot_audio_dac_state.port[chn].can_adjust_gain) {
        DBGLOG_DRIVER_INFO("[GAIN] audio dac unmute, chn : %d, target gain : %d, mode:%d\n", chn,
                        iot_audio_dac_state.port[chn].current_gain, mode);
        if(iot_tx_dfe_digital_gain_set((IOT_TX_DFE_CHN_ID)chn,iot_audio_dac_state.port[chn].current_gain,
                                    (IOT_TX_DFE_GAIN_CONTROL_MODE)mode, false) != RET_OK) {
            return RET_BUSY;
        }
    } else {
        DBGLOG_DRIVER_INFO("[GAIN] audio dac unmute, can't unmute\n");
        return RET_INVAL;
    }

    return RET_OK;
}

void iot_audio_dac_gain_adjust(IOT_AUDIO_DAC_PORT_ID port, IOT_AUDIO_DAC_GAIN_MODE mode, bool_t up, int16_t digit_gain_steps,
                               bool_t need_complete)
{
    iot_tx_dfe_adjust_digital_gain((IOT_TX_DFE_CHN_ID)port, (IOT_TX_DFE_GAIN_CONTROL_MODE)mode, up, digit_gain_steps, need_complete);
}

uint8_t iot_audio_dac_gain_target(IOT_AUDIO_DAC_CHANNEL chn, int16_t target_gain)
{
    if (target_gain < IOT_AUDIO_DAC_GAIN_MIN_THRESHOLD) {
        target_gain = IOT_AUDIO_DAC_GAIN_MIN_THRESHOLD;
    } else if (target_gain > IOT_AUDIO_DAC_GAIN_MAX_THRESHOLD) {
        target_gain = IOT_AUDIO_DAC_GAIN_MAX_THRESHOLD;
    }
    DBGLOG_DRIVER_INFO("[GAIN] audio dac adjust gain, chn : %d, target gain : %d, can adjust:%d\n",
                        chn, target_gain, iot_audio_dac_state.port[chn].can_adjust_gain);
    if (iot_audio_dac_state.port[chn].can_adjust_gain) {
        if (iot_tx_dfe_digital_gain_set((IOT_TX_DFE_CHN_ID)chn, target_gain,
                                            IOT_TX_DFE_GAIN_CONTROL_DOUBLE, false) != RET_OK) {
                return RET_BUSY;
            }
    } else {
        DBGLOG_DRIVER_INFO("[GAIN] audio dac save gain, chn : %d, target gain : %d\n", chn,
                        target_gain);
    }
    cpu_critical_enter();
    iot_audio_dac_state.port[chn].current_gain = target_gain;
    cpu_critical_exit();
    return RET_OK;
}

uint8_t iot_audio_dac_gain_down(IOT_AUDIO_DAC_PORT_ID port, int16_t steps)
{
    int16_t gain;

    gain = iot_audio_dac_state.port[port].current_gain - steps;
    if (gain <= IOT_AUDIO_DAC_GAIN_MIN_THRESHOLD) {
        return RET_FAIL;
    }
    if (iot_audio_dac_state.port[port].can_adjust_gain) {
        iot_tx_dfe_sw_digital_gain_down((IOT_TX_DFE_CHN_ID)port, steps);
        iot_audio_dac_state.port[port].current_gain -= steps;
    } else {
        iot_audio_dac_state.port[port].current_gain -= steps;
    }

    return RET_OK;
}

uint8_t iot_audio_dac_gain_up(IOT_AUDIO_DAC_PORT_ID port, int16_t steps)
{
    int16_t gain;

    gain = iot_audio_dac_state.port[port].current_gain + steps;
    if (gain >= IOT_AUDIO_DAC_GAIN_MAX_THRESHOLD) {
        return RET_FAIL;
    }

    if (iot_audio_dac_state.port[port].can_adjust_gain) {
        iot_tx_dfe_sw_digital_gain_up((IOT_TX_DFE_CHN_ID)port, steps);
        iot_audio_dac_state.port[port].current_gain += steps;
    } else {
        iot_audio_dac_state.port[port].current_gain += steps;
    }

    return RET_OK;
}

uint8_t iot_audio_dac_power_balance(IOT_AUDIO_DAC_CHANNEL chn, bool_t balance_on, bool_t close_dac)
{
    iot_tx_dfe_power_balance_cfg_t cfg;

    cfg.cb = NULL;
    cfg.balance_on = balance_on;
    cfg.mode = IOT_SDM_DAC_POWER_BALANCE_POLLING;
    cfg.close_dfe = close_dac;
    if(chn != IOT_AUDIO_DAC_CHN_STEREO) {
        cfg.chn = (IOT_TX_DFE_CHN_ID)chn;
        return iot_tx_dfe_power_balance(&cfg);
    } else {
        cfg.chn = IOT_TX_DFE_CHN_0;
        if(iot_tx_dfe_power_balance(&cfg) != RET_OK) {
            return RET_BUSY;
        }
        cfg.chn = IOT_TX_DFE_CHN_1;
        return iot_tx_dfe_power_balance(&cfg);
    }
}

static uint8_t iot_audio_dac_port_start(IOT_AUDIO_DAC_PORT_ID port)
{
    if (iot_audio_dac_state.port[port].used == false) {
        return RET_INVAL;
    }
    iot_tx_dfe_dem_data_force((IOT_TX_DFE_CHN_ID)port, false);
    if(port == IOT_AUDIO_DAC_PORT_DOUBLE) {
        iot_spk_dac_start(IOT_AUDIO_DAC_PORT_0);
        iot_spk_dac_start(IOT_AUDIO_DAC_PORT_1);
    } else {
        iot_spk_dac_start(port);
    }
    return RET_OK;
}

static uint8_t iot_audio_dac_port_stop(IOT_AUDIO_DAC_PORT_ID port)
{
    if (iot_audio_dac_state.port[port].used == false) {
        return RET_INVAL;
    }
    iot_tx_dfe_dem_data_force((IOT_TX_DFE_CHN_ID)port, true);
    if(port == IOT_AUDIO_DAC_PORT_1) {
        iot_audio_dac_state.dac_counter -= 2;
    } else {
        iot_audio_dac_state.dac_counter--;
    }

    if (iot_audio_dac_state.dac_counter == 0) {
        iot_spk_dac_deinit();
    }
    return RET_OK;
}

static uint8_t iot_audio_dac_config(IOT_AUDIO_DAC_PORT_ID port, const iot_audio_dac_config_t *cfg)
{
    if (iot_audio_dac_state.dac_counter == 0) {
        uint32_t threshold;
        int full_scale_limit = -cfg->full_scale_limit;
        assert(full_scale_limit >= 0 && full_scale_limit < IOT_AUDIO_DAC_CLIPPING_DB_MAX);

        iot_sdm_dac_mclk_enable();
        iot_tx_dfe_reset(IOT_TX_DFE_CHN_DOUBLE);
        iot_tx_dfe_gain_init(IOT_AUDIO_DAC_INITIAL_DCC_DELAY);
        iot_tx_dfe_dem_data_force(IOT_TX_DFE_CHN_DOUBLE, true);
        threshold = iot_audio_dac_clipping_threshold[full_scale_limit];
        iot_tx_dfe_clip_rate(IOT_AUDIO_DAC_CLIP_RATE, threshold);
        iot_sdm_dac_dcc_compensate(IOT_TX_DFE_CHN_DOUBLE, cfg->dc_offset_dig_calibration);
        iot_dfe_dac_ana_gain_sync(IOT_TX_DFE_CHN_DOUBLE);
        iot_tx_dfe_soft_reset();
    }
    if (iot_audio_dac_state.port[port].used) {
        return RET_INVAL;
    }

    /* step 1: sync analog digital interface */
    adi_slave_sync();

    /* step 2: enable tx def */
    iot_tx_dfe_enable((IOT_TX_DFE_CHN_ID)port, true);

    /* step 3: config tx def */
    iot_tx_dfe_config((IOT_TX_DFE_CHN_ID)port, (IOT_TX_DFE_FREQ_ID)cfg->fs);

    cpu_critical_enter();
    /* step 4: select dac data source*/
    if(port == IOT_AUDIO_DAC_PORT_DOUBLE) {
        /* step 5: config  speaker dac open the spk if need to open */
        iot_spk_dac_init(IOT_AUDIO_DAC_PORT_0);
        iot_spk_dac_init(IOT_AUDIO_DAC_PORT_1);
        iot_spk_dac_enable(IOT_AUDIO_DAC_PORT_0);
        iot_spk_dac_enable(IOT_AUDIO_DAC_PORT_1);
        audio_set_tx_dfe_src(AUDIO_TX_DFE_0, (AUDIO_TX_DEF_SRC_ID)cfg->src);
        audio_set_tx_dfe_src(AUDIO_TX_DFE_1, (AUDIO_TX_DEF_SRC_ID)cfg->src);
    } else {
        /* step 5: config  speaker dac open the spk if need to open */
        iot_spk_dac_init(port);
        iot_spk_dac_enable(port);
        audio_set_tx_dfe_src((AUDIO_TX_DFE_ID)port, (AUDIO_TX_DEF_SRC_ID)cfg->src);
    }
    /* step 5: start dfe */
    iot_tx_dfe_dem_start((IOT_TX_DFE_CHN_ID)port);
    iot_tx_dfe_start((IOT_TX_DFE_CHN_ID)port);
    iot_audio_dac_state.dac_counter++;
    iot_audio_dac_state.port[port].used = true;
    cpu_critical_exit();
    return RET_OK;
}

static uint8_t iot_audio_dac_release(IOT_AUDIO_DAC_PORT_ID port)
{
    if (iot_audio_dac_state.port[port].used == false) {
        return RET_INVAL;
    }

    cpu_critical_enter();
    iot_tx_dfe_stop((IOT_TX_DFE_CHN_ID)port);
    iot_tx_dfe_dem_stop((IOT_TX_DFE_CHN_ID)port);
    iot_tx_dfe_disable((IOT_TX_DFE_CHN_ID)port, false);
    iot_audio_dac_state.port[port].used = false;
    iot_audio_dac_state.port[port].can_adjust_gain = false;
    cpu_critical_exit();

    if (iot_audio_dac_state.dac_counter == 0) {
        iot_sdm_dac_mclk_disable();
    }

    return RET_OK;
}
