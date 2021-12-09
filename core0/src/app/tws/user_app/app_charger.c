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

#include "app_charger.h"
#include "app_main.h"
#include "app_pm.h"
#include "app_evt.h"
#include "app_bt.h"
#include "app_wws.h"
#include "charger_box.h"
#include "app_econn.h"
#include "os_utils.h"
#include "usr_cfg.h"
#include "app_conn.h"
#include "app_bat.h"
#include "boot_reason.h"

typedef struct {
    bool_t charger_received;
    bool_t charger_on;
    box_state_t box_state : 8;
    bool_t box_charging;
    uint8_t box_battery;
} charger_context_t;

static charger_context_t _context = {0};
static charger_context_t *context = &_context;

static void handle_charger_box_evt(charger_evt_t evt, void *param)
{
    if (app_econn_handle_charger_evt(evt, param)) {
        DBGLOG_EVT_DBG("handle_charger_box_evt evt:%d handled by econn\n", evt);
        return;
    }

    switch (evt) {
        case CHARGER_EVT_POWER_ON:
            DBGLOG_CHARGER_DBG("CHARGER_EVT_POWER_ON\n");
            app_evt_send(EVTUSR_POWER_ON);
            break;
        case CHARGER_EVT_POWER_OFF:
            DBGLOG_CHARGER_DBG("CHARGER_EVT_POWER_OFF\n");
            app_evt_send(EVTSYS_BOX_CMD_POWER_OFF);
            app_pm_power_off();
            break;
        case CHARGER_EVT_REBOOT:
            DBGLOG_CHARGER_DBG("CHARGER_EVT_REBOOT\n");
            app_evt_send(EVTSYS_BOX_CMD_REBOOT);
            app_pm_reboot(PM_REBOOT_REASON_CHARGER);
            break;
        case CHARGER_EVT_BT_DISABLE:
            DBGLOG_CHARGER_DBG("CHARGER_EVT_BT_DISABLE\n");
            app_evt_send(EVTSYS_BOX_CMD_BT_DISABLE);
            app_pm_handle_charger_disable_bt();
            break;
        case CHARGER_EVT_PUT_IN:
            DBGLOG_CHARGER_DBG("CHARGER_EVT_PUT_IN\n");
            context->charger_on = true;
            if (app_bt_is_in_dut_mode()) {
                DBGLOG_CHARGER_DBG("reboot for dut mode\n");
                app_pm_reboot(PM_REBOOT_REASON_CHARGER);
            }
            app_evt_send(EVTSYS_CHARGE_CONNECTED);
            context->charger_received = true;
            app_wws_send_charger(context->charger_on);
            app_bat_handle_charge_changed(true);
            app_econn_handle_charging_changed(true);
            break;
        case CHARGER_EVT_TAKE_OUT:
            DBGLOG_CHARGER_DBG("CHARGER_EVT_TAKE_OUT\n");
            context->charger_on = false;
            if (!context->charger_received) {
                DBGLOG_CHARGER_DBG("first charger off\n");
            } else {
                app_evt_send(EVTSYS_CHARGE_DISCONNECTED);
                app_pm_handle_charge_off();
            }
            context->charger_received = true;
            app_wws_send_charger(context->charger_on);
            app_bat_handle_charge_changed(false);
            app_econn_handle_charging_changed(false);
            break;
        case CHARGER_EVT_BOX_OPEN:
            DBGLOG_CHARGER_DBG("CHARGER_EVT_BOX_OPEN\n");
            context->box_state = BOX_STATE_OPENED;
            app_evt_send(EVTSYS_CMC_BOX_OPEN);
            app_wws_send_box_state(BOX_STATE_OPENED);
            app_econn_handle_box_state_changed(BOX_STATE_OPENED);
            break;
        case CHARGER_EVT_BOX_CLOSE:
            DBGLOG_CHARGER_DBG("CHARGER_EVT_BOX_CLOSE\n");
            context->box_state = BOX_STATE_CLOSED;
            app_evt_send(EVTSYS_CMC_BOX_CLOSE);
            app_wws_send_box_state(BOX_STATE_CLOSED);
            app_econn_handle_box_state_changed(BOX_STATE_CLOSED);
            break;
        case CHARGER_EVT_ENTER_DUT:
            DBGLOG_CHARGER_DBG("CHARGER_EVT_ENTER_DUT\n");
            app_evt_send(EVTUSR_ENTER_DUT_MODE);
            break;
        case CHARGER_EVT_ENTER_OTA:
            DBGLOG_CHARGER_DBG("CHARGER_EVT_ENTER_OTA\n");
            break;
        case CHARGER_EVT_RESET:
            DBGLOG_CHARGER_DBG("CHARGER_EVT_RESET\n");
            app_evt_send(EVTUSR_FACTORY_RESET);
            break;
        case CHARGER_EVT_RESET_KEEP_WWS:
            DBGLOG_CHARGER_DBG("CHARGER_EVT_RESET_KEEP_WWS\n");
            app_evt_send(EVTUSR_FACTORY_RESET_KEEP_WWS);
            break;
        case CHARGER_EVT_CLEAR_PDL:
            DBGLOG_CHARGER_DBG("CHARGER_EVT_CLEAR_PDL\n");
            app_evt_send(EVTUSR_CLEAR_PDL);
            break;
        case CHARGER_EVT_AG_PAIR:
            DBGLOG_CHARGER_DBG("CHARGER_EVT_AG_PAIR\n");
            context->box_state = BOX_STATE_OPENED;
            if (app_bt_get_sys_state() <= STATE_DISABLED) {
                app_conn_set_power_on_action(APP_CONN_POWER_ON_ACTION_PAIRING);
                app_pm_power_on();
            } else if (app_wws_is_master()) {
                if (app_bt_is_discoverable() && app_bt_is_connectable()) {
                    DBGLOG_CHARGER_DBG("CHARGER_EVT_AG_PAIR ignored\n");
                } else {
                    app_evt_send(EVTUSR_ENTER_PAIRING);
                }
            } else {
                app_evt_send(EVTUSR_ENTER_PAIRING);
            }
            app_wws_send_box_state(BOX_STATE_OPENED);
            app_econn_handle_box_state_changed(BOX_STATE_OPENED);
            break;
        case CHARGER_EVT_TWS_PAIR:
            DBGLOG_CHARGER_DBG("CHARGER_EVT_TWS_PAIR\n");
            app_evt_send(EVTUSR_START_WWS_PAIR);
            break;
        case CHARGER_EVT_TWS_MAGIC:
            DBGLOG_CHARGER_DBG("CHARGER_EVT_TWS_MAGIC magic:0x%X\n", *((uint8_t *)param));
            break;
        case CHARGER_EVT_POPUP_TOGGLE:
            DBGLOG_CHARGER_DBG("CHARGER_EVT_POPUP_TOGGLE\n");
            break;
        case CHARGER_EVT_BOX_BATTERY: {
            box_battery_param_t *battery = (box_battery_param_t *)param;
            context->box_battery = battery->battery;
            context->box_charging = battery->charging;
            DBGLOG_CHARGER_DBG("CHARGER_EVT_BOX_BATTERY battery:%d charging:%d\n", battery->battery,
                               battery->charging);
            if (app_wws_is_connected_slave() && app_charger_is_charging()
                && (!app_wws_peer_is_charging())) {
                app_wws_send_remote_msg(MSG_TYPE_CHARGER, CHARGER_EVT_BOX_BATTERY,
                                        sizeof(box_battery_param_t), (uint8_t *)battery);
            }
            break;
        }
        case CHARGER_EVT_ENTER_MONO_MODE: {
            BD_ADDR_T addr;
            memset(&addr, 0xFF, sizeof(BD_ADDR_T));
            app_bt_set_peer_addr(&addr);
            usr_cfg_reset();
            app_pm_reboot(PM_REBOOT_REASON_USER);
            break;
        }
        default:
            DBGLOG_CHARGER_DBG("unknown event received 0x%X\n", evt);
            break;
    }
}

