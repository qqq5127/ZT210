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

#ifndef _LIB_UTILS_MODULES_H
#define _LIB_UTILS_MODULES_H

/**
 * @addtogroup UTILS
 * @{
 * @addtogroup MODULES
 * @{
 */

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* define module id for each module
 * the id range is assigned as below:
 * 0    -   128     base module
 * 128  -   160     stack module
 * 160  -   192     phy module
 * 192  -   224     app module
 */
enum MODULE_ID_E {
    IOT_BASIC_MID_START = 0,
    UNKNOWN_MID = IOT_BASIC_MID_START,
    OS_TIMER_MID,
    OS_LOCK_MID,
    OS_UTILS_MID,
    IOT_OS_SHIM_MID,
    IOT_RPC_MID,
    IOT_VAD_MID,
    IOT_DBGLOG_MID,
    IOT_CLI_MID,
    IOT_DRIVER_MID,
    IOT_SHARETASK_MID,
    IOT_OTA_MID,
    IOT_LOGGER_MID,
    IOT_DATAPATH_MID,
    IOT_STREAM_MID,
    RECORD_MID,
    STORAGE_MID,
    ADC_MID,
    PLAYER_MID,
    IOT_BATTERY_MID,
    IOT_GENERIC_TRANSMISSION_MID,
    IOT_MIC_ADC_SST_MID,
    IOT_MIC_ADC_VCM_SET_MID,
    IOT_KEY_VALUE_MID,
    IOT_BASIC_MID_END = 0x5F,

    LIB_MID_START = 0x60,
    LIB_KEYMGMT_MID = LIB_MID_START,
    LIB_MICDUMP_MID,
    LIB_AUDIO_MID,
    LIB_ANC_MID,
    LIB_CPU_USAGE_MID,
    LIB_POWER_MGNT_MID,
    LIB_CODEC_MID,
    LIB_LOADER_MID,
    LIB_SUSPEND_SCHD_MID,
    LIB_EQ_MID,
    LIB_MID_END = 0x7F,

    BT_STACK_MID_START = 0x80,
    BT_STACK_CONTROLLER_MID = BT_STACK_MID_START,
    BT_STACK_HOST_MID,
    BT_STACK_APP_MID,
    BT_STACK_TDS_MID,
    BT_STACK_MID_END = 0xBF,

    BT_PHY_MID_START = 0xC0,
    BT_PHY_MID = BT_PHY_MID_START,
    BT_PHY_STATUS_MID,
    BT_PHY_MID_END = 0xDF,

    IOT_APP_MID_START = 0xE0,
    IOT_APP_MID = IOT_APP_MID_START,
    IOT_APP_BATTERY_CASE_MID,
    IOT_SENSOR_HUB_MANAGER_MID,
    IOT_LED_MANAGER_MID,
    IOT_TONE_MANAGER_MID,
    IOT_VENDOR_MESSAGE_MID,
    IOT_APP_DEMO_MID,
    IOT_APP_MID_END = 0xFF,

    MAX_MID_NUM = 0xFF,
};

typedef uint16_t module_id_t;

#ifdef __cplusplus
}
#endif


/**
 * @}
 * addtogroup MODULES
 * @}
 * addtogroup UTILS
 */

#endif   // _LIB_UTILS_MODULES_H
