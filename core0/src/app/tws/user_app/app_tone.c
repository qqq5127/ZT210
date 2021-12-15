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
#include "os_mem.h"
#include "string.h"
#include "player_api.h"
#include "app_tone.h"
#include "app_main.h"
#include "usr_cfg.h"
#include "ro_cfg.h"
#include "afifo.h"
#include "iot_rtc.h"
#include "app_wws.h"
#include "app_evt.h"
#include "play_controller.h"

#ifndef NEW_ARCH
#include "play_tone.h"
#include "dtopcore_data_mgr.h"
#else
#include "nplayer_tone_api.h"
#include "nplayer_stream_itl.h"
#include "nplayer_restart.h"
#endif

#define TONE_MESSAGE_ID_DONE            0
#define TONE_MESSAGE_ID_PLAY_REQUEST    1
#define TONE_MESSAGE_ID_PLAY            2
#define TONE_MESSAGE_ID_CANCEL          3
#define TONE_MESSAGE_ID_SLV_SYNC_CANCEL 4

/* this id means SEC dev is playing remote sync tone. */
#define TONE_SYNC_REMOTE_EVENT_ID 0xFFFFFFFF
/* invalid sync tone id means no remote tone playing. */
#define TONE_SYNC_TONE_ID_INVALID 0xFFFFFFFF

#ifndef TONE_FIFO_SIZE
#define TONE_FIFO_SIZE 8
#endif

AFIFO_INIT(tone, uint8_t, TONE_FIFO_SIZE);

// time reserved for tone sync playing
#ifndef PLAY_TONE_SYNC_DELAY_US
#define PLAY_TONE_SYNC_DELAY_US         (200*1000)
#endif

/* default delay before sending play tone msg. */
#ifndef TONE_PLAY_DEFAULT_DELAY_US
#define TONE_PLAY_DEFAULT_DELAY_US      0
#endif

#ifndef TONE_PLAY_MAX_RPC_LATENCY_US
#define TONE_PLAY_MAX_RPC_LATENCY_US    (280*1000)
#endif

static uint8_t app_tone_stop_sync(uint32_t event_id);
static void app_tone_cancel_internal(uint16_t event_id, bool_t stop_playing);
static uint32_t app_slave_cancel_sync_tone(uint32_t event_id, uint32_t stop_playing);
static uint32_t app_is_streaming(void);
uint8_t app_tone_start_sync(uint16_t tone_id, uint32_t force_sync_by_timing);
static inline uint32_t app_tone_force_sync_by_timing(uint32_t event_id);

/* msg for playing tone trigger by BT RPC call. */
typedef struct {
    uint32_t tone_id;
    uint32_t target_sn;
    uint64_t start_bts;
    uint32_t start_rtc_us;
} tone_msg_play_t;

typedef struct {
    uint32_t event_id;
    uint32_t cancel_flag;
} tone_msg_cancel_t;

/* info of pending tone. */
typedef struct {
    uint32_t is_valid;
    tone_msg_play_t tone_play_cmd;
} tone_play_pending_info_t;

static uint32_t tone_event_in_playing = 0;
/* sync playing tone id. */
static uint32_t sync_tone_id_in_playing = TONE_SYNC_TONE_ID_INVALID;
/* for case that tone sync by time but streaming exist. */
static tone_play_pending_info_t tone_play_pending_info;
/* if tone is enabled, for test and debug*/
static bool_t tone_enable = true;

static uint32_t app_is_streaming(void)
{
#ifndef NEW_ARCH
    uint32_t is_streaming = is_music_voice_stream();
#else
    uint32_t is_streaming = is_music_or_voice_stream();
#endif
    return is_streaming;
}

#ifdef NEW_ARCH

static inline void app_tone_on_restart_req_done_cb(uint8_t reason, uint32_t reserved)
{
    (void)reserved;
    DBGLOG_TONE_ERR("app_tone_on_restart_req_done_cb. reason = 0x%04X. tone_pending = %d.\n",
                    reason, tone_play_pending_info.is_valid);
    if (tone_play_pending_info.is_valid) {
        if ((!player_tone_stream_exist()) && (!app_is_streaming())) {
            player_tone_create_stream();
        }
        app_send_msg(MSG_TYPE_TONE, TONE_MESSAGE_ID_PLAY, &tone_play_pending_info.tone_play_cmd,
                     sizeof(tone_play_pending_info.tone_play_cmd));
    }
}

static inline uint32_t app_tone_restart_music_voice_stream(void)
{
    /* todo: to be implemented in new arch. */
    return RET_NOSUPP;
}

