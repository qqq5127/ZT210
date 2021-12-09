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

#include "app_conn.h"
#include "app_bt.h"
#include "usr_cfg.h"
#include "app_evt.h"
#include "app_main.h"
#include "app_wws.h"
#include "ro_cfg.h"
#include "app_pm.h"

#ifndef APP_CONN_POWER_ON_RECONNECT_CONNECTABLE
#define APP_CONN_POWER_ON_RECONNECT_CONNECTABLE 1
#endif

#define MSG_ID_POWER_ON_CONNECT 1
#define MSG_ID_RECONNECT        2

typedef enum {
    CONNECT_REASON_NONE,
    CONNECT_REASON_POWER_ON,
    CONNECT_REASON_LINK_LOSS,
    CONNECT_REASON_USER,
} connect_reason_t;

static uint8_t retry_count = 0;
static connect_reason_t connect_reason = CONNECT_REASON_NONE;
static uint8_t disconnect_reason = 0;
static app_conn_power_on_action_e power_on_action = APP_CONN_POWER_ON_ACTION_RECONENCT;
static app_conn_role_change_action_e role_change_action = APP_CONN_ROLE_CHANGE_ACTION_RECONNECT;

static void connect_last(void)
{
    uint32_t state = app_bt_get_sys_state();
    int ret;
    BD_ADDR_T addr = {0};

    if (state < STATE_IDLE) {
        DBGLOG_CONN_DBG("connect_last state error 0x%x\n", state);
        return;
    }

    if (state >= STATE_CONNECTED) {
        DBGLOG_CONN_DBG("connect_last state error 0x%x\n", state);
        return;
    }

    if (app_wws_is_slave()) {
        DBGLOG_CONN_DBG("connect_last by slave, ignored\n");
        return;
    }

    usr_cfg_pdl_get_first(&addr);
    if (bdaddr_is_zero(&addr)) {
        app_bt_set_discoverable_and_connectable(true, true);
        DBGLOG_CONN_DBG("connect_last pdl empty\n");
        return;
    }

    if (bdaddr_is_equal(app_wws_get_peer_addr(), &ftm_peer_bdaddr)) {
        app_bt_set_discoverable_and_connectable(true, true);
        DBGLOG_CONN_DBG("disable auto connect for ftm_peer_bdaddr\n");
        return;
    }

    ret = app_bt_connect(&addr);

    if (ret) {
        DBGLOG_CONN_ERR("connect_last ret:%d\n", ret);
        app_bt_set_discoverable_and_connectable(true, true);
    } else {
        DBGLOG_CONN_DBG("connect_last succeed\n");
    }
}

static void handle_bt_link_loss(void)
{
    connect_reason = CONNECT_REASON_LINK_LOSS;
    retry_count = 0;
    app_send_msg_delay(MSG_TYPE_CONN, MSG_ID_RECONNECT, NULL, 0,
                       ro_to_cfg()->link_loss_reconnect_interval * 1000);
}

