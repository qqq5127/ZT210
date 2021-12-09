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
#include "app_wqota.h"

#if WQOTA_ENABLED
#include "os_mem.h"
#include "assert.h"
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
#include "version.h"
#include "crc.h"
#include "app_ota_sync.h"
#include "iot_boot_map.h"
#include "data_dump.h"
#include "os_utils.h"

#define DBGLOG_WQOTA_DBG(fmt, ...) DBGLOG_USER_APP_LOG(DBGLOG_LEVEL_DEBUG, fmt, ##__VA_ARGS__)
#define DBGLOG_WQOTA_INF(fmt, ...) DBGLOG_USER_APP_LOG(DBGLOG_LEVEL_INFO, fmt, ##__VA_ARGS__)
#define DBGLOG_WQOTA_WRN(fmt, ...) DBGLOG_USER_APP_LOG(DBGLOG_LEVEL_WARNING, fmt, ##__VA_ARGS__)
#define DBGLOG_WQOTA_ERR(fmt, ...) DBGLOG_USER_APP_LOG(DBGLOG_LEVEL_ERROR, fmt, ##__VA_ARGS__)

#define OTA_MSG_ID_TASK_FINISH          1
#define OTA_MSG_ID_TASK_STARTED         2
#define OTA_MSG_ID_ROLE_SWITCH          3
#define OTA_MSG_ID_REBOOT               4
#define OTA_REMOTE_MSG_ID_SYNC_FILE_CRC 5

#define OTA_VID 0x076E
#define OTA_PID 0x7033

#define OTA_MTU_LEN  (gatts_connected ? 128 : 512)
#define OTA_NEED_CRC 1

#ifndef OTA_BLOCK_LEN
#define OTA_BLOCK_LEN 512
#endif

#ifndef OTA_STREAMING_DELAY_MS
#define OTA_STREAMING_DELAY_MS 1000
#endif

#ifndef OTA_UUID_SPP
#define OTA_UUID_SPP 0x7034
#endif

#ifndef OTA_UUID_SERVICE
#define OTA_UUID_SERVICE 0x7033
#endif

#ifndef OTA_UUID_CHARACTER_RX
#define OTA_UUID_CHARACTER_RX 0x2001
#endif

#ifndef OTA_UUID_CHARACTER_TX
#define OTA_UUID_CHARACTER_TX 0x2002
#endif

#ifndef OTA_MAX_PACKET_COUNT
#define OTA_MAX_PACKET_COUNT 2
#endif

#define DEV_INFO_ATTR_TYPE_VERSION          0x00
#define DEV_INFO_ATTR_TYPE_VID_AND_PID      0x01
#define DEV_INFO_ATTR_TYPE_BATTERY          0x02
#define DEV_INFO_ATTR_TYPE_IS_TWS_CONNECTED 0x03
#define DEV_INFO_ATTR_TYPE_IS_LEFT          0x04
#define DEV_INFO_ATTR_TYPE_DUAL_OTA         0x05

#define DEV_INFO_ATTR_MASK_VERSION          (BIT(DEV_INFO_ATTR_TYPE_VERSION))
#define DEV_INFO_ATTR_MASK_VID_AND_PID      (BIT(DEV_INFO_ATTR_TYPE_VID_AND_PID))
#define DEV_INFO_ATTR_MASK_BATTERY          (BIT(DEV_INFO_ATTR_TYPE_BATTERY))
#define DEV_INFO_ATTR_MASK_IS_TWS_CONNECTED (BIT(DEV_INFO_ATTR_TYPE_IS_TWS_CONNECTED))
#define DEV_INFO_ATTR_MASK_IS_LEFT          (BIT(DEV_INFO_ATTR_TYPE_IS_LEFT))
#define DEV_INFO_ATTR_MASK_DUAL_OTA         (BIT(DEV_INFO_ATTR_TYPE_DUAL_OTA))

#define DEV_INFO_MAX_SIZE 32

#define OTA_DATA_OFFSET (sizeof(packet_t) + sizeof(firmware_block_cmd_param_t))
#define OTA_PACKET_LEN \
    (sizeof(packet_t) + sizeof(firmware_block_cmd_param_t) + OTA_BLOCK_LEN + 4 + 1)

/**
 * @brief endian convert for uint16
 */
#define EC16(x) ((((x)&0xFF00) >> 8) | (((x)&0x00FF) << 8))

/**
 * @brief endian convert for uint32
 */
#define EC32(x)                                                                   \
    ((((x)&0xFF000000) >> 24) | (((x)&0x00FF0000) >> 8) | (((x)&0x0000FF00) << 8) \
     | (((x)&0x000000FF) << 24))

#define PACKET_PREFIX_VALUE 0x6E0770
#define PACKET_SUFFIX_VALUE 0x33

#define PKT_DATA_LEN(x) EC16((x)->len_be)
#define PKT_LEN(x)      (sizeof(packet_t) + PKT_DATA_LEN(x) + sizeof(uint8_t))
#define PKT_PREFIX(x)   ((x)->prefix)
#define PKT_SUFFIX(x)   ((x)->data[PKT_DATA_LEN(x)])

typedef enum {
    STATUS_SUCCESS = 0,
    STATUS_FAIL = 1,
    STATUS_UNKNOWN_CMD = 2,
    STATUS_BUSY = 3,
    STATUS_NO_RESPONSE = 4,
    STATUS_CRC_ERROR = 5,
    STATUS_ALL_DATA_CRC_ERROR = 6,
    STATUS_PARAM_ERROR = 7,
    STATUS_RESONSE_DATA_OVER_LIMIT = 8,
    STATUS_NOT_SUPPORT = 9,
    STATUS_PARTIAL_OPERATION_FAILED = 10,
    STATUS_UNREACHABLE = 11,
} status_code_t;

typedef enum {
    OPCODE_OTA_GET_DEVICE_INFO = 0x02,
    OPCODE_OTA_REBOOT = 0x03,

    OPCODE_OTA_GET_FILE_INFO_OFFSET = 0xE1,
    OPCODE_OTA_INQUIRY_IF_CAN_UPDATE = 0xE2,
    OPCODE_OTA_ENTER_UPDATE_MODE = 0xE3,
    OPCODE_OTA_EXIT_UPDATE_MODE = 0xE4,
    OPCODE_OTA_SEND_FIRMWARE_BLOCK = 0xE5,
    OPCODE_OTA_GET_REFRESH_FIRMWARE_STATUS = 0xE6,
    OPCODE_OTA_WWS_ROLE_SWITCH = 0xE7,
    OPCODE_OTA_GET_SYNC_STATE = 0xE8,

    OPCODE_OTA_CUSTOM = 0xF0,
} opcode_t;

typedef struct {
    uint32_t file_len;
    uint32_t file_crc;
    uint32_t file_offset;
} ota_file_crc_sync_param_t;

