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

#ifndef LIB_BOOT_REASON_H
#define LIB_BOOT_REASON_H

#include "iot_charger.h"

/**
 * @addtogroup LIB
 * @{
 */

/**
 * @addtogroup BOOT_REASON
 * @{
 * This section introduces the BOOT REASON module's functions and how to use this driver.
 */

#ifdef __cplusplus
extern "C" {
#endif

/** @defgroup lib_boot_reason_enum Enum
  * @{
  */
typedef enum {
    BOOT_REASON_UNKNOWN = 0,
    BOOT_REASON_POR,
    BOOT_REASON_SLEEP,
    BOOT_REASON_WDT,
    BOOT_REASON_SOFT,
    BOOT_REASON_HARD,
    BOOT_REASON_CPU,
} BOOT_REASON_TYPE;

typedef enum {
    BOOT_REASON_SOFT_REASON_UNKNOWN = 0,
    BOOT_REASON_SOFT_REASON_EXCEPTION,
    BOOT_REASON_SOFT_REASON_SBL,
    BOOT_REASON_SOFT_REASON_APP,
    BOOT_REASON_SOFT_REASON_WDT_TIMEOUT,
    BOOT_REASON_SOFT_REASON_SYS,
    BOOT_REASON_SOFT_REASON_OTA,
    BOOT_REASON_SOFT_REASON_UVP,
} BOOT_REASON_SOFT_REASON;

typedef enum {
    BOOT_REASON_WAKEUP_SRC_GPIO = 0,
    BOOT_REASON_WAKEUP_SRC_RTC0,
    BOOT_REASON_WAKEUP_SRC_RTC1,
    BOOT_REASON_WAKEUP_SRC_VBATT_RESUME,
    BOOT_REASON_WAKEUP_SRC_CHARGER_ON,
    BOOT_REASON_WAKEUP_SRC_CHARGER_2P5,
    BOOT_REASON_WAKEUP_SRC_TK,
    BOOT_REASON_WAKEUP_SRC_DEB,
    BOOT_REASON_WAKEUP_SRC_BT_DS,
    BOOT_REASON_WAKEUP_SRC_BT_LS,
    BOOT_REASON_WAKEUP_SRC_AUD_2_DTOP_WIC,
    BOOT_REASON_WAKEUP_SRC_BT_2_DTOP_WIC,
    BOOT_REASON_WAKEUP_SRC_AUD_2_BT_WIC,
    BOOT_REASON_WAKEUP_SRC_DTOP_2_BT_WIC,
    BOOT_REASON_WAKEUP_SRC_BT_2_AUD_WIC,
    BOOT_REASON_WAKEUP_SRC_DTOP_2_AUD_WIC,

    BOOT_REASON_WAKEUP_SRC_MAX,
} BOOT_REASON_WAKEUP_SOURCE;

/**
  * @}
  */

/**
 * @brief This function is to do soft reset with reason and flag.
 *
 * @param reason is soft reason.
 * @param flag is soft reset flag.
 */
void boot_reason_do_soft_reset(BOOT_REASON_SOFT_REASON reason, uint8_t flag);

/**
 * @brief This function is to get soft reset reason and flag.
 *
 * @param [out] flag is soft reset flag.
 * @return BOOT_REASON_SOFT_REASON is soft reason.
 */
BOOT_REASON_SOFT_REASON boot_reason_get_soft_reset_reason(uint8_t *flag);

/**
 * @brief This function is to get GPIO wakeup source infomation.
 *
 * @param [out] gpio is wakeup gpio number.
 * @param [out] level is wakeup gpio interrupt type.
 */
void boot_reason_get_wakeup_gpio_source(uint16_t *gpio, uint8_t *level);

/**
 * @brief This function is to get charger wakeup source flag
 *
 * @return IOT_CHARGER_INT_TYPE is charger's interrupt type.
 */
IOT_CHARGER_INT_TYPE boot_reason_get_wakeup_charger_flag(void);

/**
 * @brief Thisfunction is to get reset reason.
 *
 * @return BOOT_REASON_TYPE is reset reason.
 */
BOOT_REASON_TYPE boot_reason_get_reason(void);

/**
 * @brief This function is to get wakeup source.
 *
 * @return BOOT_REASON_WAKEUP_SOURCE is wakeup source.
 */
BOOT_REASON_WAKEUP_SOURCE boot_reason_get_wakeup_source(void);

/**
 * @brief This function is to do soft reset when system idle.
 *
 * @return BOOT_REASON_WAKEUP_SOURCE is wakeup source.
 */
void boot_reason_system_reset(BOOT_REASON_SOFT_REASON reason, uint8_t flag);

/**
 * @brief This function is to set a flag which will be shown after the system crash unexpectedly
 *
 * @param flag Flag passed to crash restart.
 */
void boot_reason_set_crash_flag(uint8_t flag);

/**
 * @brief This function is to get the flag that system booted after crash unexpectedly
 *
 * @return The crash flag.
 */
uint8_t boot_reason_get_crash_flag(void);

#ifdef __cplusplus
}
#endif

/**
 * @}
 * addtogroup BOOT_REASON
 */

/**
* @}
 * addtogroup LIB
*/
#endif /* LIB_BOOT_REASON_H */
