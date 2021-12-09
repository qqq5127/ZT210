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
#include "app_ota_sync.h"
#if OTA_SYNC_ENABLED
#include "os_mem.h"
#include "assert.h"
#include "app_main.h"
#include "app_wws.h"
#include "ota_task.h"
#include "ota.h"
#include "crc.h"
#include "app_pm.h"

#define DBGLOG_OTASYNC_DBG(fmt, ...) DBGLOG_USER_APP_LOG(DBGLOG_LEVEL_DEBUG, fmt, ##__VA_ARGS__)
#define DBGLOG_OTASYNC_INF(fmt, ...) DBGLOG_USER_APP_LOG(DBGLOG_LEVEL_INFO, fmt, ##__VA_ARGS__)
#define DBGLOG_OTASYNC_WRN(fmt, ...) DBGLOG_USER_APP_LOG(DBGLOG_LEVEL_WARNING, fmt, ##__VA_ARGS__)
#define DBGLOG_OTASYNC_ERR(fmt, ...) DBGLOG_USER_APP_LOG(DBGLOG_LEVEL_ERROR, fmt, ##__VA_ARGS__)

#define OTA_SYNC_DATA_LEN 512

#define DATA_POOL_ITEM_SIZE (sizeof(remote_msg_get_data_rsp_t) + 8)

#ifndef OTA_SYNC_USE_MEM_POOL
#define OTA_SYNC_USE_MEM_POOL 1
#endif

#ifndef OTA_MAX_SYNC_DATA_COUNT
#define OTA_MAX_SYNC_DATA_COUNT 2
#endif

#ifndef OTA_FLASH_DEFAULT_VALUE
#define OTA_FLASH_DEFAULT_VALUE 0xFFFFFFFF
#endif

#define OTA_SYNC_MSG_ID_GET_DATA            0x01
#define OTA_SYNC_MSG_ID_STOP                0x02
#define OTA_SYNC_MSG_ID_DO_REBOOT           0x03
#define OTA_SYNC_REMOTE_MSG_ID_START        0x04
#define OTA_SYNC_REMOTE_MSG_ID_STOP         0x05
#define OTA_SYNC_REMOTE_MSG_ID_REBOOT       0x06
#define OTA_SYNC_REMOTE_MSG_ID_GET_DATA_REQ 0x07
#define OTA_SYNC_REMOTE_MSG_ID_GET_DATA_RSP 0x08
#define OTA_SYNC_REMOTE_MSG_ID_DONE_REPORT  0x09

typedef struct {
    uint32_t size;
    uint32_t crc;
    ota_sync_mode_e sync_mode : 8;
} remote_msg_start_t;

typedef struct {
    uint32_t offset;
} remote_msg_get_data_req_t;

typedef struct {
    uint32_t offset;
    uint16_t len;
    uint16_t reserved;
    uint32_t crc;
    uint8_t data[OTA_SYNC_DATA_LEN];
} remote_msg_get_data_rsp_t;

static ota_sync_mode_e sync_mode = OTA_SYNC_MODE_COMPLETE;
static uint32_t m_file_length = 0;
static uint32_t m_file_crc = 0;
static uint32_t m_file_offset = 0;
static uint8_t m_sync_flag = 0;
static uint32_t data_pool_used_cnt[OTA_MAX_SYNC_DATA_COUNT];
static uint32_t data_pool_free_cnt[OTA_MAX_SYNC_DATA_COUNT];

#if OTA_SYNC_USE_MEM_POOL
static void *data_pool[OTA_MAX_SYNC_DATA_COUNT];

static bool_t data_pool_init(void)
{
    bool_t succeed = true;
    for (int i = 0; i < OTA_MAX_SYNC_DATA_COUNT; i++) {
        if (!data_pool[i]) {
            data_pool[i] = os_mem_malloc(IOT_APP_MID, DATA_POOL_ITEM_SIZE);
            data_pool_used_cnt[i] = 0;
            data_pool_free_cnt[i] = 0;
        }
        if (!data_pool[i]) {
            succeed = false;
            break;
        }
    }

    if (!succeed) {
        for (int i = 0; i < OTA_MAX_SYNC_DATA_COUNT; i++) {
            if (data_pool[i]) {
                os_mem_free(data_pool[i]);
                data_pool[i] = NULL;
            }
        }
    }
    return succeed;
}

