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
#ifndef _DRIVER_HW_WDT_H_
#define _DRIVER_HW_WDT_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    WATCHDOG0 = 0,
    WATCHDOG1,
    WATCHDOG2,
    WATCHDOG3,
    WATCHDOG4,
    WATCHDOG_MAX_NUM,
} WATCHDOG_TIMER_ID;

void wdt_init(uint8_t id);
void wdt_deinit(uint8_t id);
void wdt_reset(uint8_t id);
void wdt_feed(uint8_t id);
void wdt_disable_cpu_reset(uint8_t id);
void wdt_set_feed_period(uint8_t id, uint32_t period);
uint32_t wdt_get_feed_irq_vector(uint8_t id);
uint32_t wdt_get_timeout_irq_vector(uint8_t id);
uint8_t wdt_get_global_id(void);
uint32_t wdt_get_cnt(uint8_t id);
uint32_t wdt_get_cmp(uint8_t id);
void  wdg_get_int_reset_cmp(uint8_t id, uint32_t *timeout,
                            uint32_t *cpu_reset, uint32_t *full_reset);

#ifdef __cplusplus
}
#endif

#endif /* _DRIVER_HW_WDT_H_ */
