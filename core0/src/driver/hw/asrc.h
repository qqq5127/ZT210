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
#ifndef _DRIVER_HW_ASRC_H
#define _DRIVER_HW_ASRC_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    ASRC_CHANNEL_0,
    ASRC_CHANNEL_1,
    ASRC_CHANNEL_TX_MAX,
    ASRC_CHANNEL_2 = ASRC_CHANNEL_TX_MAX,
    ASRC_CHANNEL_3,
    ASRC_CHANNEL_MAX,
    ASRC_CHANNEL_RX_MAX = ASRC_CHANNEL_MAX,
    ASRC_CHANNEL_NONE = ASRC_CHANNEL_MAX
} ASRC_CHANNEL_ID;

typedef enum {
    ASRC_HALF_WORD_DISABLED,
    ASRC_HALF_WORD_LEAST_16BIT,
    ASRC_HALF_WORD_16BIT_PAIR,
    ASRC_HALF_WORD_RESERVED
} ASRC_HALF_WORD_MODE;

typedef enum {
    ASRC_OUT_SEL_TO_DAC,
    ASRC_OUT_SEL_TO_MEMORY,
} ASRC_OUT_SELECT;

typedef enum {
    ASRC_FARROW_TX,
    ASRC_FARROW_RX,
} ASRC_FARROW_MODE;

typedef enum {
    ASRC_COEFF_BIQUAD0_COEFF0,
    ASRC_COEFF_BIQUAD0_COEFF1,
    ASRC_COEFF_BIQUAD0_COEFF2,
    ASRC_COEFF_BIQUAD0_COEFF3,
    ASRC_COEFF_BIQUAD0_COEFF4,
    ASRC_COEFF_BIQUAD1_COEFF0,
    ASRC_COEFF_BIQUAD1_COEFF1,
    ASRC_COEFF_BIQUAD1_COEFF2,
    ASRC_COEFF_BIQUAD1_COEFF3,
    ASRC_COEFF_BIQUAD1_COEFF4,
    ASRC_COEFF_PARAM_ID_MAX,
} ASRC_COEFF_PARAM_ID;

typedef enum {
    ASRC_INT_UNDER_FLOW,
    ASRC_INT_REACH_SAMPLE_CNT,
    ASRC_INT_TYPE_MAX,
} ASRC_INT_TYPE;

typedef struct asrc_frequence_config {
    uint32_t farrow_delta_w_ppm;
    uint16_t fout_div;
    uint8_t biquad_en;
    uint8_t hb_filter_en;
    bool_t cic_en;
} asrc_frequence_config_t;

uint32_t *asrc_get_tx_dma_addr(ASRC_CHANNEL_ID chn);
uint32_t asrc_get_latched_sample_cnt(ASRC_CHANNEL_ID chn);
uint32_t asrc_get_freerun_sample_cnt(ASRC_CHANNEL_ID chn);
uint32_t asrc_int_get_status(ASRC_CHANNEL_ID chn);
void asrc_enable(ASRC_CHANNEL_ID chn, bool_t enable);
void asrc_self_start(ASRC_CHANNEL_ID chn, bool_t enable);
void asrc_set_auto_stop(ASRC_CHANNEL_ID chn, bool_t enable);
void asrc_int_disable(ASRC_CHANNEL_ID chn, ASRC_INT_TYPE type);
void asrc_int_disable_all(ASRC_CHANNEL_ID chn);
void asrc_int_clear(ASRC_CHANNEL_ID chn, ASRC_INT_TYPE type);
void asrc_int_clear_all(ASRC_CHANNEL_ID chn);
void asrc_latch_mu_status_enable(ASRC_CHANNEL_ID chn, bool_t en);
void asrc_hword_scale_enable(ASRC_CHANNEL_ID chn, bool_t hword_scale);
void asrc_set_farrow_mode(ASRC_CHANNEL_ID chn, ASRC_FARROW_MODE mode);
void asrc_set_half_word_mode(ASRC_CHANNEL_ID chn, ASRC_HALF_WORD_MODE mode);
void asrc_out_select(ASRC_CHANNEL_ID chn, ASRC_OUT_SELECT sel);
void asrc_coeff_config(ASRC_CHANNEL_ID chn, const uint32_t *coeff);
void asrc_ppm_set(ASRC_CHANNEL_ID chn, uint32_t farrow_delta_w_ppm);
void asrc_set_target_sample_cnt(ASRC_CHANNEL_ID chn, uint32_t cnt);
uint32_t asrc_get_target_sample_cnt(ASRC_CHANNEL_ID chn);
void asrc_int_enable(ASRC_CHANNEL_ID chn, ASRC_INT_TYPE type);
void asrc_set_continue(ASRC_CHANNEL_ID id, bool_t enable);
void asrc_auto_stop_throshold(ASRC_CHANNEL_ID chn, uint16_t throshold);
void asrc_auto_start_throshold(ASRC_CHANNEL_ID chn, uint8_t throshold);
void asrc_farrow_delta_set(ASRC_CHANNEL_ID chn, uint32_t farrow_delta);
void asrc_tx_start(ASRC_CHANNEL_ID chn, bool_t start);
void asrc_frequence_config(ASRC_CHANNEL_ID chn,
                             const asrc_frequence_config_t *cfg);
void asrc_coeff_param_set(ASRC_CHANNEL_ID chn, ASRC_COEFF_PARAM_ID id,
                          uint32_t value);
uint8_t asrc_write_filter_coeff(ASRC_CHANNEL_ID chn, uint8_t offect,
                             const uint32_t *filter_coeff, uint32_t lenth);
uint32_t asrc_get_tx_ibuf_ovt_thres(ASRC_CHANNEL_ID chn);
uint32_t asrc_get_start_option(ASRC_CHANNEL_ID chn);
#ifdef __cplusplus
}
#endif

#endif /* _DRIVER_HW_ASRC_H */
