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

#ifdef NEW_ARCH
#include "nplayer_api.h"
#else
#include "player_api.h"
#endif

#include "app_audio.h"
#include "app_evt.h"
#include "app_main.h"
#include "app_tone.h"
#include "app_wws.h"
#include "usr_cfg.h"
#include "ro_cfg.h"
#include "audio_anc.h"
#include "app_econn.h"
#include "app_charger.h"
#include "audio_record.h"

#define APP_AUDIO_MSG_ID_SET_LISTEN_MODE          1
#define APP_AUDIO_MSG_ID_SET_ANC_LEVEL            2
#define APP_AUDIO_MSG_ID_SET_TRANSPARENCY_LEVEL   3
#define APP_AUDIO_MSG_ID_DENOISE_MODE_SWITCH_DONE 4

#define CALL_VOLUME_LEVEL_MUTE 0xFF

#ifndef APP_PLAYER_CTRL_ON_DTOP
#define APP_PLAYER_CTRL_ON_DTOP 0
#endif

static bool_t force_normal_mode = false;
static AUDIO_DENOISE_MODE target_denoise_mode = AUDIO_DENOISE_PHYSICAL;
static bool_t demnose_mode_handling = false;
static bool_t game_mode_enabled = false;

static void app_audio_handle_msg(uint16_t msg_id, void *param)
{
    switch (msg_id) {
        case APP_AUDIO_MSG_ID_SET_LISTEN_MODE:
            app_audio_listen_mode_set((listen_mode_t)(((uint8_t *)param)[0]));
            break;
        case APP_AUDIO_MSG_ID_SET_ANC_LEVEL:
            app_audio_anc_level_set((anc_level_t)(((uint8_t *)param)[0]));
            break;
        case APP_AUDIO_MSG_ID_SET_TRANSPARENCY_LEVEL:
            app_audio_transparency_level_set((transparency_level_t)(((uint8_t *)param)[0]));
            break;
        case APP_AUDIO_MSG_ID_DENOISE_MODE_SWITCH_DONE: {
            uint8_t ret;
            AUDIO_DENOISE_MODE current_mode = ((AUDIO_DENOISE_MODE *)param)[0];
            if (app_bt_is_in_audio_test_mode() || app_bt_is_in_dut_mode()) {
                break;
            }
            DBGLOG_AUDIO_DBG("denoise mode switch done, current:%d target:%d\n", current_mode,
                             target_denoise_mode);
            if (target_denoise_mode == current_mode) {
                demnose_mode_handling = false;
                break;
            }
            ret = player_anc_switch_mode(target_denoise_mode);
            if (ret != RET_OK) {
                demnose_mode_handling = false;
                DBGLOG_AUDIO_DBG("denoise mode switch done, set mode to %d failed, ret:%d\n",
                                 target_denoise_mode, ret);
            }
            break;
        }
        default:
            break;
    }
}

static void anc_switch_done_calblack(AUDIO_DENOISE_MODE mode)
{
    app_send_msg(MSG_TYPE_AUDIO, APP_AUDIO_MSG_ID_DENOISE_MODE_SWITCH_DONE, &mode, sizeof(mode));
}

static void denoise_mode_switch_request(AUDIO_DENOISE_MODE mode)
{
    uint8_t ret;

    target_denoise_mode = mode;

    if (demnose_mode_handling) {
        DBGLOG_AUDIO_DBG("denoise_mode_switch_request %d demnose_mode_handling\n", mode);
        return;
    }

    ret = player_anc_switch_mode(mode);

    if (RET_OK == ret) {
        demnose_mode_handling = true;
    } else {
        DBGLOG_AUDIO_ERR("player_anc_switch_mode error, ret:%d\n", ret);
    }
}

static void player_event_handler(player_event_t evtid, player_evt_param_t *p_param)
{

    if (evtid == EVT_TONE && p_param->tone.state == ST_PLAY_DONE) {
        app_tone_action_end();
    }
}

