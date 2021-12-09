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

#ifndef _DRIVER_HW_RTC_MON_H
#define _DRIVER_HW_RTC_MON_H
#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    RTC_MON_ONE_SHOT = 0,
    RTC_MON_CONTINOUS,
    RTC_MON_MODE_NUM,
} RTC_MON_MODE;

typedef enum {
    RTC_MON_SRC_RTC,
    RTC_MON_SRC_EXTERN,
    RTC_MON_SRC_TYPE_NUM,
} RTC_MON_SRC_CLK;

void rtc_mon_init(void);
void rtc_mon_int_enable(bool_t en);
void rtc_mon_mode_set(RTC_MON_MODE mode);
void rtc_mon_src_clk_sel(RTC_MON_SRC_CLK src_clk_type);
void rtc_mon_config_cycle(uint8_t cycles);
void rtc_mon_threshold_set(uint32_t threshold);
void rtc_mon_start_enable(bool_t en);
uint32_t rtc_mon_get_count(void);
uint32_t rtc_mon_get_status(void);
int64_t rtc_mon_dummy_test(void);

#ifdef __cplusplus
}
#endif

#endif