static void charger_box_callback(charger_evt_t evt, void *param, uint32_t param_len)
{
    if ((evt == CHARGER_EVT_BOX_OPEN) && app_pm_is_shuting_down()) {
        DBGLOG_PM_DBG("box oepn when shuting down, reboot\n");
        app_pm_reboot(PM_REBOOT_REASON_USER);
    }
    app_send_msg(MSG_TYPE_CHARGER, evt, param, param_len);
}

static void app_charger_handle_msg(uint16_t msg_id, void *param)
{
    handle_charger_box_evt((charger_evt_t)msg_id, param);
}

bool_t app_charger_is_charging(void)
{
    return context->charger_on;
}

bool_t app_charger_is_box_open(void)
{
    return (context->box_state == BOX_STATE_OPENED);
}

bool_t app_charger_is_box_charging(void)
{
    return context->box_charging;
}

uint8_t app_charger_get_box_battery(void)
{
    return context->box_battery;
}

box_state_t app_charger_get_box_state(void)
{
    return context->box_state;
}

void app_charger_evt_send(uint8_t evt)
{
    app_send_msg(MSG_TYPE_CHARGER, evt, NULL, 0);
}

void app_charger_init(void)
{
    DBGLOG_CHARGER_DBG("app_charger_init\n");
    app_register_msg_handler(MSG_TYPE_CHARGER, app_charger_handle_msg);
    charger_box_init(charger_box_callback);
}

void app_charger_deinit(void)
{
    charger_box_deinit();
}