static void update_player_volume(audio_volume_type_t type, uint8_t volume_level)
{
    uint8_t stream = STREAM_MAX;
    int8_t db_value = 0;

    volume_level = app_econn_handle_volume(type, volume_level);

    switch (type) {
        case AUDIO_VOLUME_MUSIC:
            stream = STREAM_MUSIC;
            if (volume_level == 0) {
                db_value = PLAYER_VOLUME_MUTE_GAIN;
            } else {
                db_value = ro_vol_cfg()->volume_levels[volume_level / 8].music_gain;
            }
            break;
        case AUDIO_VOLUME_CALL:
            stream = STREAM_VOICE;
            if (volume_level == CALL_VOLUME_LEVEL_MUTE) {
                db_value = PLAYER_VOLUME_MUTE_GAIN;
            } else {
                db_value = ro_vol_cfg()->volume_levels[volume_level].call_gain;
            }
            break;
        case AUDIO_VOLUME_TONE:
            stream = STREAM_TONE;
            db_value = ro_vol_cfg()->volume_levels[volume_level].tone_gain;
            break;
        case AUDIO_VOLUME_MIC:
            break;
        default:
            break;
    }

    if (app_charger_is_charging()) {
        db_value = PLAYER_VOLUME_MUTE_GAIN;
    }

    DBGLOG_AUDIO_DBG("update_player_volume charging:%d type=%d level=%d db=%d\n",
                     app_charger_is_charging(), type, volume_level, db_value);
    player_adj_vol(stream, db_value);
}

static AUDIO_DENOISE_MODE get_current_audio_anc_mode(void)
{
    uint8_t level = usr_cfg_get_anc_level();

    switch (level) {
        case ANC_LEVEL_FLIGHT:
            return AUDIO_DENOISE_ANC_DEEP;
        case ANC_LEVEL_OUT_DOOR:
            return AUDIO_DENOISE_ANC_OUTDOOR;
        case ANC_LEVEL_IN_DOOR:
            return AUDIO_DENOISE_ANC_INDOOR;
        default:
            return AUDIO_DENOISE_ANC_DEEP;
    }
}

static AUDIO_DENOISE_MODE get_current_audio_transparency_mode(void)
{
    uint8_t level = usr_cfg_get_transparency_level();

    switch (level) {
        case TRANSPARENCY_LEVEL_FULL:
            return AUDIO_DENOISE_TRANSPARENT_FULL;
        case TRANSPARENCY_LEVEL_VOICE:
            return AUDIO_DENOISE_TRANSPARENT_VOICE;
        default:
            return AUDIO_DENOISE_TRANSPARENT_FULL;
    }
}

static void update_player_listen_mode(listen_mode_t mode)
{
    if (app_charger_is_charging()) {
        denoise_mode_switch_request(AUDIO_DENOISE_PHYSICAL);
        return;
    }

    switch (mode) {
        case LISTEN_MODE_UNKNOWN:
            denoise_mode_switch_request(AUDIO_DENOISE_PHYSICAL);
            break;
        case LISTEN_MODE_NORMAL:
            denoise_mode_switch_request(AUDIO_DENOISE_PHYSICAL);
            break;
        case LISTEN_MODE_ANC:
            denoise_mode_switch_request(get_current_audio_anc_mode());
            break;
        case LISTEN_MODE_TRANSPARENCY:
            denoise_mode_switch_request(get_current_audio_transparency_mode());
            break;
        default:
            denoise_mode_switch_request(AUDIO_DENOISE_PHYSICAL);
            break;
    }
}

