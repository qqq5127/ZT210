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

#include "app_gatts.h"
#include "bt_rpc_api.h"
#include "app_bt.h"
#include "os_mem.h"
#include "modules.h"
#include "assert.h"
#include "string.h"
#include "app_adv.h"

#define MAX_CONNECTION_CALLBACK 4

static gatts_connection_callback_t connection_callback[MAX_CONNECTION_CALLBACK] = {0};

static bool_t register_done = false;
static gatts_service_t *services = NULL;

static gatts_service_t *cur_service = NULL;
static gatts_character_t *cur_character = NULL;

static uint16_t device_name_read_callback(BD_ADDR_T *addr, gatts_character_t *character,
                                          uint8_t *data, uint16_t max_len)
{
    UNUSED(addr);
    UNUSED(character);

    const char *local_name = app_bt_get_local_name();
    strlcpy((char *)data, local_name, max_len);
    return strlen((char *)data);
}

static void register_service(gatts_service_t *service)
{
    bt_cmd_gatt_server_register_service_t cmd;
    bt_cmd_gatt_server_register_service_uuid128_t cmd_uuid128;

    if (service->handle != 0xFFFF) {
        register_done = true;
        DBGLOG_GATTS_ERR("register_service %X error, already registered handle:%X\n", service->uuid,
                         service->handle);
        app_adv_handle_gatts_register_done();
        return;
    }

    if (service->uuid) {
        cmd.uuid16 = service->uuid;
        app_bt_send_rpc_cmd(BT_CMD_GATT_SERVER_REGISTER_SERVICE, &cmd, sizeof(cmd));
    } else {
        memcpy(cmd_uuid128.uuid128, service->uuid128, 16);
        app_bt_send_rpc_cmd(BT_CMD_GATT_SERVER_REGISTER_SERVICE_UUID128, &cmd_uuid128,
                            sizeof(cmd_uuid128));
    }
}

static void register_character(gatts_service_t *service, gatts_character_t *character)
{
    bt_cmd_gatt_server_register_characteristic_t cmd;
    bt_cmd_gatt_server_register_characteristic_uuid128_t cmd_uuid128;

    assert(service);
    assert(character);

    if (character->handle != 0xFFFF) {
        register_done = true;
        DBGLOG_GATTS_ERR("register_character %X error, already registered handle:%X\n",
                         character->uuid, character->handle);
        app_adv_handle_gatts_register_done();
        return;
    }

    if (character->uuid) {
        cmd.chr_uuid16 = character->uuid;
        cmd.property = character->props;
        cmd.service_handle = service->handle;

        app_bt_send_rpc_cmd(BT_CMD_GATT_SERVER_REGISTER_CHARACTERISTIC, &cmd, sizeof(cmd));
    } else {
        memcpy(cmd_uuid128.chr_uuid128, character->uuid128, 16);
        cmd_uuid128.property = character->props;
        cmd_uuid128.service_handle = service->handle;

        app_bt_send_rpc_cmd(BT_CMD_GATT_SERVER_REGISTER_CHARACTERISTIC_UUID128, &cmd_uuid128,
                            sizeof(cmd_uuid128));
    }
}

static gatts_service_t *find_service(uint16_t uuid)
{
    gatts_service_t *sp = services;

    while (sp) {
        if (sp->uuid == uuid) {
            return sp;
        }
        sp = sp->next;
    }

    return NULL;
}

static gatts_service_t *find_service_ext(const uint8_t uuid128[16])
{
    gatts_service_t *sp = services;

    while (sp) {
        if (sp->uuid) {
            sp = sp->next;
            continue;
        }
        if (!memcmp(sp->uuid128, uuid128, 16)) {
            return sp;
        }
        sp = sp->next;
    }

    return NULL;
}

static gatts_character_t *find_character(uint16_t handle)
{
    gatts_service_t *sp = services;
    gatts_character_t *cp;

    while (sp) {
        cp = sp->characters;

        while (cp) {
            if ((cp->handle == handle) || (cp->ccd_handle == handle)) {
                return cp;
            }
            cp = cp->next;
        }

        sp = sp->next;
    }

    return NULL;
}

