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

#ifndef LIB_CLI_LED_H
#define LIB_CLI_LED_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    LED_OPERATE_TYPE_OFF,
    LED_OPERATE_TYPE_ON,
} LED_OPERATE_TYPE;

typedef struct cli_cmd_led_data {
    uint8_t led_id;
    uint8_t gpio;
    LED_OPERATE_TYPE op_type;
} __attribute__((packed)) cli_cmd_led_data_t;

/**
 * @brief control led
 *
 * @param buffer is required by function.
 * @param bufferlen required by function.
 */
void cli_led_operate(uint8_t *buffer, uint32_t bufferlen);

#ifdef __cplusplus
}
#endif

#endif /* LIB_CLI_LED_H */
