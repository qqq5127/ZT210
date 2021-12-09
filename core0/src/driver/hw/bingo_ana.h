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

#ifndef __DRIVER_HW_BINGO_ANA_INFO_H__
#define __DRIVER_HW_BINGO_ANA_INFO_H__

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    BINGO_ANA_SEL_ATB_AFE,
    BINGO_ANA_BT_AFE_RSV,
    BINGO_ANA_PLL_VCO_TMP,
    BINGO_ANA_ATB_PLL,
    BINGO_ANA_D_MIC0_ATB_CTRL,
    BINGO_ANA_D_MIC1_ATB_CTRL,
    BINGO_ANA_D_MIC2_ATB_CTRL,
    BINGO_ANA_D_RDAC_L_ATB_CTRL,
    BINGO_ANA_D_RDAC_R_ATB_CTRL,
    BINGO_ANA_D_PHO_TIA_RESV,
    BINGO_ANA_D_METER_RESV_40N,
    BINGO_ANA_MAX,
} BINGO_ANA_ATB_MODE;

/* Register Configuration of sel_atb_afe<2:0> */
typedef enum {
    MADC_LNA_VCM = 0x1,
    MADC_LNA_LDO,
    MADC_PA_LDO,
    MADC_BTGM_VCM,
    MADC_BB_LDO,
    MADC_BT_PLL_LDO,
    MADC_XTAL_LDO,
    MADC_BT_SW1_MAX,
} MADC_SEL_ATB_AFE_SW1;

/* Register Configuration of sel_atb_afe<4:3> */
typedef enum {
    MADC_BT_ADC_VCM = 0x1,
    MADC_BT_ADC_VREFP,
    MADC_BT_ADC_VREFN,
    MADC_BT_SW2_MAX,
} MADC_SEL_ATB_AFE_SW2;

/* Register Configuration of micx_atb_ctrl<1:0> */
typedef enum {
    MADC_MIC0_ADC_VREF,
    MADC_MIC0_ADC_VCM,
    MADC_MIC0_ATB_MAX,
} MADC_MIC0_ATB;

typedef enum {
    MADC_MIC1_ADC_VREF,
    MADC_MIC1_ADC_VCM,
    MADC_MIC1_ATB_MAX,
} MADC_MIC1_ATB;

typedef enum {
    MADC_MIC2_ADC_VREF,
    MADC_MIC2_ADC_VCM,
    MADC_MIC2_ATB_MAX,
} MADC_MIC2_ATB;

/* Register Configuration of d_rdac_l/r_atb_ctrl<1:0> */
typedef enum {
    MADC_L_AUDIO_DAC_VCM,
    MADC_L_AUDIO_DAC_VREF,
    MADC_L_AUDIO_DAC_MAX,
} MADC_RDAC_L_CTRL;

typedef enum {
    MADC_R_AUDIO_DAC_VCM,
    MADC_R_AUDIO_DAC_VREF,
    MADC_R_AUDIO_DAC_MAX,
} MADC_RDAC_R_CTRL;

typedef enum {
    BINGO_ANA_FIRST_MIC,
    BINGO_ANA_SECOND_MIC,
    BINGO_ANA_THIRD_MIC,
} BINGO_ANA_MIC_CHANNEL;

typedef enum {
    BINGO_ANA_ATB_NONE,
    BINGO_ANA_VREF_ADC,
    BINGO_ANA_ATB_VCM,
    BINGO_ANA_ATB_FORBID,
} BINGO_ANA_MIC_ATB_SEL;

typedef enum {
    BINGO_ANA_SPK_ATB_NONE,
    BINGO_ANA_SPK_ATB_VCM,
    BINGO_ANA_SPK_VREF_DAC,
    BINGO_ANA_SPK_FORBID,
} BINGO_ANA_SPK_ATB_SEL;

typedef enum {
    BINGO_ANA_GAIN_N_6,
    BINGO_ANA_GAIN_N_3,
    BINGO_ANA_GAIN_0,
    BINGO_ANA_GAIN_P_3,
    BINGO_ANA_GAIN_P_6,
    BINGO_ANA_GAIN_P_9,
    BINGO_ANA_GAIN_P_12,
    BINGO_ANA_GAIN_P_15,
    BINGO_ANA_GAIN_FORBID,
} BINGO_ANA_MIC_GAIN;