void app_audio_init(void)
{
    int ret = 0;
    player_config_t config;
    int16_t mic_gain;

    app_register_msg_handler(MSG_TYPE_AUDIO, app_audio_handle_msg);

    config.channel = app_wws_is_left() ? PLAYER_CH_LEFT : PLAYER_CH_RIGHT;
    config.role = app_wws_is_master() ? PLAYER_ROLE_M : PLAYER_ROLE_A;
    config.tws_mode = PLAYER_TWS_MD_SINGLE;
    config.tone_vol_mode = ro_cfg()->tone_setting->tone_volume_mode;

#ifdef NEW_ARCH
    player_set_role(config.role);
    player_set_channel(config.channel);
    player_set_tws_mode(config.tws_mode);
    ret = player_register_event_callback(player_event_handler);
#else
    ret = player_initialization(&config, player_event_handler);
#endif

    if (ret) {
        DBGLOG_AUDIO_ERR("player_initialization ret:%d\n", ret);
        return;
    }

    audio_anc_coeff_switch_done_register(anc_switch_done_calblack);

    mic_gain = ro_vol_cfg()->default_mic_gain;

    audio_mic_set_default_gain(mic_gain * 16 / 3);

    update_player_volume(AUDIO_VOLUME_MUSIC, usr_cfg_get_music_vol());
    update_player_volume(AUDIO_VOLUME_CALL, usr_cfg_get_call_vol());
    update_player_volume(AUDIO_VOLUME_TONE, ro_vol_cfg()->default_tone_volume);
}

void app_audio_deinit(void)
{
    target_denoise_mode = AUDIO_DENOISE_PHYSICAL;
    player_anc_switch_mode(AUDIO_DENOISE_PHYSICAL);
}

void app_audio_volume_up(void)
{
    uint32_t sys_state;
    uint8_t volume_level = 0;
    sys_state = app_bt_get_sys_state();
    switch (sys_state) {
        case STATE_INCOMING_CALL:
        case STATE_OUTGOING_CALL:
        case STATE_ACTIVE_CALL:
        case STATE_TWC_CALL_WAITING:
        case STATE_TWC_CALL_ON_HELD:
            volume_level = usr_cfg_get_call_vol();
            if (volume_level >= CALL_VOLUME_LEVEL_MAX) {
                app_evt_send(EVTSYS_VOLUME_MAX);
                DBGLOG_AUDIO_DBG("volume was maxmal already !\n");
            } else {
                volume_level++;
                DBGLOG_AUDIO_DBG("call volume changed to %d\n", volume_level);
                usr_cfg_set_call_vol(volume_level);
                if (app_wws_is_connected_master()) {
                    app_wws_send_volume(usr_cfg_get_call_vol(), usr_cfg_get_music_vol());
                }
                update_player_volume(AUDIO_VOLUME_CALL, volume_level);
                app_wws_handle_volume_changed();
                app_bt_report_call_volume(volume_level);
                app_evt_send(EVTSYS_CALL_VOLUME_CHANGED);
                if (volume_level >= CALL_VOLUME_LEVEL_MAX) {
                    app_evt_send(EVTSYS_VOLUME_MAX);
                }
            }
            break;
        case STATE_CONNECTED:
        case STATE_A2DP_STREAMING:
            volume_level = usr_cfg_get_music_vol();
            if (volume_level >= MUSIC_VOLUME_LEVEL_MAX) {
                app_evt_send(EVTSYS_VOLUME_MAX);
                DBGLOG_AUDIO_DBG("volume was maxmal already !\n");
            } else {
                if (volume_level == 0) {
                    volume_level = 7;
                } else {
                    volume_level += 8;
                }
                if (volume_level >= MUSIC_VOLUME_LEVEL_MAX) {
                    volume_level = MUSIC_VOLUME_LEVEL_MAX;
                    app_evt_send(EVTSYS_VOLUME_MAX);
                }
                DBGLOG_AUDIO_DBG("music volume changed to %d\n", volume_level);
                usr_cfg_set_music_vol(volume_level);
                if (app_wws_is_connected_master()) {
                    app_wws_send_volume(usr_cfg_get_call_vol(), usr_cfg_get_music_vol());
                }
                update_player_volume(AUDIO_VOLUME_MUSIC, volume_level);
                app_wws_handle_volume_changed();
                app_bt_report_music_volume(volume_level);
                app_evt_send(EVTSYS_MUSIC_VOLUME_CHANGED);
            }
            break;
        default:
            DBGLOG_AUDIO_DBG("app_audio_volume_up state:0x%X not supported\n");
            break;
    }
}

