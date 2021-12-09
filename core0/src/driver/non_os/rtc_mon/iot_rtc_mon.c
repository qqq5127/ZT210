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

#include "types.h"
#include "chip_irq_vector.h"

#include "iot_irq.h"
#include "iot_rtc.h"

#include "iot_rtc_mon.h"
#include "rtc_mon.h"

#ifdef LOW_POWER_ENABLE
#include "dev_pm.h"
#include "power_mgnt.h"
#endif

#define RTC_MON_PRE_CYCLES 7
#define RTC_MON_XTAL_32K_CLK_VAL   (512 * (1 << RTC_MON_PRE_CYCLES) - 1)

#ifdef LOW_POWER_ENABLE
struct pm_operation rtc_mon_pm;
#endif

static iot_irq_t rtc_mon_int_irq = 0;
static iot_rtc_mon_cb g_rtc_mon_cb = NULL;
static uint32_t g_current_rtc_mon_cnt = 0;

static uint32_t rtc_mon_isr_handler(uint32_t vector, uint32_t data) IRAM_TEXT(rtc_mon_isr_handler);
static uint32_t rtc_mon_isr_handler(uint32_t vector, uint32_t data)
{
    uint32_t cnt;

    (void)vector;
    (void)data;
#if RTC_CLK_HZ == RCO_32K_CLK_HZ
    cnt = rtc_mon_get_count();
#elif RTC_CLK_HZ == XTAL_32K_CLK_HZ
    cnt = RTC_MON_XTAL_32K_CLK_VAL;
#else
    assert(0);
#endif

    g_current_rtc_mon_cnt = cnt;

    cnt = (uint32_t)(16e6 / cnt);
    if (g_rtc_mon_cb) {
        g_rtc_mon_cb(cnt * (1 << RTC_MON_PRE_CYCLES));
    }
    g_rtc_mon_cb = NULL;
    rtc_mon_start_enable(false);
    rtc_mon_int_enable(false);

#ifdef LOW_POWER_ENABLE
    power_mgnt_set_sleep_vote(POWER_SLEEP_RTC_MON);
#endif
    return 0;
}

uint32_t iot_rtc_mon_restore(uint32_t data) IRAM_TEXT(iot_rtc_mon_restore);
uint32_t iot_rtc_mon_restore(uint32_t data)
{
    UNUSED(data);
    rtc_mon_start_enable(false);
    rtc_mon_init();
    rtc_mon_int_enable(false);
    return 0;
}

void iot_rtc_mon_init(void)
{
    rtc_mon_init();
    rtc_mon_int_enable(false);
    // Config interrupt
    rtc_mon_int_irq = iot_irq_create(RTC_MON_INT, 0, rtc_mon_isr_handler);
    iot_irq_unmask(rtc_mon_int_irq);
#ifdef LOW_POWER_ENABLE
    iot_dev_pm_node_init(&rtc_mon_pm);
    rtc_mon_pm.restore = iot_rtc_mon_restore;
    iot_dev_pm_register(&rtc_mon_pm);
#endif
}

uint32_t iot_rtc_mon_get_rtc_freq(void)
{
#if RTC_CLK_HZ == RCO_32K_CLK_HZ
    uint32_t cnt = 0;
    rtc_mon_start_enable(0);
    rtc_mon_src_clk_sel(RTC_MON_SRC_RTC);
    rtc_mon_config_cycle(RTC_MON_PRE_CYCLES);
    rtc_mon_mode_set(RTC_MON_ONE_SHOT);
    rtc_mon_start_enable(1);

    while (rtc_mon_get_status() != RTC_MON_END)
        ;
    cnt = rtc_mon_get_count();
    g_current_rtc_mon_cnt = cnt;

    cnt = 16e6 / cnt;
    return (cnt * (1 << RTC_MON_PRE_CYCLES));
#elif RTC_CLK_HZ == XTAL_32K_CLK_HZ
    g_current_rtc_mon_cnt = RTC_MON_XTAL_32K_CLK_VAL;
    return RTC_CLK_HZ;
#else
   assert(0);
#endif
}

void iot_rtc_mon_get_rtc_freq_with_cb(iot_rtc_mon_cb cb) IRAM_TEXT(iot_rtc_mon_get_rtc_freq_with_cb);
void iot_rtc_mon_get_rtc_freq_with_cb(iot_rtc_mon_cb cb)
{
#if RTC_CLK_HZ == RCO_32K_CLK_HZ
    g_rtc_mon_cb = cb;
    rtc_mon_start_enable(0);
    rtc_mon_src_clk_sel(RTC_MON_SRC_RTC);
    rtc_mon_config_cycle(RTC_MON_PRE_CYCLES);
    rtc_mon_mode_set(RTC_MON_ONE_SHOT);

    #ifdef LOW_POWER_ENABLE
    power_mgnt_clear_sleep_vote(POWER_SLEEP_RTC_MON);
    #endif

    rtc_mon_int_enable(1);
    rtc_mon_start_enable(1);
#elif RTC_CLK_HZ == XTAL_32K_CLK_HZ
    UNUSED(cb);
    g_current_rtc_mon_cnt = RTC_MON_XTAL_32K_CLK_VAL;
#else
   assert(0);
#endif
}

uint8_t iot_get_rtc_mon_cycles(void) IRAM_TEXT(iot_get_rtc_mon_cycles);
uint8_t iot_get_rtc_mon_cycles(void)
{
    return RTC_MON_PRE_CYCLES;
}

uint32_t iot_get_latest_rtc_mon_cnt(void)
{
    return g_current_rtc_mon_cnt;
}

float iot_get_current_rtc_freq(void) IRAM_TEXT(iot_get_current_rtc_freq);
float iot_get_current_rtc_freq(void)
{
#if RTC_CLK_HZ == RCO_32K_CLK_HZ
    if (g_current_rtc_mon_cnt) {
        return (float)((float)(1 << RTC_MON_PRE_CYCLES) * 16.0e6
            / (float)g_current_rtc_mon_cnt);
    } else {
        /*
         * must used after init
         * or used as 32768
         */
        //assert(0);
        return RTC_CLK_HZ;
    }
#elif RTC_CLK_HZ == XTAL_32K_CLK_HZ
    return RTC_CLK_HZ;
#else
   assert(0);
#endif
}
