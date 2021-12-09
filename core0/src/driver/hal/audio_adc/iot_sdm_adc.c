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

/* hw includes */
#include "aud_glb.h"
#include "aud_if.h"
#include "sdm_adc.h"

/* hal includes */
#include "iot_sdm_adc.h"

#define IOT_SDM_ADC_RX_DFE_SINC0_STG 1
#define IOT_SDM_ADC_RX_DFE_SINC1_STG 1

#define IOT_SDM_ADC_RX_DFE_PDM_SINC0_STG 3
#define IOT_SDM_ADC_RX_DFE_PDM_SINC1_STG 3

#define IOT_RX_DFE_POWER_SCALE 0
#define IOT_RX_DFE_HPF_COEFF   50

#define IOT_RX_DFE_SINC0_LSH_TOTAL     24
#define IOT_RX_DFE_PDM_SINC1_LSH_TOTAL 50
#define IOT_RX_DFE_ADC_SINC1_LSH_TOTAL 55

typedef struct iot_rx_dfe_freq_config {
    uint8_t factor;
    uint8_t sinc0_num;
    uint8_t sinc1_num;
    RX_DFE_HBF_BYPASS_MODE hbf;
} iot_rx_dfe_freq_config_t;

typedef struct iot_rx_dfe_state {
    bool_t idle[IOT_RX_DFE_CHN_MAX];
} iot_rx_dfe_state_t;

/*
 * table be used by config the sdm adc frequence
 * 1:factor, 2:sinc0_num, 3:sinc0_lsh, 4:sinc1_num, 5:hbf mode
 */
static const iot_rx_dfe_freq_config_t rx_dfe_param_table[IOT_RX_DFE_FS_MAX] = {
    {2, 5, 25, RX_DFE_HBF_DISABLE},  {4, 5, 25, RX_DFE_HBF_DISABLE}, {8, 5, 25, RX_DFE_HBF_DISABLE},
    {12, 5, 25, RX_DFE_HBF_DISABLE}, {8, 8, 8, RX_DFE_HBF_DISABLE},  {12, 5, 25, RX_DFE_HBF_0},
    {12, 5, 25, RX_DFE_HBF_0_1},
};
static iot_rx_dfe_state_t rx_dfe;

/* mode: 0: ceil log2; 1: floor */
static uint32_t iot_rx_dfe_log2(uint32_t value, uint8_t mode)
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

void iot_sdm_adc_mclk_enable(void)
{
    audio_enable_module(AUDIO_MODULE_MCLK_ADC);
}

void iot_sdm_adc_mclk_disable(void)
{
    audio_disable_module(AUDIO_MODULE_MCLK_ADC);
}

uint8_t iot_rx_dfe_enable(IOT_RX_DFE_CHN_ID chn)
{
    audio_cfg_multipath(AUDIO_MODULE_ADC_0 + (AUDIO_MODULE_ID)chn);
    return RET_OK;
}

uint8_t iot_rx_dfe_disable(IOT_RX_DFE_CHN_ID chn)
{
    audio_release_multipath(AUDIO_MODULE_ADC_0 + (AUDIO_MODULE_ID)chn);
    return RET_OK;
}

uint8_t iot_rx_dfe_start(IOT_RX_DFE_CHN_ID chn)
{
    sdm_adc_rx_dfe_enable((RX_DFE_CHN_ID)chn, true);
    return RET_OK;
}

uint8_t iot_rx_dfe_stop(IOT_RX_DFE_CHN_ID chn)
{
    sdm_adc_rx_dfe_enable((RX_DFE_CHN_ID)chn, false);
    return RET_OK;
}

uint8_t iot_rx_dfe_reset(IOT_RX_DFE_CHN_ID chn)
{
    audio_reset_module(AUDIO_MODULE_ADC_0 + (AUDIO_MODULE_ID)chn);
    return RET_OK;
}

void iot_rx_dfe_init(void)
{
    for (uint8_t i = 0; i < IOT_RX_DFE_CHN_MAX; i++) {
        rx_dfe.idle[i] = true;
    }
}

void iot_rx_dfe_deinit(void)
{
    return;
}

uint8_t iot_rx_dfe_claim_channel(IOT_RX_DFE_CHN_ID chn)
{
    if (!rx_dfe.idle[chn]) {
        return RET_BUSY;
    }
    rx_dfe.idle[chn] = false;

    return RET_OK;
}

uint8_t iot_rx_dfe_release_channel(IOT_RX_DFE_CHN_ID chn)
{
    if (rx_dfe.idle[chn]) {
        return RET_AGAIN;
    }
    rx_dfe.idle[chn] = true;

    return RET_OK;
}

uint8_t iot_rx_dfe_link_adc(IOT_RX_DFE_CHN_ID chn, uint8_t adc_id)
{
    if (adc_id >= AUDIO_RX_DEF_SRC_MAX) {
        return RET_INVAL;
    }

    audio_set_rx_dfe_src((AUDIO_RX_DFE_ID)chn, (AUDIO_RX_DEF_SRC_ID)adc_id);
    return RET_OK;
}

uint8_t iot_rx_dfe_link_pdm(IOT_RX_DFE_CHN_ID chn)
{
    audio_set_rx_dfe_src((AUDIO_RX_DFE_ID)chn, AUDIO_RX_DEF_PDM);
    return RET_OK;
}