void app_audio_volume_down(void)
{
    uint32_t sys_state;
    uint8_t volume_level = 0;
    sys_state = app_bt_get_sys_state();

    switch (sys_state) {
        case STATE_INCOMING_CALL:
        case STATE_OUTGOING_CALL:
        case STATE_ACTIVE_CALL:
        case STATE_TWC_CALL_WAITING:
        case STATE_TWC_CALL_ON_HELD:
            volume_level = usr_cfg_get_call_vol();
            if (volume_level == 0) {
                app_evt_send(EVTSYS_VOLUME_MIN);
                DBGLOG_AUDIO_DBG("volume was minimal already\n");
            } else {
                volume_level--;
                DBGLOG_AUDIO_DBG("call volume changed to %d\n", volume_level);
                usr_cfg_set_call_vol(volume_level);
                if (app_wws_is_connected_master()) {
                    app_wws_send_volume(usr_cfg_get_call_vol(), usr_cfg_get_music_vol());
                }
                update_player_volume(AUDIO_VOLUME_CALL, volume_level);
                app_wws_handle_volume_changed();
                app_bt_report_call_volume(volume_level);
                app_evt_send(EVTSYS_CALL_VOLUME_CHANGED);
                if (volume_level == 0) {
                    app_evt_send(EVTSYS_VOLUME_MIN);
                }
            }
            break;
        case STATE_CONNECTED:
        case STATE_A2DP_STREAMING:
            volume_level = usr_cfg_get_music_vol();
            if (volume_level == 0) {
                app_evt_send(EVTSYS_VOLUME_MIN);
                DBGLOG_AUDIO_DBG("volume was minimal already\n");
            } else {
                if (volume_level > 8) {
                    volume_level -= 8;
                } else {
                    volume_level = 0;
                    app_evt_send(EVTSYS_VOLUME_MIN);
                }
                DBGLOG_AUDIO_DBG("music volume changed to %d\n", volume_level);
                usr_cfg_set_music_vol(volume_level);
                if (app_wws_is_connected_master()) {
                    app_wws_send_volume(usr_cfg_get_call_vol(), usr_cfg_get_music_vol());
                }
                update_player_volume(AUDIO_VOLUME_MUSIC, volume_level);
                app_wws_handle_volume_changed();
                app_bt_report_music_volume(volume_level);
                app_evt_send(EVTSYS_MUSIC_VOLUME_CHANGED);
            }
            break;
        default:
            DBGLOG_AUDIO_DBG("app_audio_volume_down state:0x%X not supported\n");
            break;
    }
}

void app_audio_music_volume_set(uint8_t volume_level)
{
    if (volume_level > MUSIC_VOLUME_LEVEL_MAX) {
        DBGLOG_AUDIO_DBG("set volume>MUSIC_VOLUEM_LEVEL_MAX error, level:%d\n", volume_level);
        return;
    }

    update_player_volume(AUDIO_VOLUME_MUSIC, volume_level);

    if (volume_level != usr_cfg_get_music_vol()) {
        usr_cfg_set_music_vol(volume_level);
        app_wws_handle_volume_changed();
        app_evt_send(EVTSYS_MUSIC_VOLUME_CHANGED);
    }

    if (volume_level == MUSIC_VOLUME_LEVEL_MAX) {
        app_evt_send(EVTSYS_VOLUME_MAX);
    } else if (volume_level == 0) {
        app_evt_send(EVTSYS_VOLUME_MIN);
    }
}

