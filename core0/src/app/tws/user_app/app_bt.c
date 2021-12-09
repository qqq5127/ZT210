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

#include "bt_rpc_api.h"
#include "assert.h"
#include "app_bt.h"
#include "string.h"
#include "os_mem.h"
#include "app_main.h"
#include "app_evt.h"
#include "ro_cfg.h"
#include "usr_cfg.h"
#include "app_conn.h"
#include "app_pm.h"
#include "app_wws.h"
#include "app_audio.h"
#include "app_gatts.h"
#include "app_adv.h"
#include "app_cli.h"
#include "app_spp.h"
#include "app_l2cap.h"
#include "app_tone.h"
#include "app_bat.h"
#include "app_inear.h"
#include "app_sdp.h"
#include "os_utils.h"
#include "iot_memory_config.h"

#ifndef APP_BT_AVRCP_VOLUME_ANTI_SHAKE_ENABLED
#define APP_BT_AVRCP_VOLUME_ANTI_SHAKE_ENABLED 0
#endif

#ifndef APP_BT_A2DP_STREAMING_AVRCP_PAUSE_SYS_STATE
#define APP_BT_A2DP_STREAMING_AVRCP_PAUSE_SYS_STATE STATE_CONNECTED
#endif

#ifndef APP_BT_SCO_TIMEOUT_ACTIVE_CALL_MS
#define APP_BT_SCO_TIMEOUT_ACTIVE_CALL_MS 1000
#endif

#ifndef APP_BT_AVRCP_VOLUME_ANTI_SHAKE_MS
#define APP_BT_AVRCP_VOLUME_ANTI_SHAKE_MS 200
#endif

#ifndef APP_BT_RING_PLAY_DELAY_MS
#define APP_BT_RING_PLAY_DELAY_MS 500
#endif

#ifndef APP_BT_INVALID_HFP_VOLUME_MUTE_STEP
#define APP_BT_INVALID_HFP_VOLUME_MUTE_STEP 16
#endif

#define APP_BT_MSG_ID_REPORT_BATTERY_AND_CALL_VOLUME 1000
#define APP_BT_MSG_ID_CONNECT_SCO                    1001
#define APP_BT_MSG_ID_HANDLE_AVRCP_VOLUME            1002

#define DEEPCOPY(type, data_field, len_field, ptr)                                        \
    ({                                                                                    \
        type *old_var = (type *)ptr;                                                      \
        type *new_var = os_mem_malloc(IOT_APP_MID, sizeof(type));                         \
        if (new_var) {                                                                    \
            memcpy(new_var, old_var, sizeof(type));                                       \
            if (old_var->len_field) {                                                     \
                new_var->data_field = os_mem_malloc(IOT_APP_MID, old_var->len_field);     \
                if (!new_var->data_field) {                                               \
                    DBGLOG_BT_ERR("DEEPCOPY alloc for data failed\n");                    \
                } else {                                                                  \
                    memcpy(new_var->data_field, old_var->data_field, old_var->len_field); \
                }                                                                         \
            } else {                                                                      \
                new_var->data_field = NULL;                                               \
            }                                                                             \
        } else {                                                                          \
            DBGLOG_BT_ERR("DEEPCOPY alloc for var failed\n");                             \
        }                                                                                 \
        new_var;                                                                          \
    })

typedef struct {
    /** system state of bt, see STATE_XXX for detail */
    uint32_t sys_state;
    /**enabled state setted by user*/
    bool_t user_enabled;
    /** bt is enabled(opened) or not */
    bool_t enabled;
    /** bt enable(open) is pending because bt core is not ready */
    bool_t enable_pending;
    /* disable is running */
    bool_t disable_running;
    /** dut mode */
    bool_t in_dut_mode;
    /** factory test mode */
    bool_t in_ft_mode;
    /** audio test mode */
    bool_t in_audio_test_mode;
    /** sniff mode */
    bool_t in_sniff_mode;
    /** tws pairing mode */
    bool_t tws_pairing;
    /** current connection state, see CONNECTION_STATE_XXX for detail */
    bt_connection_state_t conn_state;
    /** bluetooth address of current connecting or connected device */
    BD_ADDR_T remote_addr;
    /** local bluetooth address */
    BD_ADDR_T local_addr;
    /** local device name */
    char local_name[MAX_NAME_LEN];
    /* auth failed bluetooth address */
    BD_ADDR_T auth_fail_addr;
    /** current hfp state, see HFP_STATE_XXX for detail */
    bt_hfp_state_t hfp_state;
    /** current a2dp state, see A2DP_STATE_XXX for detail */
    bt_a2dp_state_t a2dp_state;
    /** current avrcp state, see AVRCP_STATE_XXX for detail */
    bt_avrcp_state_t avrcp_state;
    /** sco connection connected or not */
    bool_t sco_connected;
    /** local can be discovered or not */
    bool_t visible;
    /** local can be connected or not */
    bool_t connectable;
    /** current recognition state */
    bool_t recognition_activated;
    /** avrcp volume which is pending by anti shake */
    uint8_t pending_avrcp_volume;
    /** connected event has been reported or not for this acl connection */
    bool_t connected_evt_reported;
    /* force discoverable when phone not connected */
    bool_t force_discoverable;
    /** play_pause action as pause */
    bool_t play_pause_as_pause;
    /** last avrcp volume change time */
    uint32_t avrcp_last_time;
} app_bt_context_t;

static app_bt_context_t _context = {0};
static app_bt_context_t *context = &_context;

/**
 * @brief check if bt core is inited or not
 *
 * @return true when bt core is ready, false when bt core is not ready
 */
static bool_t is_bt_rpc_ready(void)
{
    return (context->sys_state >= STATE_DISABLED);
}

/**
 * @brief generate the bt system state STATE_XXX based on
 *        the state of each profile
 */
static void generate_sys_state(void)
{
    uint32_t new_state = 0;
    bool_t visible;
    bool_t connectable;

    if (app_wws_is_connected_slave() && ro_sys_cfg()->features.sync_sys_state) {
        visible = app_wws_peer_is_discoverable();
        connectable = app_wws_peer_is_connectable();
    } else {
        visible = context->visible;
        connectable = context->connectable;
    }

    if (!context->enabled) {
        new_state = STATE_DISABLED;
    } else if (context->tws_pairing) {
        new_state = STATE_WWS_PAIRING;
    } else if (context->conn_state == CONNECTION_STATE_DISABLED) {
        new_state = STATE_DISABLED;
    } else if (context->conn_state < CONNECTION_STATE_CONNECTED) {
        if (visible) {
            if (connectable) {
                new_state = STATE_AG_PAIRING;
            } else {
                new_state = STATE_CONNECTABLE;
            }
        } else {
            if (connectable) {
                new_state = STATE_CONNECTABLE;
            } else {
                new_state = STATE_IDLE;
            }
        }
    } else {
        if (context->hfp_state == HFP_STATE_ON_HELD_NO_ACTIVE) {
            new_state = STATE_ACTIVE_CALL;
        } else if (context->hfp_state == HFP_STATE_TWC_CALL_ON_HELD) {
            new_state = STATE_TWC_CALL_ON_HELD;
        } else if (context->hfp_state == HFP_STATE_TWC_CALL_WAITING) {
            new_state = STATE_TWC_CALL_WAITING;
        } else if (context->hfp_state == HFP_STATE_ACTIVE_CALL) {
            new_state = STATE_ACTIVE_CALL;
        } else if (context->hfp_state == HFP_STATE_OUTGOING_CALL) {
            new_state = STATE_OUTGOING_CALL;
        } else if (context->hfp_state == HFP_STATE_INCOMING_CALL) {
            new_state = STATE_INCOMING_CALL;
        } else {
            if (context->a2dp_state == A2DP_STATE_STREAMING) {
                if (context->avrcp_state == AVRCP_STATE_PLAYING) {
                    new_state = STATE_A2DP_STREAMING;
                } else {
                    new_state = APP_BT_A2DP_STREAMING_AVRCP_PAUSE_SYS_STATE;
                }
            } else {
                new_state = STATE_CONNECTED;
            }
        }
    }

    if (new_state != context->sys_state) {
        context->sys_state = new_state;
        app_evt_send_with_param(EVTSYS_STATE_CHANGED, &new_state, sizeof(new_state));
        app_wws_handle_sys_state(new_state);
    }
}

static void read_local_name_from_bt(void)
{
    bt_cmd_get_local_name_t param;

    if (app_bt_send_rpc_cmd(BT_CMD_GET_LOCAL_NAME, &param, sizeof(param))) {
        DBGLOG_BT_ERR("get local name failed\n");
    } else {
        memset(context->local_name, 0, MAX_NAME_LEN);
        strlcpy(context->local_name, param.name, MAX_NAME_LEN);
    }
}