#else

static inline void app_tone_on_restart_req_done_cb(uint8_t reason)
{
    DBGLOG_TONE_ERR("app_tone_on_restart_req_done_cb. reason = 0x%04X. tone_pending = %d.\n",
                    reason, tone_play_pending_info.is_valid);
    if (tone_play_pending_info.is_valid) {
        if ((!player_tone_stream_exist()) && (!app_is_streaming())) {
            player_tone_create_stream();
        }

        app_send_msg(MSG_TYPE_TONE, TONE_MESSAGE_ID_PLAY, &tone_play_pending_info.tone_play_cmd,
                     sizeof(tone_play_pending_info.tone_play_cmd));
    }
}

static inline uint32_t app_tone_restart_music_voice_stream(void)
{
    return player_stream_restart_req(PLAYER_RESTART_REASON_DTOP_TONE,
                                     app_tone_on_restart_req_done_cb);
}

#endif

static inline uint32_t app_tone_force_sync_by_timing(uint32_t event_id)
{
    uint32_t force_sync_by_timing;
    switch (event_id) {
        case EVTSYS_CONNECTED:
        case EVTSYS_DISCONNECTED:
            force_sync_by_timing = 1;
            break;
        default:
            force_sync_by_timing = 0;
    }

    return force_sync_by_timing;
}

/**
 * @brief tone app requst to sync tone play.
 * @param tone_id: id of tone to be played.
 * @param force_sync_by_timing: 0 - force sync tone by timing,
 *     0 - auto select sync method.
 * @return RET_OK if it's OK to play tone sync, other value if failed.
 */
uint8_t app_tone_start_sync(uint16_t tone_id, uint32_t force_sync_by_timing)
{
    uint8_t result;
    uint32_t is_streaming = app_is_streaming();
    uint32_t target_sn = player_get_tone_start_ts(PLAY_TONE_SYNC_DELAY_US/1000);

    if (force_sync_by_timing) {
        DBGLOG_TONE_DBG("play tone id %d, target_sn = %d. force_sync_by_timing.\n", tone_id,
                        target_sn);
        target_sn = 0;
    }

    if (is_streaming && target_sn == 0) {
        /* failed to get sn in streaming mode. */
        result = app_tone_restart_music_voice_stream();
        DBGLOG_TONE_DBG("play tone id %d, restart streaming. result = %d.\n", tone_id, result);
    }

    if (is_streaming && target_sn) {
        bt_sync_play_tone_by_sn_cmd_t param;
        param.tone_id = tone_id;
        param.ms_per_sn = 15;   // it's not used as dtop specified target sn.
        param.rtc_time_ms = iot_rtc_get_global_time_ms();
        param.current_sn = target_sn;
        result = app_bt_send_rpc_cmd(BT_CMD_SYNC_PLAY_TONE_BY_SN, &param, sizeof(param));
    } else {
        bt_sync_play_tone_by_btclk_cmd_t param;
        param.tone_id = tone_id;
        param.use_ext_delay = force_sync_by_timing ? 1 : 0;
        result = app_bt_send_rpc_cmd(BT_CMD_SYNC_PLAY_TONE_BY_BTCLK, &param, sizeof(param));
    }

    DBGLOG_TONE_DBG("play tone id %d, streaming = %d, target_sn = %d. result = %d.\n", tone_id,
                    is_streaming, target_sn, result);
    return result;
}

uint8_t app_set_trigger_bt_clk(uint64_t bts)
{
    bt_set_bt_trigger_ts_command_t param;
    param.bt_clock = bts;
    DBGLOG_TONE_DBG("dtop set play tone @bts(%X%08X).\n", (uint32_t)(bts >> 32),
                    (uint32_t)(bts & 0xFFFFFFFF));
    return app_bt_send_rpc_cmd(BT_CMD_SET_BT_TRIGGER_TS, &param, sizeof(param));
}

static inline uint32_t is_valid_tone_index(uint8_t tone_index)
{
    const ro_cfg_tone_list_t *list = ro_cfg()->tone;
    return (tone_index < list->num);
}

static uint32_t get_tone_index_by_id(uint32_t tone_id, uint32_t *p_tone_index)
{
    const ro_cfg_tone_list_t *list = ro_cfg()->tone;
    for (uint32_t tone_index = 0; tone_index < list->num; ++tone_index) {
        if (list->tones[tone_index].tone_id == tone_id) {
            *p_tone_index = tone_index;
            return RET_OK;
        }
    }

    return RET_FAIL;
}

