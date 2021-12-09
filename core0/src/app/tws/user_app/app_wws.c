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
#include "bt_rpc_api.h"
#include "os_mem.h"
#include "os_utils.h"
#include "app_bt.h"
#include "app_wws.h"
#include "app_evt.h"
#include "app_inear.h"
#include "app_charger.h"
#include "app_bat.h"
#include "app_audio.h"
#include "usr_cfg.h"
#include "ro_cfg.h"
#include "app_btn.h"
#include "app_econn.h"
#include "app_conn.h"
#include "charger_box.h"
#include "version.h"
#include "ota_task.h"
#include "app_wqota.h"
#include "app_led.h"

#ifndef TWS_PAIR_TIMEOUT_MS
#define TWS_PAIR_TIMEOUT_MS (1000 * 60 * 2)
#endif

#ifndef TWS_INEAR_ROLE_SWITCH_ENABLED
#define TWS_INEAR_ROLE_SWITCH_ENABLED 0
#endif

#ifndef TWS_RSSI_ROLE_SWITCH_ENABLED
#define TWS_RSSI_ROLE_SWITCH_ENABLED 1   //battery role swtich conflict with rssi role switch
#endif

#ifndef TWS_BATTERY_ROLE_SWITCH_ENABLED
#define TWS_BATTERY_ROLE_SWITCH_ENABLED 0   //battery role swtich conflict with rssi role switch
#endif

#ifndef TWS_ROLE_SWITCH_BATTERY_LOW_LEVEL
#define TWS_ROLE_SWITCH_BATTERY_LOW_LEVEL 10
#endif

#ifndef TWS_ROLE_SWITCH_BATTERY_HIGH_LEVEL
#define TWS_ROLE_SWITCH_BATTERY_HIGH_LEVEL 30
#endif

#ifndef TWS_ROLE_SWITCH_ANTI_SHAKE_TIME_MS
#define TWS_ROLE_SWITCH_ANTI_SHAKE_TIME_MS 3000
#endif

#ifndef TWS_RSSI_DETECT_INTERVAL_MS
#define TWS_RSSI_DETECT_INTERVAL_MS 1000
#endif

#ifndef TWS_ROLE_SWITCH_GOOD_RSSI
#define TWS_ROLE_SWITCH_GOOD_RSSI -65
#endif

#ifndef TWS_ROLE_SWITCH_BAD_RSSI
#define TWS_ROLE_SWITCH_BAD_RSSI -75
#endif

#define APP_WWS_MSG_ID_TRIGGER_ROLE_SWITCH 1
#define APP_WWS_MSG_ID_RSSI_DETECT         2

typedef enum {
    WWS_MSG_ID_CONNECTED_SYNC,
    WWS_MSG_ID_USR_EVT,
    WWS_MSG_ID_IN_EAR,
    WWS_MSG_ID_INEAR_ENABLE_REQ,
    WWS_MSG_ID_CHARGER,
    WWS_MSG_ID_BATTERY,
    WWS_MSG_ID_BOX_STATE,
    WWS_MSG_ID_RESET_PDL,
    WWS_MSG_ID_VOLUME,
    WWS_MSG_ID_CALL_VOLUME,
    WWS_MSG_ID_MUSIC_VOLUME,
    WWS_MSG_ID_LISTEN_MODE,
    WWS_MSG_ID_ANC_LEVEL,
    WWS_MSG_ID_TRANSPARENCY_LEVEL,
    WWS_MSG_ID_SET_KEY_CONFIG,
    WWS_MSG_ID_GET_KEY_CONFIG,
    WWS_MSG_ID_GET_KEY_CONFIG_RSP,
    WWS_MSG_ID_RESET_KEY_CONFIG,
    WWS_MSG_ID_VISIBILITY,
    WWS_MSG_ID_VOLUME_SET_REQ,
    WWS_MSG_ID_LISTEN_MODE_SET_REQ,
    WWS_MSG_ID_PDL_CLEAR_REQ,
    WWS_MSG_ID_VOLUME_SYNC_REQ,
    WWS_MSG_ID_REMOTE_MSG,
    WWS_MSG_ID_GET_RSSI_REQ,
    WWS_MSG_ID_GET_RSSI_RSP,
    WWS_MSG_ID_GAME_MODE,
} wws_msg_id_t;

typedef enum {
    ROLE_SWITCH_REASON_CHARGING,
    ROLE_SWITCH_REASON_PEER_CHARGING,
    ROLE_SWITCH_REASON_BATTERY,
    ROLE_SWITCH_REASON_PEER_BATTERY,
    ROLE_SWITCH_REASON_IN_EAR,
    ROLE_SWITCH_REASON_PEER_IN_EAR,
    ROLE_SWITCH_REASON_RSSI,
    ROLE_SWTICH_REASON_TIMER,
    ROLE_SWITCH_REASON_CONNECTED,
} role_switch_reason_t;

#pragma pack(push)
#pragma pack(1)
typedef struct {
    wws_msg_id_t id : 8;
} wws_msg_t;

typedef struct {
    wws_msg_id_t id : 8;
    uint16_t evt;
    uint32_t state;
} wws_msg_usr_evt_t;

typedef struct {
    wws_msg_id_t id : 8;
    /**
     * in ear detection enabled or not,
     * if this message if from slave,
     * "enabled" should be ignored by master.
     */
    uint8_t enabled : 1;
    uint8_t in_ear : 1;
} wws_msg_in_ear_t;

typedef struct {
    wws_msg_id_t id : 8;
    uint8_t enable : 1;
} wws_msg_inear_enable_req_t;

typedef struct {
    wws_msg_id_t id : 8;
    uint8_t charging : 1;
} wws_msg_charger_t;

typedef struct {
    wws_msg_id_t id : 8;
    /** battery level 0-100 */
    uint8_t level;
} wws_msg_battery_t;

typedef struct {
    wws_msg_id_t id : 8;
    box_state_t box_state : 8;
} wws_msg_box_state_t;

typedef struct {
    wws_msg_id_t id : 8;
} wws_msg_reset_pdl_t;

typedef struct {
    wws_msg_id_t id : 8;
    uint8_t call;
    uint8_t music;
} wws_msg_volume_t;

typedef struct {
    wws_msg_id_t id : 8;
    uint8_t vol;
} wws_msg_call_volume_t;

typedef struct {
    wws_msg_id_t id : 8;
    uint8_t vol;
} wws_msg_music_volume_t;

typedef struct {
    wws_msg_id_t id : 8;
    uint8_t mode;
} wws_msg_listen_mode_t;

typedef struct {
    wws_msg_id_t id : 8;
    uint8_t enabled : 1;
} wws_msg_game_mode_t;

typedef struct {
    wws_msg_id_t id : 8;
    uint8_t level;
} wws_msg_anc_level_t;

typedef struct {
    wws_msg_id_t id : 8;
    uint8_t level;
} wws_msg_transparency_level_t;

typedef struct {
    wws_msg_id_t id : 8;
    uint8_t in_ear_enabled : 1;
    uint8_t in_ear : 1;
    uint8_t charging : 1;
    uint8_t discoverable : 1;
    uint8_t connectable : 1;
    uint8_t game_mode_enabled : 1;
    uint8_t reserved : 2;
    uint8_t battery_level;
    uint8_t call_volume;
    uint8_t music_volume;
    uint8_t listen_mode;
    box_state_t box_state : 8;
    uint16_t fw_ver_build;
    uint32_t fw_ver_in_kv;
} wws_msg_connected_sync_t;

typedef struct {
    uint8_t call_volume;
    uint8_t music_volume;
} wws_tds_data_t;

typedef struct {
    wws_msg_id_t id : 8;
    uint16_t state_mask;
    uint16_t event;
    uint8_t key_type;
} wws_msg_set_key_config_t;

typedef struct {
    wws_msg_id_t id : 8;
} wws_msg_reset_key_config_t;

typedef struct {
    wws_msg_id_t id : 8;
    uint8_t count;
    struct key_state {
        uint16_t state_mask;
        uint8_t key_type;
    } key_states[0];
} wws_msg_get_key_config_t;

typedef struct {
    wws_msg_id_t id : 8;
    uint8_t count;
    uint16_t events[0];
} wws_msg_get_key_config_rsp_t;

typedef struct {
    wws_msg_id_t id : 8;
    uint8_t discoverable : 1;
    uint8_t connectable : 1;
    uint8_t reserved : 6;
} wws_msg_visibility_t;

