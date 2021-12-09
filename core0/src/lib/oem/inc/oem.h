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
#ifndef LIB_OEM_DATA_H
#define LIB_OEM_DATA_H

/**
 * @addtogroup LIB
 * @{
 */

/**
 * @addtogroup OEM
 * @{
 * This section introduces the caldata module's enum, structure, functions and how to use this driver.
 */

#include "iot_memory_origin.h"

#ifdef __cplusplus
extern "C" {
#endif

#define OEM_DATA_MAGIC   0x57714F4D   //WqOM
#define OEM_DATA_VERSION 0x01000000   //1.0.0.0

#define OEM_DATA_TOTAL_SIZE 4096

#define OEM_DATA_BASE_SIZE   256
#define OEM_DATA_BASE_OFFSET FLASH_OEM_DATA_OFFSET

#define OEM_DATA_AUDMAP_SIZE   256
#define OEM_DATA_AUDMAP_OFFSET (OEM_DATA_BASE_OFFSET + OEM_DATA_BASE_SIZE)

#define OEM_DATA_IOMAP_SIZE   512
#define OEM_DATA_IOMAP_OFFSET (OEM_DATA_AUDMAP_OFFSET + OEM_DATA_AUDMAP_SIZE)

#define OEM_DATA_EQ_SIZE   512
#define OEM_DATA_EQ_OFFSET (OEM_DATA_IOMAP_OFFSET + OEM_DATA_IOMAP_SIZE)

#define OEM_DATA_BASE_HDR_SIZE  16
#define OEM_DATA_BASE_DATA_SIZE (OEM_DATA_BASE_SIZE - OEM_DATA_BASE_HDR_SIZE)

#define OEM_DATA_AUDMAP_HDR_SIZE  16
#define OEM_DATA_AUDMAP_DATA_SIZE (OEM_DATA_AUDMAP_SIZE - OEM_DATA_AUDMAP_HDR_SIZE)

#define OEM_DATA_IOMAP_HDR_SIZE  16
#define OEM_DATA_IOMAP_DATA_SIZE (OEM_DATA_IOMAP_SIZE - OEM_DATA_IOMAP_HDR_SIZE)

#define OEM_DATA_EQ_HDR_SZIE  16
#define OEM_DATA_EQ_DATA_SIZE (OEM_DATA_EQ_SIZE - OEM_DATA_EQ_HDR_SZIE)

#if ((OEM_DATA_BASE_SIZE + OEM_DATA_AUDMAP_SIZE + OEM_DATA_IOMAP_SIZE + OEM_DATA_EQ_SIZE) \
     > OEM_DATA_TOTAL_SIZE)
#error "oem size exceeds the limit, please check size!"
#endif

#define OEM_DATA_ANC_SIZE   8192
#define OEM_DATA_ANC_OFFSET FLASH_ANC_DATA_OFFSET

#define OEM_DATA_ANC_HDR_SZIE  16
#define OEM_DATA_ANC_DATA_SIZE (OEM_DATA_ANC_SIZE - OEM_DATA_ANC_HDR_SZIE)

#define MAC_ADDR_LEN 6

/** @defgroup lib_oem_data_enum Enum
 * @{
 */

/**
 * @}
 */

/** @defgroup lib_oem_data_typedef Typedef
 * @{
 */

/**
 * @}
 */

/** @defgroup lib_oem_data_struct Struct
 * @{
 */
typedef struct _oem_data_hdr_t {
    /** version of anc data struct */
    uint32_t version;
    /** data struct magic */
    uint32_t magic;
    /** crc for all data */
    uint32_t crc;
    /** length of total struct*/
    uint32_t length;
} oem_data_hdr_t;

typedef struct _oem_data_base_t {
    oem_data_hdr_t header;
    uint8_t data[OEM_DATA_BASE_DATA_SIZE];
} oem_data_base_t;

typedef struct _oem_data_iomap_t {
    oem_data_hdr_t header;
    uint8_t data[OEM_DATA_IOMAP_DATA_SIZE];
} oem_data_iomap_t;

typedef struct _oem_data_audmap_t {
    oem_data_hdr_t header;
    uint8_t data[OEM_DATA_AUDMAP_DATA_SIZE];
} oem_data_audmap_t;

typedef struct _oem_data_eq_t {
    /** EQ data header */
    oem_data_hdr_t header;
    /** EQ data */
    uint8_t data[OEM_DATA_EQ_DATA_SIZE];
} oem_data_eq_t;

typedef struct _oem_data_t {
    /** base data */
    oem_data_base_t base;
    /** audio map data*/
    oem_data_audmap_t audmap;
    /** io map data*/
    oem_data_iomap_t iomap;
    /** EQ data */
    oem_data_eq_t eq;
} oem_data_t;

typedef struct _oem_data_base_data_t {
    uint8_t ppm;
    uint8_t mac[MAC_ADDR_LEN];
    uint32_t quality;
} oem_data_base_data_t;

typedef struct _oem_data_anc_t {
    /** ANC data header */
    oem_data_hdr_t header;
    /** ANC data */
    uint8_t data[OEM_DATA_ANC_DATA_SIZE];
} oem_data_anc_t;

/**
  * @}
  */

/**
 * @brief This function is get oem data pointer
 *
 * @return oem data pointer
 */
oem_data_t *oem_data_load(void);

/**
 * @brief This function is get anc data pointer
 *
 * @return anc data pointer
 */
oem_data_anc_t *anc_data_load(void);

/**
 * @brief This function is to load base data
 *
 * @param [out] data is data pointer
 * @param [out] len  is data len
 * @return uint8_t RET_OK or RET_INVAL, RET_FAIL for success else error.
 */
uint8_t oem_data_base_load(uint8_t **data, uint32_t *len);

/**
 * @brief This function is to load audio map data
 *
 * @param data is data pointer
 * @param len  is data len
 * @return uint8_t RET_OK or RET_INVAL, RET_FAIL for success else error.
 */
uint8_t oem_data_audmap_load(uint8_t **data, uint32_t *len);

/**
 * @brief This function is to load io map data
 *
 * @param [out] data is data pointer
 * @param [out] len  is data len
 * @return uint8_t RET_OK or RET_INVAL, RET_FAIL for success else error.
 */
uint8_t oem_data_iomap_load(uint8_t **data, uint32_t *len);

/**
 * @brief This function is to load ANC data
 *
 * @param [out] data is data pointer
 * @param [out] len  is data len
 * @return uint8_t RET_OK or RET_INVAL, RET_FAIL for success else error.
 */
uint8_t oem_data_anc_load(uint8_t **data, uint32_t *len);

/**
 * @brief This function is to load EQ data
 *
 * @param [out] data is data pointer
 * @param [out] len  is data len
 * @return uint8_t RET_OK or RET_INVAL, RET_FAIL for success else error.
 */
uint8_t oem_data_eq_load(uint8_t **data, uint32_t *len);

/**
 * @brief This function is to get PPM value.
 *
 * @param [out] ppm is the ppm value
 * @return uint8_t RET_OK or RET_INVAL, RET_FAIL for success else error.
 */
uint8_t oem_data_get_ppm(uint8_t *ppm);

/**
 * @brief This function is  to get MAC address
 *
 * @param [out] mac is MAC address.
 * @return uint8_t RET_OK or RET_INVAL, RET_FAIL for success else error.
 */
uint8_t oem_data_get_mac_addr(uint8_t *mac);

#ifdef __cplusplus
}
#endif

/**
 * @}
 * addtogroup OEM
 */

/**
* @}
 * addtogroup LIB
*/

#endif   //LIB_OEM_DATA_H
