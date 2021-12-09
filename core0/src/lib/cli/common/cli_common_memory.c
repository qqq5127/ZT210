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
#include "os_mem.h"

#include "lib_dbglog.h"
#include "cli.h"
#include "cli_common_definition.h"
#include "cli_common_memory.h"
#include "iot_soc.h"
#include "crc.h"
#define CLI_DATA_OPERATION_MAX_SIZE 256

typedef struct cli_cmd_read_data {
    uint32_t addr;
    uint16_t length;
    uint8_t operation_type;
} __attribute__((packed)) cli_cmd_read_data_t;

typedef struct _cli_data_info {
    uint32_t addr;
    uint8_t operation_type;
    uint16_t len;
    uint8_t data[];
} __attribute__((packed)) cli_data_info;

/* data info */
typedef struct _cli_reg_info {
    uint32_t addr;
    uint8_t operation_type;
    uint32_t value;
} __attribute__((packed)) cli_reg_info;

/* set data ack */
typedef struct _cli_set_data_ack {
    uint32_t addr;
    uint16_t len;
    uint8_t operation_type;
    uint8_t result;
} __attribute__((packed)) cli_set_data_ack;

typedef struct cli_cmd_clac_crc {
    uint32_t addr;
    uint32_t length;
    uint8_t crc_type;
} __attribute__((packed)) cli_cmd_clac_crc_t;

typedef struct _cli_clac_crc_ack {
    uint32_t crc_value;
    uint32_t addr;
    uint32_t len;
    uint8_t crc_type;
} __attribute__((packed)) cli_clac_crc_ack;

void cli_common_memory_read_data(uint8_t *buffer, uint32_t length)
{
    cli_cmd_read_data_t *cmd = (cli_cmd_read_data_t *)buffer;
    UNUSED(length);
    DBGLOG_LIB_CLI_INFO("cmd->addr:%x cmd->operation_type:%x cmd->len:%x", cmd->addr,
                        cmd->operation_type, cmd->length);
    if (cmd->length > CLI_DATA_OPERATION_MAX_SIZE) {
        cmd->length = CLI_DATA_OPERATION_MAX_SIZE;
    }

    if (cmd->operation_type == CLI_DATA_OPERATION_TYPE_REG) {
        cli_reg_info reg_info;
        reg_info.addr = cmd->addr;
        reg_info.operation_type = cmd->operation_type;

        if (cmd->addr & 0x3) {
            reg_info.value = 0;
            cli_interface_msg_response(CLI_MODULEID_COMMON, CLI_MSGID_DATA_INFO,
                                       (uint8_t *)&reg_info, sizeof(cli_reg_info), 0, RET_INVAL);
            return;
        }

        reg_info.value = iot_soc_register_read(cmd->addr);
        cli_interface_msg_response(CLI_MODULEID_COMMON, CLI_MSGID_DATA_INFO, (uint8_t *)&reg_info,
                                   sizeof(cli_reg_info), 0, RET_OK);

    } else if (cmd->operation_type == CLI_DATA_OPERATION_TYPE_RAM) {
        cli_data_info *data_info = os_mem_malloc(IOT_CLI_MID, sizeof(cli_data_info) + cmd->length);

        if (data_info == NULL) {
            cli_data_info data_info = {0};
            data_info.addr = cmd->addr;
            data_info.len = 0;
            data_info.operation_type = cmd->operation_type;
            cli_interface_msg_response(CLI_MODULEID_COMMON, CLI_MSGID_DATA_INFO,
                                       (uint8_t *)&data_info, sizeof(cli_data_info) + cmd->length,
                                       0, RET_NOMEM);
            return;
        }

        data_info->addr = cmd->addr;
        data_info->len = cmd->length;
        data_info->operation_type = cmd->operation_type;
        memcpy(&data_info->data, (void *)cmd->addr, cmd->length);

        cli_interface_msg_response(CLI_MODULEID_COMMON, CLI_MSGID_DATA_INFO, (uint8_t *)data_info,
                                   sizeof(cli_data_info) + cmd->length, 0, RET_OK);
        os_mem_free(data_info);
    } else if (cmd->operation_type == CLI_DATA_OPERATION_TYPE_REG_GROUP) {
        if (cmd->length == 0 || cmd->length % 4 != 0 || cmd->addr % 4 != 0) {
            cli_data_info data_info = {0};
            data_info.addr = cmd->addr;
            data_info.len = 0;
            data_info.operation_type = cmd->operation_type;
            cli_interface_msg_response(CLI_MODULEID_COMMON, CLI_MSGID_DATA_INFO,
                                       (uint8_t *)&data_info, sizeof(cli_set_data_ack), 0,
                                       RET_INVAL);
            return;
        }

        cli_data_info *data_info = os_mem_malloc(IOT_CLI_MID, sizeof(cli_data_info) + cmd->length);

        if (data_info == NULL) {
            cli_data_info data_info = {0};
            data_info.addr = cmd->addr;
            data_info.len = 0;
            data_info.operation_type = cmd->operation_type;
            cli_interface_msg_response(CLI_MODULEID_COMMON, CLI_MSGID_DATA_INFO,
                                       (uint8_t *)&data_info, sizeof(cli_data_info) + cmd->length,
                                       0, RET_NOMEM);
            return;
        }

        data_info->addr = cmd->addr;
        data_info->len = cmd->length;
        data_info->operation_type = cmd->operation_type;

        for (uint16_t i = 0; i < data_info->len; i += 4) {
            uint32_t v = iot_soc_register_read(data_info->addr + i);
            data_info->data[i + 0] = v & 0xFF;
            data_info->data[i + 1] = (v >> 8) & 0xFF;
            data_info->data[i + 2] = (v >> 16) & 0xFF;
            data_info->data[i + 3] = (v >> 24) & 0xFF;
        }

        cli_interface_msg_response(CLI_MODULEID_COMMON, CLI_MSGID_DATA_INFO, (uint8_t *)data_info,
                                   sizeof(cli_data_info) + cmd->length, 0, RET_OK);
        os_mem_free(data_info);
    }
}

