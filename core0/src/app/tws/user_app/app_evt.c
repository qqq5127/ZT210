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

#include "app_main.h"
#include "os_utils.h"
#include "app_evt.h"
#include "app_bt.h"
#include "app_pm.h"
#include "app_led.h"
#include "app_tone.h"
#include "app_btn.h"
#include "app_conn.h"
#include "app_audio.h"
#include "usr_cfg.h"
#include "app_btn.h"
#include "app_wws.h"
#include "ro_cfg.h"
#include "app_econn.h"
#include "app_wqota.h"
#include "app_cli.h"
#include "app_inear.h"
#include "ota_task.h"
#include "audio_anc.h"

#define PEER_EVENT_FLAG 0x8000

static inline bool_t is_force_local_event(uint16_t event)
{
    if ((event == EVTUSR_POWER_ON) || (event == EVTUSR_POWER_OFF)
        || (event == EVTUSR_START_WWS_PAIR) || (event == EVTUSR_FACTORY_RESET)
        || (event == EVTUSR_ENTER_FACTORY_TEST_MODE) || (event == EVTUSR_FACTORY_RESET_KEEP_WWS)
        || (event == EVTUSR_ENTER_DUT_MODE) || (event == EVTUSR_ENTER_AUDIO_TEST_MODE)
        || ((event >= EVTUSR_CUSTOM_1) && (event <= EVTUSR_CUSTOM_8))) {
        return true;
    } else {
        return false;
    }
}

