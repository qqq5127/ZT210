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

#ifndef LIB_CLI_TOUCH_KEY_DEFINITION_H
#define LIB_CLI_TOUCH_KEY_DEFINITION_H

#ifdef __cplusplus
extern "C" {
#endif
#define CLI_MSGID_FTM_MADC_BASE_ADDR       3000
#define CLI_MSGID_FTM_COMMON_BASE_ADDR     5000
#define CLI_MSGID_FTM_TOUCH_KEY_BASE_ADDR  6000

typedef enum {
    // common
    CLI_MSGID_FTM_COMMON_EFUSE_WT = CLI_MSGID_FTM_COMMON_BASE_ADDR,
    CLI_MSGID_FTM_COMMON_GPIO_SET,

    /*MADC*/
    CLI_MSGID_FTM_MADC_START = CLI_MSGID_FTM_MADC_BASE_ADDR,
    CLI_MSGID_FTM_MADC_STOP,
    CLI_MSGID_FTM_MADC_INIT,
    CLI_MSGID_FTM_MADC_CHANNEL_SELECT,
    CLI_MSGID_FTM_MADC_POLL_DATA,
    CLI_MSGID_FTM_MADC_DUMP,
    CLI_MSGID_FTM_SET_AUDIO_GAIN,

    // tk commands
    CLI_MSGID_FTM_TOUCH_KEY_VREF_TRIM = CLI_MSGID_FTM_TOUCH_KEY_BASE_ADDR,

    CLI_MSGID_FTM_MAX_NUM,
} CLI_FTM_MSGID;

#ifdef __cplusplus
}
#endif

#endif /* LIB_CLI_TOUCH_KEY_DEFINITION_H */