static void handle_state_changed(bt_evt_gatt_server_state_changed_t *evt)
{
    gatts_connection_callback_t callback;
    bool_t is_ble;

    if (evt->link_type == GATT_LINK_TYPE_BLE) {
        is_ble = true;
    } else {
        is_ble = false;
    }

    DBGLOG_GATTS_DBG("handle_state_changed state:%d addr_type:%d is_ble:%d\n", evt->state,
                     evt->addr_type, is_ble);

    if (is_ble) {
        if (evt->state == GATT_STATE_DISCONNECTED) {
            app_adv_handle_ble_disconnected();
        } else if (evt->state == GATT_STATE_CONNECTED) {
            app_adv_handle_ble_connected();
        }
    }

    for (int i = 0; i < MAX_CONNECTION_CALLBACK; i++) {
        callback = connection_callback[i];
        if (!callback) {
            break;
        }
        if (evt->state == GATT_STATE_DISCONNECTED) {
            callback(&evt->addr, is_ble, false);
        } else if (evt->state == GATT_STATE_CONNECTED) {
            callback(&evt->addr, is_ble, true);
        }
    }
}

static void handle_service_registered(bt_evt_gatt_server_service_registered_t *evt)
{
    assert(cur_service);

    DBGLOG_GATTS_DBG("service registered uuid:%X handle:%X\n", evt->uuid16, evt->handle);
    cur_service->handle = evt->handle;

    if (!cur_service->characters) {
        DBGLOG_GATTS_DBG("gatt server register done, no characters\n");
        register_done = true;
        app_adv_handle_gatts_register_done();
        return;
    }

    cur_character = cur_service->characters;
    register_character(cur_service, cur_character);
}

static void handle_character_registered(bt_evt_gatt_server_characteristic_registered_t *evt)
{
    assert(cur_service);
    assert(cur_character);

    DBGLOG_GATTS_DBG("character registered uuid:0x%X handle:0x%X ccd_handle:0x%X\n", evt->uuid16,
                     evt->handle, evt->ccd_handle);

    cur_character->handle = evt->handle;
    cur_character->ccd_handle = evt->ccd_handle;
    cur_character->ccd_value = 0;

    if (cur_character->next) {
        cur_character = cur_character->next;
        register_character(cur_service, cur_character);
        return;
    }

    if (!cur_service->next) {
        DBGLOG_GATTS_DBG("gatt server register done\n");
        register_done = true;
        app_adv_handle_gatts_register_done();
        return;
    }

    cur_service = cur_service->next;
    if (!cur_service->characters) {
        DBGLOG_GATTS_DBG("gatt server register done, no characters1\n");
        register_done = true;
        app_adv_handle_gatts_register_done();
        return;
    }

    cur_character = NULL;
    register_service(cur_service);
}

static void handle_service_uuid128_registered(bt_evt_gatt_server_service_uuid128_registered_t *evt)
{
    assert(cur_service);

    DBGLOG_GATTS_DBG("service registered uuid128:%02X%02X%02X%02X handle:%X\n", evt->uuid128[0],
                     evt->uuid128[1], evt->uuid128[2], evt->uuid128[3], evt->handle);
    cur_service->handle = evt->handle;

    if (!cur_service->characters) {
        DBGLOG_GATTS_DBG("gatt server register done, no characters ext\n");
        register_done = true;
        app_adv_handle_gatts_register_done();
        return;
    }

    cur_character = cur_service->characters;
    register_character(cur_service, cur_character);
}