static void handle_usr_evt(uint16_t event, bool_t from_peer)
{
    if (app_econn_handle_usr_evt(event, from_peer)) {
        DBGLOG_EVT_DBG("handle_usr_evt event:%d peer:%d handled by econn\n", event, from_peer);
        return;
    }

    if (app_wws_is_connected_slave() && (!from_peer) && (!is_force_local_event(event))) {
        app_wws_send_usr_evt(event);
        app_led_indicate_event(event);
        app_tone_indicate_event(event);
        return;
    }

    switch (event) {
        case EVTUSR_POWER_ON:
            DBGLOG_EVT_DBG("EVTUSR_POWER_ON\n");
            app_pm_power_on();
            if ((app_bt_get_sys_state() == STATE_AG_PAIRING) && (!app_bt_is_disabling())) {
                app_conn_connect_last();
            }
            break;
        case EVTUSR_POWER_OFF:
            DBGLOG_EVT_DBG("EVTUSR_POWER_OFF\n");
            if (app_pm_is_power_off_enabled()) {
                app_pm_power_off();
            } else {
                DBGLOG_EVT_DBG("app_pm_power_off, not enabled\n");
                return;
            }
            break;
        case EVTUSR_FACTORY_RESET:
            if (app_bt_get_sys_state() <= STATE_DISABLED) {
                DBGLOG_EVT_ERR("EVTUSR_FACTORY_RESET ignored when bt disabled\n");
                break;
            }
            DBGLOG_EVT_DBG("EVTUSR_FACTORY_RESET\n");
            app_bt_factory_reset(false);
            usr_cfg_reset();
            app_btn_cus_key_reset();
            app_inear_cfg_reset();
            break;
        case EVTUSR_CLEAR_PDL:
            DBGLOG_EVT_DBG("EVTUSR_CLEAR_PDL\n");
            usr_cfg_reset_pdl();
            app_bt_clear_pair_list();
            app_wws_send_reset_pdl();
            if (app_wws_is_master()) {
                app_bt_enter_ag_pairing();
            }
            break;
        case EVTUSR_START_WWS_PAIR:
            DBGLOG_EVT_DBG("EVTUSR_START_WWS_PAIR\n");
            app_wws_start_pair(app_econn_get_vid(), app_econn_get_pid(),
                               app_econn_get_tws_pair_magic());
            break;
        case EVTUSR_WWS_ROLE_SWITCH:
            DBGLOG_EVT_DBG("EVTUSR_WWS_ROLE_SWITCH\n");
            if (!ota_task_is_running()) {
                app_wws_role_switch();
            } else {
                DBGLOG_EVT_DBG("ignored when ota running\n");
            }
            break;
        case EVTUSR_ENTER_PAIRING:
            DBGLOG_EVT_DBG("EVTUSR_ENTER_PAIRING\n");
            if ((app_bt_get_sys_state() <= STATE_DISABLED) || app_bt_is_disabling()) {
                DBGLOG_EVT_DBG("ag pairing when disabled, ignored\n");
            } else {
                if (app_wws_is_slave() && !app_wws_is_connected()) {
                    DBGLOG_EVT_DBG("EVTUSR_ENTER_PAIRING role switch for slave\n");
                    app_wws_role_switch();
                    app_conn_set_role_change_action(APP_CONN_ROLE_CHANGE_ACTION_PAIRING);
                } else {
                    app_bt_enter_ag_pairing();
                }
            }
            break;
        case EVTUSR_CANCEL_PAIRING:
            DBGLOG_EVT_DBG("EVTUSR_CANCEL_PAIRING\n");
            break;
        case EVTUSR_CONNECT_LAST:
            DBGLOG_EVT_DBG("EVTUSR_CONNECT_LAST\n");
            app_conn_connect_last();
            break;
        case EVTUSR_VOICE_TRANSFER_TOGGLE:
            DBGLOG_EVT_DBG("EVTUSR_VOICE_TRANSFER_TOGGLE\n");
            app_bt_transfer_toggle();
            break;
        case EVTUSR_VOICE_RECOGNITION_TOGGLE:
            DBGLOG_EVT_DBG("EVTUSR_VOICE_RECOGNITION_TOGGLE\n");
            app_bt_toggle_voice_recognition();
            break;
        case EVTUSR_REDIAL_LAST:
            DBGLOG_EVT_DBG("EVTUSR_REDIAL_LAST\n");
            app_bt_redail();
            break;
        case EVTUSR_ANSWER:
            DBGLOG_EVT_DBG("EVTUSR_ANSWER\n");
            app_bt_answer();
            break;
        case EVTUSR_REJECT:
            DBGLOG_EVT_DBG("EVTUSR_REJECT\n");
            app_bt_reject();
            break;
        case EVTUSR_HANGUP:
            DBGLOG_EVT_DBG("EVTUSR_HANGUP\n");
            app_bt_hangup();
            break;
        case EVTUSR_THREE_WAY_REJECT_WAITING:
            app_bt_twc_reject_waiting();
            DBGLOG_EVT_DBG("EVTUSR_THREE_WAY_REJECT_WAITING\n");
            break;
        case EVTUSR_THREE_WAY_RELEASE_ACTIVE_ACCEPT_HELD_WAITING:
            app_bt_twc_release_active_accept_held_waiting();
            DBGLOG_EVT_DBG("EVTUSR_THREE_WAY_RELEASE_ACTIVE_ACCEPT_HELD_WAITING\n");
            break;
        case EVTUSR_THREE_WAY_HOLD_ACTIVE_ACCEPT_HELD_WAITING:
            app_bt_twc_hold_active_accept_held_waiting();
            DBGLOG_EVT_DBG("EVTUSR_THREE_WAY_HOLD_ACTIVE_ACCEPT_HELD_WAITING\n");
            break;
        case EVTUSR_AUDIO_PROMPT_LANGUAGE_TOGGLE:
            DBGLOG_EVT_DBG("EVTUSR_TOGGLE_AUDIO_PROMPT_LANGUAGE\n");
            break;
        case EVTUSR_MUSIC_PLAY_PAUSE:
            DBGLOG_EVT_DBG("EVTUSR_MUSIC_PLAY_PAUSE\n");
            app_bt_play_pause();
            break;
        case EVTUSR_MUSIC_PLAY:
            DBGLOG_EVT_DBG("EVTUSR_MUSIC_PLAY\n");
            app_bt_play();
            break;
        case EVTUSR_MUSIC_PAUSE:
            DBGLOG_EVT_DBG("EVTUSR_MUSIC_PAUSE\n");
            app_bt_pause();
            break;
        case EVTUSR_MUSIC_STOP:
            DBGLOG_EVT_DBG("EVTUSR_MUSIC_STOP\n");
            app_bt_pause();
            break;
        case EVTUSR_MUSIC_FORWARD:
            DBGLOG_EVT_DBG("EVTUSR_MUSIC_FORWARD\n");
            app_bt_forward();
            break;
        case EVTUSR_MUSIC_BACKWARD:
            DBGLOG_EVT_DBG("EVTUSR_MUSIC_BACKWARD\n");
            app_bt_backward();
            break;
        case EVTUSR_VOLUME_UP:
            DBGLOG_EVT_DBG("EVTUSR_VOLUME_UP\n");
            app_audio_volume_up();
            break;
        case EVTUSR_VOLUME_DOWN:
            DBGLOG_EVT_DBG("EVTUSR_VOLUME_DOWN\n");
            app_audio_volume_down();
            break;
        case EVTUSR_MIC_MUTE:
            DBGLOG_EVT_DBG("EVTUSR_MIC_MUTE\n");
            break;
        case EVTUSR_MIC_UNMUTE:
            DBGLOG_EVT_DBG("EVTUSR_MIC_UNMUTE\n");
            break;
        case EVTUSR_MIC_MUTE_TOGGLE:
            DBGLOG_EVT_DBG("EVTUSR_MIC_MUTE_TOGGLE\n");
            break;
        case EVTUSR_SPK_MUTE:
            DBGLOG_EVT_DBG("EVTUSR_SPK_MUTE\n");
            break;
        case EVTUSR_SPK_UNMUTE:
            DBGLOG_EVT_DBG("EVTUSR_SPK_UNMUTE\n");
            break;
        case EVTUSR_SPK_MUTE_TOGGLE:
            DBGLOG_EVT_DBG("EVTUSR_SPK_MUTE_TOGGLE\n");
            break;
        case EVTUSR_USER_EQ_ON:
            DBGLOG_EVT_DBG("EVTUSR_USER_EQ_ON\n");
            break;
        case EVTUSR_USER_EQ_OFF:
            DBGLOG_EVT_DBG("EVTUSR_USER_EQ_OFF\n");
            break;
        case EVTUSR_USER_EQ_TOGGLE:
            DBGLOG_EVT_DBG("EVTUSR_USER_EQ_TOGGLE\n");
            break;
        case EVTUSR_USER_INEAR_ENABLE_TOGGLE:
            DBGLOG_EVT_DBG("EVTUSR_USER_INEAR_ENABLE_TOGGLE\n");
            app_inear_enable_set_toggle();
            break;
        case EVTUSR_LISTEN_MODE_TOGGLE:
            DBGLOG_EVT_DBG("EVTUSR_ANC_MODE_TOGGLE\n");
            app_audio_listen_mode_toggle();
            break;
        case EVTUSR_ENTER_FACTORY_TEST_MODE:
            DBGLOG_EVT_DBG("EVTUSR_ENTER_FACTORY_TEST_MODE\n");
            app_tone_cancel_all(true);
            app_bt_enter_ft_mode();
            break;
        case EVTUSR_ENTER_OTA_MODE:
            DBGLOG_EVT_DBG("EVTUSR_ENTER_OTA_MODE\n");
            app_wqota_start_adv();
            break;
        case EVTUSR_ENTER_AUDIO_TEST_MODE:
            DBGLOG_EVT_DBG("EVTUSR_ENTER_AUDIO_TEST_MODE\n");
            app_tone_cancel_all(true);
            audio_anc_howlround_enable(false);
            app_bt_enter_audio_test_mode(true);
            break;
        case EVTUSR_GAME_MODE_TOGGLE:
            DBGLOG_EVT_DBG("EVTUSR_GAME_MODE_TOGGLE\n");
            app_audio_game_mode_toggle();
            break;
        case EVTUSR_FACTORY_RESET_KEEP_WWS:
            DBGLOG_EVT_DBG("EVTUSR_FACTORY_RESET_KEEP_WWS\n");
            app_bt_factory_reset(true);
            usr_cfg_reset();
            app_btn_cus_key_reset();
            app_inear_cfg_reset();
            if (!from_peer) {
                app_wws_send_usr_evt(EVTUSR_FACTORY_RESET_KEEP_WWS);
            }
            break;
        case EVTUSR_ENTER_DUT_MODE:
            DBGLOG_EVT_DBG("EVTUSR_ENTER_DUT_MODE\n");
            app_bt_enter_dut_mode();
            break;
        case EVTUSR_VOICE_RECOGNITION_ACTIVATE:
            DBGLOG_EVT_DBG("EVTUSR_VOICE_RECOGNITION_ACTIVATE\n");
            app_bt_activate_voice_recognition();
            break;
        case EVTUSR_VOICE_RECOGNITION_DEACTIVATE:
            DBGLOG_EVT_DBG("EVTUSR_VOICE_RECOGNITION_DEACTIVATE\n");
            app_bt_deactivate_voice_recognition();
            break;
        default:
            break;
    }

    if (!from_peer) {
        app_led_indicate_event(event);
        app_tone_indicate_event(event);
    }
}

