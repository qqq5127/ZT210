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

#ifndef __DRIVER_HW_METER_ADCANA_H__
#define __DRIVER_HW_METER_ADCANA_H__

#include "types.h"
#include "meter_adc.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Register Configuration of d_pho_tia_resv<3> */
typedef enum {
    MADC_D_PHO_TIA_RSV = 0x3,
} MADC_D_PHO_TIA;

/* Register Configuration of d_meter_resv_40n<6:4> */
typedef enum {
    MADC_LDO_1P1V = 0x1,
    MADC_DIG_POWER_0P8V,
    MADC_POWER_1P2V,
    MADC_MICBIAS_1P7V_SW1,
    MADC_POWER_1P8V_SW1,
    MADC_POWER_3P1V,
    MADC_POWER_SWITCH1_MAX,
} MADC_RESV_40N_SW1;

/* Register Configuration of d_meter_resv_40n<3:0> */
typedef enum {
    MADC_POWER_1P8V_SW2,
    MADC_MICBIAS_1P7V_SW2,
    MADC_POWER_SWITCH2_MAX,
} MADC_RESV_40N_SW2;

uint32_t madc_ana_read_operate(uint32_t addr);

#define MADC_WR_REG_FIELD_(N, reg, ...)                    \
    ({                                                     \
        typeof(reg) _reg_x_;                               \
        _reg_x_.w = madc_ana_read_operate((uint32_t)&reg); \
        CONCATENATE(WR_FIELD_, N)(_reg_x_, __VA_ARGS__);   \
        reg = _reg_x_;                                     \
    })

#define MADC_WR_REG_FIELD(reg, ...) \
    ({ MADC_WR_REG_FIELD_(__REG_N_FIELD__(reg, __VA_ARGS__), reg, __VA_ARGS__); })

#define MADC_RE_REG_FIELD(reg, bitfield)               \
    ({                                                 \
        typeof(reg) __x;                               \
        __x.w = madc_ana_read_operate((uint32_t)&reg); \
        (uint32_t) __x._b.bitfield;                    \
    })

typedef enum {
    MADC_ANA_CHANNEL_0,
    MADC_ANA_CHANNEL_1,
    MADC_ANA_CHANNEL_2,
    MADC_ANA_CHANNEL_3,
    MADC_ANA_CHANNEL_4,
    MADC_ANA_CHANNEL_5,
    MADC_ANA_CHANNEL_6,
    MADC_ANA_CHANNEL_7,
    MADC_ANA_CHANNEL_MAX,
} MADC_ANA_CHANNEL_ID;

typedef struct madc_sdm_ana_cfg {
    uint8_t bias_int1a_ctrl;   //adc 1st stage int op bias1,default:2'b10
    uint8_t bias_int1b_ctrl;   //adc 1st stage int op2 bias2,default:2'b10
    uint8_t bias_int2_ctrl;    //adc 2nd stage int op bias, default:2'b10
    uint8_t vcm_ctrl;          //adc ref vcm, default:3'b011
    uint8_t vref_ctrl;         //adc vref ctrl, default:3'b100
    uint8_t vrefbuf_ctrl;      //adc buffer dc point minor adjust, default 2'b01
    uint16_t meter_idac;       //10bit idac inuput, default:10'h200
} madc_sdm_ana_cfg_t;

typedef struct tia_ana_cfg {
    uint8_t sel_in_mux;   //input select
    uint8_t itune_tia;    //low power mode,default 3'b111
    uint8_t bias_ctrl;    //bias tune,default 2'b01
    uint16_t idac;        //10bit ctrl, default:10'h200
    uint8_t idac_en;
    uint8_t tia_cap_sel;   //TIA filter cap tune, default 2'b10
    uint8_t vosn_trim;
    uint8_t vosp_trim;
    uint8_t vos_trim_en;
    uint8_t tia_gain;   //default 6'h0
    uint8_t tia_en;
} madc_tia_ana_cfg_t;

void madc_ana_restore(void);
void madc_ana_gain_bypass(MADC_ANA_CHANNEL_ID id, bool_t bypass);
void madc_ana_lowpower_enable(MADC_ANA_CHANNEL_ID id, bool_t enable);
void madc_tia_enable(MADC_ANA_CHANNEL_ID id, bool_t en);
void madc_tia_sel_in_mux(MADC_ANA_CHANNEL_ID id, uint8_t sel_val);
void madc_tia_gain_set(MADC_ANA_CHANNEL_ID id, uint8_t tia_gain);
void madc_tia_idac_en(MADC_ANA_CHANNEL_ID id, uint8_t idac_code);
void madc_tia_idac_set(MADC_ANA_CHANNEL_ID id, uint8_t idac_code);
void madc_d_pho_tia_cap_sel(MADC_ANA_CHANNEL_ID id, uint8_t idac_code);
//gain:3'h0=0dB, 3'h1=6dB, 3'h2=12dB,3'h3=18dB,3'h4=24dB,3'h5=30dB,3'h6=36dB
void madc_ana_gain_set(MADC_ANA_CHANNEL_ID id, uint8_t gain, bool_t en);
void madc_ana_channel_config(MADC_ANA_CHANNEL_ID id, uint8_t selp, uint8_t seln);
void madc_ana_set_d_pho_tia_resv(MADC_ANA_CHANNEL_ID id, MADC_D_PHO_TIA sel_atbana);
void madc_ana_clr_d_pho_tia_resv(MADC_ANA_CHANNEL_ID id, MADC_D_PHO_TIA sel_atbana);
void madc_ana_set_d_meter_resv_40n_sw1(MADC_ANA_CHANNEL_ID id, MADC_RESV_40N_SW1 sel_atbana);
void madc_ana_clr_d_meter_resv_40n_sw1(MADC_ANA_CHANNEL_ID id, MADC_RESV_40N_SW1 sel_atbana);
void madc_ana_set_d_meter_resv_40n_sw2(MADC_ANA_CHANNEL_ID id, MADC_RESV_40N_SW2 sel_atbana);
void madc_ana_clr_d_meter_resv_40n_sw2(MADC_ANA_CHANNEL_ID id, MADC_RESV_40N_SW1 sel_atbana);
void madc_ana_clr_all_d_meter_resv_40n(MADC_ANA_CHANNEL_ID id);
void madc_ana_sdm_config(MADC_ANA_CHANNEL_ID id, const madc_sdm_ana_cfg_t *cfg);
void madc_ana_d_meter_dit_en(MADC_ANA_CHANNEL_ID id, uint8_t en);

#ifdef __cplusplus
}
#endif

#endif