static uint32_t get_tone_index_by_event_id(uint32_t tone_evt_id, uint32_t *p_tone_index)
{
    const ro_cfg_tone_list_t *list = ro_cfg()->tone;
    for (uint32_t tone_index = 0; tone_index < list->num; ++tone_index) {
        if (list->tones[tone_index].event_id == tone_evt_id) {
            *p_tone_index = tone_index;
            return RET_OK;
        }
    }

    return RET_FAIL;
}

static uint32_t get_tone_id_by_event_id(uint32_t event_id, uint32_t *p_tone_id)
{
    uint32_t tone_index;
    if (RET_OK == get_tone_index_by_event_id(event_id, &tone_index)) {
        const ro_cfg_tone_list_t *list = ro_cfg()->tone;
        *p_tone_id = list->tones[tone_index].tone_id;
        return RET_OK;
    }

    return RET_FAIL;
}

static uint32_t get_tone_param_by_index(uint8_t tone_index, player_tone_param_t *p_out_param)
{
    uint8_t volume_level = 0;
    const ro_cfg_tone_list_t *list = ro_cfg()->tone;
    const ro_cfg_tone_setting_t *setting = ro_cfg()->tone_setting;
    const ro_cfg_volume_t *volume = ro_cfg()->volume;

    if (p_out_param == NULL || (!is_valid_tone_index(tone_index))) {
        return RET_INVAL;
    }

    p_out_param->tone_id = list->tones[tone_index].tone_id;
    p_out_param->spk = list->tones[tone_index].tone_channel;
    p_out_param->vol_md = setting->tone_volume_mode;
    if (p_out_param->vol_md == VOL_MD_INDEPENDENT) {
        volume_level = list->tones[tone_index].tone_volume;
        p_out_param->vol = volume->volume_levels[volume_level].tone_gain;
    } else {
        volume_level = volume->default_tone_volume;
        p_out_param->vol = volume->volume_levels[volume_level].tone_gain;
    }

    return RET_OK;
}

static uint32_t get_tone_param_by_tone_id(uint8_t tone_id, player_tone_param_t *p_out_param)
{
    uint32_t tone_index;
    if (RET_OK == get_tone_index_by_id(tone_id, &tone_index)) {
        return get_tone_param_by_index(tone_index, p_out_param);
    }

    return RET_FAIL;
}

static uint32_t is_sync_tone_event_id(uint32_t tone_event_id)
{
    player_tone_param_t tone_param;
    uint32_t tone_index;
    if (RET_OK == get_tone_index_by_event_id(tone_event_id, &tone_index)) {
        if (RET_OK == get_tone_param_by_index(tone_index, &tone_param)) {
            if (tone_param.spk == PLAYER_SPK_BOTH) {
                return 1;
            }
        }
    }

    return 0;
}

static uint32_t get_tone_event_id_by_tone_index(uint8_t tone_index, uint32_t *p_event_id)
{
    const ro_cfg_tone_list_t *list = ro_cfg()->tone;
    if (NULL == p_event_id || tone_index >= list->num) {
        return RET_INVAL;
    }

    *p_event_id = list->tones[tone_index].event_id;
    return RET_OK;
}

/*
 * @brief tone app requst to stop sync tone.
 * @param tone_id: event id of tone to be played.
 * @return: RET_OK if it's OK to play tone sync, other value if failed.
 */
static uint8_t app_tone_stop_sync(uint32_t event_id)
{
    /* 1. for sync tone,
     *    master notify slave to cancel.
     *    slave cancel local pending tone playing message.
     */
    if (is_sync_tone_event_id(event_id)) {
        if (app_wws_is_master()) {
            /* cancel sync tone playing on peer device, if necessary. */
            bt_sync_cancel_tone_t cancel_tone_cmd;
            cancel_tone_cmd.event_id = event_id;
            uint8_t result = app_bt_send_rpc_cmd(BT_CMD_SYNC_CANCEL_TONE, &cancel_tone_cmd,
                                                 sizeof(cancel_tone_cmd));
            DBGLOG_TONE_DBG("app_tone_stop_sync on PRI. result = %d.\n", result);
        } else {
            DBGLOG_TONE_DBG("app_tone_stop_sync on SEC. tone_event_in_playing = %d.\n",
                            tone_event_in_playing);
            app_send_msg(MSG_TYPE_TONE, TONE_MESSAGE_ID_CANCEL, NULL, 0);
        }
    }

    /* clear pending flag. */
    uint32_t tone_id;
    if (RET_OK == get_tone_id_by_event_id(event_id, &tone_id)
        && tone_play_pending_info.tone_play_cmd.tone_id == tone_id
        && tone_play_pending_info.is_valid) {
        tone_play_pending_info.is_valid = 0;
    }

    /* stop tone playing. */
    player_tone_stop(0);

    return RET_OK;
}