typedef enum {
    BINGO_ANA_MICBIAS_FIRST_STAGE,
    BINGO_ANA_MICBIAS_SECOND_STAGE,
} BINGO_ANA_MICBIAS_STAGE;

/*
BINGO_ANA_NORMAL_NORMAL
total current normal
power transistor quiescent current normal
*/
typedef enum {
    BINGO_ANA_NORMAL_NORMAL,
    BINGO_ANA_NORMAL_HALF,
    BINGO_ANA_HALF_NORMAL,
    BINGO_ANA_HALF_HALF,
} BINGO_ANA_MIC_CURRENT;

typedef enum {
    BINGO_ANA_FISRT_INT,
    BINGO_ANA_SECOND_INT,
} BINGO_ANA_MIC_INT_CUR;

typedef enum {
    BINGO_ANA_MICBIAS_1P45,
    BINGO_ANA_MICBIAS_1P5,
    BINGO_ANA_MICBIAS_1P55,
    BINGO_ANA_MICBIAS_1P6,
    BINGO_ANA_MICBIAS_1P65,
    BINGO_ANA_MICBIAS_1P7,
    BINGO_ANA_MICBIAS_1P75,
    BINGO_ANA_MICBIAS_1P8,
    BINGO_ANA_MICBIAS_1P85,
    BINGO_ANA_MICBIAS_1P9,
    BINGO_ANA_MICBIAS_2P05,
    BINGO_ANA_MICBIAS_2P15,
    BINGO_ANA_MICBIAS_2P25,
    BINGO_ANA_MICBIAS_2P35,
    BINGO_ANA_MICBIAS_2P45,
    BINGO_ANA_MICBIAS_2P55,
} BINGO_ANA_MICBIAS_OUT;

typedef enum {
    BINGO_ANA_MICBIAS_OFF_OFF,
    BINGO_ANA_MICBIAS_OFF_ON,
    BINGO_ANA_MICBIAS_ON_OFF,
    BINGO_ANA_MICBIAS_ON_ON,
} BINGO_ANA_MICBIAS_OUT_CTRL;

typedef enum {
    BINGO_ANA_MICBIAS_CUR_LOW_R = 1,
    BINGO_ANA_MICBIAS_CUR_HIGH_R = 7,
} BINGO_ANA_MICBIAS_CUR_FLT_R;

typedef enum {
    BINGO_ANA_MICBIAS_LOWPOWER_MODE,
    BINGO_ANA_MICBIAS_NORMAL_MODE,
} BINGO_ANA_MICBIAS_MODE;

typedef enum {
    BINGO_ANA_RDAC_VREF_0P96,
    BINGO_ANA_RDAC_VREF_0P98,
    BINGO_ANA_RDAC_VREF_1P0,
    BINGO_ANA_RDAC_VREF_1P02,
    BINGO_ANA_RDAC_VREF_1P04,
    BINGO_ANA_RDAC_VREF_1P08,
    BINGO_ANA_RDAC_VREF_1P12,
    BINGO_ANA_RDAC_VREF_1P16,
    BINGO_ANA_RDAC_VREF_1P2,
    BINGO_ANA_RDAC_VREF_1P24,
    BINGO_ANA_RDAC_VREF_1P28,
    BINGO_ANA_RDAC_VREF_1P32,
    BINGO_ANA_RDAC_VREF_1P36,
    BINGO_ANA_RDAC_VREF_1P40,
    BINGO_ANA_RDAC_VREF_1P42,
    BINGO_ANA_RDAC_VREF_1P44,
} BINGO_ANA_RDAC_VREF;

void bingo_ana_madc_enable(bool_t en);
void bingo_ana_madc_rc_filter_big_resistor_bypass(bool_t bypass);
/* div: 0 4mhz
 * div: 1 2mhz
 * div: 2 3mhz
 * div: 3 500khz */
void bingo_ana_madc_frequence_set(uint8_t div);
void bingo_ana_adc_ldo1p2_bypass_enable(bool_t bypass);
void bingo_ana_atb_afe(uint8_t atb_afe);
void bingo_ana_tia_vos_trim_enable(bool_t enable);
void bingo_ana_ldo_config(uint8_t out_valtage, uint8_t max_current);

