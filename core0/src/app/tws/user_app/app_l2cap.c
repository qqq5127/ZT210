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
#include "app_l2cap.h"
#include "os_mem.h"

#define MAX_SERVICE_COUNT 3

typedef struct {
    uint16_t psm;
    bool_t registered;
    l2cap_connection_callback_t conn_cbk;
    l2cap_data_callback_t data_cbk;
} l2cap_service_t;

static l2cap_service_t services[MAX_SERVICE_COUNT] = {0};
static uint8_t service_count = 0;
static bool_t register_done = false;

static void register_service(uint16_t psm)
{
    bt_cmd_l2cap_register_t cmd;

    cmd.local_psm = psm;
    DBGLOG_L2CAP_DBG("l2cap register 0x%04X\n", psm);

    app_bt_send_rpc_cmd(BT_CMD_L2CAP_REGISTER, &cmd, sizeof(cmd));
}

static void bt_evt_l2cap_state_changed_handler(bt_evt_l2cap_state_changed_t *param)
{
    l2cap_service_t *service = NULL;

    for (int i = 0; i < service_count; i++) {
        if (services[i].psm == param->local_psm) {
            service = &services[i];
            break;
        }
    }

    if (!service) {
        DBGLOG_L2CAP_ERR("bt_evt_l2cap_state_changed_handler unknown psm 0x%X\n", param->local_psm);
        if (service_count == 1) {
            service = &services[0];
        } else {
            return;
        }
    }

    if (!service->conn_cbk) {
        DBGLOG_L2CAP_DBG("l2cap conn_cbk==NULL psm:0x%04x\n", service->psm);
        return;
    }

    if (param->state == L2CAP_STATE_CONNECTED) {
        service->conn_cbk(&param->addr, service->psm, true);
    } else if (param->state == L2CAP_STATE_DISCONNECTED) {
        service->conn_cbk(&param->addr, service->psm, false);
    } else {
        DBGLOG_L2CAP_ERR("l2cap unknown state:%d\n", param->state);
    }
}

static void bt_evt_l2cap_data_handler(bt_evt_l2cap_data_t *param)
{
    l2cap_service_t *service = NULL;

    if (!param->data) {
        DBGLOG_L2CAP_ERR("bt_evt_l2cap_data_handler data==NULL\n");
        return;
    }

    for (int i = 0; i < service_count; i++) {
        if (services[i].psm == param->local_psm) {
            service = &services[i];
            break;
        }
    }

    if (!service) {
        DBGLOG_L2CAP_ERR("bt_evt_l2cap_data_handler unknown psm 0x%X\n", param->local_psm);
        if (service_count == 1) {
            service = &services[0];
        } else {
            os_mem_free(param->data);
            return;
        }
    }

    if (!service->data_cbk) {
        DBGLOG_L2CAP_DBG("spp data_cbk==NULL psm:0x%04x\n", service->psm);
        os_mem_free(param->data);
        return;
    }

    service->data_cbk(&param->addr, service->psm, param->data, param->len);
}

void app_l2cap_register_service(uint16_t psm, l2cap_connection_callback_t conn_cbk,
                                l2cap_data_callback_t data_cbk)
{
    if (service_count >= MAX_SERVICE_COUNT) {
        DBGLOG_L2CAP_ERR("app_l2cap_register_service failed, max count\n");
        return;
    }

    for (int i = 0; i < service_count; i++) {
        if (services[i].psm == psm) {
            DBGLOG_L2CAP_ERR("app_l2cap_register_service failed, already registered\n");
            return;
        }
    }

    services[service_count].psm = psm;
    services[service_count].conn_cbk = conn_cbk;
    services[service_count].data_cbk = data_cbk;
    service_count += 1;

    if (register_done) {
        register_done = false;
        DBGLOG_L2CAP_DBG("app_spp_register_service start register again for new service\n");
        register_service(psm);
        register_done = true;
    }
}

int app_l2cap_send_data(BD_ADDR_T *addr, uint16_t psm, const uint8_t *data, uint16_t len)
{
    bt_cmd_l2cap_send_t cmd;
    bool_t found = false;

    for (int i = 0; i < service_count; i++) {
        if (services[i].psm == psm) {
            found = true;
            break;
        }
    }

    if (!found) {
        DBGLOG_L2CAP_ERR("app_l2cap_send_data psm 0x%04X not found\n", psm);
        return RET_FAIL;
    }

    cmd.addr = *addr;
    cmd.local_psm = psm;
    cmd.data = data;
    cmd.length = len;

    return app_bt_send_rpc_cmd(BT_CMD_L2CAP_SEND, &cmd, sizeof(cmd));
}

void app_l2cap_handle_bt_evt(uint16_t evt, void *param)
{
    switch (evt) {
        case BT_EVT_L2CAP_STATE_CHANGED:
            bt_evt_l2cap_state_changed_handler(param);
            break;
        case BT_EVT_L2CAP_DATA:
            bt_evt_l2cap_data_handler(param);
            break;
        default:
            assert(0);
            break;
    }
}

void app_l2cap_handle_bt_power_on(void)
{
    if (service_count > 0) {
        register_done = false;
        for (uint8_t i = 0; i < service_count; i++) {
            register_service(services[i].psm);
        }
        register_done = true;
    } else {
        register_done = true;
    }
}
