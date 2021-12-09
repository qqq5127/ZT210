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
#ifndef __DRIVER_HW_VAD_H__
#define __DRIVER_HW_VAD_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    VAD_IIR_B0,
    VAD_IIR_B1,
    VAD_IIR_B2,
    VAD_IIR_A0,
    VAD_IIR_A1,
    VAD_IIR_MAX,
} VAD_IIR_COEFFICIENT_ID;

typedef enum {
    VAD_GMM_SUB_BAND_0,
    VAD_GMM_SUB_BAND_1,
    VAD_GMM_SUB_BAND_2,
    VAD_GMM_SUB_BAND_3,
    VAD_GMM_SUB_BAND_4,
    VAD_GMM_SUB_BAND_5,
    VAD_GMM_SUB_BAND_MAX,
} VAD_GMM_SUB_BAND_ID;

typedef struct vad_cal_config {
    uint16_t basic_len;
    uint8_t shift_len_ratio;
    uint8_t frame_len_ratio;
    uint8_t energy_shit_bit;
    uint8_t zcr_shit_bit;
    bool_t ec_power_mode;
} vad_cal_config_t;

typedef struct vad_zcr_config {
    uint16_t low_thr;
    uint16_t hight_thr;
    uint16_t zcr_min;
    uint16_t zcr_max;
    uint16_t low_bias;
    uint16_t hight_bias;
    uint8_t zcr_update_thr0;
    uint8_t zcr_update_thr1;
    uint8_t default_zcr;
    uint8_t noise_init_frm_num;
    uint8_t zcr_hold_period;
    uint8_t adj_shift;
    uint8_t low_ratio;
    uint8_t hight_ratio;
    uint8_t sts_frame_num;
    bool_t force_zcr_sel;
    bool_t zcr_sel;
    bool_t hold_in_speech_flag;
    bool_t updata_each;
} vad_zcr_config_t;

typedef struct vad_noise_control_config {
    uint16_t always_upd_thr_val;
    uint8_t sigma_init_frm_num;
    uint8_t pow_change_thr;
    uint8_t win_pre;
    uint8_t win_post;
    uint8_t alpha;
    uint8_t always_upd_thr_exp;
    bool_t mode;
    bool_t pow_change_en;
} vad_noise_control_config_t;

typedef struct vad_decision_config {
    uint32_t thr_min;
    uint16_t low_ratio;
    uint16_t high_ratio;
    uint8_t low_len;
    uint8_t high_len;
    bool_t method;
} vad_decision_config_t;

typedef struct vad_gmm_ctrl_config {
    uint8_t aud_smp_rate;
    uint8_t td_vad_src_sel;
    uint8_t frame_band_energy;
} vad_gmm_ctrl_config_t;

typedef struct vad_large_noise_config {
    uint16_t start_detection;
    uint16_t end_detection;
    uint8_t false_ararm_delay;
    bool_t hardware_detection_en;
    bool_t large_noise_flag;
} vad_large_noise_config_t;

void vad_td_enable(bool_t enable);
void vad_gmm_enable(bool_t enable);
void vad_input_source_link(bool_t src);

/* default config */
void vad_gmm_contrlo_config(const vad_gmm_ctrl_config_t *cfg);
void vad_decision_config(const vad_decision_config_t *cfg);
void vad_noise_control(const vad_noise_control_config_t *cfg);
void vad_zcr_config(const vad_zcr_config_t *cfg);
void vad_energy_calculation_config(const vad_cal_config_t *cfg);
void vad_strong_backgroud_noise_config(const vad_large_noise_config_t *cfg);
void vad_iir_coefficient_config(VAD_IIR_COEFFICIENT_ID id, int16_t coeff);
void vad_reference_noise_estimation_store_gap(uint8_t refne_store_gap);
void vad_reference_noise_estimation_signal_enable(bool_t ref_ne_en);
void vad_adc_bit_shift_bit(uint8_t shift_bit);
void vad_zcr_calculation_threshold(uint16_t zcr_calc_thr);
void vad_frequence_config(uint8_t div);
void vad_time_domain_src(bool_t td_vad_src);

