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
#include "app_adv.h"
#include "app_bt.h"
#include "app_wws.h"

#ifndef BLE_ADV_INTERVAL_MIN
#define BLE_ADV_INTERVAL_MIN 0x20
#endif

#ifndef BLE_ADV_INTERVAL_MAX
#define BLE_ADV_INTERVAL_MAX 0x800
#endif

static const uint8_t *adv_data = NULL;
static uint8_t adv_data_len = 0;
static const uint8_t *scan_rsp = NULL;
static uint8_t scan_rsp_len = 0;
static bool_t adv_enabled = false;
static uint16_t adv_interval_min = BLE_ADV_INTERVAL_MIN;
static uint16_t adv_interval_max = BLE_ADV_INTERVAL_MAX;
static uint8_t random_address[6] = {0xC8, 0x28, 0x32, 0xB4, 0xB4, 0x33};
static uint8_t addr_type = BLE_ADDR_TYPE_PUBLIC;
static bool_t gatts_register_done = false;
static bool_t ble_connected = false;

static void update_random_address(void)
{
    bt_cmd_le_set_random_addr_t random_addr_param;

    random_addr_param.addr.addr[0] = random_address[5];
    random_addr_param.addr.addr[1] = random_address[4];
    random_addr_param.addr.addr[2] = random_address[3];
    random_addr_param.addr.addr[3] = random_address[2];
    random_addr_param.addr.addr[4] = random_address[1];
    random_addr_param.addr.addr[5] = random_address[0];

    DBGLOG_ADV_DBG("[adv] set random address %02X:%02X:%02X:%02X:%02X:%02X", random_address[0],
                   random_address[1], random_address[2], random_address[3], random_address[4],
                   random_address[5]);

    app_bt_send_rpc_cmd(BT_CMD_LE_SET_RANDOM_ADDR, &random_addr_param, sizeof(random_addr_param));
}

static void update_adv_param(void)
{
    bt_cmd_le_set_adv_param_t adv_param;

    memset(&adv_param, 0, sizeof(adv_param));
    adv_param.interval_min = adv_interval_min;
    adv_param.interval_max = adv_interval_max;
    adv_param.channel_map = 0x07;
    adv_param.own_addr_type = addr_type;
    app_bt_send_rpc_cmd(BT_CMD_LE_SET_ADV_PARAM, &adv_param, sizeof(adv_param));
}

static void update_adv_data(void)
{
    bt_cmd_le_set_adv_data_t data_param;

    if ((!adv_data) || (!adv_data_len)) {
        DBGLOG_ADV_ERR("[adv] set_adv_data adv_data:%x adv_data_len:%d\n", adv_data, adv_data_len);
        return;
    }

    data_param.data = (void *)adv_data;
    data_param.len = adv_data_len;
    app_bt_send_rpc_cmd(BT_CMD_LE_SET_ADV_DATA, &data_param, sizeof(data_param));
}

static void update_scan_rsp(void)
{
    bt_cmd_le_set_scan_response_t scan_response_param;

    if ((!scan_rsp) || (!scan_rsp_len)) {
        return;
    }

    scan_response_param.data = (void *)scan_rsp;
    scan_response_param.len = scan_rsp_len;
    app_bt_send_rpc_cmd(BT_CMD_LE_SET_SCAN_RESPONSE, &scan_response_param,
                        sizeof(scan_response_param));
}

static void update_adv_enabled(void)
{
    bt_cmd_le_set_adv_enabled_t enable_param;

    enable_param.enabled = adv_enabled;
    app_bt_send_rpc_cmd(BT_CMD_LE_SET_ADV_ENABLED, &enable_param, sizeof(enable_param));

    DBGLOG_ADV_DBG("[adv] update_adv_enabled %d\n", adv_enabled);
}

void app_adv_set_enabled(bool_t enabled)
{
    DBGLOG_ADV_DBG("[adv] app_adv_set_enabled:%d,%d state:0x%X ble:%d\n", enabled, adv_enabled,
                   app_bt_get_sys_state(), ble_connected);
    if (adv_enabled == enabled) {
        return;
    }
    adv_enabled = enabled;
    if (ble_connected) {
        return;
    }

    if (app_bt_get_sys_state() > STATE_DISABLED) {
        if (!gatts_register_done) {
            DBGLOG_ADV_DBG("[adv] app_adv_set_enabled gatts register not done\n");
            return;
        }
        if (enabled) {
            update_adv_param();
        }
        update_adv_enabled();
    }
}

void app_adv_set_interval(uint16_t interval_min, uint16_t interval_max)
{
    assert(interval_min >= 0x20 && interval_min <= 0x4000);
    assert(interval_max >= 0x20 && interval_max <= 0x4000);
    assert(interval_max >= interval_min);
    adv_interval_min = interval_min;
    adv_interval_max = interval_max;
}

void app_adv_set_adv_data(const uint8_t *data, uint8_t data_len)
{
    adv_data = data;
    adv_data_len = data_len;

    if (app_bt_get_sys_state() > STATE_DISABLED) {
        update_adv_data();
    }
}

void app_adv_set_scan_response(const uint8_t *response, uint8_t reseponse_len)
{
    scan_rsp = response;
    scan_rsp_len = reseponse_len;

    if (app_bt_get_sys_state() > STATE_DISABLED) {
        update_scan_rsp();
    }
}

void app_adv_set_random_address(const uint8_t address[6])
{
    addr_type = BLE_ADDR_TYPE_RANDOM;
    memcpy(random_address, address, 6);

    if (app_bt_get_sys_state() > STATE_DISABLED) {
        update_random_address();
    }
}

void app_adv_handle_bt_power_on(void)
{
    DBGLOG_ADV_DBG("[adv] app_adv_handle_bt_power_on adv_enabled %d gatts_register_done %d\n",
                   adv_enabled, gatts_register_done);
    if (addr_type == BLE_ADDR_TYPE_RANDOM) {
        update_random_address();
    }

    if (!app_wws_is_master()) {
        return;
    }

    if (!adv_enabled) {
        return;
    }

    if (!gatts_register_done) {
        DBGLOG_ADV_DBG("[adv] app_adv_handle_bt_power_on gatts register not done\n");
        return;
    }

    update_adv_data();
    update_scan_rsp();
    update_adv_param();
    update_adv_enabled();
}

void app_adv_handle_gatts_register_done(void)
{
    gatts_register_done = true;
    DBGLOG_ADV_DBG("[adv] app_adv_handle_gatts_register_done adv_enabled %d state 0x%X\n",
                   adv_enabled, app_bt_get_sys_state());
    if (!adv_enabled) {
        return;
    }

    if (!app_wws_is_master()) {
        return;
    }

    if (app_bt_get_sys_state() > STATE_DISABLED) {
        update_adv_data();
        update_scan_rsp();
        update_adv_param();
        update_adv_enabled();
    }
}

void app_adv_handle_ble_connected(void)
{
    ble_connected = true;
    DBGLOG_ADV_DBG("[adv] app_adv_handle_ble_connected\n");
    //adv stoped automatically by controller
}

void app_adv_handle_ble_disconnected(void)
{
    ble_connected = false;
    DBGLOG_ADV_DBG("[adv] app_adv_handle_ble_disconnected adv_enabled:%d\n", adv_enabled);
    if (adv_enabled) {
        update_adv_param();
        update_adv_enabled();
    }
}