typedef struct {
    uint16_t vid_be;
    uint16_t pid_be;
    uint16_t version_be;
    uint32_t len_be;
    uint32_t reserved;
    uint32_t crc_be;
} __attribute__((packed)) ota_file_header_t;

typedef struct {
    uint32_t prefix : 24;
    uint8_t source : 3;
    uint8_t unused : 3;
    uint8_t need_rsp : 1;
    uint8_t is_cmd : 1;
    uint8_t opcode;
    uint16_t len_be;
    uint8_t data[0];
} __attribute__((packed)) packet_t;

typedef struct {
    status_code_t status : 8;
    uint8_t sn;
} __attribute__((packed)) general_rsp_param_t;

typedef struct {
    uint8_t sn;
    uint32_t mask;
} __attribute__((packed)) get_device_info_cmd_param_t;

typedef struct {
    status_code_t status : 8;
    uint8_t sn;
    uint8_t data[0];
} __attribute__((packed)) get_device_info_rsp_param_t;

typedef struct {
    uint8_t sn;
    uint8_t type;   // 0:reboot 1:shutdown
} __attribute__((packed)) reboot_cmd_param_t;

typedef struct {
    status_code_t status : 8;
    uint8_t sn;
} __attribute__((packed)) reboot_rsp_param_t;

typedef struct {
    uint8_t sn;
} __attribute__((packed)) get_file_info_offset_cmd_param_t;

typedef struct {
    status_code_t status : 8;
    uint8_t sn;
    uint32_t offset_be;
    uint16_t len_be;
} __attribute__((packed)) get_file_info_offset_rsp_param_t;

typedef struct {
    uint8_t sn;
    uint8_t file_info[0];
} __attribute__((packed)) if_can_update_cmd_param_t;

typedef struct {
    status_code_t status : 8;
    uint8_t sn;
    uint8_t result;
} __attribute__((packed)) if_can_update_rsp_param_t;

typedef struct {
    uint8_t sn;
} __attribute__((packed)) enter_update_mode_cmd_param_t;

typedef struct {
    status_code_t status : 8;
    uint8_t sn;
    uint8_t result;
    uint32_t offset_be;
    uint16_t len_be;
    uint8_t need_crc;
} __attribute__((packed)) enter_update_mode_rsp_param_t;

typedef struct {
    uint8_t sn;
} __attribute__((packed)) exit_update_mode_cmd_param_t;

typedef struct {
    status_code_t status : 8;
    uint8_t sn;
    uint8_t result;
} __attribute__((packed)) exit_update_mode_rsp_param_t;

typedef struct {
    uint8_t sn;
    uint32_t offset;
    uint8_t data[0];
} __attribute__((packed)) firmware_block_cmd_param_t;

typedef struct {
    status_code_t status : 8;
    uint8_t sn;
    uint8_t result;
    uint32_t offset_be;
    uint16_t len_be;
    uint16_t delay_ms_be;
} __attribute__((packed)) firmware_block_rsp_param_t;

typedef struct {
    uint8_t sn;
} __attribute__((packed)) get_refresh_status_cmd_param_t;

typedef struct {
    status_code_t status : 8;
    uint8_t sn;
    uint8_t result;
} __attribute__((packed)) get_refresh_status_rsp_param_t;

typedef struct {
    uint8_t sn;
} __attribute__((packed)) wws_role_switch_cmd_param_t;

typedef struct {
    status_code_t status : 8;
    uint8_t sn;
    uint8_t result;
} __attribute__((packed)) wws_role_switch_rsp_param_t;

typedef struct {
    uint8_t sn;
} __attribute__((packed)) get_sync_state_cmd_param_t;

typedef struct {
    status_code_t status : 8;
    uint8_t sn;
    uint8_t state;
} __attribute__((packed)) get_sync_state_rsp_param_t;

typedef struct {
    uint8_t flag_len;
    uint8_t flag_type;
    uint8_t flag;
    uint8_t vendor_len;
    uint8_t vendor_type;
    uint8_t vid_low;
    uint8_t vid_high;
    uint8_t name_len;
    uint8_t name_type;
    uint8_t name[6];
} __attribute__((packed)) wq_adv_data_t;

typedef enum {
    OTA_CH_UNKONWN,
    OTA_CH_SPP,
    OTA_CH_BLE,
} ota_ch_t;

static bool_t ota_started = false;
static bool_t gatts_connected = false;
static bool_t spps_connected = false;
static BD_ADDR_T remote_addr = {0};
static gatts_character_t *character_rx = NULL;
static gatts_character_t *character_tx = NULL;

static bool_t enter_update_mode_pending = false;
static ota_ch_t ota_ch = OTA_CH_UNKONWN;
static uint8_t ota_resume_flag = 0;
static uint8_t ota_dual_flag = 0;
static uint8_t ota_sn = 0;
static uint32_t ota_file_offset = 0;
static uint32_t ota_file_len = 0;
static uint32_t ota_file_crc = 0;
static uint16_t ota_file_ver = 0;
static uint8_t *ota_packet_buffer = NULL;
static uint32_t ota_packet_offset = 0;
static uint32_t ota_packet_length = 0;
static void *ota_pkt_pool[OTA_MAX_PACKET_COUNT];
static uint32_t ota_pkt_used_cnt[OTA_MAX_PACKET_COUNT];
static uint32_t ota_pkt_free_cnt[OTA_MAX_PACKET_COUNT];

static wq_adv_data_t adv_data = {
    .flag_len = 0x02,                        //
    .flag_type = 0x01,                       //
    .flag = 0x1A,                            //
    .vendor_len = 0x1B,                      //
    .vendor_type = 0xFF,                     //
    .vid_low = OTA_VID >> 8,                 //
    .vid_high = OTA_VID & 0xFF,              //
    .name_len = 0x07,                        //
    .name_type = 0x09,                       //
    .name = {'w', 'q', '-', 'o', 't', 'a'}   //
};

static get_refresh_status_rsp_param_t refresh_status_rsp_param;

static void send_ota_data(uint8_t *data, uint16_t len);
static void response_ota_enter_update_mode(uint8_t result);
static void request_ota_firmware_block_command(uint32_t offset);

static bool_t ota_pkt_pool_init(void)
{
    bool_t succeed = true;

    for (int i = 0; i < OTA_MAX_PACKET_COUNT; i++) {
        if (!ota_pkt_pool[i]) {
            ota_pkt_pool[i] = os_mem_malloc(IOT_APP_MID, OTA_PACKET_LEN);
            ota_pkt_used_cnt[i] = 0;
            ota_pkt_free_cnt[i] = 0;
        }
        if (!ota_pkt_pool[i]) {
            succeed = false;
            break;
        }
    }

    if (!succeed) {
        for (int i = 0; i < OTA_MAX_PACKET_COUNT; i++) {
            if (ota_pkt_pool[i]) {
                os_mem_free(ota_pkt_pool[i]);
                ota_pkt_pool[i] = NULL;
            }
        }
    }

    return succeed;
}

