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
#ifndef _DRIVER_HAL_CACHE_H
#define _DRIVER_HAL_CACHE_H
/**
 * @addtogroup HAL
 * @{
 * @addtogroup CACHE
 * @{
 * This section introduces the CACHE module's enum, structure, functions and how to use this driver.
 */
#include "apb_cache.h"

#ifdef __cplusplus
extern "C" {
#endif

#define IOT_CACHE_SFC_ID  APB_CACHE_SFC_ID
#define IOT_CACHE_SMC_ID  APB_CACHE_SMC_ID
#define IOT_CACHE_MINI_ID APB_CACHE_MINI_ID

/**
 * @brief This function is to clear the corresponding ID cache.
 *
 * @param cache_id is cache id
 */
void iot_cache_clear(uint8_t cache_id);

/**
 * @brief This function is to flush the corresponding ID cache.
 *
 * @param cache_id is cache id
 * @param addr is cache addr
 * @param len is offset len from cache addr
 */
void iot_cache_flush(uint8_t cache_id, uint32_t addr, uint32_t len);

/**
 * @brief This function is to invalidate the cache address data for the corresponding ID.
 *
 * @param cache_id is cache id
 * @param addr is cache addr
 * @param len is offset len from cache addr
 */
void iot_cache_invalidate(uint8_t cache_id, uint32_t addr, uint32_t len);

/**
 * @brief This function is to init Cache module.
 *
 * @param cache_id is cache id
 */
void iot_cache_init(uint8_t cache_id);

/**
 * @brief This function is to deinit Cache module.
 *
 * @param cache_id is cache id
 */
void iot_cache_deinit(uint8_t cache_id);

/**
 * @brief This function is to get the count of the cache missing.
 *
 * @param cache_id is cache id.
 * @return uint32_t is the count of the cache missing.
 */
uint32_t iot_cache_get_miss_cnt(uint8_t cache_id);

/**
 * @brief This function is to clear the cache missing.
 *
 * @param cache_id is cache id.
 */
void iot_cache_miss_clear(uint8_t cache_id);

/**
 * @brief This function is to enable/disable the cache missing.
 *
 * @param cache_id cache_id is cache id.
 * @param en is the enable or disable signal.
 */
void iot_cache_miss_enable(uint8_t cache_id, bool_t en);

/**
 * @brief This function is used to configure cache ram to inernal ram.
 *
 * @param cache_id is cache id
 */
void iot_cache_config_as_ram_mode(uint8_t cache_id);

/**
 * @brief This function is to get the restore status of the cache.
 *
 * @return bool_t is the restore status of the cache.
 */
bool_t iot_cache_get_restore_status(void);

#ifdef LOW_POWER_ENABLE
void iot_cache_restore_register(void);
#endif

#ifdef __cplusplus
}
#endif
/**
* @}
* @}
*/
#endif /* _DRIVER_HAL_CACHE_H */
