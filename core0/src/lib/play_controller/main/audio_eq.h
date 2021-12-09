
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
#ifndef __AUDIO_EQ_H__
#define __AUDIO_EQ_H__

#include "types.h"

#define AUDIO_EQ_ES_MAX  12

typedef struct _audio_per_eq_param {
    uint32_t es_biq_num         : 4;
    uint32_t reserved           : 28;
    float overall_gain;
    float gain[AUDIO_EQ_ES_MAX];
    uint32_t f0[AUDIO_EQ_ES_MAX];
    float q_value[AUDIO_EQ_ES_MAX];
} audio_per_eq_param;

typedef struct _audio_eq_coeff_param {
    uint32_t reserved[4];
    uint32_t eq_mode;
    audio_per_eq_param es_param_L;
    audio_per_eq_param es_param_R;
} audio_eq_coeff_param;

typedef enum {
    AUDIO_EQ_BYPASS_NULL,
    AUDIO_EQ_BYPASS_ES_AND_EQ,
    AUDIO_EQ_BYPASS_ES,
    AUDIO_EQ_BYPASS_EQ,
    AUDIO_EQ_BYPASS_MAX,
} audio_eq_bypass_config_t;


typedef void (*audio_eq_process_done_cb)(void *addr, uint32_t length);

/**
 * @brief audio config music eq
 * @param flt_mode EQ mode, using fix point
 * @param data_mode EQ deal data mode, using 16 bit
 * @param sample_rate data sampling frequency
 * @param param EQ gain Q freq param
 * @return int8_t be equal of greater than 0 is using eq band number else error
 */
int8_t audio_eq_coeff_music_config(uint8_t flt_mode, uint8_t data_mode, int32_t sample_rate, void *param);

/**
 * @brief audio config spk eq
 * @param sample_rate data sampling frequency
 * @return uint8_t RET_OK for success else error.
 */
uint8_t audio_eq_coeff_spk_config(int32_t sample_rate);

/**
 * @brief audio eq deal to spk buf
 * @param buf deal buf addr
 * @param len the length of buf
 * @param cb deal done the buf then call the callback
 * @param full_pkt input pkt is full package
 * @return uint8_t RET_OK for success else error.
 */
uint8_t audio_eq_buf_process(uint8_t *buf, uint32_t len, audio_eq_process_done_cb cb, bool full_pkt);

/**
 * @brief init eq coeff
 * param[in] force_read force read ES coeff from flash.
 */
void audio_eq_coeff_init(bool force_read);

/**
 * @brief to close eq
 */
void audio_eq_coeff_deinit(void);
/**
 * @brief get adj default output gain offset, one value represents 0.1875dB
 *
 * @return int16_t db value gain offset
 */
int16_t audio_spk_get_eq_gain_offset(void);

/**
 * @brief to config bypass eq
 * @param bypass using audio_eq_bypass_config_t enum.
 * @return uint8_t RET_OK for success else error
 */
uint8_t audio_eq_set_bypass_enable(audio_eq_bypass_config_t bypass);

/**
 * @brief set adj default output gain offset
 * @param gain_offset_db set db value, unit db
 */
void audio_spk_set_eq_gain_offset(float gain_offset_db);

/**
 * @brief audio_eq_set_cur_spk_gain
 * @param target_gain  set cur spk db value, unit 0.1875db
 * @return uint8_t RET_OK for success else error
 */
uint8_t audio_eq_set_cur_spk_gain(int16_t target_gain);

/**
 * @brief audio_eq_set_drc_thr.
 * @param thr soft drc threshold.
 * @param release_time check attack time responese time.
 * @param attack_time exceeds the activation threshould of the device.
 * @return uint8_t RET_OK for success else error.
 */
uint8_t audio_eq_set_drc_thr(uint32_t thr, uint16_t release_time, uint8_t attack_time);
#endif /* __AUDIO_EQ_H__ */