typedef struct {
    wws_msg_id_t id : 8;
    /** volume level. */
    uint8_t music_lvl;
    uint8_t call_lvl;
} wws_msg_vol_set_req_t;

typedef struct {
    wws_msg_id_t id : 8;
    /** listen mode. */
    listen_mode_t mode : 8;
} wws_msg_listen_mode_set_req_t;

typedef struct {
    wws_msg_id_t id : 8;
} wws_msg_pdl_clear_req_t;

typedef struct {
    wws_msg_id_t id : 8;
    uint8_t call_volume;
    uint8_t music_volume;
} wws_msg_volume_sync_req_t;

typedef struct {
    wws_msg_id_t id : 8;
    app_msg_type_t msg_type : 8;
    uint16_t msg_id;
    uint16_t param_len;
    uint16_t reserved;
    uint8_t param[];
} wws_msg_remote_msg_t;

typedef struct {
    wws_msg_id_t id : 8;
} wws_msg_get_rssi_req_t;

typedef struct {
    wws_msg_id_t id : 8;
    int8_t phone_rssi;
    int8_t wws_rssi;
} wws_msg_get_rssi_rsp_t;
#pragma pack(pop)

typedef struct {
    bt_tws_state_t state : 8;
    bt_tws_role_t role : 8;
    bt_tws_channel_t channel : 8;
    bool_t peer_discoverable;
    bool_t peer_connectable;
    bool_t peer_inear;
    bool_t peer_charging;
    box_state_t peer_box_state : 8;
    bool_t rssi_detect_enabled;
    uint8_t peer_battery;
    int8_t peer_phone_rssi;
    int8_t peer_wws_rssi;
    BD_ADDR_T peer_addr;
    uint16_t peer_fw_ver_build;
    uint32_t peer_fw_ver_in_kv;
    uint32_t last_role_switch_time_ms;
    void (*key_config_callback)(uint8_t, const uint16_t *);
} wws_context_t;

static wws_context_t _context = {0};
static wws_context_t *context = &_context;

static void app_wws_send_get_rssi_req(void);

static void wws_triger_role_switch(role_switch_reason_t reason)
{
    uint32_t tm;

    UNUSED(reason);

    if (!app_wws_is_connected_master()) {
        return;
    }

    if (ota_task_is_running()) {
        return;
    }

    if (app_bt_get_sys_state() < STATE_CONNECTED) {
        return;
    }

    tm = os_boot_time32();
    if (app_charger_is_charging() && (!app_wws_peer_is_charging())) {
        if (tm < context->last_role_switch_time_ms + TWS_ROLE_SWITCH_ANTI_SHAKE_TIME_MS) {
            DBGLOG_WWS_DBG("charging role switch delayed\n");
            app_cancel_msg(MSG_TYPE_WWS, APP_WWS_MSG_ID_TRIGGER_ROLE_SWITCH);
            app_send_msg_delay(MSG_TYPE_WWS, APP_WWS_MSG_ID_TRIGGER_ROLE_SWITCH, NULL, 0,
                               TWS_ROLE_SWITCH_ANTI_SHAKE_TIME_MS);
        } else if ((reason == ROLE_SWITCH_REASON_CHARGING)
                   || (reason == ROLE_SWTICH_REASON_TIMER)) {
            DBGLOG_WWS_DBG("role switch for charging\n");
            app_cancel_msg(MSG_TYPE_WWS, APP_WWS_MSG_ID_TRIGGER_ROLE_SWITCH);
            app_wws_role_switch();
        } else {
            app_cancel_msg(MSG_TYPE_WWS, APP_WWS_MSG_ID_TRIGGER_ROLE_SWITCH);
            app_send_msg_delay(MSG_TYPE_WWS, APP_WWS_MSG_ID_TRIGGER_ROLE_SWITCH, NULL, 0,
                               TWS_ROLE_SWITCH_ANTI_SHAKE_TIME_MS);
        }
        return;
    }

    if (app_charger_is_charging() || app_wws_peer_is_charging()) {
        return;   // ignore other role switch when charging
    }

    if (TWS_INEAR_ROLE_SWITCH_ENABLED) {
        if ((app_inear_get()) && (!app_wws_peer_is_inear())) {
            return;
        } else if ((!app_inear_get()) && (app_wws_peer_is_inear())) {
            if (tm < context->last_role_switch_time_ms + TWS_ROLE_SWITCH_ANTI_SHAKE_TIME_MS) {
                DBGLOG_WWS_DBG("in ear role switch delayed\n");
                app_cancel_msg(MSG_TYPE_WWS, APP_WWS_MSG_ID_TRIGGER_ROLE_SWITCH);
                app_send_msg_delay(MSG_TYPE_WWS, APP_WWS_MSG_ID_TRIGGER_ROLE_SWITCH, NULL, 0,
                                   TWS_ROLE_SWITCH_ANTI_SHAKE_TIME_MS);
            } else if (reason == ROLE_SWTICH_REASON_TIMER) {
                DBGLOG_WWS_DBG("role switch for in ear\n");
                app_cancel_msg(MSG_TYPE_WWS, APP_WWS_MSG_ID_TRIGGER_ROLE_SWITCH);
                app_wws_role_switch();
            } else if ((reason == ROLE_SWITCH_REASON_IN_EAR)
                       || (reason == ROLE_SWITCH_REASON_PEER_IN_EAR)) {
                app_cancel_msg(MSG_TYPE_WWS, APP_WWS_MSG_ID_TRIGGER_ROLE_SWITCH);
                app_send_msg_delay(MSG_TYPE_WWS, APP_WWS_MSG_ID_TRIGGER_ROLE_SWITCH, NULL, 0,
                                   TWS_ROLE_SWITCH_ANTI_SHAKE_TIME_MS);
            } else {
                DBGLOG_WWS_DBG("inear switch error, unkown reason:%d\n", reason);
            }

            return;
        }
    }

    if (TWS_RSSI_ROLE_SWITCH_ENABLED && (reason == ROLE_SWITCH_REASON_RSSI)) {
        const link_quality_t *quality = app_bt_get_link_quality();
        DBGLOG_WWS_DBG("ROLE_SWITCH_REASON_RSSI phone:%d wws:%d\n", quality->plink_rssi_avg,
                       quality->rlink_rssi_avg);

        if ((quality->plink_rssi_avg > TWS_ROLE_SWITCH_BAD_RSSI)
            || (quality->rlink_rssi_avg > TWS_ROLE_SWITCH_BAD_RSSI)) {
            DBGLOG_WWS_DBG("rssi role switch anti-shake\n");
            return;
        }

        if (context->peer_phone_rssi < TWS_ROLE_SWITCH_GOOD_RSSI) {
            DBGLOG_WWS_DBG("rssi role switch, peer not good enough\n");
            return;
        }

        if (tm < context->last_role_switch_time_ms + TWS_ROLE_SWITCH_ANTI_SHAKE_TIME_MS) {
            DBGLOG_WWS_DBG("rssi role switch delayed\n");
            return;
        }

        app_wws_role_switch();
        return;
    }

    if (TWS_BATTERY_ROLE_SWITCH_ENABLED
        && (app_bat_get_level() <= TWS_ROLE_SWITCH_BATTERY_LOW_LEVEL)
        && (app_wws_peer_get_battery_level() >= TWS_ROLE_SWITCH_BATTERY_HIGH_LEVEL)) {
        if (tm < context->last_role_switch_time_ms + TWS_ROLE_SWITCH_ANTI_SHAKE_TIME_MS) {
            DBGLOG_WWS_DBG("battery role switch delayed\n");
            app_cancel_msg(MSG_TYPE_WWS, APP_WWS_MSG_ID_TRIGGER_ROLE_SWITCH);
            app_send_msg_delay(MSG_TYPE_WWS, APP_WWS_MSG_ID_TRIGGER_ROLE_SWITCH, NULL, 0,
                               TWS_ROLE_SWITCH_ANTI_SHAKE_TIME_MS);
        } else if (reason == ROLE_SWTICH_REASON_TIMER) {
            DBGLOG_WWS_DBG("role switch for battery level\n");
            app_cancel_msg(MSG_TYPE_WWS, APP_WWS_MSG_ID_TRIGGER_ROLE_SWITCH);
            app_wws_role_switch();
        } else {
            app_cancel_msg(MSG_TYPE_WWS, APP_WWS_MSG_ID_TRIGGER_ROLE_SWITCH);
            app_send_msg_delay(MSG_TYPE_WWS, APP_WWS_MSG_ID_TRIGGER_ROLE_SWITCH, NULL, 0,
                               TWS_ROLE_SWITCH_ANTI_SHAKE_TIME_MS);
        }
        return;
    }
}