static void data_pool_deinit(void)
{
    for (int i = 0; i < OTA_MAX_SYNC_DATA_COUNT; i++) {
        if (data_pool[i]) {
            os_mem_free(data_pool[i]);
            data_pool[i] = NULL;
            data_pool_used_cnt[i] = 0;
            data_pool_free_cnt[i] = 0;
        }
    }
}

static void *data_pool_malloc(uint32_t size)
{
    assert(size <= DATA_POOL_ITEM_SIZE);

    for (int i = 0; i < OTA_MAX_SYNC_DATA_COUNT; i++) {
        assert(data_pool_free_cnt[i] <= data_pool_used_cnt[i]);
        if (data_pool_free_cnt[i] == data_pool_used_cnt[i]) {
            data_pool_used_cnt[i] += 1;
            return data_pool[i];
        }
    }

    return NULL;
}

static void data_pool_free(void *ptr)
{
    for (int i = 0; i < OTA_MAX_SYNC_DATA_COUNT; i++) {
        if (data_pool[i] == ptr) {
            data_pool_free_cnt[i] += 1;
            break;
        }
    }
}
#else
static inline bool_t data_pool_init(void)
{
    return true;
}

static inline void data_pool_deinit(void)
{
}

static inline void *data_pool_malloc(uint32_t size)
{
    return os_mem_malloc(IOT_APP_MID, size);
}

static inline void data_pool_free(void *ptr)
{
    os_mem_free(ptr);
}
#endif

static bool_t is_flash_written(uint32_t offset, uint32_t len)
{
    uint32_t data;
    uint8_t data8;
    uint32_t empty_count = 0;

    if (len < 32) {
        for (uint32_t i = 0; i < len; i++) {
            ota_read_data(&data8, sizeof(uint8_t), offset + i);
            if (data8 != (OTA_FLASH_DEFAULT_VALUE & 0xFF)) {
                return true;
            }
        }
        return false;
    }

    ota_read_data(&data, sizeof(uint32_t), offset);
    if (data == OTA_FLASH_DEFAULT_VALUE) {
        empty_count += 1;
    }

    ota_read_data(&data, sizeof(uint32_t), offset + len / 2);
    if (data == OTA_FLASH_DEFAULT_VALUE) {
        empty_count += 1;
    }

    ota_read_data(&data, sizeof(uint32_t), offset + len - 4);
    if (data == OTA_FLASH_DEFAULT_VALUE) {
        empty_count += 1;
    }

    if (empty_count == 0) {
        return true;
    } else if (empty_count == 3) {
        return false;
    } else {
        DBGLOG_OTASYNC_ERR("is_flash_written error offset:%d empty_count:%d\n", offset,
                           empty_count);
        return false;
    }
}

static void offset_validate(void)
{
    uint32_t len;

    while (m_file_offset < m_file_length) {
        if (m_file_offset + OTA_SYNC_DATA_LEN > m_file_length) {
            len = m_file_length - m_file_offset;
        } else {
            len = OTA_SYNC_DATA_LEN;
        }

        if (!is_flash_written(m_file_offset, len)) {
            break;
        }

        m_file_offset += len;
    }
}

static inline void send_remote_msg(app_msg_type_t type, uint16_t id, uint16_t param_len,
                                   const uint8_t *param)
{
    if (app_wws_is_master()) {
        app_wws_send_remote_msg(type, id, param_len, param);
    } else {
        app_wws_send_remote_msg_no_activate(type, id, param_len, param);
    }
}

static void send_get_data_req(uint32_t offset)
{
    app_cancel_msg(MSG_TYPE_SYNC, OTA_SYNC_MSG_ID_GET_DATA);

    if (offset >= m_file_length) {
        ota_task_finish_without_commit();
        return;
    }

    remote_msg_get_data_req_t msg;
    msg.offset = offset;
    send_remote_msg(MSG_TYPE_SYNC, OTA_SYNC_REMOTE_MSG_ID_GET_DATA_REQ, sizeof(msg),
                    (uint8_t *)&msg);

    DBGLOG_OTASYNC_INF("[ota_sync] send_get_data_req size:%d offset:%d wws_conn:%d role:%d\n",
                       m_file_length, offset, app_wws_is_connected(), app_wws_is_master());

    app_send_msg_delay(MSG_TYPE_SYNC, OTA_SYNC_MSG_ID_GET_DATA, NULL, 0, 5000);
}