/*
 * @brief: play tone api. This api shall be in tone module.
 * @param local_tone_flag: 0 - play sync tone, 1 - play tone locally.
 * @param p_param: pointer to param of tone to play.
 * @param tone_start_ts: ts(sn) to mix tone, for tone mix case.
 */
static uint32_t app_tone_play_tone(uint32_t local_tone_flag, player_tone_param_t *p_param,
                                   uint32_t tone_start_ts, uint32_t force_sync_by_timing)
{
    uint32_t ret;
    uint32_t tone_stream_created = 0;
    if (p_param == NULL) {
        return RET_INVAL;
    }

    if ((!player_tone_stream_exist()) && (!app_is_streaming())) {
        /* no stream exist, create tone stream first,
         * to choose sync method correctly. */
        player_tone_create_stream();
        tone_stream_created = 1;
    }

    if (!local_tone_flag) {
        /* play sync tone. */
        ret = app_tone_start_sync(p_param->tone_id, force_sync_by_timing);
    }

    if (local_tone_flag || ret != RET_OK) {
        /* if it's local tone, or failed to play sync tone. */
        ret = audio_player_tone_start(p_param, tone_start_ts);
    }

    if (RET_OK != ret && tone_stream_created) {
        /* tone stream created, but failed to play tone. */
        player_tone_destroy_stream();
    }

    return ret;
}

static void do_play_tone(uint8_t tone_index)
{
    uint8_t ret = RET_FAIL;
    uint32_t local_play_flag;
    uint32_t tone_event_id;
    player_tone_param_t param;
    if (RET_OK != get_tone_param_by_index(tone_index, &param)) {
        /* invalid arg for playing. */
        goto exit_label;
    }

    if (param.spk == PLAYER_SPK_BOTH && app_wws_is_slave() && app_wws_is_connected()) {
        DBGLOG_TONE_DBG(
            "play tone index:%d, id:%d, spk_mode:%d, ignored for slave in wws connected\n",
            tone_index, param.tone_id, param.spk);
        goto exit_label;
    }

    if (RET_OK != get_tone_event_id_by_tone_index(tone_index, &tone_event_id)) {
        DBGLOG_TONE_DBG("play tone index:%d, failed to get event id.\n", tone_index);
        goto exit_label;
    }

    local_play_flag = (param.spk != PLAYER_SPK_BOTH);
    ret = app_tone_play_tone(local_play_flag, &param, 0,
                             app_tone_force_sync_by_timing(tone_event_id));
    if (RET_OK == ret) {
        tone_event_in_playing = tone_event_id;
    }

exit_label:
    DBGLOG_TONE_DBG("play tone index: %d, id:%d, spk_mode = %d, ret = %d.\n", tone_index,
                    param.tone_id, param.spk, ret);
    if (ret != RET_OK) {
        /* manually trigger tone end action if failed to play tone. */
        app_tone_action_end();
    }
}

bool app_tone_is_playing(void)
{
    return tone_event_in_playing != 0;
}

void app_tone_cancel_all(bool_t stop_playing)
{
    if (stop_playing && tone_event_in_playing) {
        player_tone_stop(0);
    }
    if (ro_feat_cfg()->queue_tone) {
        AFIFO_RESET(tone);
    }
}

uint32_t is_sec_playing_sync_tone_id(uint32_t tone_id)
{
    if (app_wws_is_master()) {
        /* PRI regards all tone as local tone.
         * if event id does not match, it's not the target tone.
         */
        return 0;
    }

    /* SEC check sync playing tone. */
    if (tone_event_in_playing != TONE_SYNC_REMOTE_EVENT_ID) {
        /* no sync tone playing. */
        return 0;
    }

    /* if playing tone id matches. */
    return (sync_tone_id_in_playing == tone_id);
}

