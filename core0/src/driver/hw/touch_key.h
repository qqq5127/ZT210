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
#ifndef _DRIVER_HW_TOUCH_KEY_H_
#define _DRIVER_HW_TOUCH_KEY_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    TK_PHASE_ID_0,
    TK_PHASE_ID_1,
    TK_PHASE_ID_2,
    TK_PHASE_ID_3,
    TK_PHASE_ID_4,
    TK_PHASE_ID_5,
    TK_PHASE_ID_MAX = TK_PHASE_ID_2,
    TK_PHASE_ID_INVALID,
} TK_PHASE_ID;

typedef enum {
    TK_PAD_ID_0,
    TK_PAD_ID_1,
    TK_PAD_ID_2,
    TK_PAD_ID_3,
    TK_PAD_ID_MAX,
    TK_PAD_ID_INVALID,
} TK_PAD_ID;

typedef enum {
    TK_PHASE_NO_INT,
    TK_PHASE_FALL_INT,
    TK_PHASE_CLIMB_INT,
} TK_PHASE_INT;

typedef enum {
    TOUCH_KEY_DIV_32K_FREQ,
    TOUCH_KEY_DIV_16K_FREQ,
    TOUCH_KEY_DIV_10K_FREQ,
    TOUCH_KEY_DIV_8K_FREQ
} TOUCH_KEY_DIV_FREQ;

/**
 * @brief touch_key_clk_init - the function is to init touch_key clk
 *
 */
void touch_key_clk_init(void);

/**
 * @brief touch_key_clk_deinit - the function is to deinit touch_key clk
 *
 */
void touch_key_clk_deinit(void);

/**
 * @brief touch_key_soft_rst - the function is to reset touch key's module
 *
 */
void touch_key_soft_rst(void);

/**
 * @brief touch_key_adjust_freq - the function is to reduce freqence
 *
 * @param freq is tk's freq
 */
void touch_key_adjust_freq(TOUCH_KEY_DIV_FREQ freq);

/**
 * @brief touch_key_annalog_pad_enable - the function is to enable annalog pad
 *
 * @param enable is enable
 */
void touch_key_annalog_pad_enable(bool_t enable);

/**
 * @brief touch_key_enable_ana_ldo1 - the function is to enable touch_key analog ldo1
 *
 */
void touch_key_enable_ana_ldo1(void);

/**
 * @brief touch_key_enable_ana_ldo2 - the function is to enable touch_key analog ldo2
 *
 */
void touch_key_enable_ana_ldo2(void);

/**
 * @brief touch_key_set_ana_ldo_lpf_res_sel - the function is to set touch_key analog ldo lpf res
 *
 */
void touch_key_set_ana_ldo_lpf_res_sel(void);

/**
 * @brief touch_key_ana_ldo_trim - the function is to set work in internal ldo
 *
 * @param ldo_ctrl is set the ldo_ctrl value.
 */
void touch_key_ana_ldo_trim(uint32_t ldo_ctrl);

/**
 * @brief touch_key_ana_ldo_lp_mode_en - the function is to enable touch_key low power mode
 *
 * @param enable 1: low power(1.5uA)  0: normal(6.5uA)
 */
void touch_key_ana_ldo_lp_mode_en(uint8_t enable);

/**
 * @brief touch_key_ldo_stability_ctl - the function is to control ldo stability
 *
 */
void touch_key_ldo_stability_ctl(void);

/**
 * @brief touch_key_set_ana_circuit_enable - the function is to set analog circuit relevant value
 *
 * @param default_vref is set the default vref value
 */
void touch_key_set_ana_circuit_enable(uint32_t default_vref);

/**
 * @brief touch_key_set_pad_enable_all - the function is to set the pad all active
 *
 */
void touch_key_set_pad_enable_all(void);

/**
 * @brief touch_key_set_pad_inuse - the function is to set the pad inuse
 *
 * @param pad_id is pad id
 */
