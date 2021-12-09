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

#ifndef LIB_CLI_CHARGER_H
#define LIB_CLI_CHARGER_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Disable charger pin vbus GPIO mode.
 *
 * @param buffer is required by function.
 * @param bufferlen required by function.
 */
void cli_charger_disable_gpio_mode(uint8_t *buffer, uint32_t bufferlen);

/**
 * @brief Set charger currnet.
 *
 * @param buffer is required by function.
 * @param bufferlen required by function.
 */
void cli_charger_set_current(uint8_t *buffer, uint32_t bufferlen);

/**
 * @brief read all config of the charger current.
 *
 * @param buffer is required by function.
 * @param bufferlen required by function.
 */
void cli_charger_get_config(uint8_t *buffer, uint32_t bufferlen);

#ifdef __cplusplus
}
#endif

#endif /* LIB_CLI_CHARGER_H */
