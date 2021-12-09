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
#include "ota_task.h"
#include "os_task.h"
#include "os_queue.h"
#include "modules.h"
#include "string.h"
#include "assert.h"
#include "ota.h"
#include "crc.h"
#include "iot_rtc.h"
#include "rpc_caller.h"
#include "power_mgnt.h"
#ifdef LIB_DFS_ENABLE
#include "dfs.h"
#endif

#define DBGLOG_OTAT_DBG(fmt, ...) DBGLOG_USER_APP_LOG(DBGLOG_LEVEL_DEBUG, fmt, ##__VA_ARGS__)
#define DBGLOG_OTAT_INF(fmt, ...) DBGLOG_USER_APP_LOG(DBGLOG_LEVEL_INFO, fmt, ##__VA_ARGS__)
#define DBGLOG_OTAT_WRN(fmt, ...) DBGLOG_USER_APP_LOG(DBGLOG_LEVEL_WARNING, fmt, ##__VA_ARGS__)
#define DBGLOG_OTAT_ERR(fmt, ...) DBGLOG_USER_APP_LOG(DBGLOG_LEVEL_ERROR, fmt, ##__VA_ARGS__)

#define OTA_TASK_PRIO           OS_TASK_PRIO_LOWEST
#define OTA_TASK_MSG_QUEUE_SIZE 16

#define OTA_TASK_MSG_TYPE_START      0
#define OTA_TASK_MSG_TYPE_WRITE      1
#define OTA_TASK_MSG_TYPE_WRITE_DATA 2
#define OTA_TASK_MSG_TYPE_FINISH     3

#define INVALID_OTA_HANDLE 0

typedef struct {
    uint8_t msg_type;
    uint8_t reserved;
    uint16_t data_len;
    uint32_t data_offset;
    void *data;
} ota_task_msg_t;

static bool_t started = false;
static os_task_h task_handle = NULL;
static os_queue_h queue_handle = NULL;
static ota_task_callback_t callback = NULL;
static ota_handle_t ota_handle = INVALID_OTA_HANDLE;
static uint32_t write_size = 0;
static uint32_t write_offset = 0;
uint32_t deep_sleep_thr = 60;
uint32_t light_sleep_thr = 0xFFFFFFFF;

static void handle_start(uint32_t image_size)
{
    RET_TYPE ret;

    if (ota_handle != INVALID_OTA_HANDLE) {
        ota_end(ota_handle);
        ota_handle = INVALID_OTA_HANDLE;
    }
    write_size = image_size;
    write_offset = 0;
    DBGLOG_OTAT_INF("[auto][ota_task] ota_begin size:%d", image_size);
    ret = ota_begin(image_size, &ota_handle);
    if (ret) {
        DBGLOG_OTAT_ERR("[auto][ota_task] ota_begin error ret:%d\n", ret);
    } else {
        DBGLOG_OTAT_INF("[auto][ota_task] ota_begin succeed handle:%d\n", ota_handle);
    }
    callback(OTA_TASK_EVT_STARTED, ret, &image_size);
}

