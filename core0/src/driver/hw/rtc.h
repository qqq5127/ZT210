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

#ifndef _DRIVER_HW_RTC_H
#define _DRIVER_HW_RTC_H
#include "types.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    RTC_TIMER_0,
    RTC_TIMER_1,
    RTC_TIMER_2,
    RTC_TIMER_3,
    RTC_TIMER_4,
    RTC_TIMER_5,
    RTC_TIMER_6,
    RTC_TIMER_7,
    RTC_TIMER_MAX,
} RTC_TIMER;

void rtc_init(RTC_TIMER id);
void rtc_deinit(RTC_TIMER id);
void rtc_timer_enable(RTC_TIMER id);
void rtc_timer_disable(RTC_TIMER id);
void rtc_timer_clear(RTC_TIMER id);
void rtc_timer_count_mode_enable(RTC_TIMER id, bool_t en);
void rtc_timer_set_value(RTC_TIMER id, uint32_t val);
uint32_t rtc_timer_get_value(RTC_TIMER id);
void rtc_timer_set_current_value(RTC_TIMER id, uint32_t time);
uint32_t rtc_timer_get_pre_value(RTC_TIMER id);
void rtc_timer_int_enable(RTC_TIMER id);
void rtc_timer_int_disable(RTC_TIMER id);
void rtc_timer_int_clear(RTC_TIMER id);
uint8_t rtc_timer_get_vector(RTC_TIMER id);
uint32_t rtc_timer_get_int_sts(RTC_TIMER id);

#ifdef __cplusplus
}
#endif

#endif
