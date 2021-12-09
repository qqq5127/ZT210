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

#ifndef LIB_CLI_COMMON_MEMORY_H
#define LIB_CLI_COMMON_MEMORY_H

#ifdef __cplusplus
extern "C" {
#endif

enum CLI_DATA_OPERATION_TYPE {
    CLI_DATA_OPERATION_TYPE_REG,
    CLI_DATA_OPERATION_TYPE_RAM,
    CLI_DATA_OPERATION_TYPE_FLASH,
    CLI_DATA_OPERATION_TYPE_REG_GROUP,
};

enum CLI_CRC_CALCULATION_TYPE {
    CLI_CRC_CALC_8BIT,
    CLI_CRC_CALC_16BIT,
    CLI_CRC_CALC_16BIT_CCIT,
    CLI_CRC_CALC_32BIT_H16,
    CLI_CRC_CALC_32BIT,
};

/**
 * @brief This function is used to read data from the common memory.
 *
 * @param buffer is required by function.
 * @param length is required by function.
 */
void cli_common_memory_read_data(uint8_t *buffer, uint32_t length);

/**
 * @brief This function is used to write data from the common memory.
 *
 * @param buffer is required by function.
 * @param bufferlen is required by function.
 */
void cli_common_memory_write_data(uint8_t *buffer, uint32_t bufferlen);
/**
 * @brief This function is used to calculate the crc value of the the data.
 *
 * @param buffer is memory address where the data stored in.
 * @param length is length of the data in byte.
 */
void cli_common_memory_calculate_crc(uint8_t *buffer, uint32_t length);

/**
 * @brief write ppm to register.
 *
 * @param buffer is cli command payload.
 * @param bufferlen is cli command payload length.
 */
void cli_common_memory_set_cap_code(uint8_t *buffer, uint32_t bufferlen);

#ifdef __cplusplus
}
#endif

#endif /* LIB_CLI_COMMON_MEMORY_H */