static void app_wws_handle_msg(uint16_t msg_id, void *param)
{
    UNUSED(param);

    switch (msg_id) {
        case APP_WWS_MSG_ID_TRIGGER_ROLE_SWITCH:
            wws_triger_role_switch(ROLE_SWTICH_REASON_TIMER);
            break;
        case APP_WWS_MSG_ID_RSSI_DETECT: {
            const link_quality_t *quality = app_bt_get_link_quality();
            DBGLOG_WWS_DBG("crc_err:%d seq_err:%d phone_rssi:%d wws_rssi:%d", quality->crc_err_rate,
                           quality->seq_err_rate, quality->plink_rssi_avg, quality->rlink_rssi_avg);
            if (context->rssi_detect_enabled) {
                if (TWS_RSSI_ROLE_SWITCH_ENABLED && app_wws_is_master()
                    && (quality->plink_rssi_avg <= TWS_ROLE_SWITCH_BAD_RSSI)
                    && (quality->rlink_rssi_avg <= TWS_ROLE_SWITCH_BAD_RSSI)
                    && (quality->plink_rssi_avg != -127) && (quality->rlink_rssi_avg != -127)) {
                    app_wws_send_get_rssi_req();
                }

                if (quality->plink_rssi_avg < TWS_ROLE_SWITCH_GOOD_RSSI) {
                    app_send_msg_delay(MSG_TYPE_WWS, APP_WWS_MSG_ID_RSSI_DETECT, NULL, 0,
                                       TWS_RSSI_DETECT_INTERVAL_MS / 2);
                } else {
                    app_send_msg_delay(MSG_TYPE_WWS, APP_WWS_MSG_ID_RSSI_DETECT, NULL, 0,
                                       TWS_RSSI_DETECT_INTERVAL_MS);
                }
            }
            break;
        }
        default:
            break;
    }
}

static void wws_set_tds_data(void)
{
    wws_tds_data_t data;
    bt_cmd_tws_set_tds_data_t cmd_param;

    data.call_volume = usr_cfg_get_call_vol();
    data.music_volume = usr_cfg_get_music_vol();

    DBGLOG_BT_DBG("wws_set_tds_data %d %d\n", data.call_volume, data.music_volume);

    cmd_param.len = sizeof(wws_tds_data_t);
    cmd_param.data = (void *)&data;
    app_bt_send_rpc_cmd(BT_CMD_TWS_SET_TDS_DATA, &cmd_param, sizeof(cmd_param));
}

static void wws_send_data(void *data, uint16_t len, bool_t exit_sniff)
{
    bt_cmd_tws_send_data_t param;
    bt_result_t ret;

    if (context->state != TWS_STATE_CONNECTED) {
        DBGLOG_WWS_ERR("wws_send_data error state:%d\n", context->state);
        return;
    }

    param.exit_sniff = exit_sniff;
    param.data = data;
    param.len = len;

    ret = app_bt_send_rpc_cmd((uint32_t)BT_CMD_TWS_SEND_DATA, &param, sizeof(param));

    if (ret) {
        DBGLOG_WWS_ERR("wws_send_data error ret:%d\n", ret);
    }
}

static void wws_connected_sync(void)
{
    wws_msg_connected_sync_t msg;

    memset(&msg, 0, sizeof(msg));
    msg.id = WWS_MSG_ID_CONNECTED_SYNC;

    if (app_inear_is_enabled()) {
        msg.in_ear_enabled = 1;
    }
    if (app_inear_get()) {
        msg.in_ear = 1;
    }
    if (app_charger_is_charging()) {
        msg.charging = 1;
    }

    msg.battery_level = app_bat_get_level();
    msg.call_volume = usr_cfg_get_call_vol();
    msg.music_volume = usr_cfg_get_music_vol();
    msg.listen_mode = usr_cfg_get_listen_mode();
    msg.game_mode_enabled = app_audio_game_mode_get();
    if (app_bt_is_discoverable()) {
        msg.discoverable = 1;
    }
    if (app_bt_is_connectable()) {
        msg.connectable = 1;
    }
    msg.box_state = app_charger_get_box_state();

#ifdef CUSTOM_VERSION
    msg.fw_ver_build = CUSTOM_VERSION;
#else
    msg.fw_ver_build = FIRMWARE_VERSION_BUILD;
#endif
    msg.fw_ver_in_kv = ro_gen_cfg()->fw_version;

    DBGLOG_WWS_DBG("wws_connected_sync bat_lvl:%d charging:%d box:%d call:%d music:%d anc:%d\n",
                   msg.battery_level, msg.charging, msg.box_state, msg.call_volume,
                   msg.music_volume, msg.listen_mode);

    wws_send_data(&msg, sizeof(msg), true);
}

static void wws_handle_connected_sync(const void *data, uint8_t len)
{
    const wws_msg_connected_sync_t *msg;

    if (len != sizeof(wws_msg_connected_sync_t)) {
        DBGLOG_WWS_ERR("wws_handle_connected_sync len:%d error\n", len);
        return;
    }

    msg = (const wws_msg_connected_sync_t *)data;

    DBGLOG_WWS_DBG("wws_handle_connected_sync state:%d role:%d\n", context->state, context->role);

    context->peer_inear = msg->in_ear;
    context->peer_charging = msg->charging;
    context->peer_battery = msg->battery_level;
    context->peer_fw_ver_build = msg->fw_ver_build;
    context->peer_fw_ver_in_kv = msg->fw_ver_in_kv;
    context->peer_box_state = msg->box_state;

    if (app_wws_is_slave()) {
        context->peer_discoverable = msg->discoverable;
        context->peer_connectable = msg->connectable;
        app_inear_set_enabled((bool_t)msg->in_ear_enabled);
        app_audio_call_volume_set(msg->call_volume);
        app_audio_music_volume_set(msg->music_volume);
        app_audio_listen_mode_set((listen_mode_t)msg->listen_mode);
        app_bt_handle_peer_visibility_changed();
        app_audio_game_mode_set_enabled((bool_t)msg->game_mode_enabled);
    }

    app_econn_handle_tws_state_changed(true);
    app_led_handle_wws_state_changed(true);
    app_econn_handle_peer_box_state_changed(context->peer_box_state);
}

static void wws_handle_usr_evt(const void *data, uint8_t len)
{
    const wws_msg_usr_evt_t *msg;
    uint32_t sys_state;

    if (len != sizeof(wws_msg_usr_evt_t)) {
        DBGLOG_WWS_ERR("wws_handle_usr_evt len:%d error\n", len);
        return;
    }

    msg = (const wws_msg_usr_evt_t *)data;

    DBGLOG_WWS_DBG("wws_handle_usr_evt %d\n", msg->evt);

    sys_state = app_bt_get_sys_state();
    if ((msg->state < STATE_CONNECTED) && (sys_state >= STATE_CONNECTED)) {
        DBGLOG_WWS_ERR("peer_state:0x%X local_state:0x%X not match, event ignored\n", msg->state,
                       sys_state);
        return;
    }

    app_evt_handle_peer_evt(msg->evt);
}

static void wws_handle_in_ear(const void *data, uint8_t len)
{
    const wws_msg_in_ear_t *msg;

    if (len != sizeof(wws_msg_in_ear_t)) {
        DBGLOG_WWS_ERR("wws_handle_in_ear len:%d error\n", len);
        return;
    }

    msg = (const wws_msg_in_ear_t *)data;

    DBGLOG_WWS_DBG("wws_handle_in_ear enbl:%d ear:%d\n", msg->enabled, msg->in_ear);

    if (context->peer_inear != msg->in_ear) {
        context->peer_inear = msg->in_ear;
        app_econn_handle_peer_in_ear_changed(msg->in_ear);
    }
    if (app_wws_is_slave()) {
        app_inear_set_enabled(msg->enabled);
    }

    wws_triger_role_switch(ROLE_SWITCH_REASON_PEER_IN_EAR);
}