static void send_get_data_rsp(uint32_t offset)
{
    uint32_t block_len = 0;
    uint8_t *data;
    remote_msg_get_data_rsp_t *msg;
    uint32_t crc;

    data = data_pool_malloc(sizeof(remote_msg_get_data_rsp_t) + 8);

    if (!data) {
        DBGLOG_OTASYNC_ERR("send_get_data_rsp malloc failed\n");
        return;
    }

    msg = (remote_msg_get_data_rsp_t *)&data[8];

    if (offset < m_file_length) {
        block_len = m_file_length - offset;
        if (block_len > OTA_SYNC_DATA_LEN) {
            block_len = OTA_SYNC_DATA_LEN;
        }
    }
    msg->offset = offset;
    msg->len = block_len;
    if (offset >= ota_task_get_write_offset()) {
        msg->len = 0;
        msg->crc = 0;
        DBGLOG_OTASYNC_INF("[ota_sync] ota_task_get_write_offset 0x%X\n",
                           ota_task_get_write_offset());
    } else {
        ota_task_read_ext(msg->data, msg->len, msg->offset);
        crc = getcrc32(msg->data, msg->len);
        msg->crc = crc;
        DBGLOG_OTASYNC_INF(
            "[ota_sync] msg_send_rsp size:%d offset:%d len:%d data:0x%02X crc:0x%08X wws_conn:%d role:%d\n",
            m_file_length, msg->offset, msg->len, msg->data[0], crc, app_wws_is_connected(),
            app_wws_is_master());
    }
    app_wws_send_remote_msg_ext(MSG_TYPE_SYNC, OTA_SYNC_REMOTE_MSG_ID_GET_DATA_RSP, data,
                                sizeof(remote_msg_get_data_rsp_t), 8);
    data_pool_free(data);
}

static void ota_task_callback(ota_task_evt_t evt, int result, void *data)
{
    UNUSED(data);
    if (evt == OTA_TASK_EVT_STARTED) {
        DBGLOG_OTASYNC_INF("[ota_sync] OTA_TASK_EVT_STARTED result:%d\n", result);
        app_send_msg(MSG_TYPE_SYNC, OTA_SYNC_MSG_ID_GET_DATA, NULL, 0);
    } else if (evt == OTA_TASK_EVT_DATA_HANDLED) {
        if (data) {
            data_pool_free(data);
        }
        if (result) {
            DBGLOG_OTASYNC_ERR("[ota_sync] data error result:%d\n", result);
        }
    } else if (evt == OTA_TASK_EVT_FINISH) {
        DBGLOG_OTASYNC_INF("[ota_sync] OTA_TASK_EVT_FINISH result:%d pool used:%d,%d free:%d,%d\n",
                           result, data_pool_used_cnt[0], data_pool_used_cnt[1],
                           data_pool_free_cnt[0], data_pool_free_cnt[1]);
        m_file_offset = 0;
        m_file_length = 0;
        m_file_crc = 0;
        app_send_msg(MSG_TYPE_SYNC, OTA_SYNC_MSG_ID_STOP, NULL, 0);
    } else {
        DBGLOG_OTASYNC_ERR("[ota_sync] ota_task_callback unknown evt:%d\n", evt);
    }
}

