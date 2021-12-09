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
#include "app_spp.h"
#include "os_mem.h"

#ifndef MAX_SPP_CLIENT_COUNT
#define MAX_SPP_CLIENT_COUNT 1
#endif

#ifndef MAX_SPP_SERVICE_COUNT
#define MAX_SPP_SERVICE_COUNT 3
#endif

typedef struct {
    BD_ADDR_T addr;
    bool_t used;
    uint8_t channel;
    spp_client_connection_callback_t conn_cbk;
    spp_client_data_callback_t data_cbk;
} spp_client_t;

#ifndef MAX_SPP_CALLBACK_COUNT_PER_SERVICE
#define MAX_SPP_CALLBACK_COUNT_PER_SERVICE 1
#endif

typedef struct {
    uint8_t uuid128[16];
    uint16_t uuid;
    uint8_t channel;
    bool_t registered;
    union {
        spp_connection_callback_t conn_cbk[MAX_SPP_CALLBACK_COUNT_PER_SERVICE];
        spp_connection_callback_ext_t conn_cbk_ext[MAX_SPP_CALLBACK_COUNT_PER_SERVICE];
    };
    union {
        spp_data_callback_t data_cbk[MAX_SPP_CALLBACK_COUNT_PER_SERVICE];
        spp_data_callback_ext_t data_cbk_ext[MAX_SPP_CALLBACK_COUNT_PER_SERVICE];
    };
} spp_service_t;

static spp_client_t clients[MAX_SPP_CLIENT_COUNT];

static spp_service_t services[MAX_SPP_SERVICE_COUNT] = {0};
static uint8_t service_count = 0;
static bool_t register_done = false;

static void register_service(uint16_t uuid)
{
    bt_cmd_spp_register_t cmd;

    cmd.uuid16 = uuid;
    DBGLOG_SPP_DBG("spp register 0x%04X\n", uuid);

    app_bt_send_rpc_cmd(BT_CMD_SPP_REGISTER, &cmd, sizeof(cmd));
}

static void register_service_ext(const uint8_t uuid128[16])
{
    bt_cmd_spp_register_uuid128_t cmd;

    memcpy(cmd.uuid128, uuid128, 16);
    DBGLOG_SPP_DBG("spp register ext %02X%02X%02X%02X\n", uuid128[0], uuid128[1], uuid128[2],
                   uuid128[3]);

    app_bt_send_rpc_cmd(BT_CMD_SPP_REGISTER_UUID128, &cmd, sizeof(cmd));
}

static void client_bt_evt_spp_state_changed_handler(bt_evt_spp_state_changed_t *param)
{
    spp_client_t *client = NULL;

    for (int i = 0; i < MAX_SPP_CLIENT_COUNT; i++) {
        if (clients[i].used && bdaddr_is_equal(&param->addr, &clients[i].addr)
            && (param->channel == clients[i].channel)) {
            client = &clients[i];
            break;
        }
    }

    if (!client) {
        DBGLOG_SPP_ERR("client_bt_evt_spp_state_changed_handler not found, channel:%d\n",
                       param->channel);
        bdaddr_print(&param->addr);
        return;
    }

    if (!client->conn_cbk) {
        DBGLOG_SPP_ERR("client->conn_cbk error\n");
        return;
    }

    if (param->state == SPP_STATE_CONNECTED) {
        client->conn_cbk(&param->addr, param->channel, true);
    } else if (param->state == SPP_STATE_DISCONNECTED) {
        client->conn_cbk(&param->addr, param->channel, false);
        client->used = false;
    } else {
        DBGLOG_SPP_ERR("spp client unknown state:%d\n", param->state);
    }
}

static void client_bt_evt_spp_data_handler(bt_evt_spp_data_t *param)
{
    spp_client_t *client = NULL;

    for (int i = 0; i < MAX_SPP_CLIENT_COUNT; i++) {
        if (clients[i].used && bdaddr_is_equal(&param->addr, &clients[i].addr)
            && (param->channel == clients[i].channel)) {
            client = &clients[i];
            break;
        }
    }

    if (!client) {
        DBGLOG_SPP_ERR("client_bt_evt_spp_state_changed_handler not found, channel:%d\n",
                       param->channel);
        bdaddr_print(&param->addr);
        return;
    }

    if (!client->data_cbk) {
        return;
    }

    client->data_cbk(&param->addr, param->channel, param->data, param->len);
}