static void wws_handle_inear_enable_req(const void *data, uint8_t len)
{
    const wws_msg_inear_enable_req_t *msg;

    if (len != sizeof(wws_msg_inear_enable_req_t)) {
        DBGLOG_WWS_ERR("wws_handle_inear_enable_req len:%d error\n", len);
        return;
    }

    msg = (const wws_msg_inear_enable_req_t *)data;

    DBGLOG_WWS_DBG("wws_handle_inear_enable_req enbl:%d\n", msg->enable);

    if (app_wws_is_master()) {
        app_inear_set_enabled(msg->enable);
    }
}

static void wws_handle_charger(const void *data, uint8_t len)
{
    const wws_msg_charger_t *msg;

    if (len != sizeof(wws_msg_charger_t)) {
        DBGLOG_WWS_ERR("wws_handle_charger len:%d error\n", len);
        return;
    }

    msg = (const wws_msg_charger_t *)data;

    DBGLOG_WWS_DBG("wws_handle_charger chg:%d\n", msg->charging);

    context->peer_charging = msg->charging;
    app_econn_handle_peer_charging_changed(msg->charging);

    wws_triger_role_switch(ROLE_SWITCH_REASON_PEER_CHARGING);
}

static void wws_handle_battery(const void *data, uint8_t len)
{
    const wws_msg_battery_t *msg;

    if (len != sizeof(wws_msg_battery_t)) {
        DBGLOG_WWS_ERR("wws_handle_battery len:%d error\n", len);
        return;
    }

    msg = (const wws_msg_battery_t *)data;

    DBGLOG_WWS_DBG("wws_handle_battery %d\n", msg->level);

    context->peer_battery = msg->level;
    app_econn_handle_peer_battery_level_changed(msg->level);

    if (TWS_BATTERY_ROLE_SWITCH_ENABLED) {
        wws_triger_role_switch(ROLE_SWITCH_REASON_PEER_BATTERY);
    }
}

static void wws_handle_box_state(const void *data, uint8_t len)
{
    const wws_msg_box_state_t *msg;

    if (len != sizeof(wws_msg_box_state_t)) {
        DBGLOG_WWS_ERR("wws_handle_box_state len:%d error\n", len);
        return;
    }

    msg = (const wws_msg_box_state_t *)data;
    context->peer_box_state = msg->box_state;
    DBGLOG_WWS_DBG("wws_handle_box_state state:%d\n", msg->box_state);
    app_econn_handle_peer_box_state_changed(msg->box_state);
}

static void wws_handle_reset_pdl(const void *data, uint8_t len)
{
    UNUSED(data);

    if (len != sizeof(wws_msg_reset_pdl_t)) {
        DBGLOG_WWS_ERR("wws_handle_reset_pdl len:%d error\n", len);
        return;
    }

    DBGLOG_WWS_DBG("wws_handle_reset_pdl\n");
    usr_cfg_reset_pdl();
    app_bt_clear_pair_list();
}

static void wws_handle_volume(const void *data, uint8_t len)
{
    const wws_msg_volume_t *msg;

    if (len != sizeof(wws_msg_volume_t)) {
        DBGLOG_WWS_ERR("wws_handle_volume len:%d error\n", len);
        return;
    }

    msg = (const wws_msg_volume_t *)data;

    DBGLOG_WWS_DBG("wws_handle_volume call:%d music:%d\n", msg->call, msg->music);

    if (app_wws_is_slave()) {
        app_audio_call_volume_set(msg->call);
        app_audio_music_volume_set(msg->music);
    }
}

static void wws_handle_call_volume(const void *data, uint8_t len)
{
    const wws_msg_call_volume_t *msg;

    if (len != sizeof(wws_msg_call_volume_t)) {
        DBGLOG_WWS_ERR("wws_handle_call_volume len:%d error\n", len);
        return;
    }

    msg = (const wws_msg_call_volume_t *)data;

    DBGLOG_WWS_DBG("wws_handle_call_volume vol:%d\n", msg->vol);

    if (app_wws_is_slave()) {
        app_audio_call_volume_set(msg->vol);
    }
}

static void wws_handle_music_volume(const void *data, uint8_t len)
{
    const wws_msg_music_volume_t *msg;

    if (len != sizeof(wws_msg_music_volume_t)) {
        DBGLOG_WWS_ERR("wws_handle_music_volume len:%d error\n", len);
        return;
    }

    msg = (const wws_msg_music_volume_t *)data;

    DBGLOG_WWS_DBG("wws_handle_music_volume vol:%d\n", msg->vol);

    if (app_wws_is_slave()) {
        app_audio_music_volume_set(msg->vol);
    }
}

static void wws_handle_listen_mode(const void *data, uint8_t len)
{
    const wws_msg_listen_mode_t *msg;

    if (len != sizeof(wws_msg_listen_mode_t)) {
        DBGLOG_WWS_ERR("wws_handle_listen_mode len:%d error\n", len);
        return;
    }

    msg = (const wws_msg_listen_mode_t *)data;

    DBGLOG_WWS_DBG("wws_handle_listen_mode %d\n", msg->mode);

    if (app_wws_is_slave()) {
        app_audio_listen_mode_set((listen_mode_t)msg->mode);
    }
}

static void wws_handle_game_mode(const void *data, uint8_t len)
{
    const wws_msg_game_mode_t *msg;

    if (len != sizeof(wws_msg_game_mode_t)) {
        DBGLOG_WWS_ERR("wws_handle_game_mode len:%d error\n", len);
        return;
    }

    msg = (const wws_msg_game_mode_t *)data;

    DBGLOG_WWS_DBG("wws_handle_game_mode %d\n", msg->enabled);

    if (app_wws_is_slave()) {
        app_audio_game_mode_set_enabled((bool_t)msg->enabled);
    }
}

static void wws_handle_anc_level(const void *data, uint8_t len)
{
    const wws_msg_anc_level_t *msg;

    if (len != sizeof(wws_msg_anc_level_t)) {
        DBGLOG_WWS_ERR("wws_handle_anc_level len:%d error\n", len);
        return;
    }

    msg = (const wws_msg_anc_level_t *)data;

    DBGLOG_WWS_DBG("wws_handle_anc_level %d\n", msg->level);

    if (app_wws_is_slave()) {
        app_audio_anc_level_set((anc_level_t)msg->level);
    }
}

static void wws_handle_transparency_level(const void *data, uint8_t len)
{
    const wws_msg_transparency_level_t *msg;

    if (len != sizeof(wws_msg_transparency_level_t)) {
        DBGLOG_WWS_ERR("wws_handle_transparency_level len:%d error\n", len);
        return;
    }

    msg = (const wws_msg_transparency_level_t *)data;

    DBGLOG_WWS_DBG("wws_handle_transparency_level %d\n", msg->level);

    if (app_wws_is_slave()) {
        app_audio_transparency_level_set((transparency_level_t)msg->level);
    }
}

static void wws_handle_set_key_config(const void *data, uint8_t len)
{
    const wws_msg_set_key_config_t *msg = data;

    if (len != sizeof(wws_msg_set_key_config_t)) {
        DBGLOG_WWS_ERR("wws_handle_set_key_config len:%d error\n", len);
        return;
    }

    DBGLOG_WWS_DBG("wws_handle_set_key_config type:%d event:%d\n", msg->key_type, msg->event);

    app_btn_cus_key_add((key_pressed_type_t)msg->key_type, msg->state_mask, msg->event);
}

static void wws_handle_get_key_config(const void *data, uint8_t len)
{
    const wws_msg_get_key_config_t *msg = data;
    uint16_t event;
    wws_msg_get_key_config_rsp_t *rsp;
    uint8_t count;

    if (len < sizeof(wws_msg_get_key_config_t)) {
        DBGLOG_WWS_ERR("wws_handle_get_key_config len:%d error\n", len);
        return;
    }

    count = msg->count;
    DBGLOG_WWS_DBG("wws_handle_get_key_config count:%d\n", count);
    assert(count <= 8);

    rsp =
        os_mem_malloc(IOT_APP_MID, sizeof(wws_msg_get_key_config_rsp_t) + sizeof(uint16_t) * count);
    rsp->id = WWS_MSG_ID_GET_KEY_CONFIG_RSP;
    rsp->count = count;

    for (int i = 0; i < count; i++) {
        event = app_btn_cus_key_read((key_pressed_type_t)msg->key_states[i].key_type,
                                     msg->key_states[i].state_mask);
        rsp->events[i] = event;
    }

    wws_send_data(rsp, sizeof(wws_msg_get_key_config_rsp_t) + sizeof(uint16_t) * count, false);
    os_mem_free(rsp);
}

