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

#ifndef LIB_CLI_COMMON_DEFINITION_H
#define LIB_CLI_COMMON_DEFINITION_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    //cli common
    CLI_MSGID_GET_CHIP_INFO,
    CLI_MSGID_CHIP_INFO_RESP,
    CLI_MSGID_INIT_CMD,
    CLI_MSGID_INIT_RESP,
    CLI_MSGID_READ_DATA,
    CLI_MSGID_DATA_INFO,
    CLI_MSGID_SET_DATA,
    CLI_MSGID_DATA_ACK,
    CLI_MSGID_SET_BAUD_RATE,
    CLI_MSGID_SET_BAUD_RATE_ACK,
    CLI_MSGID_FORCE_CRASH,
    CLI_MSGID_UART_TEST,
    CLI_MSGID_UART_TEST_RESP,
    CLI_MSGID_GET_OEM_MISC_INFO,
    CLI_MSGID_OEM_MISC_INFO,

    CLI_MSGID_DATA_UL_IND,
    CLI_MSGID_QUERY_FLASH_DATA_INFO,
    CLI_MSGID_SET_KV,
    CLI_MSGID_GET_KV,

    CLI_MSGID_HOST_LOG,
    CLI_MSGID_HOST_DATA_UL,
    CLI_MSGID_HOST_DATA_DL,

    CLI_MSGID_UART_SET_PIN,
    CLI_MSGID_SOFT_RST,
    CLI_MSGID_SET_CLOCK_MODE,
    CLI_MSGID_SET_LOG_LEVEL,

    CLI_MSGID_OEM_PPM_SET,

    CLI_MSGID_GET_FW_VER,
    CLI_MSGID_GET_ATE_VER,
    CLI_MSGID_GET_CAL_DATA,
    CLI_MSGID_GET_OEM_DATA,
    CLI_MSGID_GET_ANC_DATA,
    CLI_MSGID_GET_ROM_VER,
    CLI_MSGID_CALC_CRC,

    CLI_MSGID_OEM_MAC_SET,
    CLI_MSGID_CAP_CODE_SET,

    CLI_MSGID_COREDUMP_MODE_SET,
    CLI_MSGID_ROM_CRC_CHECK,
    CLI_MSGID_COMMON_MAX_NUM,
} CLI_COMMON_MSGID;

#ifdef __cplusplus
}
#endif

#endif /* LIB_CLI_COMMON_DEFINITION_H */
