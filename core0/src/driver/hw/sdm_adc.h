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

#ifndef __DRIVER_HW_SDM_ADC_H__
#define __DRIVER_HW_SDM_ADC_H__

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    PDM_CHN_0,
    PDM_CHN_1,
    PDM_CHN_2,
    PDM_CHN_MAX,
    PDM_CHN_NONE = PDM_CHN_MAX
} PDM_CHN_ID;

typedef enum {
    RX_DFE_CHN_0,
    RX_DFE_CHN_1,
    RX_DFE_CHN_2,
    RX_DFE_CHN_3,
    RX_DFE_CHN_4,
    RX_DFE_CHN_5,
    RX_DFE_CHN_MAX,
    RX_DFE_CHN_NONE = RX_DFE_CHN_MAX
} RX_DFE_CHN_ID;

typedef enum {
    SDM_ADC_ANC_FROM_SOLE_SINC_FILTER,
    SDM_ADC_ANC_FROM_SINC_HBF_HPF,
} SDM_ADC_ANC_SEL;

typedef enum {
    RX_DFE_HBF_DISABLE,
    RX_DFE_HBF_0,
    RX_DFE_HBF_0_1,
    RX_DFE_HBF_ALL,
    RX_DFE_HBF_INVALID
} RX_DFE_HBF_BYPASS_MODE;

typedef struct sdm_adc_sinc_param {
    uint8_t sinc0_num;
    uint8_t sinc0_stg;
    uint8_t sinc0_lsh;
    uint8_t factor;

    uint8_t sinc1_num;
    uint8_t sinc1_stg;
    uint8_t sinc1_lsh;
} sdm_adc_sinc_param_t;

typedef struct sdm_adc_pdm_config {
    bool_t pdm_slave;
    uint8_t pdm_phase;
    uint8_t pdm_bck_div;
} sdm_adc_pdm_config_t;

void sdm_adc_pdm_enable(PDM_CHN_ID chn, bool_t enable);
void sdm_adc_pdm_reset(PDM_CHN_ID chn);
void sdm_adc_pdm_config(PDM_CHN_ID chn, const sdm_adc_pdm_config_t *pdm_cfg);

void sdm_adc_rx_dfe_enable(RX_DFE_CHN_ID chn, bool_t enable);
void sdm_adc_rx_dfe_reset(RX_DFE_CHN_ID chn);
void sdm_adc_rx_dfe_dump_sel(RX_DFE_CHN_ID chn, uint8_t sel);
void sdm_adc_rx_dfe_anc_only(RX_DFE_CHN_ID chn, bool_t anc_only);
void sdm_adc_rx_dfe_anc_select(RX_DFE_CHN_ID chn, SDM_ADC_ANC_SEL anc_src_sel);
void sdm_adc_rx_dfe_config_hpf(RX_DFE_CHN_ID chn, bool_t bypass, uint8_t coef);
void sdm_adc_rx_dfe_config_hbf(RX_DFE_CHN_ID chn, RX_DFE_HBF_BYPASS_MODE mode);
void sdm_adc_rx_dfe_config_pwr_scale(RX_DFE_CHN_ID chn, int16_t pwr_scale);
void sdm_adc_rx_dfe_config_sinc0(RX_DFE_CHN_ID chn,
                                 const sdm_adc_sinc_param_t *sinc_param);
void sdm_adc_rx_dfe_config_sinc1(RX_DFE_CHN_ID chn,
                                 const sdm_adc_sinc_param_t *sinc_param);
void sdm_adc_rx_dfe_config_dump(RX_DFE_CHN_ID chn, uint8_t dump_trig,
                                uint8_t dump_sel);
void sdm_adc_rx_dfe_data_clear(RX_DFE_CHN_ID chn);
bool_t sdm_adc_rx_dfe_out_flag(RX_DFE_CHN_ID chn);
uint32_t sdm_adc_rx_dfe_get_data(RX_DFE_CHN_ID chn);


#ifdef __cplusplus
}
#endif

#endif