static void bt_evt_enable_state_changed_handler(bt_evt_enable_state_changed_t *param)
{
    bool_t prev_enabled = context->enabled;

    DBGLOG_BT_DBG("BT_EVT_ENABLE_STATE_CHANGED enabled:%d\n", param->enabled);

    context->disable_running = false;
    context->enabled = param->enabled;
    if (param->enabled) {
        context->conn_state = CONNECTION_STATE_DISCONNECTED;
        context->hfp_state = HFP_STATE_DISCONNECTED;
        context->a2dp_state = A2DP_STATE_DISCONNECTED;
        context->avrcp_state = AVRCP_STATE_DISCONNECTED;
    }

    if (param->enabled && (!context->user_enabled)) {
        DBGLOG_BT_ERR("bt disabled when enable running\n");
        app_bt_power_off();
        return;
    }

    if (!is_bt_rpc_ready()) {
        bt_cmd_get_local_addr_t param;
        if (app_bt_send_rpc_cmd(BT_CMD_GET_LOCAL_ADDR, &param, sizeof(param))) {
            DBGLOG_BT_ERR("get local address failed\n");
        } else {
            memcpy(&context->local_addr, &param.addr, sizeof(BD_ADDR_T));
        }
        app_wws_handle_bt_inited();
    }

    //move EVTSYS_POWER_ON ahead of EVTSYS_STATE_CHANGED
    if (prev_enabled && (!param->enabled)) {
        app_evt_send(EVTSYS_BT_POWER_OFF);
        app_bt_set_discoverable_and_connectable(false, false);
    } else if ((!prev_enabled) && param->enabled) {
        app_evt_send(EVTSYS_BT_POWER_ON);
    }

    generate_sys_state();

    if (context->enable_pending) {
        context->enable_pending = false;
        app_bt_power_on();
    } else if ((!param->enabled) && (context->user_enabled)) {
        app_bt_power_on();
    }

    if (prev_enabled && (!param->enabled)) {
        app_wws_handle_bt_power_off();
        app_conn_handle_bt_power_off();
        app_pm_handle_bt_power_off();
        app_audio_handle_bt_power_off();
    } else if ((!prev_enabled) && param->enabled) {
        char name[MAX_NAME_LEN];
        memset(name, 0, MAX_NAME_LEN);
        usr_cfg_get_local_name(name, MAX_NAME_LEN);
        if (strlen(name)) {
            app_bt_set_local_name(name);
        }
        read_local_name_from_bt();
        //app_bt_set_3m_feature_enable(false);
        app_wws_handle_bt_power_on();
        app_conn_handle_bt_power_on();
        app_pm_handle_bt_power_on();
        app_gatts_handle_bt_power_on();
        app_adv_handle_bt_power_on();
        app_spp_handle_bt_power_on();
        app_l2cap_handle_bt_power_on();
        app_audio_handle_bt_power_on();
    }
}

static void bt_evt_factory_reset_done_handler(bt_evt_factory_reset_done_t *param)
{
    UNUSED(param);

    DBGLOG_BT_DBG("BT_EVT_FACTORY_RESET_DONE\n");

    app_evt_send(EVTSYS_FACTORY_RESET_DONE);
}

static void bt_evt_enter_dut_mode_handler(bt_evt_enter_dut_mode_t *param)
{
    UNUSED(param);

    DBGLOG_BT_DBG("BT_EVT_ENTER_DUT_MODE\n");

    context->in_dut_mode = true;
    //app_bt_set_3m_feature_enable(true);

    context->conn_state = CONNECTION_STATE_DISCONNECTED;
    generate_sys_state();

    app_pm_handle_bt_disconnected();
}

static void bt_evt_handle_hw_error(bt_evt_hw_error_t *param)
{
    UNUSED(param);

    DBGLOG_BT_ERR("BT_EVT_HW_ERROR user_enabled:%d error_code:%d\n", context->user_enabled,
                  param->error_code);
    if (context->user_enabled) {
        app_pm_reboot(PM_REBOOT_REASON_USER);
    } else {
        app_pm_reboot(PM_REBOOT_REASON_CHARGER);
    }
}

static void bt_evt_connection_state_changed_handler(bt_evt_connection_state_changed_t *param)
{
    bt_connection_state_t prev_state = context->conn_state;

    DBGLOG_BT_DBG("BT_EVT_CONNECTION_STATE_CHANGED from %d to %d\n", prev_state, param->state);

    if ((prev_state >= CONNECTION_STATE_CONNECTED) && (param->state < CONNECTION_STATE_CONNECTED)
        && (!bdaddr_is_equal(&context->remote_addr, &param->addr))) {
        DBGLOG_BT_ERR("unknown device disconnected, ignored\n");
        bdaddr_print(&param->addr);
        memset(&context->auth_fail_addr, 0, sizeof(BD_ADDR_T));
        return;
    }

    memcpy(&context->remote_addr, &param->addr, sizeof(BD_ADDR_T));
    context->conn_state = param->state;

    generate_sys_state();

    if ((prev_state < CONNECTION_STATE_CONNECTED) && (param->state >= CONNECTION_STATE_CONNECTED)) {
        memset(&context->auth_fail_addr, 0, sizeof(BD_ADDR_T));

        if (!context->in_dut_mode) {
            /* move to first */
            usr_cfg_pdl_add(&param->addr);

            uint8_t vol_level = usr_cfg_get_music_vol();
            app_bt_report_music_volume(vol_level);
            DBGLOG_BT_DBG("bt_evt_connection_state_changed_handler report vol lvl %d\n", vol_level);
        } else {
            DBGLOG_BT_DBG("pdl ignored for dut mode\n");
        }
        context->in_sniff_mode = false;
        context->connected_evt_reported = false;

        app_pm_handle_bt_connected();
        app_conn_handle_bt_conn_state(true, param->reason);
        app_wws_handle_bt_connected();
    } else if ((param->state < CONNECTION_STATE_CONNECTED)
               && (prev_state >= CONNECTION_STATE_CONNECTED)) {
        if ((!context->disable_running) && context->connected_evt_reported) {
            app_evt_send(EVTSYS_DISCONNECTED);
        }
        context->connected_evt_reported = false;
        app_pm_handle_bt_disconnected();
    }

    if (param->state == CONNECTION_STATE_DISCONNECTED) {
        if (bdaddr_is_equal(&param->addr, &context->auth_fail_addr)) {
            param->reason = BT_DISCONNECT_REASON_AUTH_FAIL;
        }
        memset(&context->auth_fail_addr, 0, sizeof(BD_ADDR_T));
        app_conn_handle_bt_conn_state(false, param->reason);
    }
}

static void bt_evt_connection_mode_changed_handler(bt_evt_connection_mode_changed_t *param)
{
    bt_connection_mode_t mode = param->mode;

    DBGLOG_BT_DBG("BT_EVT_CONNECTION_MODE_CHANGED mode:%d\n", mode);

    switch (mode) {
        case CONNECTION_MODE_ACTIVE:
            context->in_sniff_mode = false;
            break;
        case CONNECTION_MODE_HOLD:
            context->in_sniff_mode = false;
            break;
        case CONNECTION_MODE_SNIFF:
            context->in_sniff_mode = true;
            break;
        case CONNECTION_MODE_PARK:
            context->in_sniff_mode = false;
            break;
        default:
            break;
    }
}

static void bt_evt_visibility_changed_handler(bt_evt_visibility_changed_t *param)
{
    DBGLOG_BT_DBG("BT_EVT_VISIBILITY_CHANGED visible:%d connectable:%d\n", param->visible,
                  param->connectable);
}

static void bt_evt_local_name_changed_handler(bt_evt_local_name_changed_t *param)
{
    UNUSED(param);

    DBGLOG_BT_DBG("BT_EVT_LOCAL_NAME_CHANGED\n");
}

static void bt_evt_cod_changed_handler(bt_evt_cod_changed_t *param)
{
    UNUSED(param);

    DBGLOG_BT_DBG("BT_EVT_COD_CHANGED cod:0x%X\n", param->cod);
}

static void bt_evt_pin_code_changed_handler(bt_evt_pin_code_changed_t *param)
{
    UNUSED(param);

    DBGLOG_BT_DBG("BT_EVT_PIN_CODE_CHANGED code:%d\n", param->pin_code);
}

static void bt_evt_pair_added_handler(bt_evt_pair_added_t *param)
{
    DBGLOG_BT_DBG("BT_EVT_PAIR_ADDED\n");
    bdaddr_print(&param->addr);
    usr_cfg_pdl_add(&param->addr);
}