int app_audio_music_start(uint32_t sample, player_codec_type_t codec_type)
{
    int ret = RET_OK;
#if APP_PLAYER_CTRL_ON_DTOP
    uint8_t volume = 0;
    player_music_param_t param;

    volume = usr_cfg_get_music_vol();
    param.codec.sf = sample;
    param.codec.type = codec_type;
    param.vol = volume;

    ret = player_music_start(&param);
    if (ret != RET_OK) {
        DBGLOG_AUDIO_ERR("player_music_start error !! ret=%d \n ", ret);
    }
#else
    UNUSED(sample);
    UNUSED(codec_type);
#endif
    return ret;
}

int app_audio_music_stop(void)
{
    int ret = RET_OK;
#if APP_PLAYER_CTRL_ON_DTOP
    ret = player_music_stop();
    if (ret != RET_OK) {
        DBGLOG_AUDIO_ERR("app_music_stop error !!ret=%d \n ", ret);
    }
#endif
    return ret;
}

void app_audio_call_volume_set(uint8_t volume_level)
{
    if (volume_level > 15) {
        DBGLOG_AUDIO_DBG("set volume >15 error volume_level:%d !\n", volume_level);
        return;
    }

    if (volume_level != usr_cfg_get_call_vol()) {
        usr_cfg_set_call_vol(volume_level);
        update_player_volume(AUDIO_VOLUME_CALL, volume_level);
        app_wws_handle_volume_changed();
        app_evt_send(EVTSYS_CALL_VOLUME_CHANGED);
    }

    if (volume_level == CALL_VOLUME_LEVEL_MAX) {
        app_evt_send(EVTSYS_VOLUME_MAX);
    } else if (volume_level == 0) {
        app_evt_send(EVTSYS_VOLUME_MIN);
    }
}

int app_audio_call_start(uint32_t sample, player_codec_type_t codec_type)
{
    int ret = RET_OK;
#if APP_PLAYER_CTRL_ON_DTOP
    uint8_t volume = 0;
    player_voice_call_param_t param;

    volume = usr_cfg_get_call_vol();
    param.codec.sf = sample;
    param.codec.type = codec_type;
    param.vol = volume;

    ret = player_voice_call_start(&param);
    if (ret != RET_OK) {
        DBGLOG_AUDIO_ERR("app_call_start error !!ret=%d \n ", ret);
    }
#else
    UNUSED(sample);
    UNUSED(codec_type);
#endif
    return ret;
}

int app_audio_call_stop(void)
{
    int ret = RET_OK;

#if APP_PLAYER_CTRL_ON_DTOP
    ret = player_voice_call_stop();
    if (ret != RET_OK) {
        DBGLOG_AUDIO_ERR("app_call_stop error !!ret=%d \n ", ret);
    }
#endif
    return ret;
}

static void listen_mode_change_handler(listen_mode_t mode, bool_t silently)
{
    if (app_bt_is_in_audio_test_mode() || app_bt_is_in_dut_mode()) {
        return;
    }

    if (usr_cfg_get_listen_mode() == mode) {
        DBGLOG_AUDIO_DBG("listen_mode_change_handler %d same as last, ignored\n", mode);
        return;
    }

    DBGLOG_AUDIO_DBG("listen mode changed to %d, silently:%d\n", mode, silently);
    usr_cfg_set_listen_mode(mode);

    if (app_wws_is_connected_master()) {
        app_wws_send_listen_mode(mode);
    }

    if (!silently) {
        switch (mode) {
            case LISTEN_MODE_UNKNOWN:
                app_evt_send(EVTSYS_LISTEN_MODE_NORMAL);
                break;
            case LISTEN_MODE_NORMAL:
                app_evt_send(EVTSYS_LISTEN_MODE_NORMAL);
                break;
            case LISTEN_MODE_ANC:
                app_evt_send(EVTSYS_LISTEN_MODE_ANC);
                break;
            case LISTEN_MODE_TRANSPARENCY:
                app_evt_send(EVTSYS_LISTEN_MODE_TRANSPARENT);
                break;
            default:
                app_evt_send(EVTSYS_LISTEN_MODE_NORMAL);
                break;
        }
    }

    update_player_listen_mode(mode);

    app_econn_handle_listen_mode_changed(mode);
}