void bingo_ana_mic0_atb_ctrl(uint8_t mic0_atb);
void bingo_ana_mic1_atb_ctrl(uint8_t mic1_atb);
void bingo_ana_mic2_atb_ctrl(uint8_t mic2_atb);
void bingo_ana_dac_left_atb_ctrl(uint8_t dac_left_atb);
void bingo_ana_dac_right_atb_ctrl(uint8_t dac_right_atb);
void bingo_ana_atb_shutoff(void);
void bingo_ana_ldo1p2_pgate_sel(uint8_t max_current);
void bingo_ana_ldo1p2_vref_trim(uint8_t trim_code);
void bingo_ana_ldo1p2_config(void);
void bingo_ana_tia_vosn_trim(bool_t enable);
void bingo_ana_tia_vosp_trim(bool_t enable);
void bingo_ana_set_bg(void);
/* Fine tune of bandgap voltage */
void bingo_ana_set_bg_tune(uint32_t tune_vbg);
void bingo_ana_bg_tune_config(void);
void bingo_ana_ic25ua(void);
void bingo_ana_ic25ua_out_tune(uint8_t ic25ua_code);
void bingo_ana_ic25ua_out_tune_config(void);
void bingo_ana_set_sel_atb_afe_sw1(MADC_SEL_ATB_AFE_SW1 sel_atbana);
void bingo_ana_clr_sel_atb_afe_sw1(MADC_SEL_ATB_AFE_SW1 sel_atbana);
void bingo_ana_set_sel_atb_afe_sw2(MADC_SEL_ATB_AFE_SW2 sel_atbana);
void bingo_ana_clr_sel_atb_afe_sw2(MADC_SEL_ATB_AFE_SW2 sel_atbana);
void bingo_ana_clr_all_sel_atb_afe(void);
void bingo_ana_set_d_mic0_atb_ctrl(MADC_MIC0_ATB sel_atbana);
void bingo_ana_set_d_mic1_atb_ctrl(MADC_MIC1_ATB sel_atbana);
void bingo_ana_set_d_mic2_atb_ctrl(MADC_MIC2_ATB sel_atbana);
void bingo_ana_set_d_rdac_l_atb_ctrl(MADC_RDAC_L_CTRL sel_atbana);
void bingo_ana_set_d_rdac_r_atb_ctrl(MADC_RDAC_R_CTRL sel_atbana);
void bingo_ana_clr_d_mic0_atb_ctrl(MADC_MIC0_ATB sel_atbana);
void bingo_ana_clr_d_mic1_atb_ctrl(MADC_MIC1_ATB sel_atbana);
void bingo_ana_clr_d_mic2_atb_ctrl(MADC_MIC2_ATB sel_atbana);
void bingo_ana_clr_d_rdac_l_atb_ctrl(MADC_RDAC_L_CTRL sel_atbana);
void bingo_ana_clr_d_rdac_r_atb_ctrl(MADC_RDAC_R_CTRL sel_atbana);
void bingo_ana_clr_all_sel_atb_afe(void);
void bingo_ana_clr_all_d_mic0_atb_ctrl(void);
void bingo_ana_clr_all_d_mic1_atb_ctrl(void);
void bingo_ana_clr_all_d_mic2_atb_ctrl(void);
void bingo_ana_clr_all_d_rdac_l_atb_ctrl(void);
void bingo_ana_clr_all_d_rdac_r_atb_ctrl(void);

void bingo_ana_d_pho_tia_itune_tia(uint8_t itune_tia);
void bingo_ana_d_pho_tia_bias_ctrl(uint8_t ctrl_tia);
void bingo_ana_ddr_phase_signal_enable(bool_t enable);
void bingo_ana_bt_adc_ldo1p2_trim_code(uint8_t trim_code);
uint32_t bingo_ana_get_micbias_vout(void);

void bingo_ana_mic_pga_lock_prevent(BINGO_ANA_MIC_CHANNEL chn, bool_t enable);
void bingo_ana_mic_pga_gain_ctrl(BINGO_ANA_MIC_CHANNEL chn,
                                 BINGO_ANA_MIC_GAIN gain);