static inline uint32_t is_target_tone_playing(uint16_t event_id)
{
    if (tone_event_in_playing == event_id) {
        /* current playing local tone match the event_id. */
        return 1;
    }

    if (app_wws_is_master()) {
        /* PRI regards all tone as local tone.
         * if event id does not match, it's not the target tone.
         */
        return 0;
    }

    /* SEC check sync playing tone. */
    if (tone_event_in_playing != TONE_SYNC_REMOTE_EVENT_ID) {
        /* no sync tone playing. */
        return 0;
    }

    uint32_t tone_id;
    if (RET_OK == get_tone_id_by_event_id(event_id, &tone_id)) {
        DBGLOG_TONE_DBG("is_target_tone_playing evt_id:%d. tone_id:%d, sync_tone_playing:%d\n",
                        event_id, tone_id, sync_tone_id_in_playing);
        /* if playing tone id match event id to be cancelled. */
        return (sync_tone_id_in_playing == tone_id);
    }

    /* failed to get tone id from event id.*/
    return 0;
}

void app_tone_cancel_internal(uint16_t event_id, bool_t stop_playing)
{
    const ro_cfg_tone_list_t *list = ro_cfg()->tone;
    uint8_t tone_index;

    if (stop_playing && is_target_tone_playing(event_id)) {
        app_tone_stop_sync(event_id);
    }

    if (!ro_feat_cfg()->queue_tone) {
        return;
    }

    if (AFIFO_IS_EMPTY(tone)) {
        return;
    }

    for (int i = 0; i < TONE_FIFO_SIZE; i++) {
        if (AFIFO_IS_EMPTY(tone)) {
            break;
        }

        tone_index = AFIFO_OUT(tone, uint8_t);
        if (list->tones[tone_index].event_id == event_id) {
            DBGLOG_TONE_DBG("app_tone_cancel remove %d in:%d out:%d\n", event_id, tone_fifo_in,
                            tone_fifo_out);
        } else {
            if (!AFIFO_IS_FULL(tone)) {
                AFIFO_IN(tone, tone_index);
            }
        }
    }

    DBGLOG_TONE_DBG("app_tone_cancel_internal done in:%d out:%d\n", tone_fifo_in, tone_fifo_out);
}

void app_tone_cancel(uint16_t event_id, bool_t stop_playing)
{
    uint32_t is_sync_tone = is_sync_tone_event_id(event_id);
    DBGLOG_TONE_DBG(
        "app_tone_cancel event_id = %d. tone_event_in_playing = %d. stop_playing = %d. (sync = %d, is_master = %d.)\n",
        event_id, tone_event_in_playing, stop_playing ? 1 : 0, is_sync_tone, app_wws_is_master());

    if (app_wws_is_slave() && is_sync_tone) {
        DBGLOG_TONE_DBG(
            "app_tone_cancel skip on slave for sync tone. evt_id = %d, stop_flag = %d.\n", event_id,
            stop_playing);
        return;
    }

    app_tone_cancel_internal(event_id, stop_playing);
}

uint32_t app_slave_cancel_sync_tone(uint32_t event_id, uint32_t stop_playing)
{
    uint32_t is_sync_tone = is_sync_tone_event_id(event_id);
    DBGLOG_TONE_DBG(
        "app_slave_cancel_sync_tone event_id = %d. tone_event_in_playing = %d. stop_playing = %d. (sync = %d, is_master = %d.)\n",
        event_id, tone_event_in_playing, stop_playing ? 1 : 0, is_sync_tone, app_wws_is_master());

    if (app_wws_is_master() || (!is_sync_tone)) {
        /* only for slave cancel sync tone. */
        DBGLOG_TONE_DBG("app_sync_tone_cancel skip. evt_id = %d, stop_flag = %d.\n", event_id,
                        stop_playing);
        return RET_INVAL;
    }

    app_tone_cancel_internal(event_id, stop_playing);
    return RET_OK;
}

