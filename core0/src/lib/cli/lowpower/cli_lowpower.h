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

#ifndef CLI_LOWPOWER_H
#define CLI_LOWPOWER_H

#include "types.h"

enum {
    CLI_LP_WAKEUP_WIC = 0,
    CLI_LP_WAKEUP_RTC,
    CLI_LP_WAKEUP_GPIO,
};

/**
 * @brief This function is used to enter light sleep for cli lowpower.
 *
 * @param buffer UNUSED.
 * @param bufferlen UNUSED.
 */
void cli_lowpower_enter_light_sleep(uint8_t *buffer, uint32_t bufferlen);

/**
 * @brief This function is used to enter deep sleep for cli lowpower.
 *
 * @param buffer UNUSED.
 * @param bufferlen UNUSED.
 */
void cli_lowpower_enter_deep_sleep(uint8_t *buffer, uint32_t bufferlen);

/**
 * @brief This function is used to set wake up for cli lowpower.
 *
 * @param buffer UNUSED.
 * @param bufferlen UNUSED.
 */
void cli_lowpower_set_wakeup_source(uint8_t *buffer, uint32_t bufferlen);

/**
 * @brief This function is used to enter shutdown for cli lowpower.
 *
 * @param buffer UNUSED.
 * @param bufferlen UNUSED.
 */
void cli_lowpower_enter_shutdown(uint8_t *buffer, uint32_t bufferlen);

/**
 * @brief Set min idle time for os that system could enter sleep mode.
 *
 * @param buffer UNUSED.
 * @param bufferlen UNUSED.
 */
void cli_lowpower_set_sleep_thr(uint8_t *buffer, uint32_t bufferlen);

/**
 * @brief Disable ldo and dcdc compatible mode.
 *
 * @param buffer UNUSED.
 * @param bufferlen UNUSED.
 */
void cli_lowpower_disable_voltage_compatible(uint8_t *buffer, uint32_t bufferlen);

#endif