static void bt_evt_pair_removed_handler(bt_evt_pair_removed_t *param)
{
    DBGLOG_BT_DBG("BT_EVT_PAIR_REMOVED\n");
    bdaddr_print(&param->addr);
    usr_cfg_pdl_remove(&param->addr);
}

static void bt_evt_auth_result_handler(bt_evt_auth_result_t *param)
{
    DBGLOG_BT_DBG("BT_EVT_AUTH_RESULT %d\n", param->result);
    bdaddr_print(&param->addr);
    if (param->result) {
        memcpy(&context->auth_fail_addr, &param->addr, sizeof(BD_ADDR_T));
        if ((BT_AUTH_RESULT_AUTH_FAIL == param->result)
            || (BT_AUTH_RESULT_KEY_MISS == param->result)) {
            usr_cfg_pdl_remove(&param->addr);
        }
    } else {
        memset(&context->auth_fail_addr, 0, sizeof(BD_ADDR_T));
        usr_cfg_pdl_add(&param->addr);
    }
}

static void bt_evt_phone_type_report_handler(bt_evt_phone_type_report_t *param)
{
    DBGLOG_BT_DBG("BT_EVT_PHONE_TYPE_REPORT type:%d\n", param->type);

    usr_cfg_set_phone_type(param->type);
}

static void bt_evt_hfp_state_changed_handler(bt_evt_hfp_state_changed_t *param)
{
    bt_hfp_state_t prev_state;

    prev_state = context->hfp_state;
    context->hfp_state = param->state;

    DBGLOG_BT_DBG("BT_EVT_HFP_STATE_CHANGED from %d to %d\n", prev_state, context->hfp_state);

    if ((prev_state == HFP_STATE_INCOMING_CALL) && (param->state <= HFP_STATE_CONNECTED)) {
        app_evt_cancel(EVTSYS_RING);
        app_tone_cancel(EVTSYS_RING, true);
    }

    if ((prev_state < HFP_STATE_CONNECTED) && (param->state >= HFP_STATE_CONNECTED)) {
        if (!context->connected_evt_reported) {
            app_evt_send(EVTSYS_CONNECTED);
            context->connected_evt_reported = true;
        }
        app_send_msg_delay(MSG_TYPE_BT, APP_BT_MSG_ID_REPORT_BATTERY_AND_CALL_VOLUME, NULL, 0,
                           1500);
    }

    generate_sys_state();

    if ((param->state == HFP_STATE_ACTIVE_CALL) && (!context->sco_connected) && app_inear_get()) {
        app_send_msg_delay(MSG_TYPE_BT, APP_BT_MSG_ID_CONNECT_SCO, NULL, 0,
                           APP_BT_SCO_TIMEOUT_ACTIVE_CALL_MS);
    } else if (param->state < HFP_STATE_ACTIVE_CALL) {
        app_cancel_msg(MSG_TYPE_BT, APP_BT_MSG_ID_CONNECT_SCO);
    }
}

static void bt_evt_hfp_ring_handler(bt_evt_hfp_ring_t *param)
{
    UNUSED(param);

    DBGLOG_BT_DBG("BT_EVT_HFP_RING\n");

    if (!context->sco_connected) {
        app_tone_cancel(EVTSYS_RING, false);
        app_evt_send_delay(EVTSYS_RING, APP_BT_RING_PLAY_DELAY_MS);
    }
}

static void bt_evt_hfp_sco_state_changed_handler(bt_evt_hfp_sco_state_changed_t *param)
{
    context->sco_connected = param->connected;

    DBGLOG_BT_DBG("BT_EVT_HFP_SCO_STATE_CHANGED sco:%d\n", param->connected);

    if (param->connected) {
        app_cancel_msg(MSG_TYPE_BT, APP_BT_MSG_ID_CONNECT_SCO);
        app_evt_send(EVTSYS_SCOLINK_OPEN);
        app_evt_cancel(EVTSYS_RING);
        app_tone_cancel(EVTSYS_RING, true);
    } else {
        app_evt_send(EVTSYS_SCOLINK_CLOSE);
    }
}

static void bt_evt_hfp_signal_changed_handler(bt_evt_hfp_signal_changed_t *param)
{
    UNUSED(param);

    DBGLOG_BT_DBG("BT_EVT_HFP_SIGNAL_CHANGED signal:%d\n", param->signal);
}

static void bt_evt_hfp_battery_level_changed_handler(bt_evt_hfp_battery_level_changed_t *param)
{
    UNUSED(param);

    DBGLOG_BT_DBG("BT_EVT_HFP_BATTERY_LEVEL_CHANGED level:%d\n", param->level);
}

static void bt_evt_hfp_volume_changed_handler(bt_evt_hfp_volume_changed_t *param)
{
    uint8_t cur_volume = usr_cfg_get_call_vol();
    DBGLOG_BT_DBG("BT_EVT_HFP_VOLUME_CHANGED volume:%d\n", param->volume);

    if ((param->volume == 0) && (cur_volume >= APP_BT_INVALID_HFP_VOLUME_MUTE_STEP)) {
        DBGLOG_BT_DBG("invalid hfp volume from %d to 0\n", cur_volume);
        return;
    }

    app_audio_call_volume_set(param->volume);
}

static void bt_evt_hfp_voice_recognition_state_changed_handler(
    bt_evt_hfp_voice_recognition_state_changed_t *param)
{
    DBGLOG_BT_DBG("BT_EVT_HFP_VOICE_RECOGNITION_STATE_CHANGED activated:%d\n", param->activated);
    context->recognition_activated = param->activated;
    if (param->activated) {
        app_evt_send(EVTSYS_VOICE_RECOGNITION_START);
    } else {
        app_evt_send(EVTSYS_VOICE_RECOGNITION_STOP);
    }
}

static void bt_evt_a2dp_state_changed_handler(bt_evt_a2dp_state_changed_t *param)
{
    bt_a2dp_state_t prev_state = context->a2dp_state;

    context->a2dp_state = param->state;

    DBGLOG_BT_DBG("BT_EVT_A2DP_STATE_CHANGED from %d to %d\n", prev_state, param->state);

    if ((prev_state < A2DP_STATE_CONNECTED) && (param->state >= A2DP_STATE_CONNECTED)) {
        usr_cfg_set_a2dp_codec(&param->addr, param->codec);
        if (!context->connected_evt_reported) {
            app_evt_send(EVTSYS_CONNECTED);
            context->connected_evt_reported = true;
        }
    }

    generate_sys_state();

    if (param->state == A2DP_STATE_STREAMING) {
        context->play_pause_as_pause = true;
    } else {
        context->play_pause_as_pause = false;
    }
}

static void bt_evt_avrcp_state_changed_handler(bt_evt_avrcp_state_changed_t *param)
{
    bt_avrcp_state_t prev_state = context->avrcp_state;
    context->avrcp_state = param->state;

    DBGLOG_BT_DBG("BT_EVT_AVRCP_STATE_CHANGED from %d to %d\n", prev_state, param->state);

    if (param->state == AVRCP_STATE_DISCONNECTED) {
        app_cancel_msg(MSG_TYPE_BT, APP_BT_MSG_ID_HANDLE_AVRCP_VOLUME);
        context->pending_avrcp_volume = 0xFF;
    }

    generate_sys_state();

    if (param->state == AVRCP_STATE_PLAYING) {
        context->play_pause_as_pause = true;
    } else {
        context->play_pause_as_pause = false;
    }
}

static void bt_evt_avrcp_volume_changed_handler(bt_evt_avrcp_volume_changed_t *param)
{
    uint32_t cur_time = os_boot_time32();

    DBGLOG_BT_DBG("BT_EVT_AVRCP_VOLUME_CHANGED volume:%d\n", param->volume);

#if APP_BT_AVRCP_VOLUME_ANTI_SHAKE_ENABLED
    if (context->pending_avrcp_volume != 0xFF) {
        app_cancel_msg(MSG_TYPE_BT, APP_BT_MSG_ID_HANDLE_AVRCP_VOLUME);
        context->pending_avrcp_volume = 0xFF;
    }

    if (cur_time - context->avrcp_last_time < 500) {
        context->pending_avrcp_volume = param->volume;
        app_send_msg_delay(MSG_TYPE_BT, APP_BT_MSG_ID_HANDLE_AVRCP_VOLUME, &param->addr,
                           sizeof(BD_ADDR_T), APP_BT_AVRCP_VOLUME_ANTI_SHAKE_MS);
    } else {
        DBGLOG_BT_DBG("avrcp volume changed to %d\n", context->pending_avrcp_volume);
        usr_cfg_pdl_add(&param->addr);
        app_audio_music_volume_set(param->volume);
    }
#else
    if (cur_time - context->avrcp_last_time >= 500) {
        usr_cfg_pdl_add(&param->addr);
    }

    app_audio_music_volume_set(param->volume);
#endif

    context->avrcp_last_time = cur_time;
}