static void ota_pkt_pool_deinit(void)
{
    for (int i = 0; i < OTA_MAX_PACKET_COUNT; i++) {
        if (ota_pkt_pool[i]) {
            os_mem_free(ota_pkt_pool[i]);
            ota_pkt_pool[i] = NULL;
            ota_pkt_used_cnt[i] = 0;
            ota_pkt_free_cnt[i] = 0;
        }
    }
}

static void *ota_pkt_pool_malloc(uint32_t size)
{
    if (size > OTA_PACKET_LEN) {
        DBGLOG_WQOTA_ERR("ota_pkt_pool_malloc error size=%d\n", size);
        return NULL;
    }

    for (int i = 0; i < OTA_MAX_PACKET_COUNT; i++) {
        assert(ota_pkt_free_cnt[i] <= ota_pkt_used_cnt[i]);
        if (ota_pkt_free_cnt[i] == ota_pkt_used_cnt[i]) {
            ota_pkt_used_cnt[i] += 1;
            return ota_pkt_pool[i];
        }
    }

    return NULL;
}

static void ota_pkt_pool_free(void *ptr)
{
    for (int i = 0; i < OTA_MAX_PACKET_COUNT; i++) {
        if (ota_pkt_pool[i] == ptr) {
            ota_pkt_free_cnt[i] += 1;
            break;
        }
    }
}

static inline uint32_t uint32_big_decode(const uint8_t *p_encoded_data)
{
    return ((((uint32_t)p_encoded_data[0]) << 24) | (((uint32_t)p_encoded_data[1]) << 16)
            | (((uint32_t)p_encoded_data[2]) << 8) | (((uint32_t)p_encoded_data[3]) << 0));
}

static uint32_t set_device_info_data(uint32_t mask, uint8_t *data, size_t data_max_size)
{
    uint32_t offset = 0;
    if (mask & DEV_INFO_ATTR_MASK_VERSION && (offset + 2 + 4 < data_max_size)) {
        uint32_t local_ver = 0;
#ifdef CUSTOM_VERSION
        local_ver = CUSTOM_VERSION;
#else
        local_ver = FIRMWARE_VERSION_BUILD;
#endif
        uint32_t peer_ver = app_wws_peer_get_fw_ver_build();
        data[offset++] = 1 + 4;
        data[offset++] = DEV_INFO_ATTR_TYPE_VERSION;
        data[offset++] = (uint8_t)(local_ver >> 8);
        data[offset++] = (uint8_t)local_ver;
        data[offset++] = (uint8_t)(peer_ver >> 8);
        data[offset++] = (uint8_t)peer_ver;
        DBGLOG_WQOTA_INF("[wqota] ATTR_TYPE_VERSION offset %d value %d %d\n", offset, local_ver,
                         peer_ver);
    }

    if (mask & DEV_INFO_ATTR_MASK_VID_AND_PID && (offset + 2 + 4 < data_max_size)) {
        data[offset++] = 1 + 4;
        data[offset++] = DEV_INFO_ATTR_TYPE_VID_AND_PID;
        data[offset++] = (uint8_t)(OTA_VID >> 8);
        data[offset++] = (uint8_t)OTA_VID;
        data[offset++] = (uint8_t)(OTA_PID >> 8);
        data[offset++] = (uint8_t)OTA_PID;
        DBGLOG_WQOTA_INF("[wqota] ATTR_TYPE_VID_PID offset %d value 0x%X 0x%X\n", offset, OTA_VID,
                         OTA_PID);
    }

    if (mask & DEV_INFO_ATTR_MASK_BATTERY && (offset + 2 + 2 < data_max_size)) {
        data[offset++] = 1 + 2;
        data[offset++] = DEV_INFO_ATTR_TYPE_BATTERY;
        data[offset++] = app_bat_get_level();
        data[offset++] = app_wws_peer_get_battery_level();
        DBGLOG_WQOTA_INF("[wqota] ATTR_TYPE_BATTERY offset %d value %d %d\n", offset,
                         data[offset - 2], data[offset - 1]);
    }

    if (mask & DEV_INFO_ATTR_MASK_IS_TWS_CONNECTED && (offset + 2 + 1 < data_max_size)) {
        data[offset++] = 1 + 1;
        data[offset++] = DEV_INFO_ATTR_TYPE_IS_TWS_CONNECTED;
        data[offset++] = app_wws_is_connected();
        ota_dual_flag = app_wws_is_connected();
        DBGLOG_WQOTA_INF("[wqota] ATTR_TYPE_IS_TWS_CONNECTED offset %d value %d\n", offset,
                         data[offset - 1]);
    }

    if (mask & DEV_INFO_ATTR_MASK_IS_LEFT && (offset + 2 + 1 < data_max_size)) {
        data[offset++] = 1 + 1;
        data[offset++] = DEV_INFO_ATTR_TYPE_IS_LEFT;
        data[offset++] = app_wws_is_left();
        DBGLOG_WQOTA_INF("[wqota] ATTR_TYPE_IS_LEFT offset %d value %d\n", offset,
                         data[offset - 1]);
    }

    if (mask & DEV_INFO_ATTR_MASK_DUAL_OTA && (offset + 2 + 1 < data_max_size)) {
        data[offset++] = 1 + 1;
        data[offset++] = DEV_INFO_ATTR_TYPE_DUAL_OTA;
        data[offset++] = 1;   //support dual ota
        DBGLOG_WQOTA_INF("[wqota] ATTR_TYPE_DUAL_OTA value %d\n", data[offset - 1]);
    }

    DBGLOG_WQOTA_INF("[wqota] get device info mask 0x%08X size %d\n", mask, offset);
    return offset;
}

static void send_response(uint8_t opcode, void *param, uint16_t param_len)
{
    packet_t *packet = os_mem_malloc(IOT_APP_MID, sizeof(packet_t) + param_len + 1);
    if (!packet) {
        DBGLOG_WQOTA_ERR("[wqota] send_response malloc error\n");
        return;
    }

    memset(packet, 0, sizeof(packet_t) + param_len + 1);

    PKT_PREFIX(packet) = PACKET_PREFIX_VALUE;
    packet->opcode = opcode;

    packet->len_be = EC16(param_len);
    if (param && param_len) {
        memcpy(packet->data, param, param_len);
    }
    if (ota_dual_flag && app_wws_is_connected() == false) {
        packet->data[0] = STATUS_FAIL;
    }
    packet->data[param_len] = PACKET_SUFFIX_VALUE;
    DBGLOG_WQOTA_INF(
        "[wqota] response opcode:0x%X len:%d status:%d sn:%d result:%d ch:%d conn:%d %d wws_conn:%d role:%d",
        opcode, param_len, packet->data[0], packet->data[1], packet->data[2], ota_ch,
        spps_connected, gatts_connected, app_wws_is_connected(), app_wws_is_master());
    if (app_wws_is_master() || (gatts_connected && ota_ch == OTA_CH_BLE)) {
        send_ota_data((uint8_t *)packet, sizeof(packet_t) + param_len + 1);
    }
    os_mem_free(packet);
}

