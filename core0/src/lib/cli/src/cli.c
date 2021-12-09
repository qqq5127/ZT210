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
#include "types.h"
#include "string.h"
#include "stdio.h"
#include "crc.h"
#include "os_mem.h"
#include "os_event.h"
#include "os_lock.h"
#include "os_task.h"
#include "riscv_cpu.h"
#include "generic_list.h"

#include "dbglog.h"
#include "cli.h"
#include "critical_sec.h"

#include "lib_dbglog.h"
#include "generic_transmission_api.h"
#include "generic_transmission_config.h"
#include "types.h"

#define CLI_MSG_START     0x2323
#define CLI_MSG_START_LEN 2
#define CLI_MSG_END       0x4040
#define CLI_MSG_END_LEN   2

typedef enum {
    CLI_TYPE_CMD,
    CLI_TYPE_RES,
    CLI_TYPE_IND,
    CLI_TYPE_MAX,
} CLI_TYPE;

typedef struct cli_msg_header {
    uint16_t module_id;
    uint16_t crc;
    uint16_t msg_id;
    uint8_t auto_ack : 1;
    uint8_t type : 3;
    uint8_t reversed0 : 4;
    uint8_t result;
    uint16_t msg_length;
    uint16_t msg_sn;
} __attribute((packed)) cli_msg_header_t;

typedef struct cli_msg_legacy_header {
    uint8_t src_addr[6];
    uint8_t dst_addr[6];
    uint16_t module_id;
    uint16_t crc;
    uint16_t msg_id;
    uint8_t auto_ack : 1;
    uint8_t reversed0 : 7;
    uint8_t result;
    uint16_t msg_length;
    uint16_t msg_sn;
} __attribute((packed)) cli_msg_legacy_header_t;

static uint16_t g_msg_cmd_sn;
static uint16_t g_msg_ind_sn = 0;
static os_sem_h cli_receive_msg_event = NULL;
static os_task_h cli_task_handle = NULL;

struct cli_cmd_item {
    struct list_head node;
    uint8_t *cmd_data;
    uint16_t cmd_size;
};

struct cli_cmd_list_status {
    struct list_head cli_cmd_list;
    uint16_t cli_cmd_num;
};

static struct cli_cmd_list_status cli_cmd_list_info = {
    .cli_cmd_list = {0},
    .cli_cmd_num = 0,
};

static void cli_trigger_process(void)
{
    if(cli_receive_msg_event){
        os_post_semaphore(cli_receive_msg_event);
    }
}

static uint8_t cli_message_parser(uint8_t *input_buffer, uint16_t input_len,
                                  uint8_t **msg, uint32_t *msg_len,
                                  uint16_t *module_id, uint16_t *msg_id)
{
    UNUSED(input_len);
    uint16_t cli_guard = 0;
    memcpy(&cli_guard, input_buffer, 2);
    if(cli_guard != CLI_MSG_START) {
        return RET_NOT_EXIST;
    }
    input_buffer += 2;
#ifdef CLI_LEGACY_MODE
    cli_msg_legacy_header_t cli_header;
    memcpy(&cli_header, input_buffer, sizeof(cli_msg_legacy_header_t));
    input_buffer += sizeof(cli_msg_legacy_header_t);
#else
    cli_msg_header_t cli_header;
    memcpy(&cli_header, input_buffer, sizeof(cli_msg_header_t));
    input_buffer += sizeof(cli_msg_header_t);
#endif
    if(cli_header.msg_length > DEFAULT_CLI_MAX_BUFFER_LEN) {
        return RET_NOT_EXIST;
    }
    // Get CLI header
    if (cli_header.msg_length) {
        *msg_len = cli_header.msg_length;
        *module_id = cli_header.module_id;
        *msg_id = cli_header.msg_id;
        *msg = input_buffer;
        g_msg_cmd_sn = cli_header.msg_sn;
    } else if(cli_header.msg_length == 0){
        *msg = NULL;
        *msg_len = cli_header.msg_length;
        *module_id = cli_header.module_id;
        *msg_id = cli_header.msg_id;
        g_msg_cmd_sn = cli_header.msg_sn;
    }
    input_buffer += cli_header.msg_length;
    memcpy(&cli_guard, input_buffer, 2);

    if (cli_guard != CLI_MSG_END){
        return RET_NOT_EXIST;
    }
    return RET_OK;
}