static void handle_bt_conn_timeout(void)
{
    DBGLOG_CONN_DBG("handle_bt_conn_timeout reason:%d\n", connect_reason);
    switch (connect_reason) {
        case CONNECT_REASON_POWER_ON:
            if (ro_sys_cfg()->power_on_reconnect_retry_count == 0xFF) {   //TODO: change to 0xFFFF
                DBGLOG_CONN_DBG("power on reconnect infinitely\n");
                app_send_msg_delay(MSG_TYPE_CONN, MSG_ID_RECONNECT, NULL, 0,
                                   ro_to_cfg()->power_on_reconnect_interval * 1000);
            } else if (retry_count < ro_sys_cfg()->power_on_reconnect_retry_count) {
                app_send_msg_delay(MSG_TYPE_CONN, MSG_ID_RECONNECT, NULL, 0,
                                   ro_to_cfg()->power_on_reconnect_interval * 1000);
            } else {
                DBGLOG_CONN_ERR(
                    "reconnect failed, reason:CONNECT_REASON_POWER_ON, retry count:%d\n",
                    retry_count);
                app_evt_send(EVTSYS_POWER_ON_RECONNECT_FAILED);
                if (app_wws_is_master()) {
                    app_bt_enter_ag_pairing();
                }
            }
            break;
        case CONNECT_REASON_LINK_LOSS:
            if (ro_sys_cfg()->link_loss_reconnect_retry_count == 0xFF) {   //TODO: change to 0xFFFF
                DBGLOG_CONN_DBG("link loss reconnect infinitely\n");
                app_send_msg_delay(MSG_TYPE_CONN, MSG_ID_RECONNECT, NULL, 0,
                                   ro_to_cfg()->link_loss_reconnect_interval * 1000);
            } else if (retry_count < ro_sys_cfg()->link_loss_reconnect_retry_count) {
                app_send_msg_delay(MSG_TYPE_CONN, MSG_ID_RECONNECT, NULL, 0,
                                   ro_to_cfg()->link_loss_reconnect_interval * 1000);
            } else {
                DBGLOG_CONN_ERR(
                    "reconnect failed, reason:CONNECT_REASON_LINK_LOSS, retry count:%d\n",
                    retry_count);
                app_evt_send(EVTSYS_LINK_LOSS_RECONNECT_FAILED);
                if (app_wws_is_master()) {
                    app_bt_enter_ag_pairing();
                }
            }
            break;
        case CONNECT_REASON_NONE:
        case CONNECT_REASON_USER:
            if (app_wws_is_master()) {
                app_bt_enter_ag_pairing();
            }
            break;
        default:
            break;
    }
}

static void handle_bt_disconnect(uint8_t reason)
{
    disconnect_reason = reason;

    switch (reason) {
        case BT_DISCONNECT_REASON_CONN_TIMEOUT:
            handle_bt_conn_timeout();
            break;
        case BT_DISCONNECT_REASON_AUTH_FAIL:
            if (app_wws_is_master() && ro_feat_cfg()->discoverable_disconnect) {
                app_bt_enter_ag_pairing();
            }
            break;
        case BT_DISCONNECT_REASON_KEY_MISS:
            if (app_wws_is_master() && ro_feat_cfg()->discoverable_disconnect) {
                app_bt_enter_ag_pairing();
            }
            break;
        case BT_DISCONNECT_REASON_LMP_TIMEOUT:
        case BT_DISCONNECT_REASON_LINK_LOSS:
            app_evt_send(EVTSYS_LINK_LOSS);
            handle_bt_link_loss();
            break;
        case BT_DISCONNECT_REASON_ALREADY_EXISTS:
            if (app_wws_is_connected_master()) {
                if (retry_count > 0) {
                    retry_count -= 1;
                }
                handle_bt_conn_timeout();
            }
            break;
        case BT_DISCONNECT_REASON_REMOTE:
        case BT_DISCONNECT_REASON_REMOTE_POWER_OFF:
            if (app_wws_is_master() && ro_feat_cfg()->discoverable_disconnect) {
                app_bt_enter_ag_pairing();
            }
            break;
        case BT_DISCONNECT_REASON_INSUFFICIENT_RESOURCES:
            if (app_wws_is_master() && ro_feat_cfg()->discoverable_disconnect) {
                app_bt_enter_ag_pairing();
            }
            break;
        case BT_DISCONNECT_REASON_LOCAL:
            break;
        case BT_DISCONNECT_REASON_TWS_PAIRING:
        case BT_DISCONNECT_REASON_SWITCHED_SLAVE:
            disconnect_reason = 0;
            retry_count = 0;
            break;
        default:
            break;
    }
}