listen_mode_t app_audio_listen_mode_get(void)
{
    return (listen_mode_t)usr_cfg_get_listen_mode();
}

void app_audio_listen_mode_set(listen_mode_t mode)
{
    listen_mode_change_handler(mode, false);
}

void app_audio_listen_mode_set_silently(listen_mode_t mode)
{
    listen_mode_change_handler(mode, true);
}

void app_audio_listen_mode_toggle(void)
{
    listen_mode_t mode = (listen_mode_t)usr_cfg_get_listen_mode();
    listen_mode_toggle_cfg_t cfg = (listen_mode_toggle_cfg_t)usr_cfg_get_listen_mode_cfg();

    switch (cfg) {
        case LISTEN_MODE_TOGGLE_NONE:
            break;
        case LISTEN_MODE_TOGGLE_NORMAL:
            if (mode != LISTEN_MODE_NORMAL) {
                mode = LISTEN_MODE_NORMAL;
            }
            break;
        case LISTEN_MODE_TOGGLE_ANC:
            if (mode != LISTEN_MODE_ANC) {
                mode = LISTEN_MODE_ANC;
            }
            break;
        case LISTEN_MODE_TOGGLE_NORMAL_ANC:
            if (mode == LISTEN_MODE_NORMAL) {
                mode = LISTEN_MODE_ANC;
            } else if (mode == LISTEN_MODE_ANC) {
                mode = LISTEN_MODE_NORMAL;
            } else {
                mode = LISTEN_MODE_NORMAL;
            }
            break;
        case LISTEN_MODE_TOGGLE_TRANSPARENCY:
            if (mode != LISTEN_MODE_TRANSPARENCY) {
                mode = LISTEN_MODE_TRANSPARENCY;
            }
            break;
        case LISTEN_MODE_TOGGLE_NORMAL_TRANSPARENCY:
            if (mode == LISTEN_MODE_NORMAL) {
                mode = LISTEN_MODE_TRANSPARENCY;
            } else if (mode == LISTEN_MODE_TRANSPARENCY) {
                mode = LISTEN_MODE_NORMAL;
            } else {
                mode = LISTEN_MODE_NORMAL;
            }
            break;
        case LISTEN_MODE_TOGGLE_ANC_TRANSPARENCY:
            if (mode == LISTEN_MODE_ANC) {
                mode = LISTEN_MODE_TRANSPARENCY;
            } else if (mode == LISTEN_MODE_TRANSPARENCY) {
                mode = LISTEN_MODE_ANC;
            } else {
                mode = LISTEN_MODE_ANC;
            }
            break;
        case LISTEN_MODE_TOGGLE_NORMAL_ANC_TRANSPARENCY:
            if (mode == LISTEN_MODE_NORMAL) {
                mode = LISTEN_MODE_ANC;
            } else if (mode == LISTEN_MODE_ANC) {
                mode = LISTEN_MODE_TRANSPARENCY;
            } else if (mode == LISTEN_MODE_TRANSPARENCY) {
                mode = LISTEN_MODE_NORMAL;
            } else {
                mode = LISTEN_MODE_NORMAL;
            }
            break;
        case LISTEN_MODE_TOGGLE_NORMAL_TRANSPARENCY_ANC:
            if (mode == LISTEN_MODE_NORMAL) {
                mode = LISTEN_MODE_TRANSPARENCY;
            } else if (mode == LISTEN_MODE_ANC) {
                mode = LISTEN_MODE_NORMAL;
            } else if (mode == LISTEN_MODE_TRANSPARENCY) {
                mode = LISTEN_MODE_ANC;
            } else {
                mode = LISTEN_MODE_NORMAL;
            }
            break;
        default:
            DBGLOG_AUDIO_DBG("unknown listen mode config %d\n", cfg);
            break;
    }
    listen_mode_change_handler(mode, false);
}