static void recycle_cli_item(struct cli_cmd_item *item)
{
    cpu_critical_enter();
    list_del(&item->node);
    cli_cmd_list_info.cli_cmd_num--;
    cpu_critical_exit();

    os_mem_free(item->cmd_data);
    os_mem_free(item);
}

static void cli_task(void* arg)
{
    UNUSED(arg);
    uint8_t *msg;
    uint32_t msg_len;
    uint16_t module_id;
    uint16_t msg_id;
    struct list_head *saved;
    struct cli_cmd_item *item;
    while (1) {
        os_pend_semaphore(cli_receive_msg_event, 0xFFFFFFFF);
        list_for_each_entry_safe(item, &cli_cmd_list_info.cli_cmd_list, node, saved) {
            if (RET_OK != cli_message_parser(item->cmd_data, item->cmd_size, &msg, &msg_len, &module_id, &msg_id)){
                recycle_cli_item(item);
                continue;
            }
            cli_command_invoke(module_id, msg_id, msg, msg_len);
            recycle_cli_item(item);
        }
    }
}

static void cli_rx_callback(generic_transmission_tid_t tid, generic_transmission_data_type_t type,
                            uint8_t *data, uint32_t data_len,
                            generic_transmission_data_rx_cb_st_t status)
{
    cli_interface_msg_receive(tid, type, data, (uint16_t)data_len, status);
}

uint8_t cli_init(cli_config_t *cli_config)
{
    if (cli_config == NULL) {
        return RET_INVAL;
    }
    cli_receive_msg_event = os_create_semaphore(IOT_CLI_MID, MAX_TIME, false);
    assert(cli_receive_msg_event);

    // init cli command list
    list_init(&cli_cmd_list_info.cli_cmd_list);

    // use default cli config here
    if(cli_config->cli_task_size == 0) {
        cli_config->cli_task_size = DEFAULT_CLI_TASK_SIZE;
    }
    if(cli_config->cli_prio == 0) {
        cli_config->cli_prio = DEFAULT_CLI_TASK_PRIO;
    }

    generic_transmission_register_rx_callback(4, cli_rx_callback);

    cli_task_handle = os_create_task_ext(cli_task, NULL, cli_config->cli_prio, cli_config->cli_task_size, "cli");
    if(!cli_task_handle) {
        os_delete_task(cli_task_handle);
        return RET_FAIL;
    }
    return RET_OK;
}

uint8_t cli_deinit(void)
{
    os_delete_mutex(cli_receive_msg_event);
    cli_receive_msg_event = NULL;
    os_delete_task(cli_task_handle);
    return RET_OK;
}

uint8_t cli_interface_msg_receive(generic_transmission_tid_t tid,generic_transmission_data_type_t type,
                                  uint8_t *buffer, uint16_t len,
                                   generic_transmission_data_rx_cb_st_t status)
{
    UNUSED(tid);
    UNUSED(type);
    UNUSED(status);
    struct cli_cmd_item *item;
    //  CLI task has not init now
    if(!cli_task_handle) {
        return RET_NOT_READY;
    }

    item = os_mem_malloc(IOT_CLI_MID, sizeof(struct cli_cmd_item));
    if (!item) {
        return RET_NOMEM;
    }
    // TODO: should not exceed MAX cli command lenght here
    item->cmd_data = os_mem_malloc(IOT_CLI_MID, len);
    if (item->cmd_data == NULL) {
        os_mem_free(item);
        return RET_NOMEM;
    }
    memcpy(item->cmd_data, buffer, len);
    item->cmd_size = len;

    cpu_critical_enter();
    list_add_tail(&item->node, &cli_cmd_list_info.cli_cmd_list);
    cli_cmd_list_info.cli_cmd_num++;
    cpu_critical_exit();

    cli_trigger_process();
    return RET_OK;
}