static void app_conn_handle_msg(uint16_t msg_id, void *param)
{
    UNUSED(param);

    switch (msg_id) {
        case MSG_ID_POWER_ON_CONNECT:
            connect_reason = CONNECT_REASON_POWER_ON;
            if (app_pm_get_power_on_reason() == PM_POWER_ON_REASON_OTA) {
                DBGLOG_CONN_DBG("reconnect after ota\n");
                connect_reason = CONNECT_REASON_LINK_LOSS;
            }
            retry_count = 1;
            connect_last();
            break;
        case MSG_ID_RECONNECT:
            if (app_bt_get_sys_state() >= STATE_AG_PAIRING) {
                DBGLOG_CONN_DBG("connect_last state:0x%X error\n", app_bt_get_sys_state());
            } else {
                retry_count += 1;
                connect_last();
            }
            break;
        default:
            break;
    }
}

void app_conn_init(void)
{
    app_register_msg_handler(MSG_TYPE_CONN, app_conn_handle_msg);
}

void app_conn_deinit(void)
{
}

void app_conn_connect_last(void)
{
    connect_reason = CONNECT_REASON_USER;
    DBGLOG_CONN_DBG("connect_last by user\n");
    app_bt_set_discoverable(false);
    connect_last();
}

void app_conn_disconnect(void)
{
    uint32_t state = app_bt_get_sys_state();
    int ret;

    if (state < STATE_CONNECTED) {
        DBGLOG_CONN_DBG("app_conn_disconnect not connected\n");
        return;
    }

    ret = app_bt_disconnect();

    if (ret) {
        DBGLOG_CONN_ERR("app_conn_disconnect ret:%d\n", ret);
    } else {
        DBGLOG_CONN_DBG("app_conn_disconnect succeed\n");
    }
}

void app_conn_handle_bt_power_on(void)
{
    disconnect_reason = 0;
    retry_count = 0;

    if (app_wws_is_slave()) {
        DBGLOG_CONN_DBG("app_conn_handle_bt_power_on ignore for slave\n");
        return;
    }

    if (power_on_action == APP_CONN_POWER_ON_ACTION_PAIRING) {
        power_on_action = APP_CONN_POWER_ON_ACTION_RECONENCT;
        DBGLOG_CONN_DBG("app_conn_handle_bt_power_on power on action pairing\n");
        app_bt_enter_ag_pairing();
    } else if (usr_cfg_pdl_is_empty()) {
        DBGLOG_CONN_DBG("app_conn_handle_bt_power_on pdl empty\n");
        app_bt_enter_ag_pairing();
    } else {
        DBGLOG_CONN_DBG("app_conn_handle_bt_power_on start reconnect\n");
        if (ro_feat_cfg()->auto_reconnect_power_on) {
            app_send_msg(MSG_TYPE_CONN, MSG_ID_POWER_ON_CONNECT, NULL, 0);
            if (APP_CONN_POWER_ON_RECONNECT_CONNECTABLE) {
                app_bt_set_connectable(true);
            }
        } else {
            app_bt_set_connectable(true);
        }
    }
}

void app_conn_handle_bt_power_off(void)
{
    power_on_action = APP_CONN_POWER_ON_ACTION_RECONENCT;
    app_cancel_msg(MSG_TYPE_CONN, MSG_ID_POWER_ON_CONNECT);
    app_cancel_msg(MSG_TYPE_CONN, MSG_ID_RECONNECT);
}

void app_conn_handle_bt_conn_state(bool_t connected, uint8_t reason)
{
    DBGLOG_CONN_DBG("app_conn_handle_bt_conn_state master:%d connected:%d reason:0x%X",
                    app_wws_is_master(), connected, reason);
    if (connected) {
        retry_count = 0;
        connect_reason = CONNECT_REASON_NONE;
        app_cancel_msg(MSG_TYPE_CONN, MSG_ID_POWER_ON_CONNECT);
        app_cancel_msg(MSG_TYPE_CONN, MSG_ID_RECONNECT);
        app_bt_set_discoverable_and_connectable(false, false);
        power_on_action = APP_CONN_POWER_ON_ACTION_RECONENCT;
    } else {
        if (app_wws_is_master()) {
            app_bt_set_connectable(true);
        }
        handle_bt_disconnect(reason);
    }
}