uint8_t iot_rx_dfe_set_freq(IOT_RX_DFE_CHN_ID chn, const iot_rx_dfe_config_t *cfg)
{
    uint32_t temp;
    uint32_t sinc1_base;
    uint32_t sinc0_log2;
    uint32_t sinc1_log2;
    sdm_adc_sinc_param_t sinc_param;

    sinc_param.factor = rx_dfe_param_table[cfg->fs].factor;
    sinc_param.sinc0_num = rx_dfe_param_table[cfg->fs].sinc0_num - 1;
    sinc_param.sinc1_num = rx_dfe_param_table[cfg->fs].sinc1_num - 1;

    if (cfg->mode == IOT_RX_DFE_ADC) {
        sinc_param.sinc0_stg = IOT_SDM_ADC_RX_DFE_SINC0_STG;
    } else if (cfg->mode == IOT_RX_DFE_PDM) {
        sinc_param.sinc0_stg = IOT_SDM_ADC_RX_DFE_PDM_SINC0_STG;
    } else {
        return RET_INVAL;
    }

    temp = pow((sinc_param.sinc0_num + 1), (sinc_param.sinc0_stg + 2));
    sinc0_log2 = iot_rx_dfe_log2(temp, 0);
    sinc_param.sinc0_lsh = (uint8_t)(IOT_RX_DFE_SINC0_LSH_TOTAL - sinc0_log2);

    /* rx chn, sinc0 num, factor */
    sdm_adc_rx_dfe_config_sinc0((RX_DFE_CHN_ID)chn, &sinc_param);

    sinc1_base = (sinc_param.sinc0_num + 1) * (sinc_param.sinc1_num + 1);
    if (cfg->mode == IOT_RX_DFE_ADC) {
        sinc_param.sinc1_stg = IOT_SDM_ADC_RX_DFE_SINC1_STG;
        temp = pow((uint8_t)sinc1_base, (sinc_param.sinc1_stg + 2));
        sinc1_log2 = iot_rx_dfe_log2(temp, 0);
        sinc_param.sinc1_lsh =
            (uint8_t)(IOT_RX_DFE_PDM_SINC1_LSH_TOTAL - (sinc_param.sinc0_lsh + sinc1_log2));

    } else if (cfg->mode == IOT_RX_DFE_PDM) {
        sinc_param.sinc1_stg = IOT_SDM_ADC_RX_DFE_PDM_SINC1_STG;
        temp = pow((uint8_t)sinc1_base, (sinc_param.sinc1_stg + 2));
        sinc1_log2 = iot_rx_dfe_log2(temp, 0);
        sinc_param.sinc1_lsh =
            (uint8_t)(IOT_RX_DFE_ADC_SINC1_LSH_TOTAL - (sinc_param.sinc0_lsh + sinc1_log2));
    } else {
        return RET_INVAL;
    }

    /* rx chn, sinc0 num, sinc1 num, sinc0 lsh */
    sdm_adc_rx_dfe_config_sinc1((RX_DFE_CHN_ID)chn, &sinc_param);

    return RET_OK;
}

void iot_rx_dfe_config(IOT_RX_DFE_CHN_ID chn, const iot_rx_dfe_config_t *rx_dfe_cfg)
{
    iot_rx_dfe_set_freq(chn, rx_dfe_cfg);
    RX_DFE_CHN_ID ch = (RX_DFE_CHN_ID)chn;
    sdm_adc_rx_dfe_anc_select(ch, SDM_ADC_ANC_FROM_SOLE_SINC_FILTER);
    /* rx chn, disable anc only */
    sdm_adc_rx_dfe_anc_only(ch, false);
    sdm_adc_rx_dfe_config_pwr_scale(ch, IOT_RX_DFE_POWER_SCALE);
    /* rx chn,  hbf bypass mode*/
    sdm_adc_rx_dfe_config_hbf(ch, rx_dfe_param_table[rx_dfe_cfg->fs].hbf);
    sdm_adc_rx_dfe_config_hpf(ch, true, IOT_RX_DFE_HPF_COEFF);
}

void iot_rx_dfe_gain_set(IOT_RX_DFE_CHN_ID chn, int16_t gain)
{
    sdm_adc_rx_dfe_config_pwr_scale((RX_DFE_CHN_ID)chn, gain);
}
void iot_rx_dfe_hpf_enable(IOT_RX_DFE_CHN_ID chn, bool_t enable)
{
    bool_t bypass = !enable;
    sdm_adc_rx_dfe_config_hpf((RX_DFE_CHN_ID)chn, bypass, IOT_RX_DFE_HPF_COEFF);
}

void iot_rx_dfe_set_adc_format(IOT_RX_DFE_ADC_ID id, uint8_t format)
{
    audio_set_sdm_adc_format((AUDIO_SDM_ADC_ID)id, format);
}

uint8_t iot_rx_dfe_dump_module(IOT_RX_DFE_CHN_ID chn, IOT_RX_DFE_DUMP_DATA_MODULE sel)
{
    sdm_adc_rx_dfe_dump_sel((RX_DFE_CHN_ID)chn, sel);
    return RET_OK;
}

void iot_rx_dfe_dump_channel(IOT_RX_DFE_CHN_ID chn)
{
    audio_dump_dac_select(AUDIO_DUMP_ADC_0 + (AUDIO_DUMP_SRC)chn);
}

uint32_t iot_rx_dfe_receive_poll(IOT_RX_DFE_CHN_ID chn)
{
    uint32_t data;
    uint32_t n = 0;

    RX_DFE_CHN_ID ch = (RX_DFE_CHN_ID)chn;

    while (!sdm_adc_rx_dfe_out_flag(ch)) {
        assert(n <= 16000);
        n++;
    }

    data = sdm_adc_rx_dfe_get_data(ch);
    sdm_adc_rx_dfe_data_clear(ch);
    return data;
}