static uint8_t cli_interface_msg_send(uint32_t module_id, uint32_t msg_id,
                                   uint8_t *buffer, uint32_t buffer_len,
                                   uint8_t result, CLI_TYPE type)
{
    uint8_t *response;
#ifdef CLI_LEGACY_MODE
    cli_msg_legacy_header_t cli_header;
    uint32_t response_len = sizeof(cli_msg_legacy_header_t) + buffer_len
        + CLI_MSG_START_LEN + CLI_MSG_END_LEN;
#else
    cli_msg_header_t cli_header;
    uint32_t response_len = sizeof(cli_msg_header_t) + buffer_len
        + CLI_MSG_START_LEN + CLI_MSG_END_LEN;
#endif
    response = os_mem_malloc(IOT_CLI_MID, response_len);

    if (response == NULL) {
        return RET_NOMEM;
    }

    memset(response, 0, response_len);
    memset(&cli_header, 0, sizeof(cli_header));
    response[0] = CLI_MSG_START & 0xFF;
    response[1] = (CLI_MSG_START >> 8) & 0xFF;
    cli_header.module_id = module_id;
    cli_header.msg_id = msg_id;
    cli_header.msg_length = buffer_len;
    cli_header.crc = getcrc32_h16(buffer, buffer_len);
    if (type == CLI_TYPE_RES)
    {
        cli_header.msg_sn = g_msg_cmd_sn;
    } else {
        cli_header.msg_sn = g_msg_ind_sn++;
    }

    cli_header.result = result;
    cli_header.type = type;
    memcpy(response + 2, &cli_header, sizeof(cli_header));
    memcpy(response + 2 + sizeof(cli_header), buffer, buffer_len);
    response[2 + sizeof(cli_header) + buffer_len] = CLI_MSG_END & 0xFF;
    response[2 + sizeof(cli_header) + buffer_len + 1] = (CLI_MSG_END >> 8) & 0xFF;

    int ret = generic_transmission_data_tx(GENERIC_TRANSMISSION_TX_MODE_LAZY, GENERIC_TRANSMISSION_DATA_TYPE_CLI, CLI_TID,\
                                       GENERIC_TRANSMISSION_IO_UART0, (const uint8_t *)response,  \
                                       response_len, 0);
    generic_transmission_data_tx(GENERIC_TRANSMISSION_TX_MODE_LAZY, GENERIC_TRANSMISSION_DATA_TYPE_CLI, CLI_TID,\
                                GENERIC_TRANSMISSION_IO_SPP, (const uint8_t *)response,  \
                                response_len, 0);
    os_mem_free(response);
    if(ret < 0){
        return RET_FAIL;
    }
    return RET_OK;
}

uint8_t cli_interface_msg_response(uint32_t module_id, uint32_t msg_id,
                                   uint8_t *buffer, uint32_t buffer_len,
                                   uint16_t sn, uint8_t result)
{
    UNUSED(sn);
    return cli_interface_msg_send(module_id, msg_id, buffer, buffer_len, result, CLI_TYPE_RES);
}

uint8_t cli_interface_msg_ind(uint32_t module_id, uint32_t msg_id,
                                   uint8_t *buffer, uint32_t buffer_len,
                                   uint16_t sn, uint8_t result)
{
    UNUSED(sn);
    return cli_interface_msg_send(module_id, msg_id, buffer, buffer_len, result, CLI_TYPE_IND);
}

uint16_t cli_interface_get_current_cmd_sn(void)
{
    return g_msg_cmd_sn;
}