static void bt_evt_spp_state_changed_handler(bt_evt_spp_state_changed_t *param)
{
    spp_service_t *service = NULL;
    DBGLOG_SPP_DBG("BT_EVT_SPP_STATE_CHANGED is_client:%d channel:%d state:%d\n", param->is_client,
                   param->channel, param->state);

    if (param->is_client) {
        client_bt_evt_spp_state_changed_handler(param);
        return;
    }

    for (int i = 0; i < service_count; i++) {
        if (services[i].channel == param->channel) {
            service = &services[i];
            break;
        }
    }

    if (!service) {
        DBGLOG_SPP_ERR("bt_evt_spp_state_changed_handler unknown channel 0x%X\n", param->channel);
        if (service_count == 1) {
            service = &services[0];
        } else {
            return;
        }
    }

    if (service->uuid) {
        if (!service->conn_cbk[0]) {
            DBGLOG_SPP_DBG("spp conn_cbk==NULL uuid:0x%04x\n", service->uuid);
        }

        if (param->state == SPP_STATE_CONNECTED) {
            for (int i = 0; i < MAX_SPP_CALLBACK_COUNT_PER_SERVICE; i++) {
                if (service->conn_cbk[i]) {
                    service->conn_cbk[i](&param->addr, service->uuid, true);
                }
            }
        } else if (param->state == SPP_STATE_DISCONNECTED) {
            for (int i = 0; i < MAX_SPP_CALLBACK_COUNT_PER_SERVICE; i++) {
                if (service->conn_cbk[i]) {
                    service->conn_cbk[i](&param->addr, service->uuid, false);
                }
            }
        } else {
            DBGLOG_SPP_ERR("spp unknown state:%d\n", param->state);
        }
    } else {
        if (!service->conn_cbk_ext[0]) {
            DBGLOG_SPP_DBG("spp conn_cbk_ext==NULL uuid:%02X%02X%02X%02X\n", service->uuid128[0],
                           service->uuid128[1], service->uuid128[2], service->uuid128[3]);
        }

        if (param->state == SPP_STATE_CONNECTED) {
            for (int i = 0; i < MAX_SPP_CALLBACK_COUNT_PER_SERVICE; i++) {
                if (service->conn_cbk_ext[i]) {
                    service->conn_cbk_ext[i](&param->addr, service->uuid128, true);
                }
            }
        } else if (param->state == SPP_STATE_DISCONNECTED) {
            for (int i = 0; i < MAX_SPP_CALLBACK_COUNT_PER_SERVICE; i++) {
                if (service->conn_cbk_ext[i]) {
                    service->conn_cbk_ext[i](&param->addr, service->uuid128, false);
                }
            }
        } else {
            DBGLOG_SPP_ERR("spp unknown state:%d\n", param->state);
        }
    }
}

static void bt_evt_spp_data_handler(bt_evt_spp_data_t *param)
{
    spp_service_t *service = NULL;

    if (param->is_client) {
        client_bt_evt_spp_data_handler(param);
        return;
    }

    if (!param->data) {
        DBGLOG_SPP_ERR("bt_evt_spp_data_handler data==NULL\n");
        return;
    }

    for (int i = 0; i < service_count; i++) {
        if (services[i].channel == param->channel) {
            service = &services[i];
            break;
        }
    }

    if (!service) {
        DBGLOG_SPP_ERR("bt_evt_spp_data_handler unknown channel, 0x%X\n", param->channel);
        if (service_count == 1) {
            service = &services[0];
        } else {
            os_mem_free(param->data);
            return;
        }
    }

    if (service->uuid) {
        for (int i = 1; i < MAX_SPP_CALLBACK_COUNT_PER_SERVICE; i++) {
            if (service->data_cbk[i]) {
                void *new_data = os_mem_malloc(IOT_APP_MID, param->len);
                if (!new_data) {
                    DBGLOG_SPP_ERR("bt_evt_spp_data_handler malloc failed\n");
                    continue;
                }
                memcpy(new_data, param->data, param->len);
                service->data_cbk[i](&param->addr, service->uuid, new_data, param->len);
            }
        }

        if (!service->data_cbk[0]) {
            DBGLOG_SPP_DBG("spp data_cbk==NULL uuid:0x%04x\n", service->uuid);
            os_mem_free(param->data);
            return;
        } else {
            service->data_cbk[0](&param->addr, service->uuid, param->data, param->len);
        }
    } else {
        for (int i = 1; i < MAX_SPP_CALLBACK_COUNT_PER_SERVICE; i++) {
            if (service->data_cbk_ext[i]) {
                void *new_data = os_mem_malloc(IOT_APP_MID, param->len);
                if (!new_data) {
                    DBGLOG_SPP_ERR("bt_evt_spp_data_handler malloc failed\n");
                    continue;
                }
                memcpy(new_data, param->data, param->len);
                service->data_cbk_ext[i](&param->addr, service->uuid128, new_data, param->len);
            }
        }

        if (!service->data_cbk_ext[0]) {
            DBGLOG_SPP_DBG("spp data_cbk_ext==NULL uuid:%02X%02X%02X%02X\n", service->uuid128[0],
                           service->uuid128[1], service->uuid128[2], service->uuid128[3]);
            os_mem_free(param->data);
            return;
        } else {
            service->data_cbk_ext[0](&param->addr, service->uuid128, param->data, param->len);
        }
    }
}