static void ota_task_callback(ota_task_evt_t evt, int result, void *data)
{
    UNUSED(data);
    if (evt == OTA_TASK_EVT_STARTED) {
        os_delay(2000);
        app_send_msg(MSG_TYPE_OTA, OTA_MSG_ID_TASK_STARTED, NULL, 0);
        DBGLOG_WQOTA_ERR("[wqota] OTA_TASK_EVT_STARTED result:%d\n", result);
    } else if (evt == OTA_TASK_EVT_DATA_HANDLED) {
        if (data) {
            ota_pkt_pool_free((uint8_t *)data - OTA_DATA_OFFSET);
        }
        if (result) {
            DBGLOG_WQOTA_ERR("[wqota] data error result:%d\n", result);
        }
    } else if (evt == OTA_TASK_EVT_FINISH) {
        if (result) {
            DBGLOG_WQOTA_ERR("[wqota] finish error result:%d\n", result);
            if (result == RET_CRC_FAIL) {
                refresh_status_rsp_param.result = 1;   //check crc failed
            } else {
                refresh_status_rsp_param.result = 2;   //failed
            }
        } else {
            refresh_status_rsp_param.result = 0;
        }
        app_send_msg(MSG_TYPE_OTA, OTA_MSG_ID_TASK_FINISH, NULL, 0);
    } else {
        DBGLOG_WQOTA_ERR("[wqota] ota_task_callback unknown evt:%d\n", evt);
    }
}

static void handle_ota_get_device_info_command(uint8_t opcode, bool_t need_rsp, uint8_t *data,
                                               uint16_t data_len)
{
    get_device_info_cmd_param_t *param = (get_device_info_cmd_param_t *)data;
    get_device_info_rsp_param_t *rsp_param = NULL;
    uint32_t len = 0;

    UNUSED(data_len);
    UNUSED(need_rsp);
    DBGLOG_WQOTA_INF("[wqota] get_device_info_cmd_param mask 0x%08X\n", EC32(param->mask));
    rsp_param = (get_device_info_rsp_param_t *)os_mem_malloc(
        IOT_APP_MID, sizeof(get_device_info_rsp_param_t) + DEV_INFO_MAX_SIZE);

    if (!rsp_param) {
        DBGLOG_WQOTA_ERR("[wqota] rsp_param malloc error\n");
        rsp_param->status = STATUS_FAIL;
        rsp_param->sn = param->sn;
        send_response(opcode, rsp_param, sizeof(get_device_info_rsp_param_t));
        return;
    }

    rsp_param->status = STATUS_SUCCESS;
    rsp_param->sn = param->sn;
    len = set_device_info_data(EC32(param->mask), rsp_param->data, DEV_INFO_MAX_SIZE);
    send_response(opcode, rsp_param, sizeof(get_device_info_rsp_param_t) + len);
    os_mem_free(rsp_param);
}

static void handle_ota_reboot_command(uint8_t opcode, bool_t need_rsp, uint8_t *data,
                                      uint16_t data_len)
{
    reboot_cmd_param_t *param = (reboot_cmd_param_t *)data;
    reboot_rsp_param_t rsp_param;

    UNUSED(data_len);
    UNUSED(need_rsp);
    if (param->type == 0) {
        DBGLOG_WQOTA_INF("[wqota] reboot command\n");
        if (ota_dual_flag && gatts_connected && ota_ch == OTA_CH_BLE) {
            app_ota_sync_reboot();
        }
        app_send_msg_delay(MSG_TYPE_OTA, OTA_MSG_ID_REBOOT, NULL, 0, 2000);
    } else if (param->type == 1) {
        DBGLOG_WQOTA_INF("[wqota] shutdown command\n");
    }

    rsp_param.status = STATUS_SUCCESS;
    rsp_param.sn = param->sn;

    send_response(opcode, &rsp_param, sizeof(rsp_param));
}

static void handle_ota_get_file_info_offset_command(uint8_t opcode, bool_t need_rsp, uint8_t *data,
                                                    uint16_t data_len)
{
    get_file_info_offset_cmd_param_t *param = (get_file_info_offset_cmd_param_t *)data;
    get_file_info_offset_rsp_param_t rsp_param;

    UNUSED(data_len);
    UNUSED(need_rsp);

    rsp_param.status = STATUS_SUCCESS;
    rsp_param.sn = param->sn;
    rsp_param.offset_be = 0;
    rsp_param.len_be = EC16(sizeof(ota_file_header_t));

    send_response(opcode, &rsp_param, sizeof(rsp_param));
}

static void handle_ota_inquiry_if_can_update_command(uint8_t opcode, bool_t need_rsp, uint8_t *data,
                                                     uint16_t data_len)
{
    if_can_update_cmd_param_t *param = (if_can_update_cmd_param_t *)data;
    if_can_update_rsp_param_t rsp_param;
    ota_file_header_t *header = (ota_file_header_t *)(param->file_info);
    uint32_t len_be;
    uint32_t crc_be;
    uint16_t version_be;

    UNUSED(data_len);
    UNUSED(need_rsp);

    enter_update_mode_pending = false;

    if ((header->vid_be != EC16(OTA_VID)) || (header->pid_be != EC16(OTA_PID))) {
        rsp_param.status = STATUS_SUCCESS;
        rsp_param.sn = param->sn;
        rsp_param.result = 0x02;
        ota_file_len = 0;
        ota_file_crc = 0;
        DBGLOG_WQOTA_INF("[wqota] recv vid:0x%04X pid:0x%04X,ota vid:0x%04X pid:0x%04X\n",
                         header->vid_be, header->pid_be, EC16(OTA_VID), EC16(OTA_PID));
    } else {
        rsp_param.status = STATUS_SUCCESS;
        rsp_param.sn = param->sn;
        rsp_param.result = 0x03;

        len_be = EC32(header->len_be);
        crc_be = EC32(header->crc_be);
        version_be = EC16(header->version_be);
        DBGLOG_WQOTA_INF("[wqota] ver:0x%x size:%d %d crc:0x%08X 0x%08X\n", ota_file_ver, len_be,
                         ota_file_len, crc_be, ota_file_crc);
        if (ota_file_len == len_be && ota_file_crc == crc_be) {
            ota_resume_flag = 1;
        } else {
            ota_resume_flag = 0;
            ota_file_len = len_be;
            ota_file_crc = crc_be;
        }
        ota_file_ver = version_be;
    }

    send_response(opcode, &rsp_param, sizeof(rsp_param));
}

