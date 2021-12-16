#define WQ_DEMO_UI 1

#pragma GCC diagnostic ignored "-Wundef"

#if ECONN == WQ_DEMO_UI
#include "os_mem.h"
#include "assert.h"
#include "app_econn.h"
#include "app_main.h"
#include "app_gatts.h"
#include "app_pm.h"
#include "app_adv.h"
#include "app_spp.h"
#include "app_bat.h"
#include "app_wws.h"
#include "app_btn.h"
#include "app_evt.h"
#include "app_audio.h"
#include "app_charger.h"
#include "usr_cfg.h"
#include "ota_task.h"
#include "app_tone.h"
#include "app_led.h"
#include "battery_charger.h"
#include "app_ota_sync.h"
#include "key_wq_touch.h"
#include "app_conn.h"
#include "player_api.h"

#include "app_econn_wqdemo.h"
#include "app_user_flash.h"
#include "app_user_spp.h"
#include "aw8686x.h"
#include "iot_gpio.h"
#include "app_user_battery.h"

#define AUTO_SWITCH_ANC_MODE_BOTH_OUT_OF_BOX 0

#define MSG_ID_START_APP_ECONN_ADV_SLOW          1
#define REMOTE_MSGID_RESET_AUTO_PWROFF_TIME      2
#define MSG_ID_CANCAL_CHARGER_COMPLETE_STATE_LED 3
#define ECONN_MSG_ID_FORCE_DISCOVERABLE          4
#define ECONN_MSG_ID_GET_CHARGE_MODE             5
#define ECONN_MSG_ID_HANGUP_DEALY                6
#define ECONN_MSG_ID_PAUSE_TONE_DELY_PLAY        7

#define ECONN_MSG_ID_AW86862_INT        				100
#define ECONN_MSG_ID_AW86862_TIMER        			101

#define PAUSE_TONE_DELY_TIMEOUT              200   //MS
#define GET_CHARGE_MODE_INTERVAL_MS          5000
#define AUTO_POWER_OFF_TIMEOUT_WWS_PAIR_FAIL 300
#define CHARGE_COMPLETE_LED_TIMEOUT_MS       60000
#define HANGUP_DEALY_TIMEOUT_MS              500

static bool_t force_discoverable = false;
static uint8_t battery_is_full = 0;
static bool_t link_loss = false;

static void app_handle_inear_action_for_android(bool_t in_ear)
{
    UNUSED(in_ear);
}