static void bt_evt_spp_registered_handler(bt_evt_spp_registered_t *param)
{
    uint8_t found_index = 0xFF;

    for (int i = 0; i < MAX_SPP_SERVICE_COUNT; i++) {
        if (services[i].uuid == param->uuid16) {
            found_index = i;
            services[i].channel = param->channel;
            services[i].registered = true;
            break;
        }
    }

    if (found_index == 0xFF) {
        DBGLOG_SPP_ERR("spp registered, unknown uuid 0x%04X, register done", param->uuid16);
        register_done = true;
        return;
    } else {
        DBGLOG_SPP_DBG("spp registered, uuid:0x%04X, channel:%d\n", param->uuid16, param->channel);
    }

    if (found_index + 1 < service_count) {
        if (services[found_index + 1].uuid) {
            register_service(services[found_index + 1].uuid);
        } else {
            register_service_ext(services[found_index + 1].uuid128);
        }
    } else {
        DBGLOG_SPP_DBG("spp register done\n");
    }
}

static void bt_evt_spp_uuid128_registered_handler(bt_evt_spp_uuid128_registered_t *param)
{
    uint8_t found_index = 0xFF;

    for (int i = 0; i < MAX_SPP_SERVICE_COUNT; i++) {
        if (services[i].uuid) {
            continue;
        }
        if (!memcmp(services[i].uuid128, param->uuid128, 16)) {
            found_index = i;
            services[i].channel = param->channel;
            services[i].registered = true;
            break;
        }
    }

    if (found_index == 0xFF) {
        DBGLOG_SPP_ERR("spp registered, unknown uuid:%02X%02X%02X%02X, register done",
                       param->uuid128[0], param->uuid128[1], param->uuid128[2], param->uuid128[3]);
        register_done = true;
        return;
    } else {
        DBGLOG_SPP_DBG("spp registered, uuid:%02X%02X%02X%02X, channel:%d\n", param->uuid128[0],
                       param->uuid128[1], param->uuid128[2], param->uuid128[3], param->channel);
    }

    if (found_index + 1 < service_count) {
        if (services[found_index + 1].uuid) {
            register_service(services[found_index + 1].uuid);
        } else {
            register_service_ext(services[found_index + 1].uuid128);
        }
    } else {
        DBGLOG_SPP_DBG("spp register done\n");
    }
}

void app_spp_register_service(uint16_t uuid, spp_connection_callback_t conn_cbk,
                              spp_data_callback_t data_cbk)
{
    bool_t reuse = false;
    int reuse_callback_index = 0;
    int reuse_service_index = 0;

    for (int i = 0; i < service_count; i++) {
        if (services[i].uuid == uuid) {
            for (int j = 0; j < MAX_SPP_CALLBACK_COUNT_PER_SERVICE; j++) {
                if ((services[i].conn_cbk[j] == NULL) && (services[i].data_cbk[j] == NULL)) {
                    reuse = true;
                    reuse_service_index = i;
                    reuse_callback_index = j;
                    break;
                }
            }

            if (!reuse) {
                DBGLOG_SPP_ERR("app_spp_register_service failed, already registered\n");
                return;
            } else {
                break;
            }
        }
    }

    if ((!reuse) && (service_count >= MAX_SPP_SERVICE_COUNT)) {
        DBGLOG_SPP_ERR("app_spp_register_service failed, max count\n");
        return;
    }

    if (!reuse) {
        memset(&services[service_count], 0, sizeof(spp_service_t));
        services[service_count].uuid = uuid;
        services[service_count].conn_cbk[0] = conn_cbk;
        services[service_count].data_cbk[0] = data_cbk;
        service_count += 1;

        if (register_done) {
            register_done = false;
            DBGLOG_SPP_DBG("app_spp_register_service start register again for new service\n");
            register_service(uuid);
        }
    } else {
        services[reuse_service_index].conn_cbk[reuse_callback_index] = conn_cbk;
        services[reuse_service_index].data_cbk[reuse_callback_index] = data_cbk;
    }
}