static void wws_handle_get_key_config_rsp(const void *data, uint8_t len)
{
    const wws_msg_get_key_config_rsp_t *msg = data;

    if (len != (sizeof(wws_msg_get_key_config_rsp_t) + msg->count * sizeof(uint16_t))) {
        DBGLOG_WWS_ERR("wws_handle_get_key_config_rsp len:%d error\n", len);
        return;
    }

    if (!context->key_config_callback) {
        DBGLOG_WWS_ERR("wws_handle_get_key_config_rsp error: callback not exists\n");
        return;
    }

    DBGLOG_WWS_DBG("wws_handle_get_key_config_rsp count:%d\n", msg->count);
    context->key_config_callback(msg->count, msg->events);

    context->key_config_callback = NULL;
}

static void wws_handle_reset_key_config(const void *data, uint8_t len)
{
    UNUSED(data);

    if (len != sizeof(wws_msg_reset_key_config_t)) {
        DBGLOG_WWS_DBG("wws_handle_reset_key_config len:%d error\n", len);
        return;
    }

    DBGLOG_WWS_DBG("wws_handle_reset_key_config\n");
    app_btn_cus_key_reset();
}

static void wws_handle_visibility(const void *data, uint8_t len)
{
    const wws_msg_visibility_t *msg = data;
    bool_t discoverable = false;
    bool_t connectable = false;

    if (len != sizeof(wws_msg_visibility_t)) {
        DBGLOG_WWS_ERR("wws_handle_visibility len:%d error\n", len);
        return;
    }

    if (msg->discoverable) {
        discoverable = true;
    }
    if (msg->connectable) {
        connectable = true;
    }
    context->peer_discoverable = discoverable;
    context->peer_connectable = connectable;

    app_bt_handle_peer_visibility_changed();

    DBGLOG_WWS_DBG("wws_handle_visibility discoverable:%d connectable:%d\n", discoverable,
                   connectable);
}

static void wws_handle_vol_set_req(const void *data, uint8_t len)
{
    const wws_msg_vol_set_req_t *msg;

    if (len != sizeof(wws_msg_vol_set_req_t)) {
        DBGLOG_WWS_ERR("wws_msg_vol_set_req_t len:%d error\n", len);
        return;
    }

    msg = (const wws_msg_vol_set_req_t *)data;

    DBGLOG_WWS_DBG("wws_msg_vol_set_req_t call_vol:%d, music:%d\n", msg->call_lvl, msg->music_lvl);

    app_audio_call_volume_set(msg->call_lvl);
    app_bt_report_call_volume(msg->call_lvl);

    app_audio_music_volume_set(msg->music_lvl);
    app_bt_report_music_volume(msg->music_lvl);

    if (app_wws_is_connected()) {
        app_wws_send_volume(usr_cfg_get_call_vol(), usr_cfg_get_music_vol());
    }
}

static void wws_handle_listen_mode_set_req(const void *data, uint8_t len)
{
    const wws_msg_listen_mode_set_req_t *msg;

    if (len != sizeof(wws_msg_listen_mode_set_req_t)) {
        DBGLOG_WWS_ERR("wws_msg_listen_mode_set_req_t len:%d error\n", len);
        return;
    }

    msg = (const wws_msg_listen_mode_set_req_t *)data;

    DBGLOG_WWS_DBG("wws_msg_listen_mode_set_req_t mode:%d\n", msg->mode);

    app_audio_listen_mode_set(msg->mode);
}

static void wws_handle_pdl_clear_req(const void *data, uint8_t len)
{
    UNUSED(data);

    if (len != sizeof(wws_msg_pdl_clear_req_t)) {
        DBGLOG_WWS_ERR("wws_handle_pdl_clear_req len:%d error\n", len);
        return;
    }

    DBGLOG_WWS_DBG("wws_handle_pdl_clear_req\n");
    app_evt_send(EVTUSR_CLEAR_PDL);
}

static void wws_handle_volume_sync_req(const void *data, uint8_t len)
{
    wws_msg_volume_sync_req_t *param;

    if (len != sizeof(wws_msg_volume_sync_req_t)) {
        DBGLOG_WWS_ERR("wws_handle_volume_sync_req len:%d error\n", len);
        return;
    }

    param = (wws_msg_volume_sync_req_t *)data;

    DBGLOG_WWS_DBG("wws_handle_volume_sync_req call_vol:%d music_vol:%d\n", param->call_volume,
                   param->music_volume);
    if ((param->call_volume != usr_cfg_get_call_vol())
        && (param->music_volume != usr_cfg_get_music_vol())) {
        app_wws_send_volume(usr_cfg_get_call_vol(), usr_cfg_get_music_vol());
    } else if (param->call_volume != usr_cfg_get_call_vol()) {
        wws_msg_call_volume_t msg;

        msg.id = WWS_MSG_ID_CALL_VOLUME;
        msg.vol = usr_cfg_get_call_vol();
        wws_send_data(&msg, sizeof(msg), true);
    } else if (param->music_volume != usr_cfg_get_music_vol()) {
        wws_msg_music_volume_t msg;

        msg.id = WWS_MSG_ID_MUSIC_VOLUME;
        msg.vol = usr_cfg_get_music_vol();
        wws_send_data(&msg, sizeof(msg), true);
    }
}

static void wws_handle_remote_msg(void *data, uint16_t len)
{
    wws_msg_remote_msg_t *msg = data;

    if (len != sizeof(wws_msg_remote_msg_t) + msg->param_len) {
        DBGLOG_WWS_ERR("wws_handle_remote_msg error, len:%d param_len:%d\n", len, msg->param_len);
        return;
    }
    DBGLOG_WWS_DBG("wws_handle_remote_msg type:%d id:%d param_len:%d\n", msg->msg_type, msg->msg_id,
                   msg->param_len);

    app_send_msg(msg->msg_type, msg->msg_id, msg->param, msg->param_len);
}

static void wws_handle_get_rssi_rsp(void *data, uint16_t len)
{
    wws_msg_get_rssi_rsp_t *msg = data;

    if (len != sizeof(wws_msg_get_rssi_rsp_t)) {
        DBGLOG_WWS_ERR("wws_handle_get_rssi_rsp error, len:%d\n", len);
        return;
    }

    context->peer_phone_rssi = msg->phone_rssi;
    context->peer_wws_rssi = msg->wws_rssi;

    DBGLOG_WWS_DBG("wws_handle_get_rssi_rsp phone:%d wws:%d\n", msg->phone_rssi, msg->wws_rssi);

    if (TWS_RSSI_ROLE_SWITCH_ENABLED) {
        wws_triger_role_switch(ROLE_SWITCH_REASON_RSSI);
    }
}

static void app_wws_send_get_rssi_req(void)
{
    wws_msg_get_rssi_req_t msg;

    msg.id = WWS_MSG_ID_GET_RSSI_REQ;
    wws_send_data(&msg, sizeof(msg), true);

    DBGLOG_WWS_DBG("app_wws_send_get_rssi_req\n");
}

static void app_wws_send_get_rssi_rsp(void)
{
    wws_msg_get_rssi_rsp_t msg;
    const link_quality_t *quality;

    msg.id = WWS_MSG_ID_GET_RSSI_RSP;

    quality = app_bt_get_link_quality();

    msg.phone_rssi = quality->plink_rssi_avg;
    msg.wws_rssi = quality->rlink_rssi_avg;

    wws_send_data(&msg, sizeof(msg), true);

    DBGLOG_WWS_DBG("app_wws_send_get_rssi_rsp phone:%d wws:%d\n", msg.phone_rssi, msg.wws_rssi);
}

void app_wws_start_pair(uint16_t vid, uint16_t pid, uint8_t magic)
{
    if (!app_bt_send_tws_pair_cmd(vid, pid, magic, TWS_PAIR_TIMEOUT_MS)) {
        memset(&context->peer_addr, 0, sizeof(context->peer_addr));
    } else {
        DBGLOG_WWS_ERR("app_wws_start_pair failed\n");
    }
}

