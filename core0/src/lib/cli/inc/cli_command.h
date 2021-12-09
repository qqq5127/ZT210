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

#ifndef LIB_CLI_COMMAND_H
#define LIB_CLI_COMMAND_H

#ifdef __cplusplus
extern "C" {
#endif

#define CLI_COMMAND_MAGIC 0x54616E67

/*lint -esym(714, *_cmd) symbol 'command' not referenced */
#define CLI_ADD_COMMAND(module_id, msg_id, command)      \
    __attribute__((section(".cli_cmd." #command), used)) \
        const cli_command_t command##_cmd = {            \
            .magic = CLI_COMMAND_MAGIC,                  \
            .module_id_num = module_id,                  \
            .msg_id_num = msg_id,                        \
            .command_func = command,                     \
    }

typedef enum {
    CLI_MODULEID_DEBUGLOG,
    CLI_MODULEID_COMMUNICATION,
    CLI_MODULEID_HOSTINTERFACE,
    CLI_MODULEID_FTM,
    CLI_MODULEID_COMMON,
    CLI_MODULEID_LOWPOWER,
    CLI_MODULEID_APPLICATION,
    /** Product Specific Module ID Start */
    CLI_MODULEID_BT_APPL,
    CLI_MODULEID_AUDIO,
    /** Product Specific Module ID End */

    CLI_MODULEID_DRIVER_COMMON = 0x100,
    CLI_MODULEID_DRIVER_CHARGER = 0x101,
    CLI_MODULEID_DRIVER_LED = 0x102,

    CLI_MODULEID_MAX_NUM,
} CLI_MODULEID;

typedef void (*cli_command)(uint8_t *buffer, uint32_t length);

typedef struct cli_command {
    uint32_t magic;
    uint16_t module_id_num;
    uint16_t msg_id_num;
    cli_command command_func;
} cli_command_t;

/**
 * @brief This function is used to register cli command.
 *
 * @param module_id for register command's moudle id.
 * @param msg_id for register command's message id.
 * @param msg for register command's message.
 * @param msg_len for register command's message len.
 */
void cli_command_invoke(uint16_t module_id, uint16_t msg_id, uint8_t *msg,
                        uint32_t msg_len);

#ifdef __cplusplus
}
#endif

#endif /* LIB_CLI_COMMAND_H */