static uint32_t app_tone_local_play_sync_tone(tone_msg_play_t *p_tone_msg_play)
{
    uint32_t ret = RET_FAIL;
    player_tone_param_t tone_param;
    if (RET_OK != get_tone_param_by_tone_id(p_tone_msg_play->tone_id, &tone_param)) {
        DBGLOG_TONE_ERR("err: failed to get param for tone id %d.\n", p_tone_msg_play->tone_id);
        tone_event_in_playing = 0;
        return ret;
    }

    uint32_t target_sn = 0;
    if (p_tone_msg_play->target_sn) {
        /* sync by sn. */
        target_sn = p_tone_msg_play->target_sn;
    } else {
        /* sync by time. */
        if (app_is_streaming()) {
            DBGLOG_TONE_ERR("streaming but NOT sync by sn.\n");
            tone_event_in_playing = 0;
            return ret;
        }
    }

    DBGLOG_TONE_ERR("start DSP. tone_id = %d, now_rtc: %luus."
                    " (target_sn = %ld, bts = %X%08X), playing_evt_id = %d. target_sn = %d.\n",
                    p_tone_msg_play->tone_id, iot_rtc_get_global_time_us(),
                    p_tone_msg_play->target_sn, (uint32_t)(p_tone_msg_play->start_bts >> 32),
                    (uint32_t)(p_tone_msg_play->start_bts & 0xFFFFFFFF), tone_event_in_playing,
                    target_sn);

    if (app_tone_is_playing()) {
        /* if stop playing tone, and start new tone,
         * it take tens of milliseconds and may leads to tone unsync.
         * to add these feature next step if necessary.
         */
        if (app_wws_is_slave() && tone_event_in_playing != TONE_SYNC_REMOTE_EVENT_ID) {
            /* tone is playing on SEC side, ignore sync tone. */
            DBGLOG_TONE_ERR("SEC playing event_id = %lu(0x%X). ignore sync tone.\n",
                            tone_event_in_playing, tone_event_in_playing);
            ret = RET_BUSY;
            return ret;
        }
    } else {
        if (app_wws_is_master()) {
            /* no tone is playing on PRI side, it means sync tone was cancelled. */
            DBGLOG_TONE_ERR("PRI playing event_id = %lu. cancelled.\n", tone_event_in_playing);
            tone_event_in_playing = 0;
            return ret;
        }
    }

    ret = app_tone_play_tone(1, &tone_param, target_sn, 0);
    if (RET_OK == ret) {
        if (app_wws_is_slave()) {
            /* update playing event id for SEC.
             * for PRI, this value is set when try to play sync tone.
             */
            sync_tone_id_in_playing = tone_param.tone_id;
            tone_event_in_playing = TONE_SYNC_REMOTE_EVENT_ID;
        }
    } else {
        tone_event_in_playing = 0;
    }

    return ret;
}

/*
 * @brief: get duration in us to send play msg.
 *    app shall send the msg at least TONE_PLAY_DEFAULT_DELAY_MS in advance,
 *    because RPC and dsp decoding takes some time.
 * @param is_sync_by_sn: 1 - sync by sn, 0 - sync by clock.
 *    if sync by sn, app can start dsp immediately.
 * @param p_target_rtc_us: pointer to buffer of target start rtc time.
 *    if it's a invalid rtc time, this api fix it to a time 200ms later.
 * @return: duration to delay before send play msg.
 */
static inline uint32_t app_tone_get_play_msg_delay_us(uint32_t is_sync_by_sn,
                                                      uint32_t *p_target_rtc_us)
{
    uint32_t rtc_now_us = 0;
    uint32_t delay_us = 0;
    uint32_t fixed_delay_us = 0;
    uint32_t target_rtc_us = *p_target_rtc_us;

    if (is_sync_by_sn) {
        fixed_delay_us = 0;
    } else {
        rtc_now_us = iot_rtc_get_global_time_us();
        delay_us = target_rtc_us - rtc_now_us;
        fixed_delay_us = delay_us;
        if (fixed_delay_us > PLAY_TONE_MAX_RESERVED_US) {
            /* invalid value, may be too large and msg may never be sent.
             * may be caused by bt clock switch between tws link and phone link.
             * fix to a value in valid range. may leads to slight tone unsync.
             */
            fixed_delay_us = TONE_PLAY_DEFAULT_DELAY_US;
            *p_target_rtc_us = rtc_now_us + TONE_PLAY_MAX_RPC_LATENCY_US;
        }

        if (fixed_delay_us > TONE_PLAY_MAX_RPC_LATENCY_US) {
            fixed_delay_us -= TONE_PLAY_MAX_RPC_LATENCY_US;
        } else {
            fixed_delay_us = 0;
        }
    }

    DBGLOG_TONE_ERR(
        "toneplay target time: margin: (%lu - %lu), fixed_target = %lu, delay_us: %lu, fixed_delay_us = %lu.\n",
        target_rtc_us, rtc_now_us, *p_target_rtc_us, delay_us, fixed_delay_us);

    return fixed_delay_us;
}

static void app_set_slave_playing_info(uint32_t tone_id)
{
    if (!app_wws_is_slave()) {
        return;
    }

    if (TONE_SYNC_TONE_ID_INVALID == tone_id) {
        tone_event_in_playing = 0;
        sync_tone_id_in_playing = TONE_SYNC_TONE_ID_INVALID;
    } else {
        /* only SEC set these flag. */
        tone_event_in_playing = TONE_SYNC_REMOTE_EVENT_ID;
        sync_tone_id_in_playing = tone_id;
    }
}