static void response_ota_enter_update_mode(uint8_t result)
{
    enter_update_mode_rsp_param_t rsp_param;

    enter_update_mode_pending = false;

    rsp_param.status = STATUS_SUCCESS;
    rsp_param.sn = ota_sn;
    rsp_param.result = result;
    rsp_param.offset_be = EC32(ota_file_offset + sizeof(ota_file_header_t));
    rsp_param.len_be = EC16(OTA_BLOCK_LEN);
    rsp_param.need_crc = OTA_NEED_CRC;
    DBGLOG_WQOTA_INF("[wqota] request block offset:%d len:%d need_crc:%d\n", ota_file_offset,
                     OTA_BLOCK_LEN, OTA_NEED_CRC);
    send_response(OPCODE_OTA_ENTER_UPDATE_MODE, &rsp_param, sizeof(rsp_param));
}

static void request_ota_firmware_block_command(uint32_t offset)
{
    firmware_block_rsp_param_t rsp_param;
    uint32_t block_len = 0;
    uint16_t delay_ms = 0;

    if (app_bt_is_sco_connected() || (app_bt_get_a2dp_state() == A2DP_STATE_STREAMING)) {
        delay_ms = OTA_STREAMING_DELAY_MS;
    }

    rsp_param.sn = ota_sn;
    rsp_param.status = STATUS_SUCCESS;
    rsp_param.delay_ms_be = EC16(delay_ms);
    rsp_param.result = 0;
    if (offset >= ota_file_len) {
        offset = 0;
        block_len = 0;
        DBGLOG_WQOTA_INF("[wqota] request block finish\n");
    } else {
        block_len = ota_file_len - offset;
        if (block_len > OTA_BLOCK_LEN) {
            block_len = OTA_BLOCK_LEN;
        }
        DBGLOG_WQOTA_INF("[wqota] request block offset:%d len:%d delay:%d\n", offset, block_len,
                         delay_ms);
        offset += sizeof(ota_file_header_t);
    }
    rsp_param.offset_be = EC32(offset);
    rsp_param.len_be = EC16(block_len);
    send_response(OPCODE_OTA_SEND_FIRMWARE_BLOCK, &rsp_param, sizeof(rsp_param));
}

static void handle_ota_enter_update_mode_command(uint8_t opcode, bool_t need_rsp, uint8_t *data,
                                                 uint16_t data_len)
{
    UNUSED(opcode);
    UNUSED(data_len);
    UNUSED(need_rsp);
    UNUSED(data);

    ota_sn = data[0];

    if (enter_update_mode_pending) {
        DBGLOG_WQOTA_ERR("handle_ota_enter_update_mode_command enter_update_mode_pending sn:%d\n",
                         ota_sn);
        return;
    }
    enter_update_mode_pending = true;

    if (ota_pkt_pool_init() == false) {
        DBGLOG_WQOTA_ERR("[wqota] ota_pkt_pool_init error\n");
        response_ota_enter_update_mode(1);
        return;
    }
    if (ota_file_len == 0) {
        DBGLOG_WQOTA_ERR("[wqota] ota_file_len:0 error\n");
        response_ota_enter_update_mode(1);
        return;
    } else if (ota_resume_flag && (ota_file_offset != 0) && (ota_file_offset < ota_file_len)) {
        if (ota_task_resume(ota_task_callback) != 0) {
            DBGLOG_WQOTA_ERR("[wqota] ota_task_resume error\n");
            response_ota_enter_update_mode(1);
            return;
        } else {
            DBGLOG_WQOTA_INF("[wqota] resume succeed size:%d offset:%d crc:0x%08X\n", ota_file_len,
                             ota_file_offset, ota_file_crc);
            response_ota_enter_update_mode(0);
        }
    } else {
        ota_file_offset = 0;
        if (ota_task_start(ota_task_callback, ota_file_len) != 0) {
            DBGLOG_WQOTA_ERR("[wqota] ota_task_start error\n");
            response_ota_enter_update_mode(1);
            return;
        } else {
            DBGLOG_WQOTA_INF("[wqota] start succeed size:%d offset:%d crc:0x%08X\n", ota_file_len,
                             ota_file_offset, ota_file_crc);
        }
    }

    if (ota_file_len != 0 && ota_dual_flag && gatts_connected && ota_ch == OTA_CH_BLE) {
        app_ota_sync_start(ota_file_len, ota_file_crc, OTA_SYNC_MODE_COMPLETE);
    }
}

static void handle_ota_exit_update_mode_command(uint8_t opcode, bool_t need_rsp, uint8_t *data,
                                                uint16_t data_len)
{
    exit_update_mode_cmd_param_t *param = (exit_update_mode_cmd_param_t *)data;
    exit_update_mode_rsp_param_t rsp_param;

    UNUSED(data_len);
    UNUSED(need_rsp);

    rsp_param.status = STATUS_SUCCESS;
    rsp_param.sn = param->sn;
    rsp_param.result = 0;

    ota_file_crc = 0;
    ota_file_len = 0;
    ota_file_offset = 0;

    enter_update_mode_pending = false;

    iot_boot_map_set_boot_type(IOT_BOOT_MODE_NORMAL);

    if (ota_dual_flag) {
        app_ota_sync_stop();
    }

    send_response(opcode, &rsp_param, sizeof(rsp_param));
}

static void handle_ota_firmware_block_command(uint8_t opcode, bool_t need_rsp, uint8_t *data,
                                              uint16_t len)
{
    firmware_block_cmd_param_t *param = (firmware_block_cmd_param_t *)data;
    int ret = RET_OK;
    uint8_t *ota_data = param->data;
    uint16_t ota_data_len = len - sizeof(firmware_block_cmd_param_t);
    uint32_t offset = EC32(param->offset) - sizeof(ota_file_header_t);

    UNUSED(opcode);
    UNUSED(need_rsp);
    UNUSED(data);
    UNUSED(len);

    if (ota_file_len == 0) {
        DBGLOG_WQOTA_ERR("[wqota] ota_file_len:0 error\n");
        return;
    }

#if OTA_NEED_CRC
    ota_data_len -= 4;
    // check crc
    uint32_t cal_crc = getcrc32((const uint8_t *)ota_data, ota_data_len);
    uint32_t dat_crc = uint32_big_decode(&ota_data[ota_data_len]);
    if (cal_crc != dat_crc) {
        ota_pkt_pool_free(ota_data - OTA_DATA_OFFSET);
        DBGLOG_WQOTA_ERR("[wqota] crc failed,cal_crc 0x%08X dat_crc 0x%08X\n", cal_crc, dat_crc);
        dump_bytes(ota_data, len);
        return;
    }
#endif

    if (ota_sn == param->sn) {
        ota_pkt_pool_free(ota_data - OTA_DATA_OFFSET);
        DBGLOG_WQOTA_INF("[wqota] ignored opcode:0x%02X len:%d sn:%d\n", opcode, len, param->sn);
        request_ota_firmware_block_command(ota_file_offset);
        return;
    } else {
        ota_sn = param->sn;
    }

    if (offset + ota_data_len > ota_file_len) {
        DBGLOG_WQOTA_ERR("[wqota] invalid offset:%d data_len:%d file_len:%d\n", offset,
                         ota_data_len, ota_file_len);
        request_ota_firmware_block_command(ota_file_offset);
        return;
    }

    if (offset != ota_file_offset) {
        DBGLOG_WQOTA_WRN("[wqota] miss block,offset %d %d", offset, ota_file_offset);
        ota_file_offset = offset;
    }
    DBGLOG_WQOTA_INF("[wqota] size:%d offset:%d len:%d data:0x%02X crc:0x%08X\n", ota_file_len,
                     ota_file_offset, ota_data_len, ota_data[0], getcrc32(ota_data, ota_data_len));
    ret = ota_task_write_ext(ota_data, ota_data_len, ota_file_offset);
    if (ret) {
        ota_pkt_pool_free(ota_data - OTA_DATA_OFFSET);
        DBGLOG_WQOTA_WRN("[wqota] ota_task_write error %d\n", ret);
    } else {
        ota_file_offset += OTA_BLOCK_LEN;
        request_ota_firmware_block_command(ota_file_offset);
    }
}