static void bt_evt_avrcp_volume_up_handler(bt_evt_avrcp_volume_up_t *param)
{
    UNUSED(param);

    if (app_wws_is_slave()) {
        DBGLOG_BT_DBG("BT_EVT_AVRCP_VOLUME_UP ignored by slave\n");
    } else {
        DBGLOG_BT_DBG("BT_EVT_AVRCP_VOLUME_UP\n");
        app_evt_send(EVTUSR_VOLUME_UP);
    }
}

static void bt_evt_avrcp_volume_down_handler(bt_evt_avrcp_volume_down_t *param)
{
    UNUSED(param);

    if (app_wws_is_slave()) {
        DBGLOG_BT_DBG("BT_EVT_AVRCP_VOLUME_DOWN ignored by slave\n");
    } else {
        DBGLOG_BT_DBG("BT_EVT_AVRCP_VOLUME_DOWN\n");
        app_evt_send(EVTUSR_VOLUME_DOWN);
    }
}

static void
bt_evt_avrcp_absolute_volume_enabled_handler(bt_evt_avrcp_absolute_volume_enabled_t *param)
{
    UNUSED(param);

    DBGLOG_BT_DBG("BT_EVT_AVRCP_ABSOLUTE_VOLUME_ENABLED\n");
}

static void bt_evt_tws_state_changed_handler(bt_evt_tws_state_changed_t *param)
{
    DBGLOG_BT_DBG("BT_EVT_TWS_STATE_CHANGED state:%d reason:%d\n", param->state, param->reason);

    app_wws_handle_state_changed(param->state, param->reason, &param->peer_addr);

    if (param->state == TWS_STATE_DISCONNECTED) {
        generate_sys_state();
    }
}

static void bt_evt_tws_role_changed_handler(bt_evt_tws_role_changed_t *param)
{
    DBGLOG_BT_DBG("BT_EVT_TWS_ROLE_CHANGED role:%d reason:%d\n", param->role, param->reason);

    memcpy(&context->local_addr, &param->local_addr, sizeof(BD_ADDR_T));
    app_wws_handle_role_changed(param->role, &param->peer_addr, param->reason);
}

static void bt_evt_tws_recv_data_handler(bt_evt_tws_recv_data_t *param)
{
    app_wws_handle_recv_data(param->data, param->len);
}

static void bt_evt_tws_tds_data_handler(bt_evt_tws_tds_data_t *param)
{
    app_wws_handle_tds_data(&param->remote_addr, param->data, param->len);
}

static void bt_evt_tws_pair_result_handler(bt_evt_tws_pair_result_t *param)
{
    DBGLOG_BT_DBG("BT_EVT_TWS_PAIR_RESULT %d\n", param->succeed);
    context->tws_pairing = false;
    if (param->succeed) {
        app_evt_send(EVTSYS_WWS_PAIRED);
    } else {
        app_evt_send(EVTSYS_WWS_PAIR_FAILED);
    }
    generate_sys_state();
}

static void bt_evt_tws_mode_changed_handler(bt_evt_tws_mode_changed_t *param)
{
    DBGLOG_BT_DBG("BT_EVT_TWS_MODE_CHANGED single:%d\n", param->single_mode);

    app_wws_handle_mode_changed(param->single_mode);
}

static void bt_evt_le_conn_param_changed_handler(bt_evt_le_conn_param_changed_t *param)
{
    DBGLOG_BT_DBG("BT_EVT_LE_CONN_PARAM_CHANGED interval:0x%04X latency:0x%04X super_visn:0x%04X\n",
                  param->conn_interval, param->conn_latency, param->supervision_timeout);
}

static void bt_evt_hci_evt_report_handler(bt_evt_hci_evt_report_t *report)
{
    app_cli_handle_hci_evt(report->evt, report->param_len, report->param);
    if (report->param) {
        os_mem_free(report->param);
    }
}

static void bt_evt_hci_data_received_handler(bt_evt_hci_data_received_t *evt)
{
    app_cli_handle_hci_data(evt->data_len, evt->data);
    if (evt->data) {
        os_mem_free(evt->data);
    }
}

static void handle_bt_evt(bt_evt_t evt, void *param)
{
    switch (evt) {
        case BT_EVT_ENABLE_STATE_CHANGED:
            bt_evt_enable_state_changed_handler(param);
            break;
        case BT_EVT_FACTORY_RESET_DONE:
            bt_evt_factory_reset_done_handler(param);
            break;
        case BT_EVT_ENTER_DUT_MODE:
            bt_evt_enter_dut_mode_handler(param);
            break;
        case BT_EVT_HW_ERROR:
            bt_evt_handle_hw_error(param);
            break;
        case BT_EVT_CONNECTION_STATE_CHANGED:
            bt_evt_connection_state_changed_handler(param);
            break;
        case BT_EVT_CONNECTION_MODE_CHANGED:
            bt_evt_connection_mode_changed_handler(param);
            break;
        case BT_EVT_VISIBILITY_CHANGED:
            bt_evt_visibility_changed_handler(param);
            break;
        case BT_EVT_LOCAL_NAME_CHANGED:
            bt_evt_local_name_changed_handler(param);
            break;
        case BT_EVT_COD_CHANGED:
            bt_evt_cod_changed_handler(param);
            break;
        case BT_EVT_PIN_CODE_CHANGED:
            bt_evt_pin_code_changed_handler(param);
            break;
        case BT_EVT_PAIR_ADDED:
            bt_evt_pair_added_handler(param);
            break;
        case BT_EVT_PAIR_REMOVED:
            bt_evt_pair_removed_handler(param);
            break;
        case BT_EVT_AUTH_RESULT:
            bt_evt_auth_result_handler(param);
            break;
        case BT_EVT_PHONE_TYPE_REPORT:
            bt_evt_phone_type_report_handler(param);
            break;
        case BT_EVT_SDP_RFCOMM_FOUND:
            app_sdp_handle_bt_evt(evt, param);
            break;
        case BT_EVT_HFP_STATE_CHANGED:
            bt_evt_hfp_state_changed_handler(param);
            break;
        case BT_EVT_HFP_RING:
            bt_evt_hfp_ring_handler(param);
            break;
        case BT_EVT_HFP_SCO_STATE_CHANGED:
            bt_evt_hfp_sco_state_changed_handler(param);
            break;
        case BT_EVT_HFP_SIGNAL_CHANGED:
            bt_evt_hfp_signal_changed_handler(param);
            break;
        case BT_EVT_HFP_BATTERY_LEVEL_CHANGED:
            bt_evt_hfp_battery_level_changed_handler(param);
            break;
        case BT_EVT_HFP_VOLUME_CHANGED:
            bt_evt_hfp_volume_changed_handler(param);
            break;
        case BT_EVT_HFP_VOICE_RECOGNITION_STATE_CHANGED:
            bt_evt_hfp_voice_recognition_state_changed_handler(param);
            break;
        case BT_EVT_A2DP_STATE_CHANGED:
            bt_evt_a2dp_state_changed_handler(param);
            break;
        case BT_EVT_AVRCP_STATE_CHANGED:
            bt_evt_avrcp_state_changed_handler(param);
            break;
        case BT_EVT_AVRCP_VOLUME_CHANGED:
            bt_evt_avrcp_volume_changed_handler(param);
            break;
        case BT_EVT_AVRCP_VOLUME_UP:
            bt_evt_avrcp_volume_up_handler(param);
            break;
        case BT_EVT_AVRCP_VOLUME_DOWN:
            bt_evt_avrcp_volume_down_handler(param);
            break;
        case BT_EVT_AVRCP_ABSOLUTE_VOLUME_ENABLED:
            bt_evt_avrcp_absolute_volume_enabled_handler(param);
            break;
        case BT_EVT_TWS_STATE_CHANGED:
            bt_evt_tws_state_changed_handler(param);
            break;
        case BT_EVT_TWS_ROLE_CHANGED:
            bt_evt_tws_role_changed_handler(param);
            break;
        case BT_EVT_TWS_RECV_DATA:
            bt_evt_tws_recv_data_handler(param);
            break;
        case BT_EVT_TWS_TDS_DATA:
            bt_evt_tws_tds_data_handler(param);
            break;
        case BT_EVT_TWS_PAIR_RESULT:
            bt_evt_tws_pair_result_handler(param);
            break;
        case BT_EVT_TWS_MODE_CHANGED:
            bt_evt_tws_mode_changed_handler(param);
            break;
        case BT_EVT_L2CAP_STATE_CHANGED:
        case BT_EVT_L2CAP_DATA:
            app_l2cap_handle_bt_evt(evt, param);
            break;
        case BT_EVT_SPP_STATE_CHANGED:
        case BT_EVT_SPP_REGISTERED:
        case BT_EVT_SPP_UUID128_REGISTERED:
        case BT_EVT_SPP_DATA:
            app_spp_handle_bt_evt(evt, param);
            break;
        case BT_EVT_GATT_SERVER_STATE_CHANGED:
        case BT_EVT_GATT_SERVER_SERVICE_REGISTERED:
        case BT_EVT_GATT_SERVER_SERVICE_UUID128_REGISTERED:
        case BT_EVT_GATT_SERVER_CHARACTERISTIC_REGISTERED:
        case BT_EVT_GATT_SERVER_CHARACTERISTIC_UUID128_REGISTERED:
        case BT_EVT_GATT_SERVER_WRITE:
        case BT_EVT_GATT_SERVER_READ:
        case BT_EVT_GATT_SERVER_MTU_CHANGED:
            app_gatts_handle_bt_evt(evt, param);
            break;
        case BT_EVT_LE_CONN_PARAM_CHANGED:
            bt_evt_le_conn_param_changed_handler(param);
            break;
        case BT_EVT_HCI_EVT_REPORT:
            bt_evt_hci_evt_report_handler(param);
            break;
        case BT_EVT_HCI_DATA_RECEIVED:
            bt_evt_hci_data_received_handler(param);
            break;
        case BT_EVT_UNUSED:
        case BT_EVT_HFP_EXTENDED_AT:
        case BT_EVT_GATT_CLIENT_STATE_CHANGED:
        case BT_EVT_GATT_CLIENT_SERVICE_FOUND:
        case BT_EVT_GATT_CLIENT_SERVICE_UUID128_FOUND:
        case BT_EVT_GATT_CLIENT_CHARACTERISTIC_FOUND:
        case BT_EVT_GATT_CLIENT_CHARACTERISTIC_UUID128_FOUND:
        case BT_EVT_GATT_CLIENT_DISCOERY_DONE:
        case BT_EVT_GATT_CLIENT_DATA_NOTIFY:
        case BT_EVT_GATT_CLIENT_READ_RESPONSE:
        case BT_EVT_GATT_CLIENT_MTU_CHANGED:
        case BT_EVT_LAST:
            break;
        default:
            DBGLOG_BT_DBG("unknown bt_evt %d\n", evt);
            break;
    }
}