static void econn_msg_handler(uint16_t msg_id, void *param)
{
    UNUSED(param);
    switch (msg_id) {
        case REMOTE_MSGID_RESET_AUTO_PWROFF_TIME:
            DBGLOG_ECONN_DBG("[lsEconn] REMOTE_MSGID_RESET_AUTO_PWROFF_TIME\n");
            if (app_bt_get_sys_state() <= STATE_AG_PAIRING) {
                app_pm_handle_bt_disconnected();
            }
            break;

        case MSG_ID_CANCAL_CHARGER_COMPLETE_STATE_LED:
            app_led_set_enabled(false);
            app_led_set_custom_state(false);
            break;

        case ECONN_MSG_ID_FORCE_DISCOVERABLE:
            force_discoverable = true;
            DBGLOG_ECONN_DBG("ECONN_MSG_ID_FORCE_DISCOVERABLE\n");
            if (app_wws_is_master()) {
                app_bt_set_force_discoverable(true);
                app_tone_indicate_event(EVTSYS_ENTER_PAIRING);
            }
            break;
        case ECONN_MSG_ID_GET_CHARGE_MODE:
            if (battery_charger_get_mode() == BAT_CHARGER_MODE_FULL && !battery_is_full) {
                DBGLOG_ECONN_DBG("[lsEconn] BAT_CHARGER_MODE_FULL");
                app_led_set_custom_state(STATE_CUSTOM_2);
                app_cancel_msg(MSG_TYPE_ECONN, ECONN_MSG_ID_GET_CHARGE_MODE);
                app_cancel_msg(MSG_TYPE_ECONN, MSG_ID_CANCAL_CHARGER_COMPLETE_STATE_LED);
                app_send_msg_delay(MSG_TYPE_ECONN, MSG_ID_CANCAL_CHARGER_COMPLETE_STATE_LED, NULL,
                                   0, CHARGE_COMPLETE_LED_TIMEOUT_MS);
                battery_is_full = 1;
            } else if (!battery_is_full) {
                app_cancel_msg(MSG_TYPE_ECONN, ECONN_MSG_ID_GET_CHARGE_MODE);
                app_send_msg_delay(MSG_TYPE_ECONN, ECONN_MSG_ID_GET_CHARGE_MODE, NULL, 0,
                                   GET_CHARGE_MODE_INTERVAL_MS);
            }
            break;

        case ECONN_MSG_ID_HANGUP_DEALY:
            DBGLOG_EVT_DBG("[lsEconn] EVTUSR_HANGUP\n");
            app_bt_hangup();
            break;

        case ECONN_MSG_ID_PAUSE_TONE_DELY_PLAY:
            DBGLOG_EVT_DBG("[lsEconn] dely play playpause tone\n");
            app_tone_indicate_event(EVTUSR_CUSTOM_4);
            break;

				case ECONN_MSG_ID_AW86862_INT:
				case ECONN_MSG_ID_AW86862_TIMER:
#if KEY_DRIVER_SELECTION == KEY_DRIVER_AW8686X
						aw8686x_thread0_cb((uint8_t )msg_id);
#endif
						break;
        default:
            break;
    }
}

void app_econn_init(void)
{
	// user add cuixu
	app_user_battery_init();
	app_user_read_data();
	app_user_spp_init();

  app_register_msg_handler(MSG_TYPE_ECONN, econn_msg_handler);
}

void app_econn_deinit(void)
{
}
bool_t app_econn_exists(void)
{
    return false;
}
void app_econn_enter_ag_pairing(void)
{
}
void app_econn_handle_sys_state(uint32_t sys_state)
{
    UNUSED(sys_state);
}
void app_econn_handle_listen_mode_changed(uint8_t mode)
{
    UNUSED(mode);
}
void app_econn_handle_in_ear_changed(bool_t in_ear)
{
    app_handle_inear_action_for_android(in_ear);
}
void app_econn_handle_peer_in_ear_changed(bool_t in_ear)
{
    app_handle_inear_action_for_android(in_ear);
}
void app_econn_handle_battery_level_changed(uint8_t level)
{
    UNUSED(level);
}
void app_econn_handle_peer_battery_level_changed(uint8_t level)
{
    UNUSED(level);
}
void app_econn_handle_tws_state_changed(bool_t connected)
{
    UNUSED(connected);

#if AUTO_SWITCH_ANC_MODE_BOTH_OUT_OF_BOX
    if (connected) {
        if (!app_charger_is_charging() && !app_wws_peer_is_charging()) {
            usr_cfg_set_listen_mode(LISTEN_MODE_ANC);
            app_audio_listen_mode_force_normal(false);
        }
    } else {
        usr_cfg_set_listen_mode(LISTEN_MODE_NORMAL);
        app_audio_listen_mode_force_normal(false);
    }
#endif
}