static void handle_sys_evt(uint16_t event, void *param)
{
    if (app_econn_handle_sys_evt(event, param)) {
        DBGLOG_EVT_DBG("handle_sys_evt event:%d handled by econn\n", event);
        return;
    }

    switch (event) {
        case EVTSYS_BT_ON_COMPLETE:
            DBGLOG_EVT_DBG("EVTSYS_BT_ON_COMPLETE\n");
            break;
        case EVTSYS_STATE_CHANGED: {
            uint32_t state;
            if (param) {
                state = *((uint32_t *)param);
            } else {
                state = app_bt_get_sys_state();
            }
            DBGLOG_EVT_DBG("EVTSYS_STATE_CHANGED 0x%x\n", state);
            app_led_indicate_state(state);
            app_audio_handle_sys_state(state);
            app_cli_handle_sys_state(state);
            app_econn_handle_sys_state(state);
            break;
        }
        case EVTSYS_CONNECTED:
            DBGLOG_EVT_DBG("EVTSYS_CONNECTED\n");
            break;
        case EVTSYS_DISCONNECTED:
            DBGLOG_EVT_DBG("EVTSYS_DISCONNECTED\n");
            break;
        case EVTSYS_LINK_LOSS:
            DBGLOG_EVT_DBG("EVTSYS_LINK_LOSS\n");
            break;
        case EVTSYS_SCOLINK_OPEN:
            DBGLOG_EVT_DBG("EVTSYS_SCOLINK_OPEN\n");
            break;
        case EVTSYS_SCOLINK_CLOSE:
            DBGLOG_EVT_DBG("EVTSYS_SCOLINK_CLOSE\n");
            break;
        case EVTSYS_VOICE_RECOGNITION_START:
            DBGLOG_EVT_DBG("EVTSYS_VOICE_RECOGNITION_START\n");
            break;
        case EVTSYS_VOICE_RECOGNITION_STOP:
            DBGLOG_EVT_DBG("EVTSYS_VOICE_RECOGNITION_STOP\n");
            break;
        case EVTSYS_RING:
            DBGLOG_EVT_DBG("EVTSYS_RING\n");
            break;
        case EVTSYS_CALL_END:
            DBGLOG_EVT_DBG("EVTSYS_CALL_END\n");
            break;
        case EVTSYS_WWS_CONNECTED:
            DBGLOG_EVT_DBG("EVTSYS_WWS_CONNECTED\n");
            app_audio_handle_wws_event(event);
            break;
        case EVTSYS_WWS_DISCONNECTED:
            DBGLOG_EVT_DBG("EVTSYS_WWS_DISCONNECTED\n");
            app_audio_handle_wws_event(event);
            break;
        case EVTSYS_WWS_ROLE_SWITCH:
            DBGLOG_EVT_DBG("EVTSYS_WWS_ROLE_SWITCH\n");
            app_audio_handle_wws_event(event);
            break;
        case EVTSYS_CHARGE_COMPLETE:
            DBGLOG_EVT_DBG("EVTSYS_CHARGE_COMPLETE\n");
            break;
        case EVTSYS_CHARGE_CONNECTED:
            DBGLOG_EVT_DBG("EVTSYS_CHARGE_CONNECTED\n");
            app_audio_handle_charging_changed(true);
            break;
        case EVTSYS_CHARGE_DISCONNECTED:
            DBGLOG_EVT_DBG("EVTSYS_CHARGE_DISCONNECTED\n");
            app_audio_handle_charging_changed(false);
            break;
        case EVTSYS_BATTERY_LOW:
            DBGLOG_EVT_DBG("EVTSYS_BATTERY_LOW\n");
            break;
        case EVTSYS_BATTERY_LEVEL_CHANGED:
            DBGLOG_EVT_DBG("EVTSYS_BATTERY_LEVEL_CHANGED\n");
            break;
        case EVTSYS_VOLUME_MAX:
            DBGLOG_EVT_DBG("EVTSYS_VOLUME_MAX\n");
            break;
        case EVTSYS_VOLUME_MIN:
            DBGLOG_EVT_DBG("EVTSYS_VOLUME_MIN\n");
            break;
        case EVTSYS_MUSIC_VOLUME_CHANGED:
            DBGLOG_EVT_DBG("EVTSYS_MUSIC_VOLUME_CHANGED\n");
            break;
        case EVTSYS_CALL_VOLUME_CHANGED:
            DBGLOG_EVT_DBG("EVTSYS_CALL_VOLUME_CHANGED\n");
            break;
        case EVTSYS_CMC_BOX_OPEN:
            DBGLOG_EVT_DBG("EVTSYS_CMC_BOX_OPEN\n");
            break;
        case EVTSYS_CMC_BOX_CLOSE:
            DBGLOG_EVT_DBG("EVTSYS_CMC_BOX_CLOSE\n");
            break;
        case EVTSYS_CMC_LONG_3S:
            DBGLOG_EVT_DBG("EVTSYS_CMC_LONG_3S\n");
            break;
        case EVTSYS_CMC_LONG_10S:
            DBGLOG_EVT_DBG("EVTSYS_CMC_LONG_10S\n");
            break;
        case EVTSYS_CMC_LONG_20S:
            DBGLOG_EVT_DBG("EVTSYS_CMC_LONG_20S\n");
            break;
        case EVTSYS_WWS_PAIRED:
            DBGLOG_EVT_DBG("EVTSYS_WWS_PAIRED\n");
            break;
        case EVTSYS_WWS_PAIR_FAILED:
            DBGLOG_EVT_DBG("EVTSYS_WWS_PAIR_FAILED\n");
            break;
        case EVTSYS_IN_EAR:
            DBGLOG_EVT_DBG("EVTSYS_IN_EAR\n");
            break;
        case EVTSYS_OUT_OF_EAR:
            DBGLOG_EVT_DBG("EVTSYS_OUT_OF_EAR\n");
            break;
        case EVTSYS_LISTEN_MODE_NORMAL:
            DBGLOG_EVT_DBG("EVTSYS_LISTEN_MODE_NORMAL\n");
            break;
        case EVTSYS_LISTEN_MODE_ANC:
            DBGLOG_EVT_DBG("EVTSYS_LISTEN_MODE_ANC\n");
            break;
        case EVTSYS_LISTEN_MODE_TRANSPARENT:
            DBGLOG_EVT_DBG("EVTSYS_LISTEN_MODE_TRANSPARENT\n");
            break;
        case EVTSYS_BT_POWER_ON:
            DBGLOG_EVT_DBG("EVTSYS_BT_POWER_ON\n");
            break;
        case EVTSYS_BT_POWER_OFF:
            DBGLOG_EVT_DBG("EVTSYS_BT_POWER_OFF\n");
            break;
        case EVTSYS_ENTER_PAIRING:
            DBGLOG_EVT_DBG("EVTSYS_ENTER_PAIRING\n");
            app_econn_enter_ag_pairing();
            break;
        case EVTSYS_INEAR_ENABLE:
            DBGLOG_EVT_DBG("EVTSYS_INEAR_ENABLE\n");
            break;
        case EVTSYS_INEAR_DISABLE:
            DBGLOG_EVT_DBG("EVTSYS_INEAR_DISABLE\n");
            break;
        case EVTSYS_FACTORY_RESET_DONE:
            DBGLOG_EVT_DBG("EVTSYS_FACTORY_RESET_DONE\n");
            app_pm_reboot(PM_REBOOT_REASON_FACTORY_RESET);
            break;
        default:
            break;
    }

    app_led_indicate_event(event);
    app_tone_indicate_event(event);
}