void touch_key_set_pad_inuse(TK_PAD_ID pad_id);

/**
 * @brief touch_key_set_pad_unuse - the function is to set the pad  unused
 *
 * @param pad_id which pad do you unuse
 */
void touch_key_set_pad_unuse(TK_PAD_ID pad_id);

/**
 * @brief touch_key_set_pad_to_tk - the function is to set pad to touchkey function
 *
 * @param pad_id which pad do you use
 */
void touch_key_set_pad_to_tk(TK_PAD_ID pad_id);

/**
 * @brief touch_key_read_phase_used_pad - the function is to read phase pad used
 *
 * @param id is phase id
 * @return uint8_t pad id
 */
uint8_t touch_key_read_phase_used_pad(TK_PHASE_ID id);

/**
 * @brief touch_key_check_work_phase - the function is to check if it's work phase
 *
 * @param phase_id is phase id
 * @return bool_t true for work phase else for other
 */
bool_t touch_key_check_work_phase(TK_PHASE_ID phase_id);

/**
 * @brief touch_key_set_ana_circuit_disable - the function is to disable the analog circuit
 *
 */
void touch_key_set_ana_circuit_disable(void);

/**
 * @brief touch_key_set_basic_param - the function is to set the basic param
 *
 */
void touch_key_set_basic_param(void);

/**
 * @brief touch_key_set_work_phase_num - the function is to set how many working phase
 *
 * @param phase_num how many phases did you use
 */
void touch_key_set_work_phase_num(uint8_t phase_num);

/**
 * @brief touch_key_read_work_phase_num - the function is to read woek phase num.
 *
 * @return uint32_t return the number of phase
 */
uint32_t touch_key_read_work_phase_num(void);

/**
 * @brief touch_key_set_set_cdc_sample_thrs - the function is to set cdc sample threshold
 *                              - sample the data after the input of touchkey is stable for
 *                              this threshold(count clock is apb clock), and the effect is debounce.
 *
 * @param sample_thrs is default cdc sample threshold
 */
void touch_key_set_set_cdc_sample_thrs(uint8_t sample_thrs);

/**
 * @brief touch_key_set_pad_xphase_cfg0 - the function is to set cfg0 of your used
 *
 * @param phase_id phase_id which is phase you want config
 * @param pad_id  Which Pad is configured on this phase
 */
void touch_key_set_pad_xphase_cfg0(TK_PHASE_ID phase_id, TK_PAD_ID pad_id);

/**
 * @brief touch_key_set_xphase_trig_times - the function is to set phase's trig times
 *
 * @param phase_id phase_id which is phase you want config
 * @param climb_trig_times is climb trig times
 * @param fall_trig_times is fall trig times
 */
void touch_key_set_xphase_trig_times(TK_PHASE_ID phase_id, uint8_t climb_trig_times,
                                     uint8_t fall_trig_times);

/**
 * @brief touch_key_set_xphase_cfg0 - the function is to set pad config phase
 *
 * @param phase_id phase_id which is phase you want config
 * @param pad_id Which Pad is configured on this phase
 */
void touch_key_set_xphase_cfg0(TK_PHASE_ID phase_id, TK_PAD_ID pad_id);

/**
 * @brief touch_key_set_xphase_cfg0_without_pad - the function is to set cfg0 of your used without pad
 *
 * @param phase_id which is phase you want config
 * @param dummy_ena is dummy mode enable
 * @param sample_value_sel is regisetr to set absolute thrs or relative thrs
 * @param stragegy_sel is regisetr refer to set absolute thrs or relative thrs
 * @param pre_point is to get which sampling point
 */
void touch_key_set_xphase_cfg0_without_pad(TK_PHASE_ID phase_id, bool_t dummy_ena,
                                           uint32_t sample_value_sel, uint32_t stragegy_sel,
                                           uint8_t pre_point);