static void
handle_character_uuid128_registered(bt_evt_gatt_server_characteristic_uuid128_registered_t *evt)
{
    assert(cur_service);
    assert(cur_character);

    DBGLOG_GATTS_DBG("character registered uuid128:%02X%02X%02X%02X handle:0x%X ccd_handle:0x%X\n",
                     evt->uuid128[0], evt->uuid128[1], evt->uuid128[2], evt->uuid128[3],
                     evt->handle, evt->ccd_handle);

    cur_character->handle = evt->handle;
    cur_character->ccd_handle = evt->ccd_handle;
    cur_character->ccd_value = 0;

    if (cur_character->next) {
        cur_character = cur_character->next;
        register_character(cur_service, cur_character);
        return;
    }

    if (!cur_service->next) {
        DBGLOG_GATTS_DBG("gatt server register done\n");
        register_done = true;
        app_adv_handle_gatts_register_done();
        return;
    }

    cur_service = cur_service->next;
    if (!cur_service->characters) {
        DBGLOG_GATTS_DBG("gatt server register done, no characters ext1\n");
        register_done = true;
        app_adv_handle_gatts_register_done();
        return;
    }

    cur_character = NULL;
    register_service(cur_service);
}

static void handle_write(bt_evt_gatt_server_write_t *evt)
{
    gatts_character_t *character;

    if (!evt->data) {
        DBGLOG_GATTS_ERR("gatts handle_write data==NULL\n", evt->handle);
        return;
    }

    character = find_character(evt->handle);
    if (!character) {
        DBGLOG_GATTS_ERR("gatts handle_write character not found 0x%X\n", evt->handle);
        os_mem_free(evt->data);
        return;
    }

    if (evt->handle == character->handle) {
        if (!character->write_callback) {
            DBGLOG_GATTS_ERR("gatts handle_write write_callback==NULL 0x%X\n", evt->handle);
            os_mem_free(evt->data);
            return;
        }

        character->write_callback(&evt->addr, character, evt->data, evt->len);
    } else {
        if (evt->len != 2) {
            DBGLOG_GATTS_ERR("gatts handle_write invalid ccd value len:%d\n", evt->len);
            os_mem_free(evt->data);
            return;
        }

        if (!character->notify_enable_callback) {
            DBGLOG_GATTS_ERR("gatts handle_write notify_enable_callback==NULL 0x%X\n", evt->handle);
            os_mem_free(evt->data);
            return;
        }

        character->ccd_value = *((uint16_t *)evt->data);
        os_mem_free(evt->data);

        character->notify_enable_callback(&evt->addr, character, !!character->ccd_value);
    }
}

static void handle_read(bt_evt_gatt_server_read_t *evt)
{
    gatts_character_t *character;
    bt_cmd_gatt_server_send_read_response_t cmd;
    bool_t data_alloced = false;
    uint8_t default_data[2] = {0, 0};
    uint8_t *data = default_data;
    uint16_t data_len = sizeof(default_data);

    character = find_character(evt->handle);

    if (!character) {
        DBGLOG_GATTS_ERR("gatts handle_read character not found 0x%X\n", evt->handle);
        goto read_done;
    }

    if (!evt->len) {
        DBGLOG_GATTS_ERR("gatts handle_read len==0\n", evt->handle);
        goto read_done;
    }

    if (evt->handle == character->ccd_handle) {
        data = (uint8_t *)&character->ccd_value;
        data_len = sizeof(character->ccd_value);
        goto read_done;
    }

    if (!character->read_callback) {
        DBGLOG_GATTS_ERR("gatts handle_read callback==NULL 0x%X\n", evt->handle);
        goto read_done;
    }

    data = os_mem_malloc(IOT_APP_MID, evt->len);

    if (!data) {
        DBGLOG_GATTS_ERR("gatts handle_read alloc error\n", evt->handle);
        data = default_data;
        data_len = sizeof(default_data);
        goto read_done;
    }

    data_alloced = true;
    data_len = character->read_callback(&evt->addr, character, data, evt->len);
    if (!data_len) {
        DBGLOG_GATTS_ERR("gatts handle_read ret==0\n", evt->handle);
        os_mem_free(data);
        data_alloced = false;
        data = default_data;
        data_len = sizeof(default_data);
        goto read_done;
    }

read_done:
    cmd.addr = evt->addr;
    cmd.handle = evt->handle;
    cmd.offset = evt->offset;
    cmd.data = data;
    cmd.length = data_len;

    app_bt_send_rpc_cmd(BT_CMD_GATT_SERVER_SEND_READ_RESPONSE, &cmd, sizeof(cmd));

    if (data_alloced) {
        os_mem_free(data);
    }
}

