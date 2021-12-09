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
#ifndef PLAY_CONTROLLER__H_
#define PLAY_CONTROLLER__H_

#ifdef __cplusplus
extern "C" {
#endif

typedef uint32_t (*app_sync_start_tone_func_t)(uint32_t tone_id,
    uint32_t sync_by_sn, uint64_t start_point, uint32_t start_rtc_ms);
typedef uint32_t (*app_sync_cancel_tone_func_t)(uint32_t event_id, uint32_t stop_playing);
typedef uint32_t (*app_sync_start_led_action_func_t)(uint32_t led_evt_id,  uint32_t start_rtc_ms);

typedef struct {
    app_sync_start_tone_func_t sync_start_tone_cb;
    app_sync_cancel_tone_func_t sync_cancel_tone_cb;
    app_sync_start_led_action_func_t sync_start_led_action_cb;
} sync_action_callbck_t;

/** @brief AUDIO asrc channel id.*/
typedef enum {
    PLAYER_ASRC_CHANNEL_LEFT,
    PLAYER_ASRC_CHANNEL_RIGHT,
    PLAYER_ASRC_CHANNEL_MAX,
    AUDIO_MIC_ASRC_CHANNEL_0 = PLAYER_ASRC_CHANNEL_MAX,
    AUDIO_MIC_ASRC_CHANNEL_1,
    AUDIO_MIC_ASRC_CHANNEL_MAX,
} asrc_chan_id;

#define PLAYER_AUDIO_ASRC_CHANNEL_ID PLAYER_ASRC_CHANNEL_RIGHT
#define PLAYER_TONE_ASRC_CHANNEL_ID  PLAYER_AUDIO_ASRC_CHANNEL_ID

typedef struct {
    uint8_t rx_asrc_bitmap;
    uint8_t tx_asrc_bitmap;
    uint16_t reserved;
} share_asrc_config_t;

/**
 * @brief This function is to start player.
 *
 * @param start is the start signal.
 * @return uint32_t RET_OK for success else error.
 */
uint32_t player_start(uint8_t start);

/**
 * @brief: start sync play tone with peer dev. it's called from BT to dtop.
 * @param tone_id: id of tone to play.
 * @param sync_by_sn: 1 if sync by sn, 0 - sync by bt clock.
 * @param start_point_high: high 4 byte part of bt_clock if sync by bt clock,
 *      0 if sync by sn.
 * @param start_point_low: low 4 byte part of bt_clock if sync by bt clock,
 *      sn if sync by sn.
 * @param start_rtc_ms: target rtc time to play the tone,
 *      only valid when sync by bt_clock.
 * @return uint32_t RET_OK for success else error.
 */
uint32_t player_play_tone(uint32_t tone_id, uint32_t sync_by_sn,
    uint32_t start_point_high, uint32_t start_point_low, uint32_t start_rtc_ms);

/**
 * @brief: cancel sync tone. it's called from BT to dtop.
 * @param event_id: event id of tone to play.
 * @return uint32_t RET_OK for success else error.
 */
uint32_t player_cancel_tone(uint32_t event_id);

/**
 * @brief: start sync led action with peer dev. it's called from BT to dtop.
 * @param led_evt_id: id of event which trigger led action.
 * @param start_rtc_ms: rtc time in ms to perform led action.
 * @return uint32_t RET_OK for success else error.
 */
uint32_t perform_led_action(uint32_t led_evt_id, uint32_t start_rtc_ms);

/**
 * @brief This function is to start mic ,
 *    it is async,because the dri req delay 50ms+.
 *
 * @param cfg  The config param of mic.
 */
void player_cfg_mic(uint32_t cfg);

#ifndef NEW_ARCH
/**
 * @brief This function is to config player frequence.
 *
 * @param freq_in is the input frequence.
 * @param freq_out is the output frequence.
 * @param ppm is the num of asrc_ppm_unit.
 */
void player_config_frequence(uint32_t freq_in, uint32_t freq_out, int16_t ppm);
#endif
/**
 * @brief This function is to get latch sample count.
 *
 * @return uint32_t is the latch sample count.
 */
uint32_t player_get_sample_cnt(void);

/**
 * @brief This function is to get speaker current sample count.
 *
 * @return uint32_t is the speaker current sample count.
 */
uint32_t player_get_spk_current_cnt(void);

/**
 * @brief This function is to get latch sample count and timestamp.
 *
 * @param[out] p_ts         The pointer of the asrc latcn current timestamp.
 * @param[out] p_sample_num The pointer of the sample number.
 *
 * @return uint32_t is the latch sample count.
 */
uint32_t player_get_spk_latch_sample_cnt_and_ts(uint32_t *p_ts, uint32_t *p_sample_num);

/**
 * @brief This function is to get speaker current sample count and timestamp.
 *
 * @param[out] p_ts         The pointer of the asrc latcn current timestamp.
 * @param[out] p_sample_num The pointer of the sample number.
 *
 * @return uint32_t is the speaker current sample count.
 */
uint32_t player_get_spk_current_cnt_and_ts(uint32_t *p_ts, uint32_t *p_sample_num);

/**
 * @brief This function is to create dsp stream.
 *
 * @param bt_stream_id is the bt stream id.
 * @param alg is the algorithm.
 * @param sample_rate is the sample rate.
 * @param reserved is the reserved parameter.
 * @return uint32_t is call this function itself.
 */
uint32_t dsp_create_stream(uint32_t bt_stream_id, uint32_t alg,
                           uint32_t sample_rate, uint32_t reserved);

/**
 * @brief This function is to destory dsp stream.
 *
 * @param bt_stream_id is the bt stream id.
 * @return uint32_t is call this function itself.
 */
uint32_t dsp_destory_stream(uint32_t bt_stream_id);

/**
 * @brief This function is to trigger dsp tone.
 *
 * @param tone_id is the tone id.
 * @param alg is the algorithm.
 * @param sample_rate is the sample rate.
 * @return uint32_t is call this function itself.
 */
uint32_t dsp_trigger_tone(uint32_t tone_id, uint32_t alg, uint32_t sample_rate);

/**
 * @brief This function is to inform anc event.
 * @param event_id inform anc the event.
 * @param reserved for future use.
 * @return uint32_t is RET_OK else is error.
 */
uint32_t player_anc_event_inform(uint32_t event_id, uint32_t reserved);

/*
 * @brief: register a callback to process sync tone start cmd from rpc.
 * @param sync_start_tone_cb: callback to process sync tone start cmd from rpc.
 */
void sync_reg_start_tone_cb(app_sync_start_tone_func_t sync_start_tone_cb);

/*
 * @brief: register a callback to process sync tone cancel cmd from rpc.
 * @param sync_start_tone_cb: callback to process sync tone cancel cmd from rpc.
 */
void sync_reg_cancel_tone_cb(app_sync_cancel_tone_func_t sync_cancel_tone_cb);

/*
 * @brief: register a callback to process sync led start cmd from rpc.
 * @param sync_start_tone_cb: callback to process sync led start cmd from rpc.
 */
void sync_reg_start_led_action_cb(app_sync_start_led_action_func_t sync_start_led_action_cb);

#ifdef __cplusplus
}
#endif

#endif /* PLAY_CONTROLLER__H_ */
