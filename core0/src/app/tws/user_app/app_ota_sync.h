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

/**
 * @addtogroup APP
 * @{
 */

/**
 * @addtogroup APP_OTA_SYNC
 * @{
 * This section introduces the APP OTA_SYNC module's enum, structure, functions and how to use this module.
 */

#ifndef _APP_OTA_SYNC_H_
#define _APP_OTA_SYNC_H_
/**
 * @brief easy connection module
 */

#include "types.h"
#include "userapp_dbglog.h"

#ifndef OTA_SYNC_ENABLED
#define OTA_SYNC_ENABLED 1
#endif

typedef enum {
    OTA_SYNC_MODE_COMPLETE,
    OTA_SYNC_MODE_PARTIAL,
} ota_sync_mode_e;

#if OTA_SYNC_ENABLED
/**
 * @brief init the ota sync module
 */
void app_ota_sync_init(void);

/**
 * @brief deinit the ota sync module
 */
void app_ota_sync_deinit(void);

/**
 * @brief start ota sync
 * @param image_size image_size of the ota data
 * @param image_crc image_crc of the ota data
 * @param mode sync mode
 */
void app_ota_sync_start(uint32_t image_size, uint32_t image_crc, ota_sync_mode_e mode);

/**
 * @brief stop ota sync
 */
void app_ota_sync_stop(void);

/**
 * @brief reboot ota sync
 */
void app_ota_sync_reboot(void);

/**
 * @brief get ota sync state
 */
uint8_t app_ota_sync_get_state(void);
#else
/**
 * @brief init the ota sync module
 */
static inline void app_ota_sync_init(void)
{
}
/**
 * @brief deinit the ota sync module
 */
static inline void app_ota_sync_deinit(void)
{
}
/**
 * @brief start ota sync
 * @param image_size image_size of the ota data
 * @param image_size image_crc of the ota data
 * @param mode sync mode
 */
static inline void app_ota_sync_start(uint32_t image_size, uint32_t image_crc, ota_sync_mode_e mode)
{
    UNUSED(image_size);
    UNUSED(image_crc);
    UNUSED(mode);
}
/**
 * @brief stop ota sync
 */
static inline void app_ota_sync_stop(void)
{
}
/**
 * @brief reboot ota sync
 */
static inline void app_ota_sync_reboot(void)
{
}
/**
 * @brief get ota sync state
 */
static inline uint8_t app_ota_sync_get_state(void)
{
    return 0;
}
#endif

/**
 * @}
 * addtogroup APP_OTA_SYNC
 */

/**
 * @}
 * addtogroup APP
 */

#endif   //_APP_OTA_SYNC_H_