void vad_noise_begin_index(bool_t set);
void vad_speech_cfrm_index(bool_t set);
void vad_false_alarm_index(bool_t false_alarm);
void vad_ne_repl_frame_index(uint16_t ne_repl_frmidx);

void vad_strong_backgroud_noise_detection_enable(bool_t larg_bkg_en);
void vad_strong_backgroud_noise_flage(bool_t large_noise_flag);
void vad_strong_backgroud_noise_start(uint16_t start);
void vad_strong_backgroud_noise_stop(uint16_t stop);

/* td operation */
void vad_time_domain_it_mode(bool_t both_edge);
void vad_time_domain_it_clear(bool_t clr);
void vad_time_domain_it_msk(bool_t msk);
void vad_time_domain_it_enable(bool_t enable);
bool_t vad_time_domain_get_it_status(void);
bool_t vad_time_domain_get_it_raw(void);

/* dbg operation */
void vad_dbg_it_enable(bool_t enable);
void vad_dbg_it_clear(bool_t clr);
void vad_dbg_it_msk(bool_t msk);
void vad_dbg_it_mode(bool_t mode);
bool_t vad_get_dbg_it_status(void);
bool_t vad_get_dbg_it_raw(void);
bool_t vad_get_dbg_raw_vad_decision(void);
uint32_t vad_get_dbg_frame_energy(void);
uint16_t vad_get_dbg_frame_zero_cross_rate(void);
uint16_t vad_get_dbg_frame_index(void);
uint8_t vad_get_dbg_zcr_current_backgroud(void);
uint8_t vad_get_dbg_zcr_update_counter(void);
uint8_t vad_get_dbg_zcr_hold_period(void);
uint32_t vad_get_dbg_noise_ref(void);
uint32_t vad_get_dbg_sigma_ref(void);
uint32_t vad_get_dbg_frame_energy_ref_ne(void);
uint32_t vad_get_dbg_frame_energy_vad_decision(void);
uint32_t vad_get_dbg_noise_estimation_normal_ne(void);
uint32_t vad_get_dbg_sigma2_estimation_normal_ne(void);
uint8_t vad_get_dbg_state_change_counter_vad_decision(void);

/* gmm operation */
void vad_gmm_it_enable(bool_t enable);
void vad_gmm_it_clear(bool_t clr);
void vad_gmm_it_msk(bool_t msk);
void vad_gmm_it_err_clr(bool_t err_clr);
bool_t vad_gmm_get_it_status(void);
bool_t vad_gmm_get_it_origin(void);
bool_t vad_gmm_get_it_err(void);
uint32_t vad_gmm_subband_energy(VAD_GMM_SUB_BAND_ID id);
void vad_gmm_subband_enable(VAD_GMM_SUB_BAND_ID id, bool_t enable);
void vad_gmm_subband_clear(VAD_GMM_SUB_BAND_ID id);

/* read memory data */
void vad_soft_reset(bool_t reset);
void vad_noise_reset(bool_t reset);
uint16_t vad_get_buf_pointer(void);
bool_t vad_get_buf_it_status(void);
bool_t vad_get_buf_it_raw(void);
bool_t vad_get_buf_speeck_state(void);
void vad_buf_it_enable(bool_t enable);
void vad_buf_it_clear(bool_t clr);
void vad_buf_it_msk(bool_t msk);

/* get vad status */
uint8_t vad_get_debug_mode(void);
uint8_t vad_get_long_large_bkg_flag(void);
void vad_long_large_bkg_flag_clr(void);
uint32_t vad_get_noise_ne(void);
uint16_t vad_get_aud_smp_rate(void);
uint16_t vad_get_enrg_frm(void);

#ifdef __cplusplus
}
#endif

#endif /* _DRIVER_HW_SPI_H */