static void app_bt_handle_msg(uint16_t msg_id, void *param)
{
    if (msg_id <= BT_EVT_LAST) {
        handle_bt_evt((bt_evt_t)msg_id, param);
        return;
    }

    switch (msg_id) {
        case APP_BT_MSG_ID_REPORT_BATTERY_AND_CALL_VOLUME:
            app_bt_report_battery_level(app_bat_get_level());
            app_bt_report_call_volume(usr_cfg_get_call_vol());
            break;
        case APP_BT_MSG_ID_CONNECT_SCO:
            DBGLOG_BT_DBG("APP_BT_MSG_ID_CONNECT_SCO\n");
            if (app_wws_is_master()) {
                app_bt_connect_sco();
            } else {
                DBGLOG_BT_DBG("app_bt_connect_sco ignored for slave\n");
            }
            break;
        case APP_BT_MSG_ID_HANDLE_AVRCP_VOLUME:
            DBGLOG_BT_DBG("avrcp volume changed to %d\n", context->pending_avrcp_volume);
            if (context->pending_avrcp_volume != 0xFF) {
                BD_ADDR_T *addr = param;
                usr_cfg_pdl_add(addr);
                app_audio_music_volume_set(context->pending_avrcp_volume);
                context->pending_avrcp_volume = 0xFF;
            }
            break;
        default:
            break;
    }
}

static bool_t alloc_bt_evt_data(bt_evt_t evt, void **the_param, bool_t *alloced)
{
    void *new_param;
    void *param;
    bool_t match;

    param = *the_param;

    if (evt == BT_EVT_TWS_RECV_DATA) {
        new_param = DEEPCOPY(bt_evt_tws_recv_data_t, data, len, param);
        match = true;
    } else if (evt == BT_EVT_TWS_TDS_DATA) {
        new_param = DEEPCOPY(bt_evt_tws_tds_data_t, data, len, param);
        match = true;
    } else if (evt == BT_EVT_GATT_SERVER_WRITE) {
        new_param = DEEPCOPY(bt_evt_gatt_server_write_t, data, len, param);
        match = true;
    } else if (evt == BT_EVT_HCI_EVT_REPORT) {
        new_param = DEEPCOPY(bt_evt_hci_evt_report_t, param, param_len, param);
        match = true;
    } else if (evt == BT_EVT_HCI_DATA_RECEIVED) {
        new_param = DEEPCOPY(bt_evt_hci_data_received_t, data, data_len, param);
        match = true;
    } else if (evt == BT_EVT_SPP_DATA) {
        new_param = DEEPCOPY(bt_evt_spp_data_t, data, len, param);
        match = true;
    } else if (evt == BT_EVT_L2CAP_DATA) {
        new_param = DEEPCOPY(bt_evt_l2cap_data_t, data, len, param);
        match = true;
    } else {
        new_param = param;
        match = false;
    }

    if (match) {
        if (new_param) {
            *the_param = new_param;
            *alloced = true;
            return true;
        } else {
            return false;
        }
    } else {
        *alloced = false;
        return true;
    }
}

static void free_bt_evt_data(bt_evt_t evt, void *param, bool_t send_succeed)
{
    if (evt == BT_EVT_TWS_RECV_DATA) {
        bt_evt_tws_recv_data_t *recv_data_evt = param;

        if ((!send_succeed) && (recv_data_evt->data)) {
            os_mem_free(recv_data_evt->data);
        }
        os_mem_free(param);
    } else if (evt == BT_EVT_TWS_TDS_DATA) {
        bt_evt_tws_tds_data_t *tds_data_evt = param;

        if ((!send_succeed) && (tds_data_evt->data)) {
            os_mem_free(tds_data_evt->data);
        }
        os_mem_free(param);
    } else if (evt == BT_EVT_GATT_SERVER_WRITE) {
        bt_evt_gatt_server_write_t *gatt_write_evt = param;

        if ((!send_succeed) && (gatt_write_evt->data)) {
            os_mem_free(gatt_write_evt->data);
        }
        os_mem_free(param);
    } else if (evt == BT_EVT_HCI_EVT_REPORT) {
        bt_evt_hci_evt_report_t *report = param;

        if ((!send_succeed) && (report->param)) {
            os_mem_free(report->param);
        }
        os_mem_free(param);
    } else if (evt == BT_EVT_HCI_DATA_RECEIVED) {
        bt_evt_hci_data_received_t *evt = param;

        if ((!send_succeed) && (evt->data)) {
            os_mem_free(evt->data);
        }
        os_mem_free(param);
    } else if (evt == BT_EVT_SPP_DATA) {
        bt_evt_spp_data_t *spp_data_evt = param;

        if ((!send_succeed) && (spp_data_evt->data)) {
            os_mem_free(spp_data_evt->data);
        }
        os_mem_free(param);
    } else if (evt == BT_EVT_L2CAP_DATA) {
        bt_evt_l2cap_data_t *l2cap_data_evt = param;

        if ((!send_succeed) && (l2cap_data_evt->data)) {
            os_mem_free(l2cap_data_evt->data);
        }
        os_mem_free(param);
    }
}

static void ftm_bt_evt_handler(bt_evt_t evt, void *param, uint32_t param_len)
{
    UNUSED(param_len);
    if (evt == BT_EVT_HCI_EVT_REPORT) {
        bt_evt_hci_evt_report_t *report = param;

        app_cli_handle_hci_evt(report->evt, report->param_len, report->param);
    } else if (evt == BT_EVT_HCI_DATA_RECEIVED) {
        bt_evt_hci_data_received_t *evt_param = param;
        app_cli_handle_hci_data(evt_param->data_len, evt_param->data);
    }
}

/**
 * @brief callback from rpc api the handle bt events
 *
 * @param evt the bt event
 * @param the_param parameter of the event
 * @param param_len length of the event parameter
 *
 * @return 0 for success, else for the error reason.
 *         the return value is used by bt core.
 */
static bt_result_t bt_evt_cb(bt_evt_t evt, void *the_param, uint32_t param_len)
{
    int ret;
    bool_t alloced = false;
    void *param = the_param;

    if (context->in_ft_mode) {
        ftm_bt_evt_handler(evt, param, param_len);
        return BT_RESULT_SUCCESS;
    }

    if (!param_len) {
        param = NULL;
    }

    if (param) {
        if (!alloc_bt_evt_data(evt, &param, &alloced)) {
            return BT_RESULT_SUCCESS;
        }
    }

    ret = app_send_msg(MSG_TYPE_BT, evt, param, param_len);

    if (param && alloced) {
        free_bt_evt_data(evt, param, !ret);
    }

    return BT_RESULT_SUCCESS;
}