static void hanle_write(uint8_t *data, uint16_t len)
{
    RET_TYPE ret;
    if (ota_handle == INVALID_OTA_HANDLE) {
        DBGLOG_OTAT_ERR("[ota_task] hanle_write error ota_handle:%d\n", ota_handle);
        callback(OTA_TASK_EVT_DATA_HANDLED, RET_FAIL, data);
        return;
    }
    ret = ota_write(ota_handle, data, len);
    if (ret) {
        DBGLOG_OTAT_ERR("[ota_task] ota_write error ret:%d hdl:%d len:%d\n", ret, ota_handle, len);
    } else {
        write_offset += len;
        DBGLOG_OTAT_INF("[ota_task] ota_write succeed.handle:0x%X len:%d data:0x%02X crc:0x%08X\n",
                        ota_handle, len, data[0], getcrc32(data, len));
    }
    callback(OTA_TASK_EVT_DATA_HANDLED, ret, data);
}
static uint32_t rtc_last = 0;   //TODO:remove when debug done
static void hanle_write_ext(uint8_t *data, uint16_t len, uint32_t offset)
{
    RET_TYPE ret;
    if (ota_handle == INVALID_OTA_HANDLE) {
        DBGLOG_OTAT_ERR("[ota_task] hanle_write_ext error ota_handle:%d\n", ota_handle);
        callback(OTA_TASK_EVT_DATA_HANDLED, RET_FAIL, data);
        return;
    }
    uint32_t rtc_start = iot_rtc_get_global_time_ms();
    ret = ota_write_data(ota_handle, data, len, offset);
    uint32_t rtc_end = iot_rtc_get_global_time_ms();
    if (ret) {
        DBGLOG_OTAT_ERR("[ota_task] ota_write error ret:%d hdl:%d len:%d\n", ret, ota_handle, len);
    } else {
        DBGLOG_OTAT_INF(
            "[ota_task] ota_write_data size:%d offset:%d len:%d data:0x%02X crc:0x%08X run_time_ms:%d %d\n",
            write_size, offset, len, data[0], getcrc32(data, len), rtc_end - rtc_start,
            rtc_start - rtc_last);
        if (write_offset != offset) {
            DBGLOG_OTAT_ERR("[ota_task]  write_offset:%d != offset:%d\n", write_offset, offset);
            write_offset = offset + len;
        } else {
            write_offset += len;
        }
    }
    rtc_last = rtc_end;
    callback(OTA_TASK_EVT_DATA_HANDLED, ret, data);
}

static void handle_finish(bool_t commit)
{
    RET_TYPE ret;

    ret = ota_end(ota_handle);
    ota_handle = INVALID_OTA_HANDLE;
    if (ret) {
        DBGLOG_OTAT_ERR("[auto][ota_task] ota finish error,end:%d\n", ret);
        callback(OTA_TASK_EVT_FINISH, ret, NULL);
        return;
    }

    if (commit) {
        ret = ota_commit(false);
        if (ret) {
            DBGLOG_OTAT_ERR("[auto][ota_task] ota finish error,commit:%d\n", ret);
            callback(OTA_TASK_EVT_FINISH, ret, NULL);
            return;
        }
    }

    DBGLOG_OTAT_INF("[auto][ota_task] ota finish succeed\n");
    callback(OTA_TASK_EVT_FINISH, 0, NULL);

    power_mgnt_set_sleep_thr(deep_sleep_thr, light_sleep_thr);
    rpc_power_mgnt_set_sleep_thr(deep_sleep_thr, light_sleep_thr);
}

static void ota_task_handler(void *arg)
{
    ota_task_msg_t msg;

    UNUSED(arg);

    while (1) {
        if (!os_queue_receive(queue_handle, &msg)) {
            continue;
        }

        switch (msg.msg_type) {
            case OTA_TASK_MSG_TYPE_START:
                handle_start((uint32_t)msg.data);
                break;
            case OTA_TASK_MSG_TYPE_WRITE:
                hanle_write(msg.data, msg.data_len);
                break;
            case OTA_TASK_MSG_TYPE_WRITE_DATA:
                hanle_write_ext(msg.data, msg.data_len, msg.data_offset);
                break;
            case OTA_TASK_MSG_TYPE_FINISH:
                handle_finish((uint32_t)msg.data);
                break;
            default:
                break;
        }
    }
}

static int ota_task_send_msg(uint8_t type, void *data, uint16_t data_len, uint32_t data_offset)
{
    ota_task_msg_t msg;

    msg.msg_type = type;
    msg.data = data;
    msg.data_len = data_len;
    msg.data_offset = data_offset;

    if (os_queue_send(queue_handle, &msg)) {
        return 0;
    } else {
        return RET_AGAIN;
    }
}