void bingo_ana_mic_pga_enable(BINGO_ANA_MIC_CHANNEL chn, bool_t enable);
void bingo_ana_mic_atb_ctrl(BINGO_ANA_MIC_CHANNEL chn,
                            BINGO_ANA_MIC_ATB_SEL atb_sel);
void bingo_ana_mic_pga_bias_sel(BINGO_ANA_MIC_CHANNEL chn,
                                BINGO_ANA_MIC_CURRENT cur_limit);
void bingo_ana_mic_vcm_ctrl(BINGO_ANA_MIC_CHANNEL chn, uint8_t vcm_out);

void bingo_ana_mic_adc_enable(BINGO_ANA_MIC_CHANNEL chn, bool_t enable);
void bingo_ana_mic_adc_sst_n(BINGO_ANA_MIC_CHANNEL chn, bool_t enable);
void bingo_ana_mic_adc_bias_idac_ctrl(BINGO_ANA_MIC_CHANNEL chn,
                                      uint8_t bias_cur);
void bingo_ana_mic_adc_bias_int_ctrl(BINGO_ANA_MIC_CHANNEL chn,
                                     BINGO_ANA_MIC_INT_CUR stage,
                                     uint8_t bias_cur);
void bingo_ana_mic_adc_bias_vrefp_ctrl(BINGO_ANA_MIC_CHANNEL chn,
                                       uint8_t bias_cur);
void bingo_ana_mic_adc_dem_enable(BINGO_ANA_MIC_CHANNEL chn, bool_t enable);
void bingo_ana_mic_adc_vref_ctrl(BINGO_ANA_MIC_CHANNEL chn, uint8_t vref_out);

void bingo_ana_micbias_shield(void);
void bingo_ana_micbias_cap_comp_adjust(void);
void bingo_ana_ct_sdm_c_code_sel(bool_t mode);
void bingo_ana_ct_sdm_rc_tune_enable(bool_t enable);
void bingo_ana_ct_sdm_rc_tune_target(uint8_t tune_targ);

void bingo_ana_micbias_sst_n(bool_t enable);
void bingo_ana_micbias_current_limit_enable(bool_t enable);
void bingo_ana_micbias_enable(BINGO_ANA_MICBIAS_STAGE stage, bool_t enable);
void bingo_ana_micbias_mode_ctrl(BINGO_ANA_MICBIAS_MODE mode);
void bingo_ana_micbias_ctrl(BINGO_ANA_MICBIAS_OUT bias_out);
void bingo_ana_micbias_sel_vout(BINGO_ANA_MICBIAS_OUT_CTRL out_ctrl);
void bingo_ana_micbias_sel_crrnt_flt(BINGO_ANA_MICBIAS_CUR_FLT_R resistor);

/**********spk************/
void bingo_ana_rdac_data_en(uint8_t port, bool_t enable);
void bingo_ana_rdac_en(uint8_t port, bool_t enable);
void bingo_ana_lp_mode(uint8_t port, uint8_t mode);
void bingo_ana_rdac_sst_n(uint8_t port, bool_t enable);
void bingo_ana_rdac_vcm_ctrl(uint8_t port, uint8_t vcm_out);
void bingo_ana_rdac_vref_ctrl(uint8_t port, BINGO_ANA_RDAC_VREF vref_out);
void bingo_ana_rdac_vref_flt_bias_enable(uint8_t port, bool_t enable);
void bingo_ana_1st_stage_enable(uint8_t port, bool_t enable);
void bingo_ana_2nd_stage_enable(uint8_t port, bool_t enable);
void bingo_ana_3rd_stage_phase1_enable(uint8_t port, bool_t enable);
void bingo_ana_3rd_stage_phase2_enable(uint8_t port, bool_t enable);
void bingo_ana_3rd_stage_phase3_enable(uint8_t port, bool_t enable);
void bingo_ana_driver_atb_ctrl(uint8_t port, bool_t atb_sel);
void bingo_ana_driver_os_comp_trim(uint8_t port, bool_t ispos, uint16_t trim);
void bingo_ana_driver_mute_enable(uint8_t port, bool_t enable);
void bingo_ana_rdac_atb_ctrl(uint8_t port, BINGO_ANA_SPK_ATB_SEL dac_atb);

#ifdef __cplusplus
}
#endif

#endif
