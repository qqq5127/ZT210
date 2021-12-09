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

#ifndef _DRIVER_HAL_RTC_MON_H
#define _DRIVER_HAL_RTC_MON_H

/**
 * @addtogroup HAL
 * @{
 * @addtogroup RTC_MON
 * @{
 * This section introduces the RTC_MON module's enum, functions and how to use this driver.
 */

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define RTC_TO_MS(x) ((float)(x) / iot_get_current_rtc_freq() * 1000.0)
#define MS_TO_RTC(x) ((float)(x) * (iot_get_current_rtc_freq() / 1000.0))
#define RTC_TO_INT_MS(x) ((x) * 1000 / (uint32_t)iot_get_current_rtc_freq())
#define MS_TO_INT_RTC(x) ((x) * (uint32_t)iot_get_current_rtc_freq() / 1000)

#define MS_RTC_CNT          MS_TO_RTC(1)
#define MS_RTC_INT_CNT      MS_TO_INT_RTC(1)

/**
 * @defgroup hal_rtc_mon_typedef Typedef
 * @{
 */
typedef void (*iot_rtc_mon_cb)(uint32_t freq);
/**
 * @}
 */

/**
 * @defgroup hal_rtc_mon_enum Enum
 * @{
 */

/** @brief RTC_MON status. */
typedef enum {
    RTC_MON_IDLE,
    RTC_MON_BUSY,
    RTC_MON_END,
    RTC_MON_STATUS_NUM,
} RTC_MON_STATUS;
/**
 * @}
 */

/**
 * @brief This function is to restore rtc mon.
 *
 * @param data is the data to restore.
 * @return uint32_t RET_OK for success else error.
 */
uint32_t iot_rtc_mon_restore(uint32_t data);

/**
 * @brief This function is to init rtc mon.
 *
 */
void iot_rtc_mon_init(void);

/**
 * @brief This function is to get current rtc clk freq.
 *
 * @return uint32_t is current rtc clk freq.
 */
uint32_t iot_rtc_mon_get_rtc_freq(void);

/**
 * @brief This function is to get current rtc clk freq with callback.
 *
 * @param cb is callback function to transfor freq.
 */
void iot_rtc_mon_get_rtc_freq_with_cb(iot_rtc_mon_cb cb);

/**
 * @brief This function is to get rtc momery's cycles.
 *
 * @return uint8_t RTC_MON_PRE_CYCLES.
 */
uint8_t iot_get_rtc_mon_cycles(void);

/**
 * @brief This function is to get the latest rtc momery's cnt.
 *
 * @return uint32_t g_current_rtc_mon_cnt.
 */
uint32_t iot_get_latest_rtc_mon_cnt(void);

/**
 * @brief This function is to get the current rtc freq.
 *
 * @return float is current rtc freq.
 */
float iot_get_current_rtc_freq(void);

#ifdef __cplusplus
}
#endif

/**
 * @}
 * @}
 */
#endif   // _DRIVER_HAL_RTC_MON_H
