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

#ifndef __DRIVER_HW_METER_ADC_H__
#define __DRIVER_HW_METER_ADC_H__

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    MADC_CHANNEL_0,
    MADC_CHANNEL_1,
    MADC_CHANNEL_2,
    MADC_CHANNEL_3,
    MADC_CHANNEL_4,
    MADC_CHANNEL_5,
    MADC_CHANNEL_6,
    MADC_CHANNEL_7,
    MADC_CHANNEL_MAX,
} MADC_CHANNEL_ID;

typedef struct meter_adc_cfg {
    uint16_t sample_length;  /* phase channel sample number */
    uint16_t discard_length; /* phase channel discard sample number */
    uint8_t sel_atb;
    uint8_t receive;
    uint8_t seln_pad_mux;
    uint8_t selp_pad_mux;
} meter_adc_ddie_cfg_t;

typedef struct meter_adc_chn_cfg {
    uint8_t sel_atb;
    uint8_t seln_pad_mux;
    uint8_t selp_pad_mux;
} meter_adc_chn_cfg_t;

void madc_clk_en(bool_t en);
/* sync reset the sdm_adc_top module */
void madc_dfe_enable(bool_t en);
/* 0:sdm_adc raw data and noise cancel output;
 * 1:sinc4 downsample output;
 * 2:decimator fir filter output;
 * 3:sdm_adc_data, through hpf and power scaler
 */
void madc_dump_select(uint8_t sel);
void madc_dump_trigger(bool_t trig);
void madc_hpf_set(uint8_t hpf_coef);
void madc_hpf_bypass(bool_t bypass);
void madc_soft_reset(bool_t reset);
void madc_stop(void);
void madc_start(void);
void madc_dfe_flag_clear(void);
bool_t madc_dfe_data_flag(void);
uint32_t madc_dfe_data_value(void);
uint8_t madc_channel_num(uint8_t number);
void madc_out_vad(MADC_CHANNEL_ID id);
void madc_dma_channel_link(MADC_CHANNEL_ID id);
void madc_dma_channel_close(MADC_CHANNEL_ID id);
bool_t madc_average_flag(MADC_CHANNEL_ID id);
void madc_average_clear(MADC_CHANNEL_ID id);
void madc_sinc_set(uint8_t num, uint8_t stg, uint8_t lsh, uint8_t factor);
uint32_t madc_data_average(MADC_CHANNEL_ID id);
uint32_t madc_data_summation(MADC_CHANNEL_ID id);
void madc_dma_config(uint8_t threshold, uint8_t sel);
uint32_t madc_get_rx_fifo_dma_addr(void);
void madc_channel_config(MADC_CHANNEL_ID id, uint8_t sel_atb, uint8_t selp, uint8_t seln);
void madc_set_data_and_discard(uint8_t phase, uint8_t data_num, uint8_t discard_num);

#ifdef __cplusplus
}
#endif

#endif