void app_audio_listen_mode_set_toggle_cfg(listen_mode_toggle_cfg_t cfg)
{
    usr_cfg_set_listen_mode_cfg(cfg);
}

listen_mode_toggle_cfg_t app_audio_listen_mode_get_toggle_cfg(void)
{
    return (listen_mode_toggle_cfg_t)usr_cfg_get_listen_mode_cfg();
}

void app_audio_listen_mode_force_normal(bool_t force_normal)
{
    if (app_bt_is_in_audio_test_mode() || app_bt_is_in_dut_mode()) {
        return;
    }

    force_normal_mode = force_normal;

    if (app_bt_get_sys_state() <= STATE_DISABLED) {
        return;
    }

    if (force_normal) {
        update_player_listen_mode(LISTEN_MODE_NORMAL);
        return;
    }

    if (!app_charger_is_charging()) {
        update_player_listen_mode(usr_cfg_get_listen_mode());
    }
}

void app_audio_anc_level_set(anc_level_t level)
{
    usr_cfg_set_anc_level(level);

    if (app_wws_is_connected_master()) {
        app_wws_send_anc_level((uint8_t)level);
    }

    if (usr_cfg_get_listen_mode() == LISTEN_MODE_ANC) {
        update_player_listen_mode(usr_cfg_get_listen_mode());
    }
}

void app_audio_transparency_level_set(transparency_level_t level)
{
    usr_cfg_set_transparency_level(level);

    if (app_wws_is_connected_master()) {
        app_wws_send_transparency_level((uint8_t)level);
    }

    if (usr_cfg_get_listen_mode() == LISTEN_MODE_TRANSPARENCY) {
        update_player_listen_mode(usr_cfg_get_listen_mode());
    }
}

void app_audio_handle_bt_power_on(void)
{
    if (app_bt_is_in_audio_test_mode() || app_bt_is_in_dut_mode()) {
        return;
    }

    if (app_charger_is_charging()) {
        DBGLOG_AUDIO_DBG("app_audio_handle_bt_power_on mute for charging\n");
        update_player_listen_mode(LISTEN_MODE_NORMAL);
        update_player_volume(AUDIO_VOLUME_MUSIC, 0);
        update_player_volume(AUDIO_VOLUME_CALL, CALL_VOLUME_LEVEL_MUTE);
        return;
    }

    if (!force_normal_mode) {
        update_player_listen_mode(usr_cfg_get_listen_mode());
    } else {
        update_player_listen_mode(LISTEN_MODE_NORMAL);
    }

    update_player_volume(AUDIO_VOLUME_MUSIC, usr_cfg_get_music_vol());
    update_player_volume(AUDIO_VOLUME_CALL, usr_cfg_get_call_vol());
}

void app_audio_handle_bt_power_off(void)
{
    if (app_bt_is_in_audio_test_mode() || app_bt_is_in_dut_mode()) {
        return;
    }

    update_player_listen_mode(LISTEN_MODE_NORMAL);
    update_player_volume(AUDIO_VOLUME_MUSIC, 0);
    update_player_volume(AUDIO_VOLUME_CALL, CALL_VOLUME_LEVEL_MUTE);
}