void app_wws_send_usr_evt(uint16_t evt)
{
    wws_msg_usr_evt_t msg;

    msg.id = WWS_MSG_ID_USR_EVT;
    msg.evt = evt;
    msg.state = app_bt_get_sys_state();
    wws_send_data(&msg, sizeof(msg), true);
    DBGLOG_WWS_DBG("app_wws_send_usr_evt %d\n", evt);
}

void app_wws_send_in_ear(bool_t enabled, bool_t in_ear)
{
    wws_msg_in_ear_t msg;

    msg.id = WWS_MSG_ID_IN_EAR;
    msg.enabled = enabled ? 1 : 0;
    msg.in_ear = in_ear ? 1 : 0;
    wws_send_data(&msg, sizeof(msg), true);

    DBGLOG_WWS_DBG("app_wws_send_in_ear %d %d\n", enabled, in_ear);

    wws_triger_role_switch(ROLE_SWITCH_REASON_IN_EAR);
}

void app_wws_send_in_ear_enable_request(bool_t enable)
{
    wws_msg_inear_enable_req_t msg;

    msg.id = WWS_MSG_ID_INEAR_ENABLE_REQ;
    msg.enable = enable ? 1 : 0;
    wws_send_data(&msg, sizeof(msg), true);

    DBGLOG_WWS_DBG("app_wws_send_in_ear_enable_request %d\n", enable);
}

void app_wws_send_charger(bool_t charging)
{
    wws_msg_charger_t msg;

    msg.id = WWS_MSG_ID_CHARGER;
    msg.charging = charging ? 1 : 0;
    wws_send_data(&msg, sizeof(msg), true);

    DBGLOG_WWS_DBG("app_wws_send_charger %d\n", charging);

    wws_triger_role_switch(ROLE_SWITCH_REASON_CHARGING);
}

void app_wws_send_battery(uint8_t level)
{
    wws_msg_battery_t msg;

    msg.id = WWS_MSG_ID_BATTERY;
    msg.level = level;
    wws_send_data(&msg, sizeof(msg), false);

    DBGLOG_WWS_DBG("app_wws_send_battery %d\n", level);

    if (TWS_BATTERY_ROLE_SWITCH_ENABLED) {
        wws_triger_role_switch(ROLE_SWITCH_REASON_BATTERY);
    }
}

void app_wws_send_box_state(uint8_t box_state)
{
    wws_msg_box_state_t msg;

    msg.id = WWS_MSG_ID_BOX_STATE;
    msg.box_state = box_state;
    wws_send_data(&msg, sizeof(msg), false);

    DBGLOG_WWS_DBG("app_wws_send_box_state %d\n", box_state);
}

void app_wws_send_reset_pdl(void)
{
    wws_msg_reset_pdl_t msg;

    msg.id = WWS_MSG_ID_RESET_PDL;
    wws_send_data(&msg, sizeof(msg), true);

    DBGLOG_WWS_DBG("app_wws_send_reset_pdl\n");
}

void app_wws_send_volume(uint8_t call, uint8_t music)
{
    wws_msg_volume_t msg;

    msg.id = WWS_MSG_ID_VOLUME;
    msg.call = call;
    msg.music = music;
    wws_send_data(&msg, sizeof(msg), true);

    DBGLOG_WWS_DBG("app_wws_send_volume %d %d\n", call, music);
}

void app_wws_send_listen_mode(uint8_t mode)
{
    wws_msg_listen_mode_t msg;

    msg.id = WWS_MSG_ID_LISTEN_MODE;
    msg.mode = mode;

    wws_send_data(&msg, sizeof(msg), true);

    DBGLOG_WWS_DBG("app_wws_send_listen_mode %d\n", mode);
}

void app_wws_send_game_mode_set_enabled(bool_t enabled)
{
    wws_msg_game_mode_t msg;

    msg.id = WWS_MSG_ID_GAME_MODE;
    msg.enabled = enabled;

    wws_send_data(&msg, sizeof(msg), true);

    DBGLOG_WWS_DBG("app_wws_send_game_mode_set_enabled %d\n", enabled);
}

void app_wws_send_anc_level(uint8_t level)
{
    wws_msg_anc_level_t msg;

    msg.id = WWS_MSG_ID_ANC_LEVEL;
    msg.level = level;

    wws_send_data(&msg, sizeof(msg), true);

    DBGLOG_WWS_DBG("app_wws_send_anc_level %d\n", level);
}

void app_wws_send_transparency_level(uint8_t level)
{
    wws_msg_transparency_level_t msg;

    msg.id = WWS_MSG_ID_TRANSPARENCY_LEVEL;
    msg.level = level;

    wws_send_data(&msg, sizeof(msg), true);

    DBGLOG_WWS_DBG("app_wws_send_transparency_level %d\n", level);
}

void app_wws_send_set_cus_key(uint8_t type, uint16_t state_mask, uint16_t event)
{
    wws_msg_set_key_config_t msg;

    msg.id = WWS_MSG_ID_SET_KEY_CONFIG;
    msg.key_type = type;
    msg.state_mask = state_mask;
    msg.event = event;

    wws_send_data(&msg, sizeof(msg), false);

    DBGLOG_WWS_DBG("app_wws_send_set_cus_key type:%d event:%d\n", type, event);
}

void app_wws_send_get_cus_key(uint8_t count, const uint8_t *type, const uint16_t *state_mask,
                              void (*callback)(uint8_t count, const uint16_t *events))
{
    wws_msg_get_key_config_t *msg;

    assert(count <= 8);

    if (context->state != TWS_STATE_CONNECTED) {
        DBGLOG_WWS_DBG("app_wws_send_get_cus_key error: not connected\n");
        uint16_t *events = os_mem_malloc(IOT_APP_MID, sizeof(uint16_t) * count);
        memset(events, 0, sizeof(uint16_t) * count);
        callback(count, events);
        os_mem_free(events);
        return;
    }

    if (context->key_config_callback != NULL) {
        DBGLOG_WWS_ERR("app_wws_send_get_cus_key already ongoing\n");
    }
    context->key_config_callback = callback;

    msg = os_mem_malloc(IOT_APP_MID,
                        sizeof(wws_msg_get_key_config_t) + sizeof(struct key_state) * count);
    msg->id = WWS_MSG_ID_GET_KEY_CONFIG;
    msg->count = count;
    for (uint8_t i = 0; i < count; i++) {
        msg->key_states[i].key_type = type[i];
        msg->key_states[i].state_mask = state_mask[i];
    }

    wws_send_data(msg, sizeof(wws_msg_get_key_config_t) + sizeof(struct key_state) * count, true);
    os_mem_free(msg);

    DBGLOG_WWS_DBG("app_wws_send_get_cus_key count:%d\n", count);
}

void app_wws_send_reset_cus_key(void)
{
    wws_msg_reset_key_config_t msg;

    msg.id = WWS_MSG_ID_RESET_KEY_CONFIG;

    wws_send_data(&msg, sizeof(msg), false);

    DBGLOG_WWS_DBG("app_wws_send_reset_cus_key\n");
}

void app_wws_send_visibility(bool_t discoverable, bool_t connectable)
{
    wws_msg_visibility_t msg;

    memset(&msg, 0, sizeof(msg));

    msg.id = WWS_MSG_ID_VISIBILITY;
    if (discoverable) {
        msg.discoverable = 1;
    }
    if (connectable) {
        msg.connectable = 1;
    }

    wws_send_data(&msg, sizeof(msg), false);

    DBGLOG_WWS_DBG("app_wws_send_visibility discoverable:%d connectable:%d\n", discoverable,
                   connectable);
}

void app_wws_send_vol_set_request(uint8_t call, uint8_t music)
{
    wws_msg_vol_set_req_t msg;

    memset(&msg, 0, sizeof(msg));

    msg.id = WWS_MSG_ID_VOLUME_SET_REQ;
    msg.music_lvl = music;
    msg.call_lvl = call;

    wws_send_data(&msg, sizeof(msg), true);

    DBGLOG_WWS_DBG("app_wws_send_vol_set_request call: %d, music: %d\n", call, music);
}

void app_wws_send_listen_mode_set_request(listen_mode_t mode)
{
    wws_msg_listen_mode_set_req_t msg;

    memset(&msg, 0, sizeof(msg));

    msg.id = WWS_MSG_ID_LISTEN_MODE_SET_REQ;
    msg.mode = mode;

    wws_send_data(&msg, sizeof(msg), true);

    DBGLOG_WWS_DBG("app_wws_send_listen_mode_set_request listen_mode: %d\n", mode);
}