static void handle_mtu_changed(bt_evt_gatt_server_mtu_changed_t *evt)
{
    DBGLOG_GATTS_DBG("gatts mtu changed to %d\n", evt->mtu);
}

bool_t app_gatts_send_notify(BD_ADDR_T *addr, gatts_character_t *character, const uint8_t *data,
                             uint16_t len)
{
    assert(addr);
    assert(character);
    assert(data);

    bt_cmd_gatt_server_notify_t cmd;
    int ret;

    cmd.addr = *addr;
    cmd.handle = character->handle;
    cmd.data = data;
    cmd.length = len;
    DBGLOG_GATTS_DBG("gatts send notify handle 0x%X len %d\n", character->handle, len);
    ret = app_bt_send_rpc_cmd(BT_CMD_GATT_SERVER_NOTIFY, &cmd, sizeof(cmd));

    if (ret) {
        return false;
    } else {
        return true;
    }
}

bool_t app_gatts_send_indicate(BD_ADDR_T *addr, gatts_character_t *character, const uint8_t *data,
                               uint16_t len)
{
    assert(addr);
    assert(character);
    assert(data);

    bt_cmd_gatt_server_indicate_t cmd;
    int ret;

    cmd.addr = *addr;
    cmd.handle = character->handle;
    cmd.data = data;
    cmd.length = len;

    ret = app_bt_send_rpc_cmd(BT_CMD_GATT_SERVER_INDICATE, &cmd, sizeof(cmd));

    if (ret) {
        return false;
    } else {
        return true;
    }
}

gatts_service_t *app_gatts_register_service(uint16_t uuid)
{
    gatts_service_t *service;

    service = find_service(uuid);
    if (service) {
        DBGLOG_GATTS_DBG("app_gatts_register_service %X already registered\n", uuid);
        return service;
    }

    service = os_mem_malloc(IOT_APP_MID, sizeof(gatts_service_t));
    memset(service, 0, sizeof(gatts_service_t));
    service->uuid = uuid;
    service->handle = 0xFFFF;

    if (!services) {
        services = service;
    } else {
        gatts_service_t *p = services;
        while (p->next) {
            p = p->next;
        }
        p->next = service;
    }

    if (register_done) {
        register_done = false;
        cur_service = service;
        cur_character = NULL;
        DBGLOG_GATTS_DBG("app_gatts_register_service start register again for new service\n");
        register_service(cur_service);
    }

    return service;
}

gatts_character_t *app_gatts_register_character(
    gatts_service_t *service, uint16_t uuid, uint8_t props, gatts_read_callback_t read_callback,
    gatts_write_callback_t write_callback, gatts_notify_enable_callback_t notify_enable_callback)
{
    gatts_character_t *character;

    assert(service);

    character = os_mem_malloc(IOT_APP_MID, sizeof(gatts_character_t));
    memset(character, 0, sizeof(gatts_character_t));
    character->uuid = uuid;
    character->props = props;
    character->read_callback = read_callback;
    character->write_callback = write_callback;
    character->notify_enable_callback = notify_enable_callback;
    character->handle = 0xFFFF;
    character->ccd_handle = 0xFFFF;
    character->ccd_value = 0;

    if (!service->characters) {
        service->characters = character;
    } else {
        gatts_character_t *p = service->characters;
        if (p->uuid == uuid) {
            DBGLOG_GATTS_ERR("app_gatts_register_character %X error, already registered\n", uuid);
            os_mem_free(character);
            return NULL;
        }

        while (p->next) {
            if (p->uuid == uuid) {
                DBGLOG_GATTS_ERR("app_gatts_register_character %X error, already registered\n",
                                 uuid);
                os_mem_free(character);
                return NULL;
            }
            p = p->next;
        }
        p->next = character;
    }

    if (register_done) {
        register_done = false;
        cur_service = service;
        cur_character = character;
        DBGLOG_GATTS_DBG("app_gatts_register_character start register again for new character\n");
        register_character(service, cur_character);
    }
    return character;
}

