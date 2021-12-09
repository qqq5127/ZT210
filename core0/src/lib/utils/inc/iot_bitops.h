/****************************************************************************

Copyright(c) 2016 by WuQi Technologies. ALL RIGHTS RESERVED.

This Information is proprietary to WuQi Technologies and MAY NOT
be copied by any method or incorporated into another program without
the express written consent of WuQi. This Information or any portion
thereof remains the property of WuQi. The Information contained herein
is believed to be accurate and WuQi assumes no responsibility or
liability for its use in any way and conveys no license or title under
any patent or copyright and makes no representation or warranty that this
Information is free from patent or copyright infringement.

****************************************************************************/

#ifndef IOT_BITOPS_API_H
#define IOT_BITOPS_API_H

/**
 * @addtogroup UTILS
 * @{
 * @addtogroup BITOPS
 * @{
 */

/* os shim includes */
#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* define table size used for bit operation against one byte */
#define IOT_BITOPS_TAB_SIZE      (1 << 8)

/* table used for ffs and ffz operation */
extern const uint8_t iot_bitops_ffs_tab[IOT_BITOPS_TAB_SIZE];

/* table used for fls and flz operation */
extern const uint8_t iot_bitops_fls_tab[IOT_BITOPS_TAB_SIZE];

/* table used for cbs and cbz operation */
extern const uint8_t iot_bitops_cbs_tab[IOT_BITOPS_TAB_SIZE];

/**
 * @brief This function is to find first bit set to 1 from the least significant bit.
 *
 * @param b is byte to be checked
 * @return uint8_t the number of bits set to 0
 *         0        --  no bit set to 0
 *        1 ~ 8    --  the first bit set to 0
 */
static inline uint8_t iot_bitops_ffs(uint8_t b)
{
    return iot_bitops_ffs_tab[b];
}

/**
 * @brief This function is to find first bit set to 0 from the least significant bit.
 *
 * @param b is byte to be checked
 * @return uint8_t The number of bits set to 0
 *         0        --  no bit set to 0
 *         1 ~ 8    --  the first bit set to 0.
 */
static inline uint8_t iot_bitops_ffz(uint8_t b)
{
    b = ~b;
    return iot_bitops_ffs_tab[b];
}

/**
 * @brief  iot_bitops_fls() - find last bit set to 1 from the least significant bit.
 *
 * @param b byte to be checked
 * @retval 0         no bit set to 1
 * @retval 1 ~ 8     the last bit set to 1.
 */
#define iot_bitops_fls(b) (iot_bitops_fls_tab[b])

/**
 * @brief This function is to find last bit set to 0 from the least significant bit.
 *
 * @param b byte to be checked
 * @return uint8_t The number of bits set to 0
 * 0 -  no bit set to 0,1 ~ 8 - the last bit set to 0.
 */
static inline uint8_t iot_bitops_flz(uint8_t b)
{
    b = ~b;
    return iot_bitops_fls_tab[b];
}

/**
 * @brief This function is to count the number of bits set to 1 in the byte
 *
 * @param b is byte to be checked
 * @return uint8_t the number of bits set to 1
 */
static inline uint8_t iot_bitops_cbs(uint8_t b)
{
    return iot_bitops_cbs_tab[b];
}

/**
 * @brief This function is to count the number of bits set to 0 in the byte
 *
 * @param b is byte to be checked
 * @return uint8_t the number of bits set to 0
 */
static inline uint8_t iot_bitops_cbz(uint8_t b)
{
    b = ~b;
    return iot_bitops_cbs_tab[b];
}

/**
 * @brief iot_bitops_init() - init the bitops feature
 */
void iot_bitops_init(void);

#ifdef __cplusplus
}
#endif


/**
 * @}
 * addtogroup BITOPS
 * @}
 * addtogroup UTILS
 */

#endif /* IOT_BITOPS_H */
