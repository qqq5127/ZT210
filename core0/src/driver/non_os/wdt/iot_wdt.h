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

#ifndef _DRIVER_NON_OS_WDT_H_
#define _DRIVER_NOS_OS_WDT_H_

/**
 * @addtogroup HAL
 * @{
 * @addtogroup WDT
 * @{
 * This section introduces the WDT module's functions and how to use this driver.
 */

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* wdt bind to core */
/**
 * @brief This function is to init watchdog moudle.
 *
 */
void iot_wdt_init(void);

/**
 * @brief This function is to deinitialize watchdog module.
 *
 */
void iot_wdt_deinit(void);

/**
 * @brief This function is to confirm whether the used watchdog need the feedback or not.
 *
 * @return bool_t true or false.
 */
bool_t iot_wdt_need_feed(void);

/**
 * @brief This function is to set the feedback of the used watchdog.
 *
 */
void iot_wdt_do_feed(void);

/**
 * @brief This function is to reset the watchdog.
 *
 */
void iot_wdt_do_reset(void);

/**
 * @brief This function is to set the period of the watchdog's feedback.
 *
 * @param period is feed interval, unit is second.
 */
void iot_wdt_set_feed_period(uint32_t period);

/* global chip watchdog */
/**
 * @brief This function is to init the gobal watchdog module.
 *
 */
void iot_wdt_global_init(void);

/**
 * @brief This function is to deinitialize the gobal watchdog module.
 *
 */
void iot_wdt_global_deinit(void);

/**
 * @brief This function is to confirm whether the global used watchdog need the feedback or not.
 *
 * @return bool_t true or false.
 */
bool_t iot_wdt_global_need_feed(void);

/**
 * @brief This function is to set the feedback of the global used WDT.
 *
 */
void iot_wdt_global_do_feed(void);

/**
 * @brief This function is to reset the global watchdog.
 *
 * @param chip_reset is chip reset(reset scratch register) or not(keep scratch register)
 */
void iot_wdt_global_do_reset(bool_t chip_reset);

/**
 * @brief This function is to set the period of the global watchdog's feed.
 *
 * @param period
 */
void iot_wdt_global_set_feed_period(uint32_t period);

/**
 * @brief This function is to get the total feed count of global watchdog.
 *        it usually used as feed cnt log to show the system whether itis still alive.
 *        if this cnt changed then means it's still alive.
 *
 * @return  the totally feed count value
 */
uint32_t iot_wdt_global_get_feed_cnt(void);

/**
 * @brief This function is to disable all system watchdog and global watchdog.
 *
 */
void iot_wdt_disable_all(void);

/**
 * @brief This function is to get the total feed cnt
 *        usually used as feed cnt log to show the system
 *        is still alive. if this cnt changed then means
 *        it's still alive.
 *
 * @return  the totally feed count value
 */
uint32_t iot_wdt_get_feed_cnt(void);

/**
 * @brief This function is to get watchdog timeout's status.
 *
 * @param [out] id is timeout watchdog id.
 * @return bool_t is true if watchdog time occured.
 */
bool_t iot_wdt_is_timeout(uint8_t *id);

/**
 * @brief This function is to enable system watchdog.
 *
 */
void iot_wdt_enable(void);

/**
 * @brief This function is to disable system watchdog.
 *
 */
void iot_wdt_disable(void);

/**
 * @brief This function is to enable global watchdog.
 *
 */
void iot_wdt_global_enable(void);

/**
 * @brief This function is to disable global watchdog.
 *
 */
void iot_wdt_global_disable(void);

/**
 * @brief This fucntion is to dump watchdog reigster;
 *
 */
void iot_wdt_dump_reg(void);

#ifdef __cplusplus
}
#endif

/**
 * @}
 * @}
 */
#endif /*_DRIVER_HAL_WDT_H_ */