static uint32_t app_tone_play_request_handler(tone_msg_play_t *p_tone_msg_play)
{
    uint32_t ret = RET_OK;
    uint32_t target_sn = p_tone_msg_play->target_sn;
    uint32_t fixed_delay_us;

    /* workaround: create stream early to avoid music/voice stream. */
    if ((!player_tone_stream_exist()) && (!app_is_streaming())) {
        player_tone_create_stream();
    } else {
        uint32_t is_streaming = app_is_streaming();
        uint32_t local_target_sn = player_get_tone_start_ts(PLAY_TONE_SYNC_DELAY_US/1000);
        if (is_streaming && ((target_sn == 0) || (local_target_sn == 0))) {
            /* stream while NOT sync by sn, or local is not really playing stream
             * if local target sn is 0.
             */
            ret = RET_NOT_READY;
            if (tone_play_pending_info.is_valid == 0) {
                /* request to restart stream, so sync tone have chance to play. */
                tone_play_pending_info.is_valid = 1;
                tone_play_pending_info.tone_play_cmd = *p_tone_msg_play;
                ret = app_tone_restart_music_voice_stream();
                if (ret == RET_OK || ret == RET_BUSY) {
                    /* request restarting successfully, or a restrting operation is in progress.
                     * to play tone later when stream stopped.
                     */
                    app_set_slave_playing_info(p_tone_msg_play->tone_id);
                } else {
                    tone_play_pending_info.is_valid = 0;
                }
            }

            DBGLOG_TONE_ERR(
                "error: app_is_streaming = %d, target_sn = %lu, local_target_sn = %lu, valid = %d. ret = %d.\n",
                is_streaming, target_sn, local_target_sn, tone_play_pending_info.is_valid, ret);
            return ret;
        }

        if (app_wws_is_slave()) {
            DBGLOG_TONE_ERR(
                "[tone]mix: app_is_streaming = %d, target_sn = %lu, local_target_sn = %lu. tone_stream_exist: %d, app_streaming: %d.\n",
                is_streaming, target_sn, local_target_sn, player_tone_stream_exist(),
                app_is_streaming());
        }
    }

    app_set_slave_playing_info(p_tone_msg_play->tone_id);
    /* send msg for playing tone. */
    fixed_delay_us =
        app_tone_get_play_msg_delay_us(p_tone_msg_play->target_sn, &p_tone_msg_play->start_rtc_us);
    DBGLOG_TONE_ERR("post msg. fixed_delay_ms = %lu, start_rtc_us = %lu.\n", fixed_delay_us,
                    p_tone_msg_play->start_rtc_us);
    app_send_msg_delay(MSG_TYPE_TONE, TONE_MESSAGE_ID_PLAY, p_tone_msg_play,
                       sizeof(tone_msg_play_t), fixed_delay_us/1000);
    return ret;
}

static void app_tone_handle_msg(uint16_t msg_id, void *param)
{
    UNUSED(param);
    uint8_t tone_index;

    if (TONE_MESSAGE_ID_DONE == msg_id) {
        app_cancel_msg(MSG_TYPE_TONE, TONE_MESSAGE_ID_DONE);
        tone_event_in_playing = 0;
        app_set_slave_playing_info(TONE_SYNC_TONE_ID_INVALID);
        /* if queue is enabled and there event pending, play the next one */
        if (ro_feat_cfg()->queue_tone && !AFIFO_IS_EMPTY(tone)) {
            tone_index = AFIFO_OUT(tone, uint8_t);
            DBGLOG_TONE_DBG("tone out %d in:%d out:%d", tone_index, tone_fifo_in, tone_fifo_out);
            do_play_tone(tone_index);
        }
    } else if (TONE_MESSAGE_ID_PLAY == msg_id) {
        tone_msg_play_t *p_tone_msg_play = (tone_msg_play_t *)param;
#ifndef NEW_ARCH
        player_asrc_set_start_rtc_time(p_tone_msg_play->start_rtc_us,
                                       p_tone_msg_play->target_sn == 0);
#endif
        tone_play_pending_info.is_valid = 0;
        uint32_t result = app_tone_local_play_sync_tone(p_tone_msg_play);
        if (RET_OK != result && RET_BUSY != result) {
            player_tone_destroy_stream();
        }
    } else if (TONE_MESSAGE_ID_PLAY_REQUEST == msg_id) {
        /* tone play request from BT. */
        tone_msg_play_t *p_tone_msg_play = (tone_msg_play_t *)param;
        app_tone_play_request_handler(p_tone_msg_play);
    } else if (TONE_MESSAGE_ID_CANCEL == msg_id) {
        app_cancel_msg(MSG_TYPE_TONE, TONE_MESSAGE_ID_PLAY_REQUEST);
        app_cancel_msg(MSG_TYPE_TONE, TONE_MESSAGE_ID_PLAY);
        DBGLOG_TONE_ERR("cancel msg: TONE_MESSAGE_ID_PLAY.\n");
    } else if (TONE_MESSAGE_ID_SLV_SYNC_CANCEL == msg_id) {
        tone_msg_cancel_t *p_tone_cancel = (tone_msg_cancel_t *)param;
        app_slave_cancel_sync_tone(p_tone_cancel->event_id, p_tone_cancel->cancel_flag);
    }
}