gatts_service_t *app_gatts_register_service_ext(const uint8_t uuid128[16])
{
    gatts_service_t *service;

    service = find_service_ext(uuid128);
    if (service) {
        DBGLOG_GATTS_DBG("app_gatts_register_service %02X%02X%02X%02X already registered\n",
                         uuid128[0], uuid128[1], uuid128[2], uuid128[3]);
        return service;
    }

    service = os_mem_malloc(IOT_APP_MID, sizeof(gatts_service_t));
    memset(service, 0, sizeof(gatts_service_t));
    memcpy(service->uuid128, uuid128, 16);
    service->handle = 0xFFFF;

    if (!services) {
        services = service;
    } else {
        gatts_service_t *p = services;
        while (p->next) {
            p = p->next;
        }
        p->next = service;
    }

    if (register_done) {
        register_done = false;
        cur_service = service;
        cur_character = NULL;
        DBGLOG_GATTS_DBG("app_gatts_register_service start register again for new service\n");
        register_service(cur_service);
    }

    return service;
}

gatts_character_t *
app_gatts_register_character_ext(gatts_service_t *service, const uint8_t uuid128[16], uint8_t props,
                                 gatts_read_callback_t read_callback,
                                 gatts_write_callback_t write_callback,
                                 gatts_notify_enable_callback_t notify_enable_callback)
{
    gatts_character_t *character;

    assert(service);

    character = os_mem_malloc(IOT_APP_MID, sizeof(gatts_character_t));
    memset(character, 0, sizeof(gatts_character_t));
    memcpy(character->uuid128, uuid128, 16);
    character->props = props;
    character->read_callback = read_callback;
    character->write_callback = write_callback;
    character->notify_enable_callback = notify_enable_callback;
    character->handle = 0xFFFF;
    character->ccd_handle = 0xFFFF;
    character->ccd_value = 0;

    if (!service->characters) {
        service->characters = character;
    } else {
        gatts_character_t *p = service->characters;
        if ((!p->uuid) && (!memcmp(p->uuid128, uuid128, 16))) {
            DBGLOG_GATTS_ERR(
                "app_gatts_register_character %02X%02X%02X%02X error, already registered\n",
                uuid128[0], uuid128[1], uuid128[2], uuid128[3]);
            os_mem_free(character);
            return NULL;
        }

        while (p->next) {
            if (p->uuid) {
                continue;
            }
            if (!memcmp(p->uuid128, uuid128, 16)) {
                DBGLOG_GATTS_ERR(
                    "app_gatts_register_character %02X%02X%02X%02X error, already registered\n",
                    uuid128[0], uuid128[1], uuid128[2], uuid128[3]);
                os_mem_free(character);
                return NULL;
            }
            p = p->next;
        }
        p->next = character;
    }

    if (register_done) {
        register_done = false;
        cur_service = service;
        cur_character = character;
        DBGLOG_GATTS_DBG("app_gatts_register_character start register again for new character\n");
        register_character(service, cur_character);
    }
    return character;
}

void app_gatts_register_connection_callback(gatts_connection_callback_t callback)
{
    bool_t found = false;
    for (int i = 0; i < MAX_CONNECTION_CALLBACK; i++) {
        if (connection_callback[i] == NULL) {
            found = true;
            connection_callback[i] = callback;
            break;
        }
    }

    if (!found) {
        DBGLOG_GATTS_ERR("app_gatts_register_connection_callback error: full\n");
    }
}

int app_gatts_connect(const BD_ADDR_T *addr, uint8_t addr_type, uint8_t link_type)
{
    int ret;
    bt_cmd_gatt_client_connect_t cmd;

    assert(addr);
    memcpy(&cmd.addr, addr, sizeof(BD_ADDR_T));
    cmd.addr_type = addr_type;
    cmd.link_type = link_type;
    ret = app_bt_send_rpc_cmd(BT_CMD_GATT_CLIENT_CONNECT, &cmd, sizeof(cmd));
    DBGLOG_GATTS_DBG("gatts connect %02X:%02X:%02X:%02X:%02X:%02X addr_type %d link_type %d ret %d",
                     addr->addr[5], addr->addr[4], addr->addr[3], addr->addr[2], addr->addr[1],
                     addr->addr[0], addr_type, link_type, ret);
    return ret;
}

