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

#ifndef LIB_CLI_LOWPOWER_DEFINITION_H
#define LIB_CLI_LOWPOWER_DEFINITION_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    // low power commands
    CLI_MSGID_LOW_POWER_ENTER_LIGHT_SLEEP,
    CLI_MSGID_LOW_POWER_ENTER_DEEP_SLEEP,
    CLI_MSGID_LOW_POWER_SET_WAKEUP_SRC,
    CLI_MSGID_LOW_POWER_ENTER_SHUTDOWN,
    CLI_MSGID_LOW_POWER_SET_SLEEP_THR,
    CLI_MSGID_LOW_POWER_DISABLE_VOLTAGE_COMPATIBLE,

    CLI_MSGID_LOW_POWER_MAX_NUM,
} CLI_AUDIO_MSGID;

#ifdef __cplusplus
}
#endif

#endif /* LIB_CLI_LOWPOWER_DEFINITION_H */