int app_spp_send_data(const BD_ADDR_T *addr, uint16_t uuid, const uint8_t *data, uint16_t len)
{
    bt_cmd_spp_send_t cmd;
    uint8_t channel = 0xFF;

    for (int i = 0; i < service_count; i++) {
        if (services[i].uuid == uuid) {
            channel = services[i].channel;
            break;
        }
    }

    if (channel == 0xFF) {
        DBGLOG_SPP_ERR("app_spp_send_data service 0x%04X not found\n", uuid);
        return RET_FAIL;
    }

    cmd.addr = *addr;
    cmd.is_client = false;
    cmd.channel = channel;
    cmd.data = data;
    cmd.length = len;

    return app_bt_send_rpc_cmd(BT_CMD_SPP_SEND, &cmd, sizeof(cmd));
}

void app_spp_register_service_ext(const uint8_t uuid128[16],
                                  spp_connection_callback_ext_t conn_cbk_ext,
                                  spp_data_callback_ext_t data_cbk_ext)
{
    bool_t reuse = false;
    int reuse_callback_index = 0;
    int reuse_service_index = 0;

    for (int i = 0; i < service_count; i++) {
        if (services[i].uuid) {
            continue;
        }
        if (!memcmp(services[i].uuid128, uuid128, 16)) {
            for (int j = 0; j < MAX_SPP_CALLBACK_COUNT_PER_SERVICE; j++) {
                if ((services[i].conn_cbk_ext[j] == NULL)
                    && (services[i].data_cbk_ext[j] == NULL)) {
                    reuse = true;
                    reuse_service_index = i;
                    reuse_callback_index = j;
                    break;
                }
            }

            if (!reuse) {
                DBGLOG_SPP_ERR("app_spp_register_service_ext failed, already registered\n");
                return;
            } else {
                break;
            }
        }
    }

    if ((!reuse) && (service_count >= MAX_SPP_SERVICE_COUNT)) {
        DBGLOG_SPP_ERR("app_spp_register_service_ext failed, max count\n");
        return;
    }

    if (!reuse) {
        memset(&services[service_count], 0, sizeof(spp_service_t));
        memcpy(services[service_count].uuid128, uuid128, 16);
        services[service_count].conn_cbk_ext[0] = conn_cbk_ext;
        services[service_count].data_cbk_ext[0] = data_cbk_ext;
        service_count += 1;

        if (register_done) {
            register_done = false;
            DBGLOG_SPP_DBG("app_spp_register_service_ext start register again for new service\n");
            register_service_ext(uuid128);
        }
    } else {
        services[reuse_service_index].conn_cbk_ext[reuse_callback_index] = conn_cbk_ext;
        services[reuse_service_index].data_cbk_ext[reuse_callback_index] = data_cbk_ext;
    }
}

int app_spp_send_data_ext(const BD_ADDR_T *addr, const uint8_t uuid128[16], const uint8_t *data,
                          uint16_t len)
{
    bt_cmd_spp_send_t cmd;
    uint8_t channel = 0xFF;

    for (int i = 0; i < service_count; i++) {
        if (services[i].uuid != 0) {
            continue;
        }
        if (!memcmp(services[i].uuid128, uuid128, 16)) {
            channel = services[i].channel;
            break;
        }
    }

    if (channel == 0xFF) {
        DBGLOG_SPP_ERR("app_spp_send_data service %02X%02X%02X%02X not found\n", uuid128[0],
                       uuid128[1], uuid128[2], uuid128[3]);
        return RET_FAIL;
    }

    cmd.is_client = false;
    cmd.addr = *addr;
    cmd.channel = channel;
    cmd.data = data;
    cmd.length = len;

    return app_bt_send_rpc_cmd(BT_CMD_SPP_SEND, &cmd, sizeof(cmd));
}

