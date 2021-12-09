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

#ifndef _CHARGER_BOX_H__
#define _CHARGER_BOX_H__
/**
 * @addtogroup LIB
 * @{
 */

/**
 * @addtogroup CHARGER_BOX
 * @{
 * This section introduces the LIB CHARGER_BOX module's enum, structure, functions and how to use this module.
 */

#include "types.h"

/** @defgroup lib_charger_box_enum Enum
 * @{
 */

typedef enum {
    CHARGER_EVT_POWER_ON,
    CHARGER_EVT_POWER_OFF,
    CHARGER_EVT_REBOOT,
    CHARGER_EVT_BT_DISABLE,
    CHARGER_EVT_TAKE_OUT,
    CHARGER_EVT_PUT_IN,
    CHARGER_EVT_BOX_OPEN,
    CHARGER_EVT_BOX_CLOSE,
    CHARGER_EVT_ENTER_DUT,
    CHARGER_EVT_ENTER_OTA,
    CHARGER_EVT_RESET,
    CHARGER_EVT_RESET_KEEP_WWS,
    CHARGER_EVT_CLEAR_PDL,
    CHARGER_EVT_AG_PAIR,
    CHARGER_EVT_TWS_PAIR,
    CHARGER_EVT_TWS_MAGIC,
    CHARGER_EVT_POPUP_TOGGLE,
    CHARGER_EVT_BOX_BATTERY,
    CHARGER_EVT_ENTER_MONO_MODE,
} charger_evt_t;

/**
 * @}
 */

/** @defgroup lib_charger_box_struct Struct
 * @{
 */
/**
 * @brief paramater of CHARGER_EVT_BOX_BATTERY
 */
typedef struct {
    uint8_t battery : 7;
    uint8_t charging : 1;
} box_battery_param_t;
/**
 * @}
 */

/**
 * @brief callback to handle charger
 * @param evt charger event
 * @param param event parameters
 * @param param_len length of parameters
 */
typedef void (*charger_callback_t)(charger_evt_t evt, void *param, uint32_t param_len);

/**
 * @brief init the charger box module
 * @param callback handler to handle chager events
 */
void charger_box_init(charger_callback_t callback);

/**
 * @brief deinit the charger box module
 */
void charger_box_deinit(void);

/**
 * @}
 * addtogroup CHARGER_BOX
 */

/**
 * @}
 * addtogroup LIB
 */

#endif   //_CHARGER_BOX_H__
