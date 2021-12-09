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

#ifndef LIB_UTILS_CRC_H
#define LIB_UTILS_CRC_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief This function is to get the crc32 update object.
 *
 * @param init_vect is value to clear certain positions
 * @param buffer is a buffer that requires A CRC check
 * @param len is buffer len
 * @return uint32_t return 32 bits unsigned CRC32 value
 */
uint32_t getcrc32_update(uint32_t init_vect, const uint8_t *buffer, uint32_t len);

/**
 * @brief This function is to get crc32 value.
 *
 * @param buffer is a buffer that requires A CRC check
 * @param len is buffer len
 * @return uint32_t return 32 bits unsigned CRC32 check value
 */
uint32_t getcrc32(const uint8_t *buffer, uint32_t len);

/**
 * @brief This function is to get the crc16 update object
 *
 * @param init_vect is value to clear certain positions
 * @param buffer is a buffer that requires A CRC check
 * @param len is buffer len
 * @return uint16_t 16 bits unsigned CRC32 check value
 */
uint16_t getcrc16_update(uint16_t init_vect, const uint8_t *buffer, uint32_t len);

/**
 * @brief This function is to get the crc32 h16 object
 *
 * @param buffer is a buffer that requires A CRC check
 * @param len is buffer len
 * @return uint16_t 16 bits unsigned CRC32 check value
 */
uint16_t getcrc32_h16(const uint8_t *buffer, uint32_t len);

/**
 * @brief This function is to get crc16 value.
 *
 * @param buffer is a buffer that requires A CRC check
 * @param len is buffer len
 * @return uint16_t 16 bits unsigned CRC16 check value
 */
uint16_t getcrc16(const uint8_t *buffer, uint32_t len);

/**
 * @brief This function is to get the crc16 ccitt update object
 *
 * @param init_vect is value to clear certain positions
 * @param buffer is a buffer that requires A CRC check
 * @param len is buffer len
 * @return uint16_t 16 bits unsigned CRC16 ccitt check value
 */
uint16_t getcrc16_ccitt_update(uint16_t init_vect, const uint8_t *buffer,
                               uint32_t len);

/**
 * @brief This function is to Get the crc16 ccitt object
 *
 * @param buffer is a buffer that requires A CRC check
 * @param len is buffer len
 * @return uint16_t 16 bits unsigned CRC16 ccitt check value
 */
uint16_t getcrc16_ccitt(const uint8_t *buffer, uint32_t len);

/**
 * @brief This function is to Get the crc8 update object
 *
 * @param init_vect is value to clear certain positions
 * @param buffer is a buffer that requires A CRC check
 * @param len is buffer len
 * @return uint8_t 8 bits unsigned CRC8 check value
 */
uint8_t getcrc8_update(uint8_t init_vect, const uint8_t *buffer, uint32_t len);

/**
 * @brief This function is to get crc8 value.
 *
 * @param buffer is a buffer that requires A CRC check
 * @param len is buffer len
 * @return uint8_t 8 bits unsigned CRC8 check value
 */
uint8_t getcrc8(const uint8_t *buffer, uint32_t len);

#ifdef __cplusplus
}
#endif

#endif /* LIB_UTILS_CRC_H */