bool_t app_econn_handle_usr_evt(uint16_t event, bool_t from_peer)
{
    bool_t ret = false;

    switch (event) {
        case EVTUSR_CUSTOM_1:
            if (!app_wws_is_connected() && app_charger_is_charging()) {
                app_evt_send(EVTUSR_ENTER_PAIRING);
            } else if (app_wws_is_connected() && app_charger_is_charging()
                       && app_wws_peer_is_charging()) {
                app_evt_send(EVTUSR_ENTER_PAIRING);
            }
            ret = true;
            break;
        case EVTUSR_LISTEN_MODE_TOGGLE:
            if (!app_wws_is_connected()) {
                //force start pairing if outdoor power on hold 4s
                if (!app_pm_is_power_off_enabled() && !app_charger_is_charging()) {
                    app_evt_send(EVTUSR_ENTER_PAIRING);
                }
                ret = true;
            } else if (app_charger_is_charging() || app_wws_peer_is_charging()) {
                ret = true;
            }
            break;
        case EVTUSR_FACTORY_RESET:
            if (app_charger_is_charging())
                ret = false;
            else {
                ret = true;
            }
            break;
        case EVTUSR_MUSIC_PLAY_PAUSE:
            if ((from_peer) && !app_wws_peer_is_charging()) {
                app_send_msg_delay(MSG_TYPE_ECONN, ECONN_MSG_ID_PAUSE_TONE_DELY_PLAY, NULL, 0,
                                   PAUSE_TONE_DELY_TIMEOUT);
            } else if (!from_peer && !app_charger_is_charging()) {
                app_send_msg_delay(MSG_TYPE_ECONN, ECONN_MSG_ID_PAUSE_TONE_DELY_PLAY, NULL, 0,
                                   PAUSE_TONE_DELY_TIMEOUT);
            } else {
                DBGLOG_ECONN_DBG("ignore USRKEY when in box\n");
                ret = true;
            }
            break;
        case EVTUSR_MUSIC_FORWARD:
        case EVTUSR_MUSIC_BACKWARD:
        case EVTUSR_ANSWER:
        case EVTUSR_THREE_WAY_RELEASE_ACTIVE_ACCEPT_HELD_WAITING:
        case EVTUSR_THREE_WAY_HOLD_ACTIVE_ACCEPT_HELD_WAITING:
            if ((from_peer) && !app_wws_peer_is_charging()) {
                app_tone_indicate_event(event);
                ret = false;
            } else if (!from_peer && !app_charger_is_charging()) {
                ret = false;
            } else {
                DBGLOG_ECONN_DBG("ignore USRKEY when in box\n");
                ret = true;
            }
            break;

        case EVTUSR_HANGUP:
            if (app_wws_is_connected_slave()) {
                app_wws_send_usr_evt(event);
                ret = true;
                break;
            }

            if (app_wws_is_master()) {
                if ((from_peer) && !app_wws_peer_is_charging()) {
                    app_tone_indicate_event(event);
                    app_cancel_msg(MSG_TYPE_ECONN, ECONN_MSG_ID_HANGUP_DEALY);
                    app_send_msg_delay(MSG_TYPE_ECONN, ECONN_MSG_ID_HANGUP_DEALY, NULL, 0,
                                       HANGUP_DEALY_TIMEOUT_MS);
                } else if (!from_peer && !app_charger_is_charging()) {
                    app_tone_indicate_event(event);
                    app_cancel_msg(MSG_TYPE_ECONN, ECONN_MSG_ID_HANGUP_DEALY);
                    app_send_msg_delay(MSG_TYPE_ECONN, ECONN_MSG_ID_HANGUP_DEALY, NULL, 0,
                                       HANGUP_DEALY_TIMEOUT_MS);
                } else {
                    DBGLOG_ECONN_DBG("ignore USRKEY when in box\n");
                }
            }
            ret = true;
            break;

        case EVTUSR_POWER_ON:
            DBGLOG_EVT_DBG(" econn EVTUSR_POWER_ON\n");
            app_pm_power_on();
            if ((app_charger_is_charging()
                 || app_conn_get_disconnect_reason() != BT_DISCONNECT_REASON_REMOTE)
                && (app_bt_get_sys_state() == STATE_AG_PAIRING) && (!app_bt_is_disabling())) {
                app_conn_connect_last();
            }
            ret = true;
            break;

            /*EVTUSR_CONNECT_LAST has no tone,so add for button press tone*/
        case EVTUSR_CONNECT_LAST:
            app_tone_indicate_event(EVTUSR_CUSTOM_3);
            break;
        default:
            break;
    }
    return ret;
}

