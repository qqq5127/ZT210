/****************************************************************************

Copyright(c) 2021 by WuQi Technologies. ALL RIGHTS RESERVED.

This Information is proprietary to WuQi Technologies and MAY NOT
be copied by any method or incorporated into another program without
the express written consent of WuQi. This Information or any portion
thereof remains the property of WuQi. The Information contained herein
is believed to be accurate and WuQi assumes no responsibility or
liability for its use in any way and conveys no license or title under
any patent or copyright and makes no representation or warranty that this
Information is free from patent or copyright infringement.

****************************************************************************/
#include "types.h"
#include "lib_dbglog.h"
#include "dbglog.h"
#include "os_mem.h"

#include "cli.h"
#include "cli_common_definition.h"
#include "cli_common_kv.h"
#include "storage_controller.h"

#define MAX_KV_LENGTH   192

#pragma pack(push) /* save the pack status */
#pragma pack(1)    /* 1 byte align */
typedef struct cli_kv_data {
    uint32_t module_id;
    uint32_t id;
    uint32_t length;
    uint8_t data[];
} cli_kv_data_t;

typedef struct cli_kv_data_get {
    uint32_t module_id;
    uint32_t id;
} cli_kv_data_get_t;
#pragma pack(pop)

void cli_common_basic_set_kv(uint8_t *buffer, uint32_t bufferlen)
{
    uint32_t ret = RET_OK;
    cli_kv_data_t *kv_data = (cli_kv_data_t *)buffer;

    assert(bufferlen > sizeof(cli_kv_data_t));
    ret = storage_write(kv_data->module_id, kv_data->id, (void *)kv_data->data, kv_data->length);
    cli_interface_msg_response(CLI_MODULEID_COMMON, CLI_MSGID_SET_KV, NULL, 0, 0, RET_OK);
    DBGLOG_LIB_CLI_INFO("cli_common_basic_set_kv module:%x, id:%x, length:%x, ret %d\n",
                        kv_data->module_id, kv_data->id, kv_data->length, ret);
}

void cli_common_basic_get_kv(uint8_t *buffer, uint32_t bufferlen)
{
    uint32_t ret = RET_OK;
    cli_kv_data_get_t *req = (cli_kv_data_get_t *)buffer;
    cli_kv_data_t *res = NULL;
    assert(bufferlen == sizeof(cli_kv_data_get_t));
    assert(buffer != NULL);
    res = (cli_kv_data_t *)os_mem_malloc(IOT_CLI_MID, MAX_KV_LENGTH + sizeof(cli_kv_data_t));
    assert(res);
    res->length = MAX_KV_LENGTH;
    ret = storage_read(req->module_id, req->id, (void *)res->data, &(res->length));
    DBGLOG_LIB_CLI_INFO("cli_common_basic_get_kv module:%x, id:%x, length:%x, ret %d\n",
                        req->module_id, req->id, res->length, ret);
    if ((ret == RET_OK) && res->length <= MAX_KV_LENGTH) {
        res->module_id = req->module_id;
        res->id = req->id;
    } else {
        res->length = 0;
    }
    cli_interface_msg_response(CLI_MODULEID_COMMON, CLI_MSGID_GET_KV, (uint8_t *)res,
                               res->length + sizeof(cli_kv_data_t), 0, ret);
    if (res) {
        os_mem_free(res);
    }
}

CLI_ADD_COMMAND(CLI_MODULEID_COMMON, CLI_MSGID_SET_KV, cli_common_basic_set_kv);
CLI_ADD_COMMAND(CLI_MODULEID_COMMON, CLI_MSGID_GET_KV, cli_common_basic_get_kv);