/**
 * @brief touch_key_set_xphase_cfg1 - the function is to set cfg1 of your used
 *
 * @param phase_id which is phase you want config
 * @param int_enable is whether the current phase uses interrupt
 *                      1: use interrupt  0: not use interrupt
 */
void touch_key_set_xphase_cfg1(TK_PHASE_ID phase_id, bool_t int_enable);

/**
 * @brief touch_key_set_xphase_cfg3 - the function is to set cfg3 of your used
 *                              the api will set the climb/fall threshold
 *
 * @param phase_id which is phase you want config
 */
void touch_key_set_xphase_cfg3(TK_PHASE_ID phase_id);

/**
 * @brief touch_key_set_xphase_cfg0_dummy_mode - the function is to set dummy mode
 *
 * @param phase_id which is phase
 * @param dummy_ena is dummy mode enable
 */
void touch_key_set_xphase_cfg0_dummy_mode(TK_PHASE_ID phase_id, bool_t dummy_ena);

/**
 * @brief touch_key_check_dummy_phase - the function is to check phase whether is dummy mode.
 *
 * @param phase_id which is phase
 * @return bool_t true for dummy mode false for normal mode
 */
bool_t touch_key_check_dummy_phase(TK_PHASE_ID phase_id);

/**
 * @brief touch_key_set_xphase_cfg3_change_thrs - the function is t0 set phase thrs
 *
 * @param phase_id which is phase id
 * @param fall_thrs is fall thrs
 * @param climb_thrs is climb thrs
 */
void touch_key_set_xphase_cfg3_change_thrs(TK_PHASE_ID phase_id, uint32_t fall_thrs,
                                           uint32_t climb_thrs);

/**
 * @brief touch_key_set_xphase_init_config - the function is init phase config
 *
 * @param phase_id which is phase id
 * @param dummy_ena is dummy enable
 * @param phase_aver_num is phase_aver_num register
 */
void touch_key_set_xphase_init_config(TK_PHASE_ID phase_id, bool_t dummy_ena,
                                      uint8_t phase_aver_num);

/**
 * @brief touch_key_read_phase_aver_num - the function is to read phase aver num
 *
 * @param id is phase id
 * @return uint8_t RET_OK for success else for error.
 */
uint8_t touch_key_read_phase_aver_num(TK_PHASE_ID id);

/**
 * @brief touch_key_start_work - the function is to start touch_key modele
 *
 */
void touch_key_start_work(void);

/**
 * @brief touch_key_read_start_work - the function is to read start work
 *
 * @return uint8_t RET_OK for success else for error
 */
uint8_t touch_key_read_start_work(void);

/**
 * @brief touch_key_stop_work - the function is to stop touch_key modele
 *
 */
void touch_key_stop_work(void);

/**
 * @brief touch_key_get_xphase_int_st - the function is to get climb/fall interrupt status
 *
 * @param phase_id which phase
 * @return TK_PHASE_INT_e is climb/fall interrupt status
 */
TK_PHASE_INT touch_key_get_xphase_int_st(TK_PHASE_ID phase_id);

/**
 * @brief touch_key_read_fall_ena - the function is to read fall int enable
 *
 * @param phase_id is phase id
 * @return bool_t true for enable else for disable
 */
bool_t touch_key_read_fall_ena(TK_PHASE_ID phase_id);

/**
 * @brief touch_key_read_climb_ena - the function is to read climb int enable
 *
 * @param phase_id is phase id
 * @return bool_t true for enable else for disable
 */
bool_t touch_key_read_climb_ena(TK_PHASE_ID phase_id);

/**
 * @brief touch_key_clr_xphase_intr - the function is to clear the interrupt status which you want
 *
 * @param phase_id which is phase you want clear
 * @param climb_clr is climb int clr
 * @param fall_clr is fall int clr
 */
void touch_key_clr_xphase_intr(TK_PHASE_ID phase_id, bool_t climb_clr, bool_t fall_clr);

