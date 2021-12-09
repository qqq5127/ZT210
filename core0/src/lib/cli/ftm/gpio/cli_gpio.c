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

#include "lib_dbglog.h"
#include "cli.h"
#include "cli_ftm_definition.h"
#include "cli_gpio.h"
#include "iot_gpio.h"

#pragma pack(push)  /* save the pack status */
#pragma pack(1)     /* 1 byte align */

typedef enum {
    CLI_GPIO_IDLE = 0,
    CLI_GPIO_INPUT,
    CLI_GPIO_OUPUT,
} cli_gpio_set_t;

typedef struct _cli_gpio_set_cmd {
    uint32_t gpio;
    cli_gpio_set_t func;
    uint32_t val;
} cli_gpio_set_cmd_t;

typedef struct _cli_gpio_ack {
    uint16_t result;
    uint8_t reserved[8];
} cli_gpio_ack;

#pragma pack(pop)   /* restore the pack status */

void cli_ftm_gpio_handler(uint8_t *buffer, uint32_t bufferlen)
{
    (void)bufferlen;
    uint8_t gpio_num, gpio_val;
    cli_gpio_set_cmd_t *gpio_set = (cli_gpio_set_cmd_t *)buffer;
    cli_gpio_ack ack = {0};

    gpio_num = gpio_set->gpio;
    gpio_val = (gpio_set->val == 0)? 0: 1;

    /* set gpio */
    if (gpio_set->func == CLI_GPIO_OUPUT) {
        iot_gpio_open(gpio_num, IOT_GPIO_DIRECTION_OUTPUT);
        iot_gpio_write(gpio_num, gpio_val);
        DBGLOG_LIB_CLI_INFO("cli ouput gpio:%d, val:%d\n", gpio_num, gpio_val);

    } else if (gpio_set->func == CLI_GPIO_INPUT) {
        iot_gpio_open(gpio_num, IOT_GPIO_DIRECTION_INPUT);
        ack.reserved[0] = iot_gpio_read(gpio_num);
        DBGLOG_LIB_CLI_INFO("cli input gpio:%d, val:%d\n", gpio_num, ack.reserved[0]);
    }

    ack.result = 0;
    cli_interface_msg_response(CLI_MODULEID_COMMON, CLI_MSGID_FTM_COMMON_GPIO_SET,
                               (uint8_t *)&ack, sizeof(ack), 0, RET_OK);
}

CLI_ADD_COMMAND(CLI_MODULEID_FTM, CLI_MSGID_FTM_COMMON_GPIO_SET,
                cli_ftm_gpio_handler);
