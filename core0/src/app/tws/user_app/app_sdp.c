#include "app_sdp.h"
#include "string.h"

#ifndef APP_SDP_MAX_RFCOMM_FOUND_HANDLER_COUNT
#define APP_SDP_MAX_RFCOMM_FOUND_HANDLER_COUNT 1
#endif

static app_sdp_rfcomm_found_handler_t handlers[APP_SDP_MAX_RFCOMM_FOUND_HANDLER_COUNT];

static void bt_evt_sdp_rfcomm_found_handler(const bt_evt_sdp_rfcomm_found_t *param)
{
    DBGLOG_BT_DBG("BT_EVT_RFCOMM  found:%d \n", param->count);
    for (uint8_t j = 0; j < APP_SDP_MAX_RFCOMM_FOUND_HANDLER_COUNT; j++) {
        if (!handlers[j]) {
            continue;
        }
        for (uint8_t i = 0; i < param->count; i++) {
            handlers[j](param->rfcomm_channels[i].uuid128, param->rfcomm_channels[i].channel);
        }
    }
}

uint8_t app_sdp_add_uuid(const uint8_t uuid[16])
{
    bt_cmd_sdp_add_uuid_t cmd;

    cmd.profile_index = 0;
    memcpy(cmd.uuid, uuid, 16);

    if (!app_bt_send_rpc_cmd(BT_CMD_SDP_ADD_UUID, &cmd, sizeof(cmd))) {
        return cmd.profile_index;
    } else {
        DBGLOG_SDP_ERR("app_sdp_add_uuid failed\n");
        return 0xFF;
    }
}

uint8_t app_sdp_get_uuid_index(const uint8_t uuid[16])
{
    bt_cmd_sdp_get_uuid_index_t cmd;

    cmd.profile_index = 0;
    memcpy(cmd.uuid, uuid, 16);

    if (!app_bt_send_rpc_cmd(BT_CMD_SDP_GET_UUID_INDEX, &cmd, sizeof(cmd))) {
        return cmd.profile_index;
    } else {
        DBGLOG_SDP_ERR("app_sdp_get_uuid_index failed\n");
        return 0xFF;
    }
}

int app_sdp_register_record(uint8_t profile_index_num, uint8_t *profile_index_list,
                            uint8_t attribute_num, const bt_service_attribute_t *attribute_list)
{
    bt_cmd_sdp_register_record_t cmd;

    cmd.profile_index_num = profile_index_num;
    cmd.profile_index_list = profile_index_list;
    cmd.attribute_num = attribute_num;
    cmd.attribute_list = attribute_list;

    return app_bt_send_rpc_cmd(BT_CMD_SDP_REGISTER_RECORD, &cmd, sizeof(cmd));
}

bool_t app_sdp_client_register_rfcomm_found_handler(app_sdp_rfcomm_found_handler_t handler)
{
    for (int i = 0; i < APP_SDP_MAX_RFCOMM_FOUND_HANDLER_COUNT; i++) {
        if (handlers[i] == handler) {
            return true;
        }
    }

    for (int i = 0; i < APP_SDP_MAX_RFCOMM_FOUND_HANDLER_COUNT; i++) {
        if (handlers[i] == NULL) {
            handlers[i] = handler;
            return true;
        }
    }

    return false;
}

void app_sdp_handle_bt_evt(uint16_t evt, void *param)
{
    switch (evt) {
        case BT_EVT_SDP_RFCOMM_FOUND:
            bt_evt_sdp_rfcomm_found_handler(param);
            break;
        default:
            assert(0);
            break;
    }
}
