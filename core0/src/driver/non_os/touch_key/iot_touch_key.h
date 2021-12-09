
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

#ifndef _DRIVER_NON_OS_TOUCH_KEY_H_
#define _DRIVER_NON_OS_TOUCH_KEY_H_

#ifdef __cplusplus
extern "C" {
#endif

#define IOT_TOUCH_KEY_MAX_THRS 0x7FF

#define IOT_TOUCH_KEY_SAMPLE_POINT_VALUE_SUM  0
#define IOT_TOUCH_KEY_SAMPLE_POINT_VALUE_AVER 1

#define IOT_TOUCH_KEY_RELATIVE_THRS_SEL 1
#define IOT_TOUCH_KEY_ABSOLUTE_THRS_SEL 0

#define IOT_TOUCH_KEY_ABS_MODE_PRE_POINT 0
#define IOT_TOUCH_KEY_RELA_PRE_POINT     2

#define IOT_TOUCH_KEY_DEFAULT_CLIMB_TRIG_TIMES    2
#define IOT_TOUCH_KEY_DEFAULT_FALL_TRIG_TIMES     2
#define IOT_TOUCH_KEY_ANTI_SHAKE_CLIMB_TRIG_TIMES 3
#define IOT_TOUCH_KEY_ANTI_SHAKE_FALL_TRIG_TIMES  4

#define IOT_TOUCH_KEY_TOUCH_FALL_THRS  (1 << IOT_TOUCH_KEY_PHASE_POINT_NUM_6) * 10
#define IOT_TOUCH_KEY_TOUCH_CLIMB_THRS (1 << IOT_TOUCH_KEY_PHASE_POINT_NUM_6) * 15

/*a set of data about demo board*/
#define IOT_TOUCH_KEY_EAR_FALL_THRS  900
#define IOT_TOUCH_KEY_EAR_CLIMB_THRS 920

#define IOT_TOUCH_KEY_EAR_PHASE_TOLERAN   5
#define IOT_TOUCH_KEY_TOUCH_PHASE_TOLERAN 10

typedef enum {
    IOT_TK_TYPE_NO_DETECT,
    IOT_TK_TYPE_TOUCH_DETECT,
    IOT_TK_TYPE_EAR_DETECT,
} IOT_TK_DETECT_TYPE;

typedef enum {
    IOT_TK_NO_MODE,
    IOT_TK_RELATIVE_MODE,
    IOT_TK_ABSOLUTE_MODE,
} IOT_TK_WORK_MODE;

typedef enum {
    IOT_TOUCH_KEY_NO_INT,
    IOT_TOUCH_KEY_INT_PRESS_RELEASE = 1,
    IOT_TOUCH_KEY_INT_PRESS_MID,
    IOT_TOUCH_KEY_INT_MAX,
} IOT_TOUCH_KEY_INT;

typedef enum {
    IOT_TK_PHASE_IDLE,
    IOT_TK_PHASE_BUSY,
} IOT_TK_PHASE_STATE;

typedef enum {
    IOT_TK_PHASE_ID_0,
    IOT_TK_PHASE_ID_1,
    IOT_TK_PHASE_ID_2,
    IOT_TK_PHASE_ID_3,
    IOT_TK_PHASE_ID_4,
    IOT_TK_PHASE_ID_5,
    IOT_TK_PHASE_ID_MAX = IOT_TK_PHASE_ID_4,
    IOT_TK_PHASE_ID_INVALID,
} IOT_TK_PHASE_ID;

typedef enum {
    IOT_TK_PAD_ID_0,
    IOT_TK_PAD_ID_1,
    IOT_TK_PAD_ID_2,
    IOT_TK_PAD_ID_3,
    IOT_TK_PAD_ID_MAX,
    IOT_TK_PAD_ID_INVALID = 0xf,
} IOT_TK_PAD_ID;

typedef enum {
    IOT_TOUCH_KEY_DIV_32K_FREQ,
    IOT_TOUCH_KEY_DIV_16K_FREQ,
    IOT_TOUCH_KEY_DIV_10K_FREQ,
    IOT_TOUCH_KEY_DIV_8K_FREQ,
} IOT_TOUCH_KEY_DIV_FREQ;

typedef enum {
    IOT_TOUCH_KEY_PHASE_NUM_INVALID = 0,
    IOT_TOUCH_KEY_PHASE_POINT_NUM_5 = 5,
    IOT_TOUCH_KEY_PHASE_POINT_NUM_6,
    IOT_TOUCH_KEY_PHASE_POINT_NUM_7,
    IOT_TOUCH_KEY_PHASE_POINT_NUM_8,
    IOT_TOUCH_KEY_PHASE_POINT_NUM_MAX = IOT_TOUCH_KEY_PHASE_POINT_NUM_8,
} IOT_TOUCH_KEY_PHASE_POINT_NUM;

#define IOT_TOUCK_KEY_DEBUG
#ifdef IOT_TOUCK_KEY_DEBUG
typedef enum {
    IOT_TK_DBG_READ_CDC = 1,
    IOT_TK_DBG_PHASE_PROX_VLD,
    IOT_TK_DBG_PHASE_PROX_FD,
    IOT_TK_DBG_INT_SUMMARY0,
    IOT_TK_DBG_INT_SUMMARY1,
    IOT_TK_DBG_DBG_BUS,
} IOT_TK_IO_DBG;

/**
 * @brief  A structure point to tk's debug info
 */
typedef struct iot_touch_key_debug_info {
    /** if the value of 'cur_cdc' minus 'intr_cdc' bigger than toleran, pad state is judged as press or else release*/
    uint8_t toleran;
    /** the current cdc of the pad's monitor phase*/
    uint32_t cur_cdc;
    /** the recorded cdc of the pad's work phase when the intr occur*/
    uint32_t intr_cdc;
    /**
     * 'intr_cdc':0 pad release,ignore 'diff_value' directly.
     * 'diff_value' <= 0 ,pad release.
     *  0 <'diff_value'<'toleran',pad release or else pad pressed.
     */
    uint32_t diff_value;
    /** pad's monior phase id*/
    IOT_TK_PHASE_ID monior_phase;
    /** pad's work phase id*/
    IOT_TK_PHASE_ID work_phase;
    IOT_TOUCH_KEY_INT pad_state;
} iot_touch_key_debug_info_t;
#endif

/**
 * @brief A structure point to tk's threshold values
 */
typedef struct iot_touch_key_thrs {
    uint32_t fall_thrs;
    uint32_t climb_thrs;
} iot_touch_key_thrs_t;

/**
 * @brief A structure point to tk's thrs trig times
 */
typedef struct iot_touch_key_trig_times {
    uint8_t fall_trig_times;
    uint8_t climb_trig_times;
} iot_touch_key_trig_times_t;

/**
 * @brief A structure point to tk's point num values for phase
 */
typedef struct iot_touch_key_phase_num {
    IOT_TOUCH_KEY_PHASE_POINT_NUM monitor_point_num;
    IOT_TOUCH_KEY_PHASE_POINT_NUM work_point_num;
} iot_touch_key_point_num_t;

/**
 * @brief A structure that points to the phase mode
 */
typedef struct iot_touch_key_phase_mode {
    uint8_t monitor_phase;
    uint8_t work_phase;
} iot_touch_key_phase_mode_t;

/**
 * @brief  A structure point to tk's config param
 */
typedef struct iot_touch_key_config {
    uint32_t fall_thrs;
    uint32_t climb_thrs;
    uint8_t fall_trig_times;
    uint8_t climb_trig_times;
} iot_touch_key_config_t;

/**
 * @brief  A structure point to tk's pad info
 */
typedef struct iot_touch_key_pad_info {
    IOT_TK_PAD_ID pad_id;
    IOT_TK_WORK_MODE work_mode;
    iot_touch_key_point_num_t point_num;
} iot_touch_key_pad_info_t;

typedef void (*iot_touch_key_callback)(IOT_TK_PAD_ID pad_id, IOT_TOUCH_KEY_INT int_type);

/**
 * @brief the function is to init touch_key module
 */
void iot_touch_key_init(void);

/**
 * @brief the function is to set touch_key pad info
 * Attention:all the 'iot_touch_key_set_pad_info' must be called before 'open'
 * @param pad_id is pad id
 * @param work_mode is work mode
 * @param point_num is point number
 */
void iot_touch_key_set_pad_info(IOT_TK_PAD_ID pad_id, IOT_TK_WORK_MODE work_mode,
                                const iot_touch_key_point_num_t *point_num);
/**
 * @brief the function is to init touch_key work enable
 * @note 450us delay occurs during the function call
 */
void iot_touch_key_work_enable(void);

/**
 * @brief the function is to init touch_key work enable with phases.
 *
 * @note 450us delay occurs during the function call
 */
void iot_touch_key_work_enable_with_phase(void);

/**
 * @brief the function is to deinit touch_key module
 */
void iot_touch_key_deinit(void);

/**
 * @brief the function is to adjust tk's freq.
 * @param freq touch key's freq value
 */
void iot_touch_key_adjust_freq(IOT_TOUCH_KEY_DIV_FREQ freq);

/**
 * @brief the function is to disable touch_key work.
 */
void iot_touch_key_disable_work(void);

/**
 * @brief the function is to set touch_id's number.
 *
 * @param touch_id_num touch_id's number
 */
void iot_touch_key_set_use_num(uint8_t touch_id_num);

/**
 * @brief the function is to open touch key.
 *
 * @param pad_id pad id
 * @param work_mode work mode
 * @param param touch key's config param
 * @param cb callback func that is invoked in the intr

 * @return uint8_t RET_OK for success else for error
 */
uint8_t iot_touch_key_open(IOT_TK_PAD_ID pad_id, IOT_TK_WORK_MODE work_mode,
                           const iot_touch_key_config_t *param, iot_touch_key_callback cb);

/**
 * @brief the function is to check if the pad is used.
 *
 * @param pad_id touch key's pad id
 * @param work_mode work mode
 * @param cb callback func that is invoked in the phase intr
 * @return uint8_t RET_OK for not used else for pad used
 */
uint8_t iot_touch_key_read_pad_used(IOT_TK_PAD_ID pad_id, IOT_TK_WORK_MODE work_mode,
                                    iot_touch_key_callback cb);

/**
 * @brief the function is to get two idle phase.
 *
 * @param phase_id a ptr point to struct of phase_mode
 * @return uint8_t RET_OK for not used else for pad used
 */
uint8_t iot_touch_key_get_two_idle_phase(iot_touch_key_phase_mode_t *phase_id);

/**
 * @brief the function is to get the number of phases.
 *
 * @return uint8_t the number of phases
 */
uint8_t iot_touch_key_get_phase_num(void);

/**
 * @brief the function is to enable the pad's intr, please delay of 20ms before call.
 *
 * @param pad_id is pad id
 * @return uint8_t RET_OK for success else for error
 */
uint8_t iot_touch_key_enable_intr(IOT_TK_PAD_ID pad_id);

/**
 * @brief the function is to get the phase of pad, pad phase had configured.
 *
 * @param pad_id is pad id.
 * @param start_phase is phase id that start to get.
 * @param phase_ptr is a ptr point to phase id
 * @return uint8_t RET_OK for success else for error
 */
uint8_t iot_touch_key_get_pad_phase(IOT_TK_PAD_ID pad_id, IOT_TK_PHASE_ID start_phase,
                                    IOT_TK_PHASE_ID *phase_ptr);
/**
 * @brief the function is to config and bind pad for monitor phase.
 *
 * @param pad_id touch key's pad id
 * @param work_mode work mode
 * @param phase_ptr a ptr point to struct of phase mode
 * @param cb callback func that is invoked in the phase intr
 * @return uint8_t RET_OK RET_OK for success else for error
 */
uint8_t iot_touch_key_bind_pad_for_monitor_phase(IOT_TK_PAD_ID pad_id, IOT_TK_WORK_MODE work_mode,
                                                 iot_touch_key_phase_mode_t *phase_ptr,
                                                 iot_touch_key_callback cb);

/**
 * @brief the function is to config and bind pad for phase.
 *
 * @param pad_id touch key's pad id
 * @param work_mode work_mode
 * @param phase_ptr a ptr point to struct of phase mode
 * @param set_param
 * @param cb callback func that is invoked in the phase intr
 * @return uint8_t RET_OK for success else for error
 */
uint8_t iot_touch_key_bind_pad_for_work_phase(IOT_TK_PAD_ID pad_id, IOT_TK_WORK_MODE work_mode,
                                              iot_touch_key_phase_mode_t *phase_ptr,
                                              const iot_touch_key_config_t *set_param,
                                              iot_touch_key_callback cb);

/**
 * @brief the function is to change pad's thrs
 *
 * @param pad_id touch key's pad id
 * @param fall_thrs touch key's fall thrs
 * @param climb_thrs touch key's climb thrs
 * @return uint8_t RET_OK for success else for error
 */
uint8_t iot_touch_key_change_pad_thrs(IOT_TK_PAD_ID pad_id, uint32_t fall_thrs,
                                      uint32_t climb_thrs);

/**
 * @brief  the function is to change pad's trig times
 *
 * @param pad_id touch key's pad id
 * @param climb_trig_times touch key's climb_trig_times
 * @param fall_trig_times touch key's fall_trig_times
 * @return uint8_t RET_OK for success else for error
 */
uint8_t iot_touch_key_change_pad_trig_times(IOT_TK_PAD_ID pad_id, uint8_t climb_trig_times,
                                            uint8_t fall_trig_times);

/**
 * @brief the function is to get monitor phase
 *
 * @param pad_id touch key's pad id
 * @param phase_ptr a ptr point to monitor phase of the pad
 * @return uint8_t RET_OK for success else for error
 */
uint8_t iot_touch_key_get_monitor_phase(IOT_TK_PAD_ID pad_id, IOT_TK_PHASE_ID *phase_ptr);

/**
 * @brief the function is to get work phase
 *
 * @param pad_id touch key's pad id
 * @param phase_ptr a ptr point to work phase of the pad
 * @return uint8_t RET_OK for success else for error
 */
uint8_t iot_touch_key_get_work_phase(IOT_TK_PAD_ID pad_id, IOT_TK_PHASE_ID *phase_ptr);

/**
 * @brief the function is to get work phase without config intr
 *
 * @param pad_id touch key's pad id
 * @param phase_ptr a ptr point to work phase of the pad
 * @return uint8_t RET_OK for success else for error
 */
uint8_t iot_touch_key_get_work_phase_no_intr(IOT_TK_PAD_ID pad_id, IOT_TK_PHASE_ID *phase_ptr);
/**
 * @brief the function is to get work phase and monitor phase structure.
 *
 * @param pad_id is pad id
 * @param phase_id is phase id
 * @return uint8_t  RET_OK for success else for error
 */
uint8_t iot_touch_key_get_pad_all_phase(IOT_TK_PAD_ID pad_id, iot_touch_key_phase_mode_t *phase_id);

/**
 * @brief the function is to read cdc of the phase.
 *
 * @param id phase id
 * @return uint32_t phase id's cdc
 */
uint32_t iot_touch_key_read_phase_cdc(IOT_TK_PHASE_ID id);

/**
 * @brief the function is to get cdc of the phase which bounds to the pad.
 *
 * @param pad_id touch key's pad id
 * @param cdc_value a ptr point to cdc value
 * @return uint8_t RET_OK for success else for error
 */
uint8_t iot_touch_key_read_pad_cdc(IOT_TK_PAD_ID pad_id, uint32_t *cdc_value);

/**
 * @brief the function is to close touch_key module.
 *
 * @param pad_id touch key's pad id
 * @return uint8_t RET_OK for success else for error
 */
uint8_t iot_touch_key_close(IOT_TK_PAD_ID pad_id);

/**
 * @brief the function is to close pad.
 *
 * @param pad_id touch key's pad_id
 * @return uint8_t RET_OK for success else for error
 */
uint8_t iot_touch_key_close_pad(IOT_TK_PAD_ID pad_id);

/**
 * @brief the function is to check pad state.
 *
 * @param pad_id touch key's pad_id
 * @param pad_state a ptr point to pad state
 * @return uint8_t RET_OK for success else for error
 */
uint8_t iot_touch_key_check_pad_state(IOT_TK_PAD_ID pad_id, IOT_TOUCH_KEY_INT *pad_state);

/**
 * @brief the function is to set phase can be enable.
 */
void iot_touch_key_set_phase_enable(void);

/**
 * @brief the function is to set phase to dummy mode.
 *
 * @param phase_id idle phase id
 * @param dummy_ena dummy phase mode enable
 */
void iot_touch_key_set_phase_dummy_config(IOT_TK_PHASE_ID phase_id, bool_t dummy_ena);

/**
 * @brief the function is to get summy phase number.
 * @return uint8_t RET_OK for success else for error
 */
uint8_t iot_touch_key_get_dummy_phase_num(void);

/**
 * @brief the function is to check idle phase.
 *
 * @param start_phase the starting position of the phase
 * @param phase_id idle phase id
 * @return uint8_t RET_OK for success else for error
 */
uint8_t iot_touch_key_check_idle_phase(IOT_TK_PHASE_ID start_phase, IOT_TK_PHASE_ID *phase_id);

/**
 * @brief the function is to reset pad which you use
 *
 * @param pad_id pad id
 * @return uint8_t RET_OK for success else for error
 */
uint8_t iot_touch_key_reset_pad(IOT_TK_PAD_ID pad_id);

/**
 * @brief the function is to set touch_key vref control value
 * @param vref_ctrl is vref control value
 */
void iot_touch_key_vref_ctrl_cfg(uint32_t vref_ctrl);

/**
 * @brief the function is to get phase array which have the same pad.
 *
 * @param phase_id phase id
 * @param phase_arr the phase array which have the same pad.
 * @param total_phase_num a ptr point to total phase number
 * @return uint8_t RET_OK for success else for error
 */
uint8_t iot_touch_key_get_phase_arr(IOT_TK_PHASE_ID phase_id, IOT_TK_PHASE_ID *phase_arr,
                                    uint8_t *total_phase_num);

/**
 * @brief the function is to read phase cdc with nodelay
 *
 * @param id phase id
 * @return uint32_t cdc of the phase
 */
uint32_t iot_touch_key_read_phase_nodelay_cdc(IOT_TK_PHASE_ID id);

#ifdef IOT_TOUCK_KEY_DEBUG
/**
 * @brief the function is to set touch_key debug bus sel.
 * @param dbg_bus_sel dbg_bus_sel
 */
void iot_touch_key_io_dbg_bus_cfg(IOT_TK_IO_DBG dbg_bus_sel);

/**
 * @brief the function is to set touch_key debug sig sel.
 * @param phase_id the phase id
 * @param dbg_sig_sel debug sig sel
 */
void iot_touch_key_xphase_dbg_sig_cfg(IOT_TK_PHASE_ID phase_id, uint8_t dbg_sig_sel);

/**
 * @brief the function is to set which pad use phase0.
 * @param pad_id pad id
 */
void iot_touch_key_phase0_pad_id_cfg(IOT_TK_PAD_ID pad_id);

/**
 * @brief the function is to set phase0's aver_num
 * @param aver_num phase0's aver num
 */
void iot_touch_key_phase0_aver_num_cfg(uint8_t aver_num);

/**
 * @brief the function is to read phase0 cdc raw value.
 * @return uint16_t cdc value
 */
uint16_t iot_touch_key_read_cdc_raw(void);

/**
 * @brief the function is to get touch key's debug info
 *
 * @param pad_id pad id
 * @param touch_debug_info a ptr point to touch_debug_info
 * @return uint8_t  RET_OK for success else for error
 */
uint8_t iot_touch_key_get_debug_info(IOT_TK_PAD_ID pad_id,
                                     iot_touch_key_debug_info_t *touch_debug_info);

/**
 * @brief the function is to get touch key's all phase config info
 *
 */
void iot_touch_key_dump_all_config(void);
#endif

#ifdef __cplusplus
}
#endif

#endif /* _DRIVER_NON_OS_TOUCH_KEY_H_*/