void app_wws_send_pdl_clear_request(void)
{
    wws_msg_pdl_clear_req_t msg;

    memset(&msg, 0, sizeof(msg));

    msg.id = WWS_MSG_ID_PDL_CLEAR_REQ;

    wws_send_data(&msg, sizeof(msg), true);

    DBGLOG_WWS_DBG("wws_msg_pdl_clear_req_t\n");
}

void app_wws_send_remote_msg(app_msg_type_t type, uint16_t id, uint16_t param_len,
                             const uint8_t *param)
{
    wws_msg_remote_msg_t *msg;

    if (param_len > MAX_REMOTE_MSG_PARAM_LEN) {
        DBGLOG_WWS_ERR("app_wws_send_remote_msg param_len:%d error\n", param_len);
        return;
    }

    msg = os_mem_malloc(IOT_APP_MID, sizeof(wws_msg_remote_msg_t) + param_len);
    memset(msg, 0, sizeof(wws_msg_remote_msg_t) + param_len);

    msg->id = WWS_MSG_ID_REMOTE_MSG;
    msg->msg_type = type;
    msg->msg_id = id;
    msg->param_len = param_len;
    if (param && param_len) {
        memcpy(msg->param, param, param_len);
    }
    wws_send_data(msg, sizeof(wws_msg_remote_msg_t) + param_len, true);

    DBGLOG_WWS_DBG("app_wws_send_remote_msg type:%d id:%d param_len:%d\n", type, id, param_len);

    os_mem_free(msg);
}

void app_wws_send_remote_msg_no_activate(app_msg_type_t type, uint16_t id, uint16_t param_len,
                                         const uint8_t *param)
{
    wws_msg_remote_msg_t *msg;

    if (param_len > MAX_REMOTE_MSG_PARAM_LEN) {
        DBGLOG_WWS_ERR("app_wws_send_remote_msg_no_activate param_len:%d error\n", param_len);
        return;
    }

    msg = os_mem_malloc(IOT_APP_MID, sizeof(wws_msg_remote_msg_t) + param_len);
    memset(msg, 0, sizeof(wws_msg_remote_msg_t) + param_len);

    msg->id = WWS_MSG_ID_REMOTE_MSG;
    msg->msg_type = type;
    msg->msg_id = id;
    msg->param_len = param_len;
    if (param && param_len) {
        memcpy(msg->param, param, param_len);
    }
    wws_send_data(msg, sizeof(wws_msg_remote_msg_t) + param_len, false);

    DBGLOG_WWS_DBG("app_wws_send_remote_msg_no_activate type:%d id:%d param_len:%d\n", type, id,
                   param_len);

    os_mem_free(msg);
}

void app_wws_send_remote_msg_ext(app_msg_type_t type, uint16_t id, uint8_t *param,
                                 uint16_t param_data_len, uint8_t param_data_offset)
{
    assert(param_data_offset >= sizeof(wws_msg_remote_msg_t) && param != NULL);
    uint8_t param_offset = param_data_offset - sizeof(wws_msg_remote_msg_t);
    wws_msg_remote_msg_t *msg = (wws_msg_remote_msg_t *)&param[param_offset];
    msg->id = WWS_MSG_ID_REMOTE_MSG;
    msg->msg_type = type;
    msg->msg_id = id;
    msg->param_len = param_data_len;
    wws_send_data(msg, sizeof(wws_msg_remote_msg_t) + param_data_len, true);

    DBGLOG_WWS_DBG("app_wws_send_remote_msg_ext type:%d id:%d param_len:%d\n", type, id,
                   param_data_len);
}

void app_wws_init(void)
{
    app_register_msg_handler(MSG_TYPE_WWS, app_wws_handle_msg);
}

void app_wws_deinit(void)
{
}

void app_wws_role_switch(void)
{
    bt_cmd_tws_role_switch_t param;
    int ret;

    ret = app_bt_send_rpc_cmd(BT_CMD_TWS_ROLE_SWITCH, &param, sizeof(param));

    DBGLOG_WWS_DBG("app_wws_role_switch ret=%d\n", ret);

    context->last_role_switch_time_ms = os_boot_time32();
}

bool_t app_wws_is_connected(void)
{
    return context->state >= TWS_STATE_CONNECTED;
}

bool_t app_wws_is_slave(void)
{
    return context->role == TWS_ROLE_SLAVE;
}

bool_t app_wws_is_master(void)
{
    return context->role == TWS_ROLE_MASTER || context->role == TWS_ROLE_UNKNOWN;
}

bool_t app_wws_is_left(void)
{
    return context->channel == TWS_CHANNEL_LEFT;
}

bool_t app_wws_is_right(void)
{
    return context->channel == TWS_CHANNEL_RIGHT;
}

bool_t app_wws_is_enabled(void)
{
    return true;
}

bool_t app_wws_is_connected_slave(void)
{
    return app_wws_is_connected() && app_wws_is_slave();
}

bool_t app_wws_is_connected_master(void)
{
    return app_wws_is_connected() && app_wws_is_master();
}

bool_t app_wws_is_pairing(void)
{
    return context->state == TWS_STATE_PAIRING;
}

bool_t app_wws_is_connecting(void)
{
    return context->state == TWS_STATE_CONNECTING;
}

bool_t app_wws_peer_is_inear(void)
{
    return context->peer_inear;
}

bool_t app_wws_peer_is_charging(void)
{
    return context->peer_charging;
}

box_state_t app_wws_peer_get_box_state(void)
{
    return context->peer_box_state;
}

uint8_t app_wws_peer_get_battery_level(void)
{
    return context->peer_battery;
}

const BD_ADDR_T *app_wws_get_peer_addr(void)
{
    return &context->peer_addr;
}

uint16_t app_wws_peer_get_fw_ver_build(void)
{
    return context->peer_fw_ver_build;
}

uint32_t app_wws_peer_get_fw_in_kv(void)
{
    return context->peer_fw_ver_in_kv;
}

bool_t app_wws_peer_is_discoverable(void)
{
    return context->peer_discoverable;
}

bool_t app_wws_peer_is_connectable(void)
{
    return context->peer_connectable;
}

void app_wws_handle_role_changed(uint8_t new_role, BD_ADDR_T *peer_addr,
                                 tws_role_changed_reason_t reason)
{
    DBGLOG_WWS_DBG("app_wws_handle_role_changed from %d to %d, reason:%d\n", context->role,
                   new_role, reason);
    if (new_role != context->role) {
        context->role = (bt_tws_role_t)new_role;
        app_evt_send(EVTSYS_WWS_ROLE_SWITCH);
    }

    if (peer_addr) {
        memcpy(&context->peer_addr, peer_addr, sizeof(BD_ADDR_T));
    } else {
        DBGLOG_WWS_ERR("peer address is NULL\n");
    }

    app_econn_handle_tws_role_changed(context->role == TWS_ROLE_MASTER);
    app_conn_handle_wws_role_changed(context->role == TWS_ROLE_MASTER, reason);
    app_led_handle_wws_role_changed(context->role == TWS_ROLE_MASTER);

    if (new_role == TWS_ROLE_MASTER) {
        app_bt_report_battery_level(app_bat_get_level());
    }

    context->last_role_switch_time_ms = os_boot_time32();
}

void app_wws_handle_state_changed(uint8_t state, uint8_t reason, BD_ADDR_T *peer_addr)
{
    bt_tws_state_t prev_state;

    prev_state = context->state;
    context->state = (bt_tws_state_t)state;
    DBGLOG_WWS_DBG("app_wws_handle_state_changed from %d to %d, reason:%d\n", prev_state, state,
                   reason);
    if ((state == TWS_STATE_CONNECTED) && (prev_state < TWS_STATE_CONNECTED)) {
        if (peer_addr) {
            memcpy(&context->peer_addr, peer_addr, sizeof(BD_ADDR_T));
        } else {
            DBGLOG_WWS_ERR("peer address is NULL\n");
        }
        DBGLOG_WWS_DBG("[auto]tws connected, peer_addr:");
        bdaddr_print(&context->peer_addr);
        wws_connected_sync();
        app_evt_send(EVTSYS_WWS_CONNECTED);
        app_wqota_handle_tws_state_changed(true);
    } else if ((state == TWS_STATE_DISCONNECTED) && (prev_state >= TWS_STATE_CONNECTED)) {
        if ((app_bt_get_sys_state() <= STATE_DISABLED) || app_bt_is_disabling()) {
            DBGLOG_WWS_DBG("wws disconnect ignored when disabled\n");
        } else {
            app_evt_send(EVTSYS_WWS_DISCONNECTED);
        }
        app_econn_handle_tws_state_changed(false);
        app_wqota_handle_tws_state_changed(false);
        app_led_handle_wws_state_changed(false);
    }
}