static void handle_ota_get_refresh_firmware_status_command(uint8_t opcode, bool_t need_rsp,
                                                           uint8_t *data, uint16_t data_len)
{
    get_refresh_status_cmd_param_t *param = (get_refresh_status_cmd_param_t *)data;

    UNUSED(opcode);
    UNUSED(data_len);
    UNUSED(need_rsp);

    refresh_status_rsp_param.status = STATUS_SUCCESS;
    refresh_status_rsp_param.sn = param->sn;

    DBGLOG_WQOTA_INF("ota finish, pool used:%d,%d free:%d,%d\n", ota_pkt_used_cnt[0],
                     ota_pkt_used_cnt[1], ota_pkt_free_cnt[0], ota_pkt_free_cnt[1]);

    if (ota_file_len == 0) {
        DBGLOG_WQOTA_ERR("[wqota] ota_file_len:0 error\n");
        refresh_status_rsp_param.status = STATUS_FAIL;
        refresh_status_rsp_param.result = 0;
        send_response(OPCODE_OTA_GET_REFRESH_FIRMWARE_STATUS, &refresh_status_rsp_param,
                      sizeof(refresh_status_rsp_param));
        return;
    }

    if (ota_dual_flag && (ota_ch == OTA_CH_SPP) && app_wws_is_slave()) {
        DBGLOG_WQOTA_INF("ota finish ignored for slave\n");
        return;
    }
    if (ota_task_finish_without_commit()) {
        DBGLOG_WQOTA_ERR("[wqota] ota_task_finish error\n");
    }
}

static void handle_ota_wws_role_switch_command(uint8_t opcode, bool_t need_rsp, uint8_t *data,
                                               uint16_t data_len)
{
    wws_role_switch_cmd_param_t *param = (wws_role_switch_cmd_param_t *)data;
    wws_role_switch_rsp_param_t rsp_param;

    UNUSED(opcode);
    UNUSED(data_len);
    UNUSED(need_rsp);

    if (app_wws_is_connected()) {
        rsp_param.status = STATUS_SUCCESS;
        rsp_param.result = 0;
        app_send_msg_delay(MSG_TYPE_OTA, OTA_MSG_ID_ROLE_SWITCH, NULL, 0, 100);
    } else {
        rsp_param.status = STATUS_FAIL;
        rsp_param.result = 1;
        DBGLOG_WQOTA_WRN("[wqota] wws role switch failed,wws is disconnected\n");
    }
    rsp_param.sn = param->sn;
    send_response(opcode, &rsp_param, sizeof(rsp_param));
}

static void handle_ota_get_sync_state_command(uint8_t opcode, bool_t need_rsp, uint8_t *data,
                                              uint16_t data_len)
{
    get_sync_state_cmd_param_t *param = (get_sync_state_cmd_param_t *)data;
    get_sync_state_rsp_param_t rsp_param;
    uint8_t sync_state;

    UNUSED(opcode);
    UNUSED(data_len);
    UNUSED(need_rsp);

    sync_state = app_ota_sync_get_state();

    rsp_param.status = STATUS_SUCCESS;
    rsp_param.sn = param->sn;
    rsp_param.state = sync_state;

    DBGLOG_WQOTA_INF("handle_ota_get_sync_state_command sync_state:%d\n", sync_state);

    send_response(opcode, &rsp_param, sizeof(rsp_param));
}

static void handle_command(packet_t *packet)
{
    uint8_t opcode = packet->opcode;
    bool_t rsp = packet->need_rsp;
    uint8_t sn = packet->data[0];
    uint8_t *data = packet->data;
    uint16_t len = PKT_DATA_LEN(packet);
    DBGLOG_WQOTA_INF("[wqota] command opcode:0x%02X len:%d rsp:%d sn:%d\n", opcode, len, rsp, sn);
    switch (opcode) {
        case OPCODE_OTA_GET_DEVICE_INFO:
            handle_ota_get_device_info_command(opcode, rsp, data, len);
            break;
        case OPCODE_OTA_REBOOT:
            handle_ota_reboot_command(opcode, rsp, data, len);
            break;
        case OPCODE_OTA_GET_FILE_INFO_OFFSET:
            handle_ota_get_file_info_offset_command(opcode, rsp, data, len);
            break;
        case OPCODE_OTA_INQUIRY_IF_CAN_UPDATE:
            handle_ota_inquiry_if_can_update_command(opcode, rsp, data, len);
            break;
        case OPCODE_OTA_ENTER_UPDATE_MODE:
            handle_ota_enter_update_mode_command(opcode, rsp, data, len);
            break;
        case OPCODE_OTA_EXIT_UPDATE_MODE:
            handle_ota_exit_update_mode_command(opcode, rsp, data, len);
            break;
        case OPCODE_OTA_SEND_FIRMWARE_BLOCK:
            handle_ota_firmware_block_command(opcode, rsp, data, len);
            break;
        case OPCODE_OTA_GET_REFRESH_FIRMWARE_STATUS:
            handle_ota_get_refresh_firmware_status_command(opcode, rsp, data, len);
            break;
        case OPCODE_OTA_WWS_ROLE_SWITCH:
            handle_ota_wws_role_switch_command(opcode, rsp, data, len);
            break;
        case OPCODE_OTA_GET_SYNC_STATE:
            handle_ota_get_sync_state_command(opcode, rsp, data, len);
            break;
        default:
            DBGLOG_WQOTA_INF("[wqota] unknown opcode 0x%X\n", opcode);
            if (rsp) {
                general_rsp_param_t rsp_param = {STATUS_UNKNOWN_CMD, sn};
                send_response(opcode, &rsp_param, sizeof(rsp_param));
            }
            break;
    }
}

static void hanle_response(packet_t *packet)
{
    DBGLOG_WQOTA_DBG("[wqota] hanle_response op=0x%X\n", packet->opcode);
}