static void app_evt_handle_msg(uint16_t msg_id, void *param)
{
    uint16_t event = msg_id;
    bool_t from_peer = false;

    if (event & PEER_EVENT_FLAG) {
        from_peer = true;
        event = event & (~PEER_EVENT_FLAG);
    }

    if ((event >= EVTUSR_BASE) && (event <= EVTUSR_LAST)) {
        handle_usr_evt(event, from_peer);
    } else if ((event >= EVTSYS_BASE) && (event <= EVTSYS_LAST)) {
        handle_sys_evt(event, param);
    }
}

void app_evt_init(void)
{
    DBGLOG_EVT_DBG("app_evt_init\n");
    app_register_msg_handler(MSG_TYPE_EVT, app_evt_handle_msg);
}

void app_evt_deinit(void)
{
}

void app_evt_send(uint16_t evt)
{
    app_send_msg(MSG_TYPE_EVT, evt, NULL, 0);
}

void app_evt_send_with_param(uint16_t evt, void *param, uint16_t param_len)
{
    app_send_msg(MSG_TYPE_EVT, evt, param, param_len);
}

void app_evt_send_delay(uint16_t evt, uint32_t delay_ms)
{
    app_send_msg_delay(MSG_TYPE_EVT, evt, NULL, 0, delay_ms);
}

void app_evt_cancel(uint16_t evt)
{
    app_cancel_msg(MSG_TYPE_EVT, evt);
}

void app_evt_handle_peer_evt(uint16_t evt)
{
    app_send_msg(MSG_TYPE_EVT, evt | PEER_EVENT_FLAG, NULL, 0);
}