static bool_t is_force_discoverable(void)
{
    if ((context->conn_state < CONNECTION_STATE_CONNECTED) && (!context->disable_running)
        && (context->enabled) && (context->force_discoverable)) {
        DBGLOG_BT_DBG("force discoverable\n");
        return true;
    } else {
        return false;
    }
}

void bdaddr_print(BD_ADDR_T *addr)
{
    DBGLOG_BT_DBG("%02X:%02X:%02X:%02X:%02X:%02X\n", addr->addr[5], addr->addr[4], addr->addr[3],
                  addr->addr[2], addr->addr[1], addr->addr[0]);
}

int app_bt_init(void)
{
    memset(context, 0, sizeof(app_bt_context_t));
    app_register_msg_handler(MSG_TYPE_BT, app_bt_handle_msg);
    bt_register_evt_cb(bt_evt_cb);

    return 0;
}

int app_bt_deinit(void)
{
    return 0;
}

uint32_t app_bt_get_sys_state(void)
{
    return context->sys_state;
}

bt_a2dp_state_t app_bt_get_a2dp_state(void)
{
    return context->a2dp_state;
}

bt_avrcp_state_t app_bt_get_avrcp_state(void)
{
    return context->avrcp_state;
}

bt_hfp_state_t app_bt_get_hfp_state(void)
{
    return context->hfp_state;
}

bool_t app_bt_is_sco_connected(void)
{
    return context->sco_connected;
}

int app_bt_factory_reset(bool_t keep_peer_addr)
{
    bt_cmd_factory_reset_t param;

    param.keep_peer_addr = keep_peer_addr;

    return app_bt_send_rpc_cmd(BT_CMD_FACTORY_RESET, &param, sizeof(param));
}

int app_bt_power_on(void)
{
    bt_cmd_set_enabled_t param;

    DBGLOG_BT_DBG("app_bt_power_on\n");

    context->user_enabled = true;

    if (context->enabled) {
        DBGLOG_BT_DBG("already power on\n");
        return 0;
    }

    if (!is_bt_rpc_ready()) {
        context->enable_pending = true;
        DBGLOG_BT_DBG("bt enable pending\n");
        return 0;
    }

    param.enabled = true;
    return app_bt_send_rpc_cmd(BT_CMD_SET_ENABLED, &param, sizeof(param));
}

bool_t app_bt_is_in_dut_mode(void)
{
    return context->in_dut_mode;
}

bool_t app_bt_is_in_ft_mode(void)
{
    return context->in_ft_mode;
}

bool_t app_bt_is_in_audio_test_mode(void)
{
    return context->in_audio_test_mode;
}

int app_bt_power_off(void)
{
    bt_cmd_set_enabled_t param;

    DBGLOG_BT_DBG("app_bt_power_off\n");

    if (context->in_dut_mode) {
        DBGLOG_BT_DBG("exit dut mode\n");
        context->in_dut_mode = false;
    }

    context->user_enabled = false;

    if (context->enable_pending) {
        context->enable_pending = false;
    }

    if (!context->enabled) {
        DBGLOG_BT_DBG("bt already power off\n");
        return 0;
    }

    context->disable_running = true;
    param.enabled = false;
    return app_bt_send_rpc_cmd(BT_CMD_SET_ENABLED, &param, sizeof(param));
}

int app_bt_connect(BD_ADDR_T *addr)
{
    bt_cmd_connect_t param;

    assert(addr);

    if (context->conn_state >= CONNECTION_STATE_CONNECTING) {
        DBGLOG_BT_ERR("app_bt_connect error conn_state:%d\n", context->conn_state);
        return BT_RESULT_ALREADY_EXISTS;
    }

    memcpy(&context->remote_addr, addr, sizeof(BD_ADDR_T));
    memcpy(&param.addr, &context->remote_addr, sizeof(BD_ADDR_T));

    param.prefered_a2dp_codec = usr_cfg_get_a2dp_codec(addr);

    DBGLOG_BT_DBG("app_bt_connect codec:%d\n", param.prefered_a2dp_codec);
    bdaddr_print(addr);

    return app_bt_send_rpc_cmd(BT_CMD_CONNECT, &param, sizeof(param));
}

int app_bt_disconnect(void)
{
    bt_cmd_disconnect_t param;

    if (context->conn_state < CONNECTION_STATE_CONNECTING) {
        return BT_RESULT_NOT_EXISTS;
    }

    memcpy(&param.addr, &context->remote_addr, sizeof(BD_ADDR_T));

    return app_bt_send_rpc_cmd(BT_CMD_DISCONNECT, &param, sizeof(param));
}

int app_bt_clear_pair_list(void)
{
    bt_cmd_clear_pair_list_t param;

    return app_bt_send_rpc_cmd(BT_CMD_CLEAR_PAIR_LIST, &param, sizeof(param));
}

int app_bt_enter_ag_pairing(void)
{
    bt_cmd_set_visibility_t param;
    int ret;

    if (app_wws_is_slave()) {
        DBGLOG_BT_DBG("app_bt_enter_ag_pairing ignored for slave\n");
        return RET_OK;
    }

    if (context->conn_state >= CONNECTION_STATE_CONNECTING) {
        app_bt_disconnect();
    }

    context->visible = true;
    context->connectable = true;

    param.visible = true;
    param.connectable = true;

    ret = app_bt_send_rpc_cmd(BT_CMD_SET_VISIBILITY, &param, sizeof(param));
    if (!ret) {
        if (app_bt_get_sys_state() != STATE_AG_PAIRING) {
            app_evt_send(EVTSYS_ENTER_PAIRING);
        }
        generate_sys_state();
        if (app_wws_is_connected_master()) {
            app_wws_send_visibility(context->visible, context->connectable);
        }
    } else {
        DBGLOG_BT_ERR("BT_CMD_SET_VISIBILITY failed ret:%d\n", ret);
    }

    return ret;
}

int app_bt_activate_voice_recognition(void)
{
    bt_cmd_hfp_activate_voice_recognition_t param;

    if (context->hfp_state < HFP_STATE_CONNECTED) {
        return BT_RESULT_NOT_EXISTS;
    }

    memcpy(&param.addr, &context->remote_addr, sizeof(BD_ADDR_T));

    return app_bt_send_rpc_cmd(BT_CMD_HFP_ACTIVATE_VOICE_RECOGNITION, &param, sizeof(param));
}

int app_bt_deactivate_voice_recognition(void)
{
    bt_cmd_hfp_deactivate_voice_recognition_t param;

    if (context->hfp_state < HFP_STATE_CONNECTED) {
        return BT_RESULT_NOT_EXISTS;
    }

    memcpy(&param.addr, &context->remote_addr, sizeof(BD_ADDR_T));

    return app_bt_send_rpc_cmd(BT_CMD_HFP_DEACTIVATE_VOICE_RECOGNITION, &param, sizeof(param));
}

int app_bt_toggle_voice_recognition(void)
{
    if (context->recognition_activated) {
        return app_bt_deactivate_voice_recognition();
    } else {
        return app_bt_activate_voice_recognition();
    }
}

int app_bt_redail(void)
{
    bt_cmd_hfp_redial_t param;

    if (context->hfp_state < HFP_STATE_CONNECTED) {
        return BT_RESULT_NOT_EXISTS;
    }

    memcpy(&param.addr, &context->remote_addr, sizeof(BD_ADDR_T));

    return app_bt_send_rpc_cmd(BT_CMD_HFP_REDIAL, &param, sizeof(param));
}

int app_bt_answer(void)
{
    bt_cmd_hfp_answer_t param;

    if (context->hfp_state < HFP_STATE_CONNECTED) {
        return BT_RESULT_NOT_EXISTS;
    }

    memcpy(&param.addr, &context->remote_addr, sizeof(BD_ADDR_T));

    return app_bt_send_rpc_cmd(BT_CMD_HFP_ANSWER, &param, sizeof(param));
}

int app_bt_reject(void)
{
    bt_cmd_hfp_reject_t param;

    if (context->hfp_state < HFP_STATE_CONNECTED) {
        return BT_RESULT_NOT_EXISTS;
    }

    memcpy(&param.addr, &context->remote_addr, sizeof(BD_ADDR_T));

    return app_bt_send_rpc_cmd(BT_CMD_HFP_REJECT, &param, sizeof(param));
}

