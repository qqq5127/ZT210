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
#include "cli_ftm_definition.h"
#include "cli_touch_key.h"


typedef struct _cli_vref_trim_param {
    uint16_t vref_value;
} __attribute__((packed)) cli_touch_key_vref_trim_param;

/* set data ack */
typedef struct cli_vref_trim_ack {
    uint16_t result;
    uint8_t reversed[8];
} __attribute__((packed)) cli_touch_key_vref_trim_ack;

void cli_touch_key_vref_trim_handler(uint8_t *buffer, uint32_t bufferlen)
{
    (void)bufferlen;
    cli_touch_key_vref_trim_param *param_info = NULL;
    cli_touch_key_vref_trim_ack ack = {0};

    param_info = (cli_touch_key_vref_trim_param *)buffer;
    DBGLOG_LIB_CLI_INFO("cli vref trim voltage %x %x\n", param_info->vref_value);

    ack.result = 0;
    ack.reversed[0] = 0xff;
    ack.reversed[2] = 0xff;
    ack.reversed[4] = 0xff;
    ack.reversed[6] = 0xff;

    cli_interface_msg_response(
        CLI_MODULEID_FTM, CLI_MSGID_FTM_TOUCH_KEY_VREF_TRIM, (uint8_t *)&ack,
        sizeof(cli_touch_key_vref_trim_ack), 0, RET_OK);
}

CLI_ADD_COMMAND(CLI_MODULEID_FTM, CLI_MSGID_FTM_TOUCH_KEY_VREF_TRIM,
                cli_touch_key_vref_trim_handler);
