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

#ifndef _DRIVER_NONE_OS_TIMER_H
#define _DRIVER_NONE_OS_TIMER_H

/**
 * @addtogroup HAL
 * @{
 * @addtogroup TIMER
 * @{
 * This section introduces the TIMER module's functions and how to use this driver.
 */

#include "types.h"
//delay and get time use default timer 0
#define IOT_DEFAULT_US_TIMER_ID 0

//interrupt timer from 1 to 3
#define IOT_DEFAULT_USER_TIMER_ID    1
#define IOT_USER_TIMER_MAX_VALID_NUM 3
#define IOT_USER_TIMER_INVALID 0XFF

typedef uint8_t timer_isr_handler_t(iot_addrword_t data);

/**
 * @brief This function is to init GTMR0 timer0 for get realtimer timer count.
 *
 */
void iot_timer_init(void);

/**
 * @brief This function is to get the count val of the default timer(IOT_DEFAULT_US_TIMER_ID).
 *
 * @return uint32_t is current time.
 */
uint32_t iot_timer_get_time(void);

/**
 * @brief This function is to circular wait for delay time using timer in us.
 *
 * @param delay_us is delay target(us).
 */
void iot_timer_delay_us(uint32_t delay_us);

/**
 * @brief This function is to circular wait for delay time using timer in ms.
 *
 * @param ms is delay target(ms).
 */
void iot_timer_delay_ms(uint32_t ms);

/**
 * @brief This function is to obtain a timer and init its' interrupt.
 *
 * @param isr_handle is interrupt callback in interrput context.
 * @param data is reserved data, used for customer.
 * @return uint8_t RET_FAIL-fail or RET_OK-success.
 */
uint8_t iot_timer_create(timer_isr_handler_t *isr_handle, iot_addrword_t data);

/**
 * @brief This function is to set a target val to generate interrput.
 *
 * @param id is timer id.
 * @param time is target time(us).
 * @param repeat is target time(us).
 */
void iot_timer_set(uint8_t id, uint32_t time, bool_t repeat);

/**
 * @brief This function is to start counting..
 *
 * @param id is timer id.
 */
void iot_timer_start(uint8_t id);

/**
 * @brief This function is to stop counting.
 *
 * @param id is timer id.
 */
void iot_timer_stop(uint8_t id);

/**
 * @brief This function is to relrase counting.
 *
 * @param id is timer id.
 */
void iot_timer_release(uint8_t id);

/**
 * @brief Save timer state
 *
 * @param data Pointer of data struct
 * @return RET_OK for success otherwise fail
 */
uint32_t iot_timer_save(uint32_t data);

/**
 * @brief Restore timer state
 *
 * @param data Pointer of data struct
 * @return RET_OK for success otherwise fail
 */
uint32_t iot_timer_restore(uint32_t data);

/**
 * @}
 * addtogroup TIMER
 * @}
 * addtogroup HAL
 */

#endif /* _DRIVER_NONE_OS_TIMER_H */
