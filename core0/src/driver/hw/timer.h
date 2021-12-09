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

#ifndef _DRIVER_HW_TIMER_H
#define _DRIVER_HW_TIMER_H

#ifdef __cplusplus
extern "C" {
#endif

void timer_init(uint8_t id);

void timer_set(uint8_t id, uint32_t val, bool_t repeat);

void timer_set_value(uint8_t id, uint32_t val);

void timer_start(uint8_t id);

void timer_stop(uint8_t id);

void timer_int_enable(uint8_t id, bool_t en);

void timer_int_clear(uint8_t id);

uint32_t timer_get_time(uint8_t id);

uint32_t timer_int_get(uint8_t id);

#ifdef __cplusplus
}
#endif
#endif   //_DRIVER_HW_TIMER_H