int app_spp_disconnect(const BD_ADDR_T *addr, uint16_t uuid)
{
    uint8_t channel = 0xFF;
    bt_cmd_spp_disconnect_t cmd;

    for (int i = 0; i < service_count; i++) {
        if (services[i].uuid == uuid) {
            channel = services[i].channel;
            break;
        }
    }

    if (channel == 0xFF) {
        DBGLOG_SPP_ERR("app_spp_disconnect service 0x%04X not found\n", uuid);
        return RET_FAIL;
    }

    cmd.addr = *addr;
    cmd.channel = channel;
    cmd.is_client = false;

    return app_bt_send_rpc_cmd(BT_CMD_SPP_DISCONNECT, &cmd, sizeof(cmd));
}

int app_spp_disconnect_ext(const BD_ADDR_T *addr, const uint8_t uuid128[16])
{
    uint8_t channel = 0xFF;
    bt_cmd_spp_disconnect_t cmd;

    for (int i = 0; i < service_count; i++) {
        if (services[i].uuid != 0) {
            continue;
        }
        if (!memcmp(services[i].uuid128, uuid128, 16)) {
            channel = services[i].channel;
            break;
        }
    }

    if (channel == 0xFF) {
        DBGLOG_SPP_ERR("app_spp_disconnect_ext service %02X%02X%02X%02X not found\n", uuid128[0],
                       uuid128[1], uuid128[2], uuid128[3]);
        return RET_FAIL;
    }

    cmd.addr = *addr;
    cmd.channel = channel;
    cmd.is_client = false;

    return app_bt_send_rpc_cmd(BT_CMD_SPP_DISCONNECT, &cmd, sizeof(cmd));
}

int app_spp_client_connect(const BD_ADDR_T *addr, uint8_t channel,
                           spp_client_connection_callback_t conn_cbk,
                           spp_client_data_callback_t data_cbk)
{
    bt_cmd_spp_connect_t cmd;
    spp_client_t *client = NULL;
    int ret;

    for (int i = 0; i < MAX_SPP_CLIENT_COUNT; i++) {
        if (!clients[i].used) {
            client = &clients[i];
            break;
        }
    }

    if (!client) {
        DBGLOG_SPP_ERR("app_spp_client_connect error: max connection\n");
        return BT_RESULT_ALREADY_EXISTS;
    }

    cmd.addr = *addr;
    cmd.channel = channel;

    client->used = true;
    client->addr = cmd.addr;
    client->channel = cmd.channel;
    client->conn_cbk = conn_cbk;
    client->data_cbk = data_cbk;

    ret = app_bt_send_rpc_cmd(BT_CMD_SPP_CONNECT, &cmd, sizeof(cmd));

    if (ret) {
        DBGLOG_SPP_DBG("app_spp_client_connect error ret:%d\n", ret);
        client->used = false;
    }

    return ret;
}

int app_spp_client_disconnect(const BD_ADDR_T *addr, uint8_t channel)
{
    bt_cmd_spp_disconnect_t cmd;
    cmd.addr = *addr;
    cmd.channel = channel;
    cmd.is_client = true;

    return app_bt_send_rpc_cmd(BT_CMD_SPP_DISCONNECT, &cmd, sizeof(cmd));
}

int app_spp_client_send_data(const BD_ADDR_T *addr, uint8_t channel, const uint8_t *data,
                             uint16_t len)
{
    bt_cmd_spp_send_t cmd;

    cmd.addr = *addr;
    cmd.is_client = true;
    cmd.channel = channel;
    cmd.data = data;
    cmd.length = len;

    return app_bt_send_rpc_cmd(BT_CMD_SPP_SEND, &cmd, sizeof(cmd));
}

void app_spp_handle_bt_evt(uint16_t evt, void *param)
{
    switch (evt) {
        case BT_EVT_SPP_STATE_CHANGED:
            bt_evt_spp_state_changed_handler(param);
            break;
        case BT_EVT_SPP_REGISTERED:
            bt_evt_spp_registered_handler(param);
            break;
        case BT_EVT_SPP_UUID128_REGISTERED:
            bt_evt_spp_uuid128_registered_handler(param);
            break;
        case BT_EVT_SPP_DATA:
            bt_evt_spp_data_handler(param);
            break;
        default:
            assert(0);
            break;
    }
}

void app_spp_handle_bt_power_on(void)
{
    if (service_count > 0) {
        register_done = false;
        if (services[0].uuid) {
            register_service(services[0].uuid);
        } else {
            register_service_ext(services[0].uuid128);
        }
    } else {
        register_done = true;
    }
}