void app_conn_handle_wws_role_changed(bool_t is_master, tws_role_changed_reason_t reason)
{
    if (!is_master) {
        app_bt_set_discoverable_and_connectable(false, false);
        role_change_action = APP_CONN_ROLE_CHANGE_ACTION_RECONNECT;
        return;
    }

    if (app_bt_get_sys_state() >= STATE_CONNECTED) {
        role_change_action = APP_CONN_ROLE_CHANGE_ACTION_RECONNECT;
        return;
    }

    if (role_change_action == APP_CONN_ROLE_CHANGE_ACTION_PAIRING) {
        DBGLOG_CONN_DBG("role change action pairing\n");
        app_bt_enter_ag_pairing();
        role_change_action = APP_CONN_ROLE_CHANGE_ACTION_RECONNECT;
        return;
    }

    if (app_wws_is_connected()) {
        if (app_wws_peer_is_discoverable()) {
            app_bt_set_discoverable_and_connectable(true, true);
            DBGLOG_CONN_DBG("enter ag pairing for wws master\n");
            return;
        }
    } else {
        if (power_on_action == APP_CONN_POWER_ON_ACTION_PAIRING) {
            power_on_action = APP_CONN_POWER_ON_ACTION_RECONENCT;
            DBGLOG_CONN_DBG("role changed to master, power on action pairing\n");
            app_bt_enter_ag_pairing();
            return;
        }

        if (usr_cfg_pdl_is_empty()) {
            if (reason == TWS_ROLE_CHANGED_REASON_PEER_NOT_FOUND) {
                app_bt_enter_ag_pairing();
                DBGLOG_CONN_DBG("role changed to master, boot, pdl empty, enter ag pairing\n");
            } else {
                app_bt_set_discoverable_and_connectable(true, true);
                DBGLOG_CONN_DBG("role changed to master, pdl empty, enter ag pairing\n");
            }
            return;
        }
    }

    DBGLOG_CONN_DBG("app_conn_handle_wws_role_changed disc_reason:%d\n", disconnect_reason);

    if (reason == TWS_ROLE_CHANGED_REASON_LINK_LOSS) {
        disconnect_reason = BT_DISCONNECT_REASON_LINK_LOSS;
        DBGLOG_CONN_DBG("app_conn_handle_wws_role_changed because of link loss\n");
    }

    if ((disconnect_reason == 0) || (disconnect_reason == BT_DISCONNECT_REASON_CONN_TIMEOUT)) {
        if (ro_feat_cfg()->auto_reconnect_power_on) {
            app_cancel_msg(MSG_TYPE_CONN, MSG_ID_POWER_ON_CONNECT);
            app_send_msg_delay(MSG_TYPE_CONN, MSG_ID_POWER_ON_CONNECT, NULL, 0, 500);
            if (APP_CONN_POWER_ON_RECONNECT_CONNECTABLE) {
                app_bt_set_connectable(true);
            }
        } else {
            app_bt_set_connectable(true);
        }
    } else if ((disconnect_reason == BT_DISCONNECT_REASON_LINK_LOSS)
               || disconnect_reason == BT_DISCONNECT_REASON_LMP_TIMEOUT) {
        app_bt_set_connectable(true);
        handle_bt_link_loss();
    } else if (disconnect_reason == BT_DISCONNECT_REASON_REMOTE) {
        app_bt_set_discoverable_and_connectable(true, true);
        DBGLOG_CONN_DBG("enter ag pairing for role changed after remote disconnect\n");
    } else {
        app_bt_set_discoverable_and_connectable(true, true);
        DBGLOG_CONN_ERR("app_conn_handle_wws_role_changed unkonwn disc reason 0x%X\n",
                        disconnect_reason);
    }
}

uint8_t app_conn_get_disconnect_reason(void)
{
    return disconnect_reason;
}

void app_conn_set_power_on_action(app_conn_power_on_action_e action)
{
    DBGLOG_CONN_DBG("app_conn_set_power_on_action %d\n", action);
    power_on_action = action;
}

void app_conn_set_role_change_action(app_conn_role_change_action_e action)
{
    role_change_action = action;
}