void app_tone_action_end(void)
{
    app_send_msg(MSG_TYPE_TONE, TONE_MESSAGE_ID_DONE, NULL, 0);
}

uint32_t app_tone_send_play_msg(uint32_t tone_id, uint32_t target_sn, uint64_t start_bts,
                                uint32_t start_rtc_us)
{
    tone_msg_play_t tone_msg_play;
    tone_msg_play.tone_id = tone_id;
    tone_msg_play.target_sn = target_sn;
    tone_msg_play.start_rtc_us = start_rtc_us;
    tone_msg_play.start_bts = start_bts;

    app_send_msg(MSG_TYPE_TONE, TONE_MESSAGE_ID_PLAY_REQUEST, &tone_msg_play,
                 sizeof(tone_msg_play));
    return RET_OK;
}

uint32_t app_tone_send_cancel_msg(uint32_t tone_evt_id, uint32_t cancel_flag)
{
    tone_msg_cancel_t tone_msg_cancel;
    tone_msg_cancel.event_id = tone_evt_id;
    tone_msg_cancel.cancel_flag = cancel_flag;
    app_send_msg(MSG_TYPE_TONE, TONE_MESSAGE_ID_SLV_SYNC_CANCEL, &tone_msg_cancel,
                 sizeof(tone_msg_cancel));
    return RET_OK;
}

void app_tone_indicate_event(uint16_t event_id)
{
    if (app_bt_is_in_audio_test_mode() || (!tone_enable)) {
        return;
    }

    /*bypass the tone evnet of EVTUSR_ENTER_DUT_MODE in dut mode*/
    if (event_id != EVTUSR_ENTER_DUT_MODE && app_bt_is_in_dut_mode()) {
        return;
    }

    if ((tone_event_in_playing == TONE_SYNC_TONE_ID_INVALID) && is_target_tone_playing(event_id)) {
        DBGLOG_TONE_DBG("app_tone_indicate_event sync tone %d playing, ignored\n", event_id);
        return;
    }

    const ro_cfg_tone_list_t *list = ro_cfg()->tone;
    for (uint32_t i = 0; i < list->num; i++) {
        if (event_id == list->tones[i].event_id) {
            DBGLOG_TONE_DBG("app_tone_indicate_event:%d, playing_evt = %d.\n", event_id,
                            tone_event_in_playing);
            if (ro_feat_cfg()->queue_tone) {
                if (tone_event_in_playing == 0) {
                    do_play_tone(i);
                } else {
                    if (!AFIFO_IS_FULL(tone)) {
                        AFIFO_IN(tone, i);
                        DBGLOG_TONE_DBG("tone in %d in:%d out:%d", i, tone_fifo_in, tone_fifo_out);
                    } else {
                        DBGLOG_TONE_ERR("tone fifo full in:%d out:%d\n", tone_fifo_in,
                                        tone_fifo_out);
                    }
                }
            } else {
                if (tone_event_in_playing) {
                    tone_event_in_playing = 0;
                    player_tone_stop(0);
                }
                do_play_tone(i);
            }

            break; /*found a event tone,break out*/
        }
    }
}

void app_tone_enable(bool_t enable)
{
    tone_enable = enable;
}

void app_tone_init(void)
{
    sync_reg_start_tone_cb(app_tone_send_play_msg);
    sync_reg_cancel_tone_cb(app_tone_send_cancel_msg);
    app_register_msg_handler(MSG_TYPE_TONE, app_tone_handle_msg);
    player_set_language(usr_cfg_get_cur_language());
}

void app_tone_deinit(void)
{
    if (ro_feat_cfg()->queue_tone) {
        AFIFO_RESET(tone);
    }
}
