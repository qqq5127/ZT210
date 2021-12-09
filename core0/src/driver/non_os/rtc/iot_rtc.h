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

#ifndef _DRIVER_HAL_RTC_H
#define _DRIVER_HAL_RTC_H

/**
 * @addtogroup HAL
 * @{
 * @addtogroup RTC
 * @{
 * This section introduces the RTC module's enum, functions and how to use this driver.
 */

#ifdef __cplusplus
extern "C" {
#endif

#define XTAL_32K_CLK_HZ 31250
#define RCO_32K_CLK_HZ 32768

#define RTC_CLK_HZ XTAL_32K_CLK_HZ

// CPU should only see the rtc at it's own
/** @brief RTC timer. */
typedef enum {
    IOT_RTC_TIMER_0,
    IOT_RTC_TIMER_1,
    IOT_RTC_TIMER_MAX,
    IOT_RTC_TIMER_NONE = 0xFF,
} IOT_RTC_TIMER;

typedef void (*iot_rtc_timer_callback)(void *param);

/**
 * @brief This function is to init rtc module.
 *
 * @return uint8_t RET_OK for success else error.
 */
uint8_t iot_rtc_init(void);

/**
 * @brief This function is to deinitialize rtc module.
 *
 * @return uint8_t RET_OK for success else error.
 */
uint8_t iot_rtc_deinit(void);

/**
 * @brief
 *
 * @param data
 * @return uint32_t
 */

uint32_t iot_rtc_save(uint32_t data);

/**
 * @brief
 *
 * @param data
 * @return uint32_t
 */
uint32_t iot_rtc_restore(uint32_t data);

/**
 * @brief This function is to set time for rtc.
 *
 * @param timer is the rtc timer.
 * @param time is the time to set.
 * @return uint8_t RET_OK for success else error.
 */
uint8_t iot_rtc_set_time(IOT_RTC_TIMER timer, uint32_t time);

/**
 * @brief This function is to add timer for rtc.
 *
 * @param time is the time to set.
 * @param cb is the callback function of rtc timer.
 * @param param is the address of the parameter for timer.
 * @return uint32_t is handler_id.
 */
uint32_t iot_rtc_add_timer(uint32_t time, iot_rtc_timer_callback cb,
                             void *param);

/**
 * @brief This function is to add timer for rtc in milliseconds.
 *
 * @param time_ms is the time to set in milliseconds.
 * @param cb is the callback function of rtc timer.
 * @param param is the address of the parameter for timer.
 * @return uint32_t is handler_id.
 */
uint32_t iot_rtc_add_timer_ms(uint32_t time_ms, iot_rtc_timer_callback cb,
                             void *param);

/**
 * @brief This function is to add timer for rtc in seconds.
 *
 * @param time_s is the time to set in seconds.
 * @param cb is the callback function of rtc timer.
 * @param param is the address of the parameter for timer.
 * @return uint32_t is handler_id.
 */
uint32_t iot_rtc_add_timer_s(uint32_t time_s, iot_rtc_timer_callback cb,
                             void *param);

/**
 * @brief This function is to get current rtc time.
 *
 * @return uint32_t is current time.
 */
uint32_t iot_rtc_get_time(void);

/**
 * @brief This function is to get current rtc time in Microseconds.
 *
 * @return uint32_t is current time in Microseconds.
 */
uint32_t iot_rtc_get_time_us(void);

/**
 * @brief This function is to get current rtc time in milliseconds.
 *
 * @return uint32_t is current time in milliseconds.
 */
uint32_t iot_rtc_get_time_ms(void);

/**
 * @brief This function is to get current rtc time in seconds.
 *
 * @return uint32_t is current time in seconds.
 */
uint32_t iot_rtc_get_time_s(void);

/**
 * @brief This function is to delete rtc timer.
 *
 * @param timer_id is the timer's id to delete.
 */
void iot_rtc_delete_timer(uint32_t timer_id);

/**
 * @brief This function is to init rtc globally.
 *
 * @return uint8_t RET_OK for success else error.
 */
uint8_t iot_rtc_global_init(void);

/**
 * @brief This function is to get current global time.
 *
 * @return uint32_t is current global time.
 */
uint32_t iot_rtc_get_global_time(void);

/**
 * @brief This function is to get current global time in Microseconds.
 *
 * @return uint32_t is current global time in Microseconds.
 */
uint32_t iot_rtc_get_global_time_us(void);

/**
 * @brief This function is to get current global time in milliseconds.
 *
 * @return uint32_t is current global time in milliseconds.
 */
uint32_t iot_rtc_get_global_time_ms(void);

/**
 * @brief This function is to get current global time in seconds.
 *
 * @return uint32_t is current global time in seconds.
 */
uint32_t iot_rtc_get_global_time_s(void);

#ifdef __cplusplus
}
#endif

/**
 * @}
 * addtogroup OTP
 * @}
 * addtogroup HAL
 */

#endif   //_DRIVER_HAL_RTC_H