int app_bt_hangup(void)
{
    bt_cmd_hfp_hangup_t param;

    if (context->hfp_state < HFP_STATE_CONNECTED) {
        return BT_RESULT_NOT_EXISTS;
    }

    memcpy(&param.addr, &context->remote_addr, sizeof(BD_ADDR_T));

    return app_bt_send_rpc_cmd(BT_CMD_HFP_HANGUP, &param, sizeof(param));
}

int app_bt_twc_reject_waiting(void)
{
    bt_cmd_hfp_twc_reject_waiting_t param;

    if (context->hfp_state < HFP_STATE_CONNECTED) {
        return BT_RESULT_NOT_EXISTS;
    }

    memcpy(&param.addr, &context->remote_addr, sizeof(BD_ADDR_T));

    return app_bt_send_rpc_cmd(BT_CMD_HFP_TWC_REJECT_WAITING, &param, sizeof(param));
}

int app_bt_twc_release_active_accept_held_waiting(void)
{
    bt_cmd_hfp_twc_release_active_accept_waiting_t param;

    if (context->hfp_state < HFP_STATE_CONNECTED) {
        return BT_RESULT_NOT_EXISTS;
    }

    memcpy(&param.addr, &context->remote_addr, sizeof(BD_ADDR_T));

    return app_bt_send_rpc_cmd(BT_CMD_HFP_TWC_RELEASE_ACTIVE_ACCEPT_WAITING, &param, sizeof(param));
}

int app_bt_twc_hold_active_accept_held_waiting(void)
{
    bt_cmd_hfp_twc_hold_active_accept_waiting_t param;

    if (context->hfp_state < HFP_STATE_CONNECTED) {
        return BT_RESULT_NOT_EXISTS;
    }

    memcpy(&param.addr, &context->remote_addr, sizeof(BD_ADDR_T));

    return app_bt_send_rpc_cmd(BT_CMD_HFP_TWC_HOLD_ACTIVE_ACCEPT_WAITING, &param, sizeof(param));
}

int app_bt_connect_sco(void)
{
    bt_cmd_hfp_connect_sco_t param;

    if (context->hfp_state < HFP_STATE_CONNECTED) {
        return BT_RESULT_NOT_EXISTS;
    }
    if (context->sco_connected) {
        return BT_RESULT_ALREADY_EXISTS;
    }

    memcpy(&param.addr, &context->remote_addr, sizeof(BD_ADDR_T));
    return app_bt_send_rpc_cmd(BT_CMD_HFP_CONNECT_SCO, &param, sizeof(param));
}

int app_bt_disconnect_sco(void)
{
    bt_cmd_hfp_disconnect_sco_t param;

    if (context->hfp_state < HFP_STATE_CONNECTED) {
        return BT_RESULT_NOT_EXISTS;
    }
    if (!context->sco_connected) {
        return BT_RESULT_NOT_EXISTS;
    }

    memcpy(&param.addr, &context->remote_addr, sizeof(BD_ADDR_T));
    return app_bt_send_rpc_cmd(BT_CMD_HFP_DISCONNECT_SCO, &param, sizeof(param));
}

int app_bt_transfer_toggle(void)
{
    if (context->hfp_state < HFP_STATE_CONNECTED) {
        return BT_RESULT_NOT_EXISTS;
    }

    if (context->sco_connected) {
        return app_bt_disconnect_sco();
    } else {
        return app_bt_connect_sco();
    }
}

int app_bt_play_pause(void)
{
    if (context->avrcp_state < AVRCP_STATE_CONNECTED) {
        return BT_RESULT_NOT_EXISTS;
    }

    if (context->play_pause_as_pause) {
        context->play_pause_as_pause = false;
        return app_bt_pause();
    } else {
        context->play_pause_as_pause = true;
        return app_bt_play();
    }
}

int app_bt_play(void)
{
    bt_cmd_avrcp_play_t param;

    if (context->avrcp_state < AVRCP_STATE_CONNECTED) {
        return BT_RESULT_NOT_EXISTS;
    }

    memcpy(&param.addr, &context->remote_addr, sizeof(BD_ADDR_T));
    return app_bt_send_rpc_cmd(BT_CMD_AVRCP_PLAY, &param, sizeof(param));
}

int app_bt_pause(void)
{
    bt_cmd_avrcp_pause_t param;

    if (context->avrcp_state < AVRCP_STATE_CONNECTED) {
        return BT_RESULT_NOT_EXISTS;
    }

    memcpy(&param.addr, &context->remote_addr, sizeof(BD_ADDR_T));
    return app_bt_send_rpc_cmd(BT_CMD_AVRCP_PAUSE, &param, sizeof(param));
}

int app_bt_forward(void)
{
    bt_cmd_avrcp_forward_t param;

    if (context->avrcp_state < AVRCP_STATE_CONNECTED) {
        return BT_RESULT_NOT_EXISTS;
    }

    memcpy(&param.addr, &context->remote_addr, sizeof(BD_ADDR_T));
    return app_bt_send_rpc_cmd(BT_CMD_AVRCP_FORWARD, &param, sizeof(param));
}

int app_bt_backward(void)
{
    bt_cmd_avrcp_backward_t param;

    if (context->avrcp_state < AVRCP_STATE_CONNECTED) {
        return BT_RESULT_NOT_EXISTS;
    }

    memcpy(&param.addr, &context->remote_addr, sizeof(BD_ADDR_T));
    return app_bt_send_rpc_cmd(BT_CMD_AVRCP_BACKWARD, &param, sizeof(param));
}

int app_bt_report_call_volume(uint8_t level)
{
    bt_cmd_hfp_report_volume_t param;
    if (context->hfp_state < HFP_STATE_CONNECTED) {
        return BT_RESULT_NOT_EXISTS;
    }

    if (app_wws_is_slave()) {
        DBGLOG_BT_DBG("app_bt_report_call_volume ignore for slave\n");
        return 0;
    }

    param.volume = level;
    memcpy(&param.addr, &context->remote_addr, sizeof(BD_ADDR_T));
    return app_bt_send_rpc_cmd(BT_CMD_HFP_REPORT_VOLUME, &param, sizeof(param));
}

int app_bt_report_music_volume(uint8_t level)
{
    bt_cmd_avrcp_report_volume_t param;

    if (app_wws_is_slave()) {
        DBGLOG_BT_DBG("app_bt_report_music_volume ignore for slave\n");
        return 0;
    }

    param.volume = level;

    memcpy(&param.addr, &context->remote_addr, sizeof(BD_ADDR_T));
    return app_bt_send_rpc_cmd(BT_CMD_AVRCP_REPORT_VOLUME, &param, sizeof(param));
}

int app_bt_report_battery_level(uint8_t level)
{
    bt_cmd_hfp_report_battery_level_t param;
    uint8_t hfp_lvl;
		uint8_t peer_hfp_lvl;

    if (context->hfp_state < HFP_STATE_CONNECTED) {
        return BT_RESULT_NOT_EXISTS;
    }

    if (app_wws_is_slave()) {
        DBGLOG_BT_DBG("app_bt_report_battery_level ignore for slave\n");
        return 0;
    }
		
		// user add cuixu
		if(app_wws_is_connected())
		{
			peer_hfp_lvl = app_wws_peer_get_battery_level();
			if(peer_hfp_lvl < level)
			{
				level = peer_hfp_lvl;
			}
		}

		//user add end

    hfp_lvl = level / 10;

    if (hfp_lvl > 9) {
        hfp_lvl = 9;
    }

    param.level = hfp_lvl;

    memcpy(&param.addr, &context->remote_addr, sizeof(BD_ADDR_T));
    return app_bt_send_rpc_cmd(BT_CMD_HFP_REPORT_BATTERY_LEVEL, &param, sizeof(param));
}

int app_bt_enter_ft_mode(void)
{
    bt_cmd_enter_ft_mode_t param;
    int ret;

    DBGLOG_BT_DBG("app_bt_enter_ftm\n");

    ret = app_bt_send_rpc_cmd(BT_CMD_ENTER_FT_MODE, &param, sizeof(param));

    context->in_ft_mode = true;

    return ret;
}

int app_bt_enter_audio_test_mode(bool_t close_bt)
{
    bt_cmd_set_enabled_t param;

    DBGLOG_BT_DBG("app_bt_enter_audio_test_mode\n");

    context->in_audio_test_mode = true;

    if (close_bt) {
        param.enabled = false;
        return app_bt_send_rpc_cmd(BT_CMD_SET_ENABLED, &param, sizeof(param));
    } else {
        return 0;
    }
}