static void handle_packet_data(const uint8_t *data, uint16_t len)
{
    UNUSED(data);
    UNUSED(len);
    DBGLOG_WQOTA_INF("[wqota] handle_packet_data len:%d offset:%d\n", ota_packet_length,
                     ota_packet_offset);
    if (ota_packet_buffer) {
        if ((ota_packet_offset + len) <= ota_packet_length) {
            memcpy(&ota_packet_buffer[ota_packet_offset], data, len);
            ota_packet_offset += len;
        }
        if ((ota_packet_offset >= ota_packet_length)) {
            if (data[len - 1] == PACKET_SUFFIX_VALUE) {
                packet_t *packet = (packet_t *)ota_packet_buffer;
                if ((packet->opcode != OPCODE_OTA_SEND_FIRMWARE_BLOCK) || (!packet->is_cmd)) {
                    assert(0);
                    return;
                }
                handle_command(packet);
                ota_packet_buffer = NULL;
            } else {
                DBGLOG_WQOTA_ERR("[wqota] suffix 0x%02X error\n", data[len - 1]);
                ota_pkt_pool_free(ota_packet_buffer);
                ota_packet_buffer = NULL;
            }
        }
    }
}

static void handle_ota_data(const uint8_t *data, uint16_t len)
{
    DBGLOG_WQOTA_DBG("[wqota] handle_ota_data len:%d\n", len);
    packet_t *packet = (packet_t *)data;
    if (PKT_PREFIX(packet) != PACKET_PREFIX_VALUE) {
        handle_packet_data(data, len);
        return;
    }

    if (len < sizeof(packet_t) + sizeof(uint8_t)) {
        DBGLOG_WQOTA_ERR("[wqota] data len:%d error\n", len);
        return;
    }

    if (ota_packet_buffer) {
        ota_pkt_pool_free(ota_packet_buffer);
        ota_packet_buffer = NULL;
    }

    if (PKT_LEN(packet) != len) {
        ota_packet_length = PKT_LEN(packet);
        ota_packet_offset = 0;

        if ((packet->opcode != OPCODE_OTA_SEND_FIRMWARE_BLOCK) || (!packet->is_cmd)) {
            DBGLOG_WQOTA_ERR("[wqota] unknown opcode:0x%X\n", packet->opcode);
            return;
        }

        if (ota_packet_length > OTA_PACKET_LEN) {
            DBGLOG_WQOTA_ERR("[wqota] invalid packet len:%d\n", ota_packet_length);
            return;
        }

        ota_packet_buffer = ota_pkt_pool_malloc(ota_packet_length);
        if (ota_packet_buffer) {
            handle_packet_data(data, len);
        } else {
            DBGLOG_WQOTA_ERR("[wqota] packet os_mem_malloc len:%d error\n", ota_packet_length);
        }
        return;
    }

    if (PKT_SUFFIX(packet) != PACKET_SUFFIX_VALUE) {
        DBGLOG_WQOTA_ERR("[wqota] suffix 0x%02X error\n", PKT_SUFFIX(packet));
        return;
    }

    if ((packet->opcode == OPCODE_OTA_SEND_FIRMWARE_BLOCK) && (packet->is_cmd)) {
        ota_packet_buffer = ota_pkt_pool_malloc(len);
        if (ota_packet_buffer) {
            packet = (packet_t *)ota_packet_buffer;
            memcpy(packet, data, len);
        } else {
            DBGLOG_WQOTA_ERR("[wqota] block os_mem_malloc len:%d error\n", len);
            return;
        }
    }

    if (packet->is_cmd) {
        handle_command(packet);
    } else {
        hanle_response(packet);
    }

    ota_packet_buffer = NULL;
}

static void spp_connection_callback(BD_ADDR_T *addr, uint16_t uuid, bool_t connected)
{
    UNUSED(uuid);

    if (connected) {
        DBGLOG_WQOTA_INF("[wqota] spp connection connected\n");
        spps_connected = true;
        remote_addr = *addr;
    } else {
        DBGLOG_WQOTA_INF("[wqota] spp connection disconnected\n");
        spps_connected = false;
    }
}

static void spp_data_callback(BD_ADDR_T *addr, uint16_t uuid, uint8_t *data, uint16_t len)
{
    UNUSED(uuid);
    UNUSED(addr);

    if (ota_ch != OTA_CH_SPP) {
        ota_ch = OTA_CH_SPP;
        ota_file_len = 0;   // reset
        ota_file_crc = 0;   // reset
    }
    handle_ota_data(data, len);
    os_mem_free(data);
}

static void gatts_connection_callback(BD_ADDR_T *addr, bool_t is_ble, bool_t connected)
{
    UNUSED(is_ble);
    DBGLOG_WQOTA_INF("[wqota] gatt connection is_ble:%d connected:%d\n", is_ble, connected);

    if (connected && gatts_connected) {
        DBGLOG_WQOTA_ERR("gatts_connection_callback connected when gatts_connected\n");
        gatts_connected = false;
    }

    if ((!connected) && bdaddr_is_equal(&remote_addr, addr)) {
        gatts_connected = false;
        if (ota_dual_flag && app_ota_sync_get_state()) {
            app_ota_sync_stop();
        }
    }
}

static void send_ota_data(uint8_t *data, uint16_t len)
{
    if ((!gatts_connected) && (!spps_connected)) {
        DBGLOG_WQOTA_ERR("[wqota] send_ota_data error, not connected\n");
        return;
    }

    if (gatts_connected && ota_ch == OTA_CH_BLE) {
        if (app_gatts_send_notify(&remote_addr, character_tx, data, len) != true) {
            DBGLOG_WQOTA_ERR("[wqota] send_ota_data failed\n");
            app_gatts_send_notify(&remote_addr, character_tx, data, len);
        }
    } else if (spps_connected) {
        if (app_spp_send_data(&remote_addr, OTA_UUID_SPP, data, len) != 0) {
            DBGLOG_WQOTA_ERR("[wqota] send_ota_data failed\n");
            app_spp_send_data(&remote_addr, OTA_UUID_SPP, data, len);
        }
    }
}

static void write_callback(BD_ADDR_T *addr, gatts_character_t *character, uint8_t *data,
                           uint16_t len)
{
    UNUSED(character);

    if (!gatts_connected) {
        DBGLOG_WQOTA_INF("[wqota] gatt connection connected\n");
        gatts_connected = true;
        remote_addr = *addr;
        if (ota_ch != OTA_CH_BLE) {
            ota_ch = OTA_CH_BLE;
            ota_file_len = 0;   // reset
            ota_file_crc = 0;   // reset
        }
    }
    DBGLOG_WQOTA_DBG("[wqota] gatt write_callback len:%d\n", len);
    handle_ota_data(data, len);
    os_mem_free(data);
}

