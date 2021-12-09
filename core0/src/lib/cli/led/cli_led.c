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

#include "lib_dbglog.h"
#include "dbglog.h"
#include "iot_gpio.h"
#include "iot_ledc.h"

#include "iot_resource.h"
#include "led_manager.h"
#include "cli.h"
#include "cli_led_definition.h"
#include "cli_led.h"

void cli_led_operate(uint8_t *buffer, uint32_t bufferlen)
{
    UNUSED(bufferlen);
    cli_cmd_led_data_t *cmd = (cli_cmd_led_data_t *)buffer;

    if (cmd->op_type == LED_OPERATE_TYPE_OFF) {
        led_off(cmd->led_id);
        iot_gpio_write(cmd->gpio, 0);
        iot_gpio_set_pull_mode(cmd->gpio, IOT_GPIO_PULL_DOWN);
    }

    cli_interface_msg_response(CLI_MODULEID_DRIVER_LED, CLI_MSGID_LED_OPERATE, NULL, 0, 0, RET_OK);
}

CLI_ADD_COMMAND(CLI_MODULEID_DRIVER_LED, CLI_MSGID_LED_OPERATE, cli_led_operate);