static void handle_msg(uint16_t msg_id, void *param)
{
    UNUSED(msg_id);
    UNUSED(param);
    switch (msg_id) {
        case OTA_SYNC_MSG_ID_GET_DATA:
            if (m_sync_flag && app_wws_is_connected()) {
                if (sync_mode == OTA_SYNC_MODE_PARTIAL) {
                    offset_validate();
                }
                send_get_data_req(m_file_offset);
            }
            break;
        case OTA_SYNC_MSG_ID_STOP:
            m_sync_flag = 0;
            DBGLOG_OTASYNC_INF("[ota_sync] local stop wws_conn:%d role:%d\n",
                               app_wws_is_connected(), app_wws_is_master());
            send_remote_msg(MSG_TYPE_SYNC, OTA_SYNC_REMOTE_MSG_ID_DONE_REPORT, 0, NULL);
            break;
        case OTA_SYNC_REMOTE_MSG_ID_STOP:
            DBGLOG_OTASYNC_INF("[ota_sync] remote stop\n");
            m_sync_flag = 0;
            break;
        case OTA_SYNC_REMOTE_MSG_ID_REBOOT:
            DBGLOG_OTASYNC_INF("[ota_sync] remote reboot\n");
            app_send_msg_delay(MSG_TYPE_SYNC, OTA_SYNC_MSG_ID_DO_REBOOT, NULL, 0, 1000);
            break;
        case OTA_SYNC_REMOTE_MSG_ID_START:
            if (param) {
                remote_msg_start_t *msg = (remote_msg_start_t *)param;
                assert(data_pool_init());
                sync_mode = msg->sync_mode;
                if (sync_mode == OTA_SYNC_MODE_PARTIAL) {
                    m_file_offset = 0;
                    m_file_length = msg->size;
                    m_file_crc = msg->crc;
                }
                DBGLOG_OTASYNC_INF(
                    "[ota_sync] msg_recv_start size:%d %d crc:0x%08X 0x%08X wws_conn:%d role:%d mode:%d\n",
                    msg->size, m_file_length, msg->crc, m_file_crc, app_wws_is_connected(),
                    app_wws_is_master(), sync_mode);
                if (m_file_length == msg->size && m_file_crc == msg->crc) {
                    if (ota_task_resume(ota_task_callback) != 0) {
                        DBGLOG_OTASYNC_ERR("[ota_sync] ota_task_resume error\n");
                    } else {
                        DBGLOG_OTASYNC_INF(
                            "[ota_sync] resume succeed size:%d offset:%d crc:0x%08X\n",
                            m_file_length, m_file_offset, m_file_crc);
                        app_send_msg(MSG_TYPE_SYNC, OTA_SYNC_MSG_ID_GET_DATA, NULL, 0);
                        m_sync_flag = 1;
                    }
                } else {
                    m_file_offset = 0;
                    m_file_length = msg->size;
                    m_file_crc = msg->crc;
                    if (ota_task_start(ota_task_callback, msg->size) != 0) {
                        DBGLOG_OTASYNC_ERR("[ota_sync] ota_task_start error\n");
                    } else {
                        DBGLOG_OTASYNC_INF(
                            "[ota_sync] start succeed size:%d offset:%d crc:0x%08X\n",
                            m_file_length, m_file_offset, m_file_crc);
                        m_sync_flag = 1;
                    }
                }
            }
            break;
        case OTA_SYNC_REMOTE_MSG_ID_GET_DATA_REQ:
            if (param) {
                remote_msg_get_data_req_t *msg = (remote_msg_get_data_req_t *)param;
                DBGLOG_OTASYNC_INF(
                    "[ota_sync] msg_recv_req size:%d offset:%d wws_conn:%d role:%d\n",
                    m_file_length, msg->offset, app_wws_is_connected(), app_wws_is_master());
                if (app_wws_is_connected()) {
                    send_get_data_rsp(msg->offset);
                }
            }
            break;
        case OTA_SYNC_REMOTE_MSG_ID_GET_DATA_RSP:
            if (param) {
                remote_msg_get_data_rsp_t *msg = (remote_msg_get_data_rsp_t *)param;
                uint32_t crc;
                uint8_t *buffer;

                if (msg->len && msg->offset == m_file_offset) {
                    crc = getcrc32(msg->data, msg->len);
                    DBGLOG_OTASYNC_INF(
                        "[ota_sync] msg_recv_rsp size:%d offset:%d len:%d data:0x%02X crc:0x%08X wws_conn:%d role:%d\n",
                        m_file_length, msg->offset, msg->len, msg->data[0], crc,
                        app_wws_is_connected(), app_wws_is_master());
                    if (crc != msg->crc) {
                        DBGLOG_OTASYNC_ERR(
                            "[ota_sync] msg_recv_rsp crc error expected:0x%X calc:0x%X\n", msg->crc,
                            crc);
                        app_cancel_msg(MSG_TYPE_SYNC, OTA_SYNC_MSG_ID_GET_DATA);
                        app_send_msg_delay(MSG_TYPE_SYNC, OTA_SYNC_MSG_ID_GET_DATA, NULL, 0, 500);
                        break;
                    }
                    buffer = data_pool_malloc(msg->len);
                    if (buffer) {
                        memcpy(buffer, msg->data, msg->len);
                        if (ota_task_write_ext(buffer, msg->len, msg->offset) == RET_OK) {
                            m_file_offset += msg->len;
                        } else {
                            data_pool_free(buffer);
                        }
                        app_send_msg(MSG_TYPE_SYNC, OTA_SYNC_MSG_ID_GET_DATA, NULL, 0);
                    } else {
                        DBGLOG_OTASYNC_ERR("malloc for task failed\n");
                        app_cancel_msg(MSG_TYPE_SYNC, OTA_SYNC_MSG_ID_GET_DATA);
                        app_send_msg_delay(MSG_TYPE_SYNC, OTA_SYNC_MSG_ID_GET_DATA, NULL, 0, 500);
                    }
                } else {
                    DBGLOG_OTASYNC_INF(
                        "[ota_sync] msg_recv_rsp size:%d offset:%d len:%d wws_conn:%d role:%d",
                        m_file_length, msg->offset, msg->len, app_wws_is_connected(),
                        app_wws_is_master());
                    app_cancel_msg(MSG_TYPE_SYNC, OTA_SYNC_MSG_ID_GET_DATA);
                    app_send_msg_delay(MSG_TYPE_SYNC, OTA_SYNC_MSG_ID_GET_DATA, NULL, 0, 500);
                }
            }
            break;
        case OTA_SYNC_REMOTE_MSG_ID_DONE_REPORT:
            DBGLOG_OTASYNC_INF(
                "[ota_sync] msg_recv_finish wws_conn:%d role:%d pool used:%d,%d free:%d,%d\n",
                app_wws_is_connected(), app_wws_is_master(), data_pool_used_cnt[0],
                data_pool_used_cnt[1], data_pool_free_cnt[0], data_pool_free_cnt[1]);
            m_sync_flag = 0;
            break;
        case OTA_SYNC_MSG_ID_DO_REBOOT:
            DBGLOG_OTASYNC_INF("[ota_sync] OTA_SYNC_MSG_ID_DO_REBOOT\n");
            ota_task_commit();
            app_pm_reboot(PM_REBOOT_REASON_OTA);
            break;
        default:
            break;
    }
}