int app_bt_enter_dut_mode(void)
{
    bt_cmd_enter_dut_mode_t param;
    int ret;

    //app_bt_set_3m_feature_enable(true);

    ret = app_bt_send_rpc_cmd(BT_CMD_ENTER_DUT_MODE, &param, sizeof(param));

    if (!ret) {
        context->in_dut_mode = true;
        app_pm_handle_bt_disconnected();
    } else {
        DBGLOG_BT_ERR("BT_CMD_ENTER_DUT_MODE error:%d\n", ret);
    }

    return ret;
}

int app_bt_get_connected_device(BD_ADDR_T *addr)
{
    assert(addr);

    if (!context->enabled) {
        return BT_RESULT_DISABLED;
    }

    if (context->conn_state < CONNECTION_STATE_CONNECTED) {
        return BT_RESULT_NOT_EXISTS;
    }

    memcpy(addr, &context->remote_addr, sizeof(BD_ADDR_T));
    return 0;
}

const BD_ADDR_T *app_bt_get_local_address(void)
{
    return &context->local_addr;
}

const char *app_bt_get_local_name(void)
{
    return context->local_name;
}

bool_t app_bt_is_discoverable(void)
{
    return context->visible;
}

bool_t app_bt_is_connectable(void)
{
    return context->connectable;
}

bool_t app_bt_is_disabling(void)
{
    return context->disable_running;
}

int app_bt_set_local_name(const char *name)
{
    bt_cmd_set_local_name_t *param;
    int ret;

    DBGLOG_BT_DBG("app_bt_set_local_name\n");
    assert(name);

    param = os_mem_malloc(IOT_APP_MID, sizeof(bt_cmd_set_local_name_t));
    if (!param) {
        DBGLOG_BT_ERR("app_bt_set_local_name malloc failed\n");
        return RET_NOMEM;
    }

    strlcpy(param->name, name, MAX_NAME_LEN);

    memset(context->local_name, 0, MAX_NAME_LEN);
    strlcpy(context->local_name, name, MAX_NAME_LEN);

    ret = app_bt_send_rpc_cmd(BT_CMD_SET_LOCAL_NAME, param, sizeof(bt_cmd_set_local_name_t));
    os_mem_free(param);

    return ret;
}

int app_bt_set_peer_addr(const BD_ADDR_T *addr)
{
    bt_cmd_tws_set_peer_addr_t param;

    memcpy(&param.addr, addr, sizeof(BD_ADDR_T));

    return app_bt_send_rpc_cmd(BT_CMD_TWS_SET_PEER_ADDR, &param, sizeof(param));
}

int app_bt_set_discoverable(bool_t discoverable)
{
    bt_cmd_set_visibility_t param;
    int ret;

    if (discoverable == context->visible) {
        return 0;
    }

    context->visible = discoverable;

    if (is_force_discoverable()) {
        param.visible = true;
    } else {
        param.visible = discoverable;
    }
    param.connectable = context->connectable;

    ret = app_bt_send_rpc_cmd(BT_CMD_SET_VISIBILITY, &param, sizeof(param));
    if (!ret) {
        generate_sys_state();
        if (app_wws_is_connected_master()) {
            app_wws_send_visibility(context->visible, context->connectable);
        }
    }

    return ret;
}

void app_bt_set_force_discoverable(bool_t force_discoverable)
{
    bt_cmd_set_visibility_t param;

    context->force_discoverable = force_discoverable;

    DBGLOG_BT_DBG("app_bt_set_force_discoverable %d\n", force_discoverable);

    if (is_force_discoverable()) {
        param.visible = true;
    } else {
        param.visible = context->visible;
    }
    param.connectable = context->connectable;
    app_bt_send_rpc_cmd(BT_CMD_SET_VISIBILITY, &param, sizeof(param));
}

int app_bt_set_connectable(bool_t connectable)
{
    bt_cmd_set_visibility_t param;
    int ret;

    if (connectable == context->connectable) {
        return 0;
    }

    context->connectable = connectable;
    if (is_force_discoverable()) {
        param.visible = true;
    } else {
        param.visible = context->visible;
    }
    param.connectable = connectable;

    ret = app_bt_send_rpc_cmd(BT_CMD_SET_VISIBILITY, &param, sizeof(param));
    if (!ret) {
        generate_sys_state();
        if (app_wws_is_connected_master()) {
            app_wws_send_visibility(context->visible, context->connectable);
        }
    }

    return ret;
}

int app_bt_set_discoverable_and_connectable(bool_t discoverable, bool_t connectable)
{
    bt_cmd_set_visibility_t param;
    int ret;

    context->visible = discoverable;
    context->connectable = connectable;

    if (is_force_discoverable()) {
        param.visible = true;
    } else {
        param.visible = discoverable;
    }
    param.connectable = connectable;

    ret = app_bt_send_rpc_cmd(BT_CMD_SET_VISIBILITY, &param, sizeof(param));
    if (!ret) {
        generate_sys_state();
        if (app_wws_is_connected_master()) {
            app_wws_send_visibility(context->visible, context->connectable);
        }
    }

    return ret;
}

int app_bt_set_3m_feature_enable(bool_t enable)
{
    bt_cmd_set_3m_feature_enable_t param;

    param.enable = enable;

    return app_bt_send_rpc_cmd(BT_CMD_SET_3M_FEATURE_ENABLE, &param, sizeof(param));
}

int app_bt_set_fix_tx_power(uint8_t type, uint8_t level)
{
    bt_cmd_set_tx_pwr_level_t param;

    param.power_type = type;
    param.power_lvl = level;

    return app_bt_send_rpc_cmd(BT_CMD_SET_TX_PWR_LEVEL, &param, sizeof(param));
}

int app_bt_send_rpc_cmd(uint32_t cmd, void *param, uint32_t param_len)
{
    int ret;

    if (context->in_dut_mode) {
        DBGLOG_BT_DBG("app_bt_send_rpc_cmd dut mode ignored\n");
        return BT_RESULT_DISABLED;
    }

    if (context->in_ft_mode) {
        DBGLOG_BT_DBG("app_bt_send_rpc_cmd factory test mode ignored\n");
        return BT_RESULT_DISABLED;
    }

    ret = bt_handle_user_cmd((bt_cmd_t)cmd, param, param_len);
    DBGLOG_BT_DBG("app_bt_send_rpc_cmd %d ret=%d\n", cmd, ret);

    return ret;
}

int app_bt_send_tws_pair_cmd(uint16_t vid, uint16_t pid, uint8_t magic, uint32_t timeout)
{
    bt_cmd_tws_start_pair_t cmd;
    int ret;

    cmd.vid = vid;
    cmd.pid = pid;
    cmd.magic = magic;
    cmd.timeout_ms = timeout;

    ret = app_bt_send_rpc_cmd(BT_CMD_TWS_START_PAIR, &cmd, sizeof(cmd));

    if (!ret) {
        context->tws_pairing = true;
        generate_sys_state();
    } else {
        DBGLOG_BT_ERR("BT_CMD_TWS_START_PAIR error %d\n", ret);
    }

    return ret;
}

void app_bt_handle_peer_visibility_changed(void)
{
    generate_sys_state();
}

bool_t app_bt_is_recognition_activated(void)
{
    return context->recognition_activated;
}

bool_t app_bt_is_in_sniff_mode(void)
{
    return context->in_sniff_mode;
}

const link_quality_t *app_bt_get_link_quality(void)
{
    return ((const link_quality_t *)BT_SHARE_DATA_START);
}

void app_bt_update_le_conn_param(BD_ADDR_T *addr, uint16_t conn_interval_min,
                                 uint16_t conn_interval_max, uint16_t conn_latency,
                                 uint16_t supervision_timeout)
{
    DBGLOG_BT_DBG("app_le_conn_param_update interval:0x%04X-0x%04X latency:0x%04X timeout:0x%04X\n",
                  conn_interval_min, conn_interval_max, conn_latency, supervision_timeout);
    assert(addr != NULL);
    assert(conn_interval_max >= conn_interval_min);
    assert(conn_interval_min >= 0x0006);
    assert(conn_interval_max <= 0x0C80);
    assert(conn_latency <= 0x1F3);
    assert(supervision_timeout >= 0x000A && supervision_timeout <= 0x0C80);
    bt_cmd_le_conn_update_param_t conn_param;
    conn_param.addr = *addr;
    conn_param.conn_interval_min = conn_interval_min;
    conn_param.conn_interval_max = conn_interval_max;
    conn_param.conn_latency = conn_latency;
    conn_param.supervision_timeout = supervision_timeout;
    conn_param.minimum_ce_length = 0x0000;
    conn_param.maximum_ce_length = 0xFFFF;
    app_bt_send_rpc_cmd(BT_CMD_LE_CONN_UPDATE_PARAM, &conn_param, sizeof(conn_param));
}