int app_gatts_disconnect(const BD_ADDR_T *addr)
{
    bt_cmd_gatt_client_disconnect_t cmd;

    assert(addr);
    memcpy(&cmd.addr, addr, sizeof(BD_ADDR_T));

    return app_bt_send_rpc_cmd(BT_CMD_GATT_CLIENT_DISCONNECT, &cmd, sizeof(cmd));
}

void app_gatts_init(void)
{
    gatts_service_t *gap_service = app_gatts_register_service(0x1800);
    assert(gap_service);
    gatts_service_t *gatt_service = app_gatts_register_service(0x1801);
    assert(gatt_service);

    gatts_character_t *character_device_name = app_gatts_register_character(
        gap_service, 0x2A00, GATT_PROP_READ, device_name_read_callback, NULL, NULL);
    assert(character_device_name);

    gatts_character_t *character_service_changed =
        app_gatts_register_character(gatt_service, 0x2A05, GATT_PROP_INDICATE, NULL, NULL, NULL);
    assert(character_service_changed);
}

void app_gatts_deinit(void)
{
}

void app_gatts_handle_bt_evt(uint16_t evt, void *param)
{
    switch (evt) {
        case BT_EVT_GATT_SERVER_STATE_CHANGED:
            DBGLOG_GATTS_DBG("BT_EVT_GATT_SERVER_STATE_CHANGED\n");
            handle_state_changed(param);
            break;
        case BT_EVT_GATT_SERVER_SERVICE_REGISTERED:
            DBGLOG_GATTS_DBG("BT_EVT_GATT_SERVER_SERVICE_REGISTERED\n");
            handle_service_registered(param);
            break;
        case BT_EVT_GATT_SERVER_SERVICE_UUID128_REGISTERED:
            DBGLOG_GATTS_DBG("BT_EVT_GATT_SERVER_SERVICE_UUID128_REGISTERED\n");
            handle_service_uuid128_registered(param);
            break;
        case BT_EVT_GATT_SERVER_CHARACTERISTIC_REGISTERED:
            DBGLOG_GATTS_DBG("BT_EVT_GATT_SERVER_CHARACTERISTIC_REGISTERED\n");
            handle_character_registered(param);
            break;
        case BT_EVT_GATT_SERVER_CHARACTERISTIC_UUID128_REGISTERED:
            DBGLOG_GATTS_DBG("BT_EVT_GATT_SERVER_CHARACTERISTIC_UUID128_REGISTERED\n");
            handle_character_uuid128_registered(param);
            break;
        case BT_EVT_GATT_SERVER_WRITE:
            DBGLOG_GATTS_DBG("BT_EVT_GATT_SERVER_WRITE\n");
            handle_write(param);
            break;
        case BT_EVT_GATT_SERVER_READ:
            DBGLOG_GATTS_DBG("BT_EVT_GATT_SERVER_READ\n");
            handle_read(param);
            break;
        case BT_EVT_GATT_SERVER_MTU_CHANGED:
            DBGLOG_GATTS_DBG("BT_EVT_GATT_SERVER_MTU_CHANGED");
            handle_mtu_changed(param);
        default:
            break;
    }
}

void app_gatts_handle_bt_power_on(void)
{
    if (register_done) {
        DBGLOG_GATTS_DBG("app_gatts_handle_bt_power_on already registered\n");
        return;
    }

    if (!services) {
        DBGLOG_GATTS_DBG("app_gatts_handle_bt_power_on no service found\n");
        register_done = true;
        app_adv_handle_gatts_register_done();
        return;
    }

    if (!services->characters) {
        DBGLOG_GATTS_DBG("app_gatts_handle_bt_power_on no character found\n");
        register_done = true;
        app_adv_handle_gatts_register_done();
        return;
    }

    cur_service = services;
    cur_character = NULL;

    DBGLOG_GATTS_DBG("app_gatts_handle_bt_power_on start register service\n");

    register_service(cur_service);
}