void app_audio_handle_charging_changed(bool_t charging)
{
    if (app_bt_is_in_audio_test_mode() || app_bt_is_in_dut_mode()) {
        return;
    }

    if (app_bt_get_sys_state() <= STATE_DISABLED) {
        DBGLOG_AUDIO_DBG("app_audio_handle_charging_changed mute for bt off\n");
        update_player_listen_mode(LISTEN_MODE_NORMAL);
        update_player_volume(AUDIO_VOLUME_MUSIC, 0);
        update_player_volume(AUDIO_VOLUME_CALL, CALL_VOLUME_LEVEL_MUTE);
        return;
    }

    if (charging) {
        update_player_listen_mode(LISTEN_MODE_NORMAL);
        update_player_volume(AUDIO_VOLUME_MUSIC, 0);
        update_player_volume(AUDIO_VOLUME_CALL, CALL_VOLUME_LEVEL_MUTE);
    } else {
        if (!force_normal_mode) {
            update_player_listen_mode(usr_cfg_get_listen_mode());
        }
        update_player_volume(AUDIO_VOLUME_MUSIC, usr_cfg_get_music_vol());
        update_player_volume(AUDIO_VOLUME_CALL, usr_cfg_get_call_vol());
    }
}

void app_audio_handle_sys_state(uint16_t new_state)
{
    static uint16_t prev_state = 0;

    if ((prev_state < STATE_CONNECTED) && (new_state >= STATE_CONNECTED)) {
        update_player_volume(AUDIO_VOLUME_MUSIC, usr_cfg_get_music_vol());
        update_player_volume(AUDIO_VOLUME_CALL, usr_cfg_get_call_vol());
        app_wws_handle_volume_changed();

        player_update_app_event(A_EVT_CONNECTED, NULL);
    } else if ((prev_state >= STATE_CONNECTED) && (new_state < STATE_CONNECTED)) {
        player_update_app_event(A_EVT_DISCONNECTED, NULL);
    }

    prev_state = new_state;
}

void app_audio_handle_wws_event(uint16_t new_event)
{
    uint8_t role = 0;

    if (new_event == EVTSYS_WWS_DISCONNECTED) {
        player_update_app_event(A_EVT_TWS_DISCON, NULL);
    } else if (new_event == EVTSYS_WWS_CONNECTED) {
        player_update_app_event(A_EVT_TWS_CON, NULL);
    } else if (new_event == EVTSYS_WWS_ROLE_SWITCH) {
        /*we could take with role value*/
        if (app_wws_is_left()) {
            role = PLAYER_ROLE_A;
        } else {
            role = PLAYER_ROLE_M;
        }
        player_update_app_event(A_EVT_ROLE_UPDATE, &role);
    }

    /*TODO:  */
    //   player_update_app_event(A_EVT_TWS_MD_UPDATE, NULL);
}

bool_t app_audio_game_mode_set_enabled(bool_t enabled)
{
    uint8_t ret = RET_OK;
    player_delay_mode_t mode;

    DBGLOG_AUDIO_DBG("player_delay_mode_set enabled=%d\n", enabled);

    if (enabled) {
        mode = PLAYER_DLY_MD_GAME_MUSIC;
    } else {
        mode = PLAYER_DLY_MD_DYNAMIC_MUSIC;
    }

    if (game_mode_enabled != enabled) {
        ret = player_delay_mode_set(mode);
        if (ret != RET_OK) {
            DBGLOG_AUDIO_ERR("player_delay_mode_set failed, ret=%d\n", ret);
            return false;
        }
    } else {
        DBGLOG_AUDIO_DBG("app_audio_game_mode_set_enabled same as prev enabled=%d\n", enabled);
    }

    if (app_wws_is_connected_master()) {
        app_wws_send_game_mode_set_enabled(enabled);
    }

    if (enabled) {
        app_evt_send(EVTSYS_GAME_MODE_ON);
    } else {
        app_evt_send(EVTSYS_GAME_MODE_OFF);
    }

    game_mode_enabled = enabled;

    return true;
}

bool_t app_audio_game_mode_toggle(void)
{
    if (game_mode_enabled) {
        return app_audio_game_mode_set_enabled(false);
    } else {
        return app_audio_game_mode_set_enabled(true);
    }
}

bool_t app_audio_game_mode_get(void)
{
    return game_mode_enabled;
}
