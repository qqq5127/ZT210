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

#ifndef LIB_CLI_COMMON_BASIC_H
#define LIB_CLI_COMMON_BASIC_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief get software version
 *
 * @param buffer UNUSED.
 * @param bufferlen UNUSED.
 */
void cli_common_basic_get_fw_ver(uint8_t *buffer, uint32_t bufferlen);

/**
 * @brief get ate version
 *
 * @param buffer UNUSED.
 * @param bufferlen UNUSED.
 */
void cli_common_basic_get_ate_ver(uint8_t *buffer, uint32_t bufferlen);

/**
 * @brief dump all cal data
 *
 * @param buffer UNUSED.
 * @param bufferlen UNUSED.
 */
void cli_common_basic_get_cal_data(uint8_t *buffer, uint32_t bufferlen);

/**
 * @brief dump all oem data
 *
 * @param buffer UNUSED.
 * @param bufferlen UNUSED.
 */
void cli_common_basic_get_oem_data(uint8_t *buffer, uint32_t bufferlen);

/**
 * @brief dump all anc data
 *
 * @param buffer UNUSED.
 * @param bufferlen UNUSED.
 */
void cli_common_basic_get_anc_data(uint8_t *buffer, uint32_t bufferlen);

/**
 * @brief This function is used to get basic common chip info.
 *
 * @param buffer is required by function.
 * @param bufferlen required by function.
 */
void cli_common_basic_get_chip_info(uint8_t *buffer, uint32_t bufferlen);

/**
 * @brief This function is used to making software chip reset.
 *
 * @param buffer is required by function.
 * @param bufferlen required by function.
 */
void cli_common_basic_reset(uint8_t *buffer, uint32_t bufferlen);

/**
 * @brief This function is used to adjust the clock mode of chip.
 *
 * @param buffer is required by function.
 * @param bufferlen required by function.
 */
void cli_common_basic_set_clock_mode(uint8_t *buffer, uint32_t bufferlen);

/**
 * @brief Set output log level
 *
 * @param buffer Cli command payload
 * @param bufferlen Cli command payload length
 */
void cli_common_basic_set_log_level(uint8_t *buffer, uint32_t bufferlen);

/**
 * @brief Get rom version
 *
 * @param buffer Cli command payload
 * @param bufferlen Cli command payload length
 */
void cli_common_basic_get_romlib_ver(uint8_t *buffer, uint32_t bufferlen);

/**
 * @brief Run ROM crc check test.
 *
 * @param buffer Cli command payload
 * @param bufferlen Cli command payload length
 */
void cli_common_basic_rom_crc_check(uint8_t *buffer, uint32_t bufferlen);

#ifdef __cplusplus
}
#endif

#endif /* LIB_CLI_COMMON_BASIC_H */