int ota_task_resume(ota_task_callback_t _callback)
{
    callback = _callback;

    power_mgnt_get_sleep_thr(&deep_sleep_thr, &light_sleep_thr);
    power_mgnt_set_sleep_thr(0xFFFFFFFF, 0xFFFFFFFF);
    rpc_power_mgnt_set_sleep_thr(0xFFFFFFFF, 0xFFFFFFFF);
#ifdef LIB_DFS_ENABLE
    uint32_t ret = dfs_start(DFS_OBJ_DTOP_OTA, 0);
    DBGLOG_OTAT_INF("[ota_task] dfs_request OTA START ret %d\n", ret);
#endif

    if (started) {
        DBGLOG_OTAT_INF("[auto][ota_task] ota_task_start, already started\n");
    }

    DBGLOG_OTAT_INF("[auto][ota_task] ota_task_start\n");

    if (queue_handle == NULL) {
        DBGLOG_OTAT_INF("[auto][ota_task] ota queue create\n");
        queue_handle =
            os_queue_create(LIB_MID_START, OTA_TASK_MSG_QUEUE_SIZE, sizeof(ota_task_msg_t));
        if (queue_handle == NULL) {
            DBGLOG_OTAT_ERR("[auto][ota_task] ota queue create failed\n");
            return RET_FAIL;
        }
    }

    if (task_handle == NULL) {
        DBGLOG_OTAT_INF("[auto][ota_task] ota task create\n");
        task_handle = os_create_task(ota_task_handler, NULL, OTA_TASK_PRIO);
        if (task_handle == NULL) {
            DBGLOG_OTAT_ERR("[auto][ota_task] ota task create failed\n");
            return RET_FAIL;
        }
    }

    started = true;
    DBGLOG_OTAT_INF("[auto][ota_task] ota_task_start started\n");
    return 0;
}

int ota_task_start(ota_task_callback_t _callback, uint32_t image_size)
{
    int ret = ota_task_resume(_callback);
    if (ret != RET_OK) {
        return ret;
    } else {
        return ota_task_send_msg(OTA_TASK_MSG_TYPE_START, (void *)image_size, 0, 0);
    }
}

int ota_task_write(uint8_t *data, uint16_t len)
{
    if (!started) {
        DBGLOG_OTAT_ERR("[ota_task] ota_task_write error: not started\n");
        return RET_NOT_EXIST;
    }
    return ota_task_send_msg(OTA_TASK_MSG_TYPE_WRITE, data, len, 0);
}

int ota_task_write_ext(uint8_t *data, uint16_t len, uint32_t offset)
{
    if (!started) {
        DBGLOG_OTAT_ERR("ota_task_write_ext error: not started\n");
        return RET_NOT_EXIST;
    }
    return ota_task_send_msg(OTA_TASK_MSG_TYPE_WRITE_DATA, data, len, offset);
}

int ota_task_read_ext(uint8_t *data, uint16_t len, uint32_t offset)
{
    ota_read_data(data, len, offset);
    return RET_OK;
}

uint32_t ota_task_get_write_offset(void)
{
    return write_offset;
}

int ota_task_finish(void)
{
    if (!started) {
        DBGLOG_OTAT_ERR("[auto][ota_task] ota finish error,not started\n");
        return RET_FAIL;
    }
    return ota_task_send_msg(OTA_TASK_MSG_TYPE_FINISH, (void *)1, 0, 0);
}

bool_t ota_task_is_running(void)
{
    return started;
}

int ota_task_finish_without_commit(void)
{
    if (!started) {
        DBGLOG_OTAT_ERR("[auto][ota_task] ota finish error,not started\n");
        return RET_FAIL;
    }
    return ota_task_send_msg(OTA_TASK_MSG_TYPE_FINISH, (void *)0, 0, 0);
}

int ota_task_commit(void)
{
    RET_TYPE ret;
    if (!started) {
        DBGLOG_OTAT_ERR("[auto][ota_task] ota commit error, not started\n");
        return RET_FAIL;
    }
    ret = ota_commit(false);
    if (ret) {
        DBGLOG_OTAT_ERR("[auto][ota_task] ota commit error, ret:%d\n", ret);
    } else {
        DBGLOG_OTAT_DBG("[auto][ota_task] ota commit succeed\n");
    }

    return ret;
}