void app_wws_handle_recv_data(uint8_t *data, uint16_t len)
{
    if (!data) {
        DBGLOG_WWS_ERR("app_wws_handle_recv_data data==NULL\n");
        return;
    }

    if (len < sizeof(wws_msg_t)) {
        DBGLOG_WWS_ERR("app_wws_handle_recv_data len:%d error\n", len);
        return;
    }

    wws_msg_t *msg = (wws_msg_t *)data;

    switch (msg->id) {
        case WWS_MSG_ID_CONNECTED_SYNC:
            wws_handle_connected_sync(data, len);
            break;
        case WWS_MSG_ID_USR_EVT:
            wws_handle_usr_evt(data, len);
            break;
        case WWS_MSG_ID_IN_EAR:
            wws_handle_in_ear(data, len);
            break;
        case WWS_MSG_ID_INEAR_ENABLE_REQ:
            wws_handle_inear_enable_req(data, len);
            break;
        case WWS_MSG_ID_CHARGER:
            wws_handle_charger(data, len);
            break;
        case WWS_MSG_ID_BATTERY:
            wws_handle_battery(data, len);
            break;
        case WWS_MSG_ID_BOX_STATE:
            wws_handle_box_state(data, len);
            break;
        case WWS_MSG_ID_RESET_PDL:
            wws_handle_reset_pdl(data, len);
            break;
        case WWS_MSG_ID_VOLUME:
            wws_handle_volume(data, len);
            break;
        case WWS_MSG_ID_CALL_VOLUME:
            wws_handle_call_volume(data, len);
            break;
        case WWS_MSG_ID_MUSIC_VOLUME:
            wws_handle_music_volume(data, len);
            break;
        case WWS_MSG_ID_LISTEN_MODE:
            wws_handle_listen_mode(data, len);
            break;
        case WWS_MSG_ID_GAME_MODE:
            wws_handle_game_mode(data, len);
            break;
        case WWS_MSG_ID_ANC_LEVEL:
            wws_handle_anc_level(data, len);
            break;
        case WWS_MSG_ID_TRANSPARENCY_LEVEL:
            wws_handle_transparency_level(data, len);
            break;
        case WWS_MSG_ID_SET_KEY_CONFIG:
            wws_handle_set_key_config(data, len);
            break;
        case WWS_MSG_ID_GET_KEY_CONFIG:
            wws_handle_get_key_config(data, len);
            break;
        case WWS_MSG_ID_GET_KEY_CONFIG_RSP:
            wws_handle_get_key_config_rsp(data, len);
            break;
        case WWS_MSG_ID_RESET_KEY_CONFIG:
            wws_handle_reset_key_config(data, len);
            break;
        case WWS_MSG_ID_VISIBILITY:
            wws_handle_visibility(data, len);
            break;
        case WWS_MSG_ID_VOLUME_SET_REQ:
            wws_handle_vol_set_req(data, len);
            break;
        case WWS_MSG_ID_LISTEN_MODE_SET_REQ:
            wws_handle_listen_mode_set_req(data, len);
            break;
        case WWS_MSG_ID_PDL_CLEAR_REQ:
            wws_handle_pdl_clear_req(data, len);
            break;
        case WWS_MSG_ID_VOLUME_SYNC_REQ:
            wws_handle_volume_sync_req(data, len);
            break;
        case WWS_MSG_ID_REMOTE_MSG:
            wws_handle_remote_msg(data, len);
            break;
        case WWS_MSG_ID_GET_RSSI_REQ:
            app_wws_send_get_rssi_rsp();
            break;
        case WWS_MSG_ID_GET_RSSI_RSP:
            wws_handle_get_rssi_rsp(data, len);
            break;
        default:
            DBGLOG_WWS_ERR("app_wws_handle_recv_data id:%d error\n", msg->id);
            break;
    }

    os_mem_free(data);
}

void app_wws_handle_tds_data(BD_ADDR_T *remote_addr, uint8_t *data, uint16_t len)
{
    wws_tds_data_t *tds_data;

    if (!data) {
        DBGLOG_WWS_ERR("app_wws_handle_tds_data data==NULL\n");
        return;
    }

    if (len != sizeof(wws_tds_data_t)) {
        DBGLOG_WWS_ERR("app_wws_handle_tds_data len:%d error\n", len);
        return;
    }

    tds_data = (wws_tds_data_t *)data;

    DBGLOG_WWS_DBG("app_wws_handle_tds_data music_vol:%d call_vol:%d\n", tds_data->music_volume,
                   tds_data->call_volume);

    if (!bdaddr_is_zero(remote_addr)) {
        usr_cfg_pdl_add(remote_addr);
    }
    app_audio_music_volume_set(tds_data->music_volume);
    app_audio_call_volume_set(tds_data->call_volume);

    os_mem_free(data);

    wws_msg_volume_sync_req_t msg;
    msg.id = WWS_MSG_ID_VOLUME_SYNC_REQ;
    msg.call_volume = tds_data->call_volume;
    msg.music_volume = tds_data->music_volume;
    wws_send_data(&msg, sizeof(msg), true);
}

void app_wws_handle_bt_inited(void)
{
    bt_cmd_tws_get_state_t param;
    bt_result_t ret;

    ret = app_bt_send_rpc_cmd((uint32_t)BT_CMD_TWS_GET_STATE, &param, sizeof(param));

    if (ret) {
        DBGLOG_WWS_DBG("BT_CMD_TWS_GET_STATE ret:%d\n", ret);
        return;
    }

    context->state = param.state;
    context->channel = param.channel;
    context->role = param.role;

    DBGLOG_WWS_DBG("tws get state succeed state:%d channel:%d role:%d\n", param.state,
                   param.channel, param.role);
}

void app_wws_handle_bt_power_on(void)
{
    bt_cmd_tws_get_peer_addr_t cmd;

    wws_set_tds_data();

    app_wws_handle_bt_inited();

    if (app_bt_send_rpc_cmd(BT_CMD_TWS_GET_PEER_ADDR, &cmd, sizeof(cmd))) {
        DBGLOG_WWS_ERR("get peer addr failed\n");
        return;
    }

    if (!bdaddr_is_equal(&cmd.addr, &auto_tws_pair_peer_bdaddr)) {
        context->peer_addr = cmd.addr;
        return;
    }

    if (!ro_sys_cfg()->features.stereo_mode) {
        app_wws_start_pair(app_econn_get_vid(), app_econn_get_pid(),
                           app_econn_get_tws_pair_magic());
    }
}

void app_wws_handle_bt_power_off(void)
{
    context->state = TWS_STATE_DISABLED;
}

void app_wws_handle_volume_changed(void)
{
    wws_set_tds_data();
}

void app_wws_handle_sys_state(uint32_t sys_state)
{
    if (sys_state < STATE_A2DP_STREAMING) {
        if (context->rssi_detect_enabled) {
            app_cancel_msg(MSG_TYPE_WWS, APP_WWS_MSG_ID_RSSI_DETECT);
            context->rssi_detect_enabled = 0;
            DBGLOG_WWS_DBG("app_wws_handle_sys_state rssi detect disabled\n");
        }
    } else {
        if (!context->rssi_detect_enabled) {
            context->rssi_detect_enabled = 1;
            app_send_msg_delay(MSG_TYPE_WWS, APP_WWS_MSG_ID_RSSI_DETECT, NULL, 0,
                               TWS_RSSI_DETECT_INTERVAL_MS);
            DBGLOG_WWS_DBG("app_wws_handle_sys_state rssi detect enabled\n");
        }
    }
}

void app_wws_handle_bt_connected(void)
{
    context->last_role_switch_time_ms = os_boot_time32();
    wws_triger_role_switch(ROLE_SWITCH_REASON_CONNECTED);
}

void app_wws_handle_mode_changed(bool_t single_mode)
{
    if ((!single_mode) && (app_wws_is_master())) {
        app_wws_send_listen_mode(usr_cfg_get_listen_mode());
    }
}