bool_t app_econn_handle_sys_evt(uint16_t event, void *param)
{
    UNUSED(param);
    switch (event) {
        case EVTSYS_BT_POWER_OFF:
            app_cancel_msg(MSG_TYPE_ECONN, MSG_ID_START_APP_ECONN_ADV_SLOW);
            if (app_charger_is_charging()) {
                DBGLOG_ECONN_DBG("customer EVTSYS_BT_POWER_OFF ignored when chargin\n");
                return true;
            }
            break;

        case EVTSYS_CHARGE_CONNECTED:
            DBGLOG_ECONN_DBG("EVTSYS_CHARGE_CONNECTED");
            app_cancel_msg(MSG_TYPE_ECONN, MSG_ID_CANCAL_CHARGER_COMPLETE_STATE_LED);
            app_led_set_custom_state(STATE_CUSTOM_1);
            if (!battery_is_full) {
                app_cancel_msg(MSG_TYPE_ECONN, ECONN_MSG_ID_GET_CHARGE_MODE);
                app_send_msg_delay(MSG_TYPE_ECONN, ECONN_MSG_ID_GET_CHARGE_MODE, NULL, 0,
                                   GET_CHARGE_MODE_INTERVAL_MS);
            }

            if (app_wws_is_connected()) {
                if (app_wws_peer_is_charging() && app_bt_get_sys_state() == STATE_A2DP_STREAMING) {
                    app_evt_send(EVTUSR_MUSIC_PAUSE);
                }
            } else if (app_bt_get_sys_state() == STATE_A2DP_STREAMING) {
                app_evt_send(EVTUSR_MUSIC_PAUSE);
            }

            break;

        case EVTSYS_CHARGE_DISCONNECTED:
            DBGLOG_ECONN_DBG("[EVTSYS_CHARGE_DISCONNECTED");
            app_cancel_msg(MSG_TYPE_ECONN, ECONN_MSG_ID_GET_CHARGE_MODE);
            app_cancel_msg(MSG_TYPE_ECONN, MSG_ID_CANCAL_CHARGER_COMPLETE_STATE_LED);
            app_led_set_custom_state(false);
            app_led_set_enabled(true);
            battery_is_full = 0;
            break;

        case EVTSYS_POWER_ON_RECONNECT_FAILED:
            DBGLOG_ECONN_DBG("EVTSYS_POWER_ON_RECONNECT_FAILED state = 0x%x\n",
                             app_bt_get_sys_state());

            if (app_bt_get_sys_state() <= STATE_AG_PAIRING) {
                if (app_wws_is_connected()) {
                    app_wws_send_remote_msg(MSG_TYPE_ECONN, REMOTE_MSGID_RESET_AUTO_PWROFF_TIME, 0,
                                            NULL);
                }
                app_pm_handle_bt_disconnected();
            }
            break;

        case EVTSYS_LINK_LOSS:
            link_loss = true;
            app_send_msg_delay(MSG_TYPE_ECONN, ECONN_MSG_ID_FORCE_DISCOVERABLE, NULL, 0, 1000);
            DBGLOG_ECONN_DBG("customer rcv EVTSYS_LINK_LOSS\n");
            break;

        case EVTSYS_CONNECTED:
            force_discoverable = false;
            app_cancel_msg(MSG_TYPE_ECONN, ECONN_MSG_ID_FORCE_DISCOVERABLE);
            app_bt_set_force_discoverable(false);
            if (link_loss) {
                link_loss = false;
            }
            DBGLOG_ECONN_DBG("customer rcv EVTSYS_CONNECTED\n");
            break;

        case EVTSYS_WWS_ROLE_SWITCH:
            DBGLOG_ECONN_DBG("customer rcv EVTSYS_WWS_ROLE_SWITCH\n");
            if (link_loss && force_discoverable) {
                if (app_wws_is_master() && !app_bt_is_disabling()) {
                    app_bt_set_force_discoverable(true);
                    app_tone_indicate_event(EVTSYS_ENTER_PAIRING);
                } else {
                    app_bt_set_force_discoverable(false);
                }
            } else {
                app_bt_set_force_discoverable(false);
                force_discoverable = false;
            }
            break;

        case EVTSYS_WWS_PAIR_FAILED:
            app_pm_set_auto_power_off_timeout(AUTO_POWER_OFF_TIMEOUT_WWS_PAIR_FAIL);
            app_pm_handle_bt_disconnected();
            app_bt_set_discoverable_and_connectable(true, true);
            app_tone_indicate_event(EVTSYS_ENTER_PAIRING);
            break;

        case EVTSYS_CMC_BOX_OPEN:
            if (battery_is_full) {
                app_led_set_enabled(true);
                app_led_set_custom_state(STATE_CUSTOM_2);
                app_cancel_msg(MSG_TYPE_ECONN, MSG_ID_CANCAL_CHARGER_COMPLETE_STATE_LED);
                app_send_msg_delay(MSG_TYPE_ECONN, MSG_ID_CANCAL_CHARGER_COMPLETE_STATE_LED, NULL,
                                   0, CHARGE_COMPLETE_LED_TIMEOUT_MS);
            }
            break;

        default:
            break;
    }
    return false;
}