void cli_common_memory_write_data(uint8_t *buffer, uint32_t bufferlen)
{
    cli_data_info *data_info = NULL;
    cli_set_data_ack ack = {0};

    if ((!buffer) || (bufferlen < sizeof(cli_reg_info))) {
        return;
    }

    data_info = (cli_data_info *)buffer;
    DBGLOG_LIB_CLI_INFO("cli set data 0x%08x %x\n", data_info->addr,
                   data_info->operation_type);

    if (data_info->operation_type == CLI_DATA_OPERATION_TYPE_RAM) {
        ack.addr = data_info->addr;
        ack.len = data_info->len;

        if (bufferlen < sizeof(cli_data_info)) {
            ack.result = 0;
            cli_interface_msg_response(CLI_MODULEID_COMMON, CLI_MSGID_DATA_ACK, (uint8_t *)&ack,
                                       sizeof(cli_set_data_ack), 0, RET_INVAL);
            return;
        }

        memcpy((void *)data_info->addr, data_info->data, data_info->len);
        ack.result = 0;
    } else if (data_info->operation_type == CLI_DATA_OPERATION_TYPE_REG) {
        cli_reg_info *reg = (cli_reg_info *)data_info;
        ack.addr = reg->addr;
        ack.len = 4;

        if (reg->addr % 4 != 0) {
            ack.result = RET_INVAL;
            cli_interface_msg_response(CLI_MODULEID_COMMON, CLI_MSGID_DATA_ACK, (uint8_t *)&ack,
                                       sizeof(cli_set_data_ack), 0, RET_INVAL);
            return;
        }
        /*double check to see if the written value correct!
        * The wrritten value may not equal to the value should be written if there is read-only bit field exist in the target register!*/
        uint32_t tempvalue = iot_soc_register_write(reg->addr, reg->value);
        if (tempvalue != reg->value) {
            DBGLOG_LIB_WARNING(
                "cli write register Warning! reg_addr:%x reg_written_value:%x reg_read_value:%x",
                reg->addr, reg->value, tempvalue);
        }
        ack.result = 0;
    } else if (data_info->operation_type == CLI_DATA_OPERATION_TYPE_REG_GROUP) {
        ack.addr = data_info->addr;
        ack.len = 4;

        if (data_info->addr % 4 != 0 || data_info->len == 0 || data_info->len % 4 != 0) {
            ack.result = RET_INVAL;
            cli_interface_msg_response(CLI_MODULEID_COMMON, CLI_MSGID_DATA_ACK, (uint8_t *)&ack,
                                       sizeof(cli_set_data_ack), 0, RET_INVAL);
            return;
        }
        uint32_t reg_data;
        for (uint16_t i = 0; i < data_info->len; i += 4) {
            reg_data = (data_info->data[i + 3] << 24) | (data_info->data[i + 2] << 16)
                | (data_info->data[i + 1] << 8) | (data_info->data[i]);
            /*double check to see if the written value correct!
            * The wrritten value may not equal to the value should be written if there is read-only bit field exist in the target register!*/
            uint32_t tempvalue = iot_soc_register_write(data_info->addr + i, reg_data);
            if (tempvalue != reg_data) {
                DBGLOG_LIB_WARNING(
                    "cli write register group Warning! reg_addr:%x reg_write_value:%x reg_read_value:%x",
                    data_info->addr, reg_data, tempvalue);
            }
        }

        ack.result = 0;
    }

    cli_interface_msg_response(CLI_MODULEID_COMMON, CLI_MSGID_DATA_ACK, (uint8_t *)&ack,
                               sizeof(cli_set_data_ack), 0, RET_OK);
}
void cli_common_memory_calculate_crc(uint8_t *buffer, uint32_t length)
{
    cli_cmd_clac_crc_t *cmd = (cli_cmd_clac_crc_t *)buffer;
    UNUSED(length);
    DBGLOG_LIB_CLI_INFO(
        "[ CLI ] cli received crc calculation cmd: cmd->addr:%x cmd->length:%x cmd->crc_type:%x",
        cmd->addr, cmd->length, cmd->crc_type);

    RET_TYPE ret = RET_OK;
    cli_clac_crc_ack crc_info;
    crc_info.addr = cmd->addr;
    crc_info.len = cmd->length;
    crc_info.crc_type = cmd->crc_type;
    crc_info.crc_value = 0xFFFFFFFF;

    if (cmd->crc_type == CLI_CRC_CALC_8BIT) {
        crc_info.crc_value = getcrc8((uint8_t *)cmd->addr, cmd->length);
    } else if (cmd->crc_type == CLI_CRC_CALC_16BIT) {
        crc_info.crc_value = getcrc16((uint8_t *)cmd->addr, cmd->length);
    } else if (cmd->crc_type == CLI_CRC_CALC_16BIT_CCIT) {
        crc_info.crc_value = getcrc16_ccitt((uint8_t *)cmd->addr, cmd->length);
    } else if (cmd->crc_type == CLI_CRC_CALC_32BIT_H16) {
        crc_info.crc_value = getcrc32_h16((uint8_t *)cmd->addr, cmd->length);
    } else if (cmd->crc_type == CLI_CRC_CALC_32BIT) {
        crc_info.crc_value = getcrc32((uint8_t *)cmd->addr, cmd->length);
    } else {
    }

    DBGLOG_LIB_CLI_INFO("[ CLI ] crc value = %x ", crc_info.crc_value);

    cli_interface_msg_response(CLI_MODULEID_COMMON, CLI_MSGID_CALC_CRC, (uint8_t *)&crc_info,
                               sizeof(cli_clac_crc_ack), 0, ret);
}

void cli_common_memory_set_cap_code(uint8_t *buffer, uint32_t bufferlen)
{
    uint8_t cap_code = buffer[0];

    assert(bufferlen == 1);

    iot_soc_set_ppm(cap_code);
    DBGLOG_LIB_CLI_INFO("[ CLI ] set cap code:%d\n", cap_code);

    cli_interface_msg_response(CLI_MODULEID_COMMON, CLI_MSGID_CAP_CODE_SET,
                               NULL, 0, 0, RET_OK);
}

CLI_ADD_COMMAND(CLI_MODULEID_COMMON, CLI_MSGID_READ_DATA, cli_common_memory_read_data);

CLI_ADD_COMMAND(CLI_MODULEID_COMMON, CLI_MSGID_SET_DATA, cli_common_memory_write_data);

CLI_ADD_COMMAND(CLI_MODULEID_COMMON, CLI_MSGID_CALC_CRC, cli_common_memory_calculate_crc);

CLI_ADD_COMMAND(CLI_MODULEID_COMMON, CLI_MSGID_CAP_CODE_SET, cli_common_memory_set_cap_code);