/**
 * @brief touch_key_clr_xphase_int - the function is to clear the interrupt status which you want
 *
 * @param phase_id which is phase you want clear
 */
void touch_key_clr_xphase_int(TK_PHASE_ID phase_id);

/**
 * @brief touch_key_reset_xphase - the function is to reset the phase which you want
 *
 * @param phase_id which phase
 */
void touch_key_reset_xphase(TK_PHASE_ID phase_id);

/**
 * @brief touch_key_set_xphase_rst - the function is to set phase reset
 *
 * @param phase_id which phase
 */
void touch_key_set_xphase_rst(TK_PHASE_ID phase_id);

/**
 * @brief touch_key_release_xphase_rst - the function is to release reset
 *
 * @param phase_id  which phase
 */
void touch_key_release_xphase_rst(TK_PHASE_ID phase_id);

/**
 * @brief touch_key_get_irq_vector - the function is to get touch_key vector
 *
 * @return uint32_t is touch_key vector
 */
uint32_t touch_key_get_irq_vector(void);

#define TOUCK_KEY_DEBUG

#ifdef TOUCK_KEY_DEBUG
/**
 * @brief touch_key_cdc_sample_cnt_threshold - the function is to set touch_key cdc sample threshold
 *
 * @param cdc_sample_cnt_thrs is sample the data after the input of touchkey is stable for this threshold
 */
void touch_key_cdc_sample_cnt_threshold(uint32_t cdc_sample_cnt_thrs);

/**
 * @brief touch_key_phase_read_cdc_flag - the function is to get touch_key of you input phase_id cdc flag
 *
 * @param id is phase id
 * @return bool_t is cdc flag
 */
bool_t touch_key_phase_read_cdc_flag(TK_PHASE_ID id);

/**
 * @brief touch_key_phase_read_cdc - the function is to get touch_key of you input phase_id cdc value
 *
 * @param id is phase id
 * @return uint32_t is cdc value
 */
uint32_t touch_key_phase_read_cdc(TK_PHASE_ID id);

/**
 * @brief touch_key_phase_cdc_clear - the function is to clear touch_key of you input phase_id
 *
 * @param id is phase id
 */
void touch_key_phase_cdc_clear(TK_PHASE_ID id);

/**
 * @brief touch_key_read_tk_dbg_bus - the function is to read touch_key debug bus data
 *
 * @return uint16_t is the debug bus data[16bit]
 */
uint16_t touch_key_read_tk_dbg_bus(void);

/**
 * @brief touch_key_dbg_bus_sel_cfg - the function is to set debug bus sel
 *
 * @param dbg_bus_sel is the value that you will set
 */
void touch_key_dbg_bus_sel_cfg(uint32_t dbg_bus_sel);

/**
 * @brief touch_key_pmm_dbg_sig_sel_cfg - the function is to set pmm dbg sig sel
 *
 * @param phase_id is you set phase_id
 * @param dbg_sig_sel is the value that you will set
 */
void touch_key_pmm_dbg_sig_sel_cfg(TK_PHASE_ID phase_id, uint8_t dbg_sig_sel);

/**
 * @brief touch_key_vref_ctrl_cfg - the function is to set vref control value
 *
 * @param vref_ctrl is vref control value
 */
void touch_key_vref_ctrl_cfg(uint32_t vref_ctrl);

/**
 * @brief touch_key_phase0_pad_id_cfg - the function is to set pad to phase0
 *
 * @param pad_id is pad id
 */
void touch_key_phase0_pad_id_cfg(TK_PAD_ID pad_id);

/**
 * @brief touch_key_phase0_aver_num_cfg - the function is to set touch_key phase0 aver_num
 *
 * @param aver_num is aver num
 */
void touch_key_phase0_aver_num_cfg(uint8_t aver_num);

#endif

#ifdef __cplusplus
}
#endif

#endif /* _DRIVER_HW_TOUCH_KEY_H_ */