static void notify_enable_callback(BD_ADDR_T *addr, gatts_character_t *character, bool_t enabled)
{
    UNUSED(addr);
    UNUSED(character);
    UNUSED(enabled);

    DBGLOG_WQOTA_INF("[wqota] gatt notify_enable_callback enabled:%d\n", enabled);
}

static void app_ota_handle_msg(uint16_t msg, void *param)
{
    switch (msg) {
        case OTA_MSG_ID_TASK_STARTED:
            response_ota_enter_update_mode(0);
            break;
        case OTA_MSG_ID_TASK_FINISH:
            if (ota_file_len != 0 && ota_dual_flag && (ota_ch == OTA_CH_SPP) && app_wws_is_master()
                && (refresh_status_rsp_param.result == 0)) {
                app_ota_sync_start(ota_file_len, ota_file_crc, OTA_SYNC_MODE_PARTIAL);
            }
            send_response(OPCODE_OTA_GET_REFRESH_FIRMWARE_STATUS, &refresh_status_rsp_param,
                          sizeof(refresh_status_rsp_param));
            ota_packet_offset = 0;
            ota_file_offset = 0;
            ota_file_len = 0;
            ota_file_crc = 0;
            if (ota_packet_buffer) {
                ota_pkt_pool_free(ota_packet_buffer);
                ota_packet_buffer = NULL;
            }
            break;
        case OTA_MSG_ID_ROLE_SWITCH:
            app_wws_role_switch();
            break;
        case OTA_MSG_ID_REBOOT:
            ota_task_commit();
            app_pm_reboot(PM_REBOOT_REASON_OTA);
            break;
        case OTA_REMOTE_MSG_ID_SYNC_FILE_CRC: {
            ota_file_crc_sync_param_t *sync_param = param;
            DBGLOG_WQOTA_INF(
                "[wqota] ch:%d remote size:%d offset:%d crc:0x%08X local size:%d offset:%d crc:0x%08X\n",
                ota_ch, sync_param->file_len, sync_param->file_offset, sync_param->file_crc,
                ota_file_len, ota_file_offset, ota_file_crc);
            if (ota_ch == OTA_CH_BLE || ota_dual_flag == 0) {
                break;
            }
            if ((sync_param->file_crc != ota_file_crc) || (sync_param->file_len != ota_file_len)
                || (sync_param->file_offset != ota_file_offset)) {
                ota_file_offset = 0;
            }
            break;
        }
        default:
            break;
    }
}

void app_wqota_init(void)
{
    gatts_service_t *service = NULL;

    app_gatts_register_connection_callback(gatts_connection_callback);

#ifdef OTA_UUID_SERVICE_128
    service = app_gatts_register_service_ext((const uint8_t[])OTA_UUID_SERVICE_128);
#else    //def OTA_UUID_SERVICE_128
    service = app_gatts_register_service(OTA_UUID_SERVICE);
#endif   //def OTA_UUID_SERVICE_128
    assert(service);

#ifdef OTA_UUID_CHARACTER_RX_128
#ifndef OTA_UUID_CHARACTER_TX_128
    character_rx =
        app_gatts_register_character_ext(service, (const uint8_t[])OTA_UUID_CHARACTER_RX_128,
                                         GATT_PROP_WRITE_WITHOUT_RSP | GATT_PROP_NOTIFY, NULL,
                                         write_callback, notify_enable_callback);
    assert(character_rx);
    character_tx = character_rx;
#else    //ndef OTA_UUID_CHARACTER_TX_128
    character_rx =
        app_gatts_register_character_ext(service, (const uint8_t[])OTA_UUID_CHARACTER_RX_128,
                                         GATT_PROP_WRITE_WITHOUT_RSP, NULL, write_callback, NULL);
    assert(character_rx);
    character_tx =
        app_gatts_register_character_ext(service, (const uint8_t[])OTA_UUID_CHARACTER_TX_128,
                                         GATT_PROP_NOTIFY, NULL, NULL, notify_enable_callback);
    assert(character_tx);
#endif   //ndef OTA_UUID_CHARACTER_TX_128
#else    //def OTA_UUID_CHARACTER_RX_128
#if OTA_UUID_CHARACTER_RX == OTA_UUID_CHARACTER_TX
    character_rx = app_gatts_register_character(service, OTA_UUID_CHARACTER_RX,
                                                GATT_PROP_WRITE_WITHOUT_RSP | GATT_PROP_NOTIFY,
                                                NULL, write_callback, notify_enable_callback);
    assert(character_rx);
    character_tx = character_rx;
#else    //OTA_UUID_CHARACTER_RX == OTA_UUID_CHARACTER_TX
    character_rx = app_gatts_register_character(
        service, OTA_UUID_CHARACTER_RX, GATT_PROP_WRITE_WITHOUT_RSP, NULL, write_callback, NULL);
    assert(character_rx);

    character_tx = app_gatts_register_character(service, OTA_UUID_CHARACTER_TX, GATT_PROP_NOTIFY,
                                                NULL, NULL, notify_enable_callback);
    assert(character_tx);
#endif   //OTA_UUID_CHARACTER_RX == OTA_UUID_CHARACTER_TX
#endif   //def OTA_UUID_CHARACTER_RX_128

    app_spp_register_service(OTA_UUID_SPP, spp_connection_callback, spp_data_callback);
    app_register_msg_handler(MSG_TYPE_OTA, app_ota_handle_msg);
}

void app_wqota_deinit(void)
{
    UNUSED(ota_pkt_pool_init);
    UNUSED(ota_pkt_pool_deinit);
    UNUSED(ota_pkt_pool_malloc);
    UNUSED(ota_pkt_pool_free);
}

void app_wqota_start_adv(void)
{
    if (ota_started) {
        DBGLOG_WQOTA_ERR("[wqota] app_wqota_start_ota already started\n");
        return;
    }

    ota_started = true;
    DBGLOG_WQOTA_INF("[wqota] app_wqota_start_ota done\n");

    if (!app_wws_is_master()) {
        DBGLOG_WQOTA_ERR("[wqota] role must be master\n");
        return;
    }

    app_adv_set_enabled(false);
    app_adv_set_adv_data((uint8_t *)&adv_data, sizeof(adv_data));
    app_adv_set_enabled(true);
}

void app_wqota_handle_tws_state_changed(bool_t connected)
{
    ota_file_crc_sync_param_t param;

    if (connected) {
        DBGLOG_WQOTA_ERR("[wqota] send msg size:%d offset:%d crc:0x%08X\n", ota_file_len,
                         ota_file_offset, ota_file_crc);
        param.file_len = ota_file_len;
        param.file_crc = ota_file_crc;
        param.file_offset = ota_file_offset;
        app_wws_send_remote_msg(MSG_TYPE_OTA, OTA_REMOTE_MSG_ID_SYNC_FILE_CRC, sizeof(param),
                                (uint8_t *)&param);
    }
}

#endif   //WQOTA_ENABLED
