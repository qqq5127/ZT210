/*
* Copyright © 2020—2021 Shanghai Awinic Technology Co., Ltd. All Rights Reserved.
* Description: Structures and functions related to Touch Algorithm
*/
#ifndef __AW_TOUCH_ALGO_LIB_H__
#define __AW_TOUCH_ALGO_LIB_H__

#include "aw_type.h"

#ifdef __cplusplus
extern "C" {
#endif // endif __cplusplus

#define TOUCH_CHANNEL_MAX				(0x08) // Request space for the number of channels
#define AW_ALG_BUFF_CLEAR				(0)

/* Temperature parameter structure */
struct aw_algo_tempera {
	AW_BOOL temp_add_flag; // Positive correlation sign
	AW_S16 temperature; // temperature
};
typedef struct aw_algo_tempera AW_ALGO_TEMPERA_T;


/* exp filter structure */
struct aw_algo_ch_exp {
	AW_U8 channel_max;
	AW_U8 filter_flag; // 0-none/1-sm/2-alpha/3-sm+alpha
	AW_S16 exp_data;
};
typedef struct aw_algo_ch_exp AW_ALGO_CH_EXP_T;

/* Filter options */
enum filter_open {
	NONE_FILTER,
	ALP_FILTER,
	SM_FILTER,
	SM_ALP_FILTER,
};

/* offset calibration structure */
struct aw_algo_ch_sill_value {
	AW_U8 channel;
	AW_U16 sill_value;
};
typedef struct aw_algo_ch_sill_value AW_ALGO_CH_VALUE_T;

/* This structure is used to receive parameters from outside */
struct aw_algo_params {
	AW_U8 all_channel_num; // Number of all channels
	AW_U8 use_key_num; // Number of key used

	AW_U16 count_long; // After clicking, the count is less than this value and judged as long press
	AW_U16 count_vlong; // long_press_II
	AW_U16 count_vvlong; // long_press_III
	AW_U16 count_double; // After clicking, the count is less than this value and judged as double-click

	AW_U16 data_smooth_range; // Data fluctuation does not exceed this value, complete baseline tracking
	AW_U16 k_smooth_range; // Determine whether the data fluctuation is smooth
	AW_U16 start_sill; // Start threshold after baseline is exceeded
	AW_U16 stop_sill; // Stop threshold after baseline is exceeded

	AW_U16 baseline_up; // BASE_LINE threshold
	AW_U16 k_sill; // The threshold is the slope to respond

	AW_U16 count_data; // The upper count limit for refreshing the baseline
	AW_U16 recount_data; // When the update fails, refresh the baseline count upper limit
	AW_U16 touch_heavy_sill; // Set threshold for heavy_sill_flag when exceeding the baseline
	AW_U16 touch_heavy_reply_count; // Start response times threshold beyond the heavy threshold
	AW_U16 noise_pt; // Range of noise
	AW_U16 touch_st_cnt_len; // Data length required for algorithm initialization
	AW_U32 timeout_updata_baseline; // This count is set to determine whether the long press has timed out

	AW_U16 key_channels[TOUCH_CHANNEL_MAX]; // Four channel signs 0000 1100B
};
typedef struct aw_algo_params AW_ALGO_PATAM_T;

/* Parameters to be uploaded for each channel*/
struct aw_data_node {
	AW_U8 m_raw_l; // The lower eight bits of the original value come from AFE sampling
	AW_U8 m_raw_h; // The high eight bits of the original value come from AFE sampling
	AW_U8 m_baseline_l;
	AW_U8 m_baseline_h;
	AW_U8 m_status;
};
typedef struct aw_data_node AW_DATA_NODE_T;

/* This structure is used to send data to UI */
struct aw_data_send {
	AW_U8 m_sensor_start_flag; // The press mark of each channel corresponds to each bit of the flag
	AW_U8 m_sensor_stop_flag;
	AW_U8 m_key_start_flag; // This flag is used to indicate the pressing state
	AW_U8 m_key_stop_flag; // After pressing, this mark is used to indicate the response area of double pressing
	AW_U8 m_key_heavy_flag; // This flag is used to indicate press Heavily
	AW_U8 m_key_status[TOUCH_CHANNEL_MAX]; // Indicates the current click status
	AW_DATA_NODE_T data[TOUCH_CHANNEL_MAX];
	AW_U8 m_check_sum; // checksum
	AW_U8 reserved1; // Reserved
	AW_U8 reserved2; // Reserved
};
typedef struct aw_data_send AW_DATA_SEND_T;

/* This structure is used to get the final pressed state */
struct aw_touch_algo_status {
	AW_U8 key_flg[TOUCH_CHANNEL_MAX]; // This flag is used to indicate whether the sensor is pressed currently
	AW_U8 key_status[TOUCH_CHANNEL_MAX]; // Variables are used to record state changes
	AW_U8 key_event[TOUCH_CHANNEL_MAX]; // Output the state of pressing the button (the user should clear it to 0 after reading)
};
typedef struct aw_touch_algo_status AW_TOUCH_STATUS_S;

/* Get algorithm parameter flag */
AW_BOOL aw_get_alg_param_flag(void *cfg, AW_BOOL *flag);
/* Set algorithm parameter flag */
AW_BOOL aw_set_alg_param_flag(void *cfg, AW_BOOL flag);
/* Controllable algorithm parameter structure initialization */
AW_BOOL aw_alg_param_init(void *cfg, AW_ALGO_PATAM_T *p_param_lib);
/* Initialization algorithm calibration coefficient */
AW_BOOL aw_alg_ch_calib_coef_init(void *cfg, AW_U16 *coef_buff);
/* Set algorithm calibration coefficient */
AW_BOOL aw_alg_set_ch_calib_coef(void *cfg, AW_U16 *buff);
/* Clear algorithm coef to 100 */
AW_BOOL aw_alg_clear_ch_coef(void *cfg);
/* Algorithm data initialization */
AW_BOOL aw_touch_alg_init(void *cfg);

/* Clear algorithm parameters */
AW_BOOL aw_touch_alg_clear(void *cfg);
/* Comprehensive algorithm recognition function */
AW_BOOL aw_touch_alg(void *cfg, AW_S16 *p_afe_buff, AW_ALGO_CH_EXP_T *p_exp_data);
/* Comprehensive algorithm recognition function(increase temperature drift detection) */
AW_BOOL aw_touch_alg_add_temp(void *cfg, AW_S16 *p_afe_buff, AW_ALGO_TEMPERA_T *p_temperature, AW_ALGO_CH_EXP_T *p_exp_data);

/* Get the param address of the algorithm data structure */
AW_BOOL aw_get_data_params(void *cfg, AW_ALGO_PATAM_T **data_param);
/* Get the calib_coef_buff address of the algorithm data structure */
AW_BOOL aw_alg_get_ch_calib_coef(void *cfg, AW_U16 **p_buff);
/* Get the address of the algorithm data structure */
AW_BOOL aw_get_data_send(void *cfg, AW_DATA_SEND_T **data_send);
/* Get the status_out of keys */
AW_BOOL aw_get_key_status(void *cfg, AW_TOUCH_STATUS_S **touch_status);

/* Reset baseline */
AW_BOOL aw_reset_baseline_new(void *cfg, AW_U8 channel, AW_S16 data);
/* Get the version of the algorithm */
AW_U32 aw_get_version(void);
/* Get the creation date of the algorithm */
AW_U32 aw_get_creat_date(void);

/* Get stop_press_update_count of chx */
AW_BOOL aw_get_stop_press_update_cnt(void *cfg, AW_U8 channel, AW_U16 *count);
/* Get baseline of chx */
AW_BOOL aw_get_baseline_chx(void *cfg, AW_U8 channel, AW_S16 *baseline);
/* Get diff of chx */
AW_BOOL aw_get_diff_chx(void *cfg, AW_U8 channel, AW_S16 *diff);
/* Get coef of chx */
AW_BOOL aw_get_coef_chx(void *cfg, AW_U8 channel, AW_U16 *coef);
/* Get coef of chxs */
AW_BOOL aw_get_coef_buff_chx(void *cfg, AW_U16 *coef_buff);
/* Get reverse_updata_offset_flag */
AW_BOOL aw_get_reverse_updata_offset_flag(void *cfg, AW_BOOL *calib_flag, AW_ALGO_CH_VALUE_T *p_ch_sill_value);

/* Get size of alg_param */
AW_U32 aw_get_alg_param_size(void);
/* Get size of data_send */
AW_U32 aw_get_alg_data_send_size(void);
/* Get size of key_status */
AW_U32 aw_get_alg_key_status_size(void);
/* Get size of alg */
AW_U32 aw_get_alg_size(void);


#ifdef __cplusplus
}
#endif // endif __cplusplus

#endif // endif __AW_TOUCH_ALGO_LIB_H__