void app_ota_sync_init(void)
{
    UNUSED(data_pool_deinit);

    app_register_msg_handler(MSG_TYPE_SYNC, handle_msg);
}

void app_ota_sync_deinit(void)
{
}

void app_ota_sync_start(uint32_t image_size, uint32_t image_crc, ota_sync_mode_e mode)
{
    remote_msg_start_t msg;

    assert(data_pool_init());

    sync_mode = mode;
    m_file_length = image_size;
    m_file_crc = image_crc;
    if (app_wws_is_connected()) {
        m_sync_flag = 1;
        msg.size = image_size;
        msg.crc = image_crc;
        msg.sync_mode = mode;
        send_remote_msg(MSG_TYPE_SYNC, OTA_SYNC_REMOTE_MSG_ID_START, sizeof(msg), (uint8_t *)&msg);
        DBGLOG_OTASYNC_INF("[ota_sync] msg_send_start size:%d crc:0x%08X wws:%d role:%d mode:%d\n",
                           image_size, image_crc, app_wws_is_connected(), app_wws_is_master(),
                           sync_mode);
    }
}

void app_ota_sync_stop(void)
{
    m_sync_flag = 0;
    if (app_wws_is_connected()) {
        send_remote_msg(MSG_TYPE_SYNC, OTA_SYNC_REMOTE_MSG_ID_STOP, 0, NULL);
        DBGLOG_OTASYNC_INF("[ota_sync] msg_send_stop wws_conn:%d role:%d\n", app_wws_is_connected(),
                           app_wws_is_master());
    }
    DBGLOG_OTASYNC_INF("[ota_sync] app_ota_sync_stop\n");
}

void app_ota_sync_reboot(void)
{
    if (app_wws_is_connected()) {
        send_remote_msg(MSG_TYPE_SYNC, OTA_SYNC_REMOTE_MSG_ID_REBOOT, 0, NULL);
        DBGLOG_OTASYNC_INF("[ota_sync] remote_msg_send_reboot wws_conn:%d role:%d\n",
                           app_wws_is_connected(), app_wws_is_master());
    }
    DBGLOG_OTASYNC_INF("[ota_sync] app_ota_sync_reboot\n");
}

uint8_t app_ota_sync_get_state(void)
{
    return m_sync_flag;
}

#endif   //OTA_SYNC_ENABLED
