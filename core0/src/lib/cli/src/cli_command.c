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
#include "riscv_cpu.h"
#include "crc.h"

#include "dbglog.h"
#include "lib_dbglog.h"
#include "cli.h"
#include "cli_command.h"

extern uint32_t _cli_cmd_start;
extern uint32_t _cli_cmd_end;

void cli_command_invoke(uint16_t module_id, uint16_t msg_id, uint8_t *msg,
                        uint32_t msg_len)
{
    cli_command_t *cmd = NULL;

    for (cmd = (cli_command_t *)(&_cli_cmd_start);
         (uint32_t)cmd < (uint32_t)(&_cli_cmd_end); cmd++) {
        if (cmd->magic == CLI_COMMAND_MAGIC && cmd->module_id_num == module_id
            && cmd->msg_id_num == msg_id) {
            if (cmd->command_func) {
                cmd->command_func(msg, msg_len);
                return;
            }
        }
    }

    DBGLOG_LIB_CLI_INFO("invalid command, module id:%d, msg id:%d\n", module_id, msg_id);
}