void app_econn_handle_ntc_value(int8_t value)
{
    UNUSED(value);
}

void app_econn_handle_tws_role_changed(bool_t is_master)
{
    UNUSED(is_master);
}

void app_econn_handle_charging_changed(bool_t charging)
{
    UNUSED(charging);

#if AUTO_SWITCH_ANC_MODE_BOTH_OUT_OF_BOX
    DBGLOG_ECONN_DBG("app_econn_handle_charging_changed\n");

    if (app_wws_is_connected()) {
        if (!app_charger_is_charging() && !app_wws_peer_is_charging()) {
            usr_cfg_set_listen_mode(LISTEN_MODE_ANC);
        } else if (!app_charger_is_charging() && app_wws_peer_is_charging()) {
            usr_cfg_set_listen_mode(LISTEN_MODE_NORMAL);
        } else if (app_charger_is_charging()) {
            usr_cfg_set_listen_mode(LISTEN_MODE_NORMAL);
        }
    } else {
        usr_cfg_set_listen_mode(LISTEN_MODE_NORMAL);
    }

    app_audio_listen_mode_force_normal(false);

    if (app_charger_is_charging() && app_wws_peer_is_charging()) {
        app_audio_game_mode_set_enabled(false);
    }
#endif
}

void app_econn_handle_peer_charging_changed(bool_t charging)
{
    UNUSED(charging);

#if AUTO_SWITCH_ANC_MODE_BOTH_OUT_OF_BOX
    DBGLOG_ECONN_DBG("app_econn_handle_peer_charging_changed\n");

    if (app_wws_is_connected()) {
        if (!app_charger_is_charging() && !app_wws_peer_is_charging()) {
            usr_cfg_set_listen_mode(LISTEN_MODE_ANC);
        } else if (!app_charger_is_charging() && app_wws_peer_is_charging()) {
            usr_cfg_set_listen_mode(LISTEN_MODE_NORMAL);
        }
        app_audio_listen_mode_force_normal(false);
    }
    if (app_charger_is_charging() && app_wws_peer_is_charging()) {
        app_audio_game_mode_set_enabled(false);
    }
#endif
}

void app_econn_handle_box_state_changed(box_state_t state)
{
    UNUSED(state);
}

void app_econn_handle_peer_box_state_changed(box_state_t state)
{
    UNUSED(state);
}

#if KEY_DRIVER_SELECTION == KEY_DRIVER_AW8686X
void app_econn_aw8686x_send_msg(uint32_t msg)
{
	app_send_msg(MSG_TYPE_ECONN, msg, NULL, 0);

}

#endif

#endif   // WQ_DEMO_UI
