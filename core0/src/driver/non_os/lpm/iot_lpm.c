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
/* common includes */
#include "types.h"
#include "riscv_cpu.h"
#include "chip_irq_vector.h"

#include "iot_lpm.h"
#include "iot_irq.h"
#include "iot_rtc.h"
#include "iot_timer.h"
#include "iot_rtc_mon.h"

/* hw includes */
#include "intc.h"
#include "apb.h"
#include "rtc.h"
#include "pmm.h"
#include "gpio.h"
#include "bingo_pmm.h"
#include "spinlock.h"
#include "pmm_ana.h"
#include "driver_dbglog.h"

#ifdef BUILD_CORE_CORE0
#include "clock.h"
#endif

#define IOT_LPM_PMM_RTC_ISR_DELAY_US    70

#define IOT_LPM_WAKEUP_TIMER RTC_TIMER_6

// If core wakeup early than this time, it will change the rtc to use by itself
#define IOT_LPM_PMM_RTC_WAKEUP_EARLY_MS 5

/* LOCK is meaning: set rtc timer and enable irq.
 * REF is meaning: whatever set rtc timer and enable irq, it
 *                 depends on existed rtc timer to wakeup.
 * | LOCK | REF |    Description
 * |   0  |  0  |  Not use RTC timer.
 * |   0  |  1  |  Not set RTC timer, but need existed RTC timer to wakeup self.
 * |   1  |  0  |  No meaningfull. Never use this combine values.
 * |   1  |  1  |  Set RTC timer and need it to wakeup self.
 *
 */

#define IOT_LPM_PMM_RTC_LOCK_MASK           (0x00000003)
#define IOT_LPM_PMM_RTC_REF_MASK            (0x00000030)

#if defined(BUILD_CORE_CORE0)
#define IOT_LPM_PMM_RTC_LOCK_SELF_BIT       (0x00000001)
#define IOT_LPM_PMM_RTC_LOCK_OTHER_BIT      (0x00000002)
#define IOT_LPM_PMM_RTC_REF_SELF_BIT        (0x00000010)
#define IOT_LPM_PMM_RTC_REF_OTHER_BIT       (0x00000020)
#elif defined(BUILD_CORE_CORE1)
#define IOT_LPM_PMM_RTC_LOCK_SELF_BIT       (0x00000002)
#define IOT_LPM_PMM_RTC_LOCK_OTHER_BIT      (0x00000001)
#define IOT_LPM_PMM_RTC_REF_SELF_BIT        (0x00000020)
#define IOT_LPM_PMM_RTC_REF_OTHER_BIT       (0x00000010)
#endif
#define IOT_LPM_PMM_RTC_LOCK_HAS_OTHER(magic_num) \
    ((((magic_num) & IOT_LPM_PMM_RTC_LOCK_MASK) & IOT_LPM_PMM_RTC_LOCK_OTHER_BIT) != 0)

#define IOT_LPM_PMM_RTC_REF_HAS_OTHER(magic_num) \
    ((((magic_num) & IOT_LPM_PMM_RTC_REF_MASK) & IOT_LPM_PMM_RTC_REF_OTHER_BIT) != 0)

/*lint -esym(526,iot_lpm_warm_entry)  iot_lpm_warm_entry defined in file "iot_lpm_entry.S"*/
extern void iot_lpm_warm_entry(void);

/*
 * statistic for light sleep enter
 */
static uint32_t light_sleep_cnt = 0;
static uint32_t deep_sleep_cnt = 0;
static uint32_t light_sleep_timer_id;
static iot_irq_t pmm_wakeup_rtc_irq;
#ifdef BUILD_CORE_CORE0
static iot_irq_t pmm_wakeup_early_irq;
#endif
uint32_t g_clr_pmm_ts_rtc = 0;
uint32_t g_pmm_rtc_isr_cnt = 0;

uint32_t iot_lpm_pmm_rtc_get_sts(void) IRAM_TEXT(iot_lpm_pmm_rtc_get_sts);
uint32_t iot_lpm_pmm_rtc_get_sts(void)
{
    return rtc_timer_get_int_sts(IOT_LPM_WAKEUP_TIMER);
}

static uint32_t iot_lpm_pmm_rtc_isr_handler(uint32_t vector, uint32_t data)
    IRAM_TEXT(iot_lpm_pmm_rtc_isr_handler);
static uint32_t iot_lpm_pmm_rtc_isr_handler(uint32_t vector, uint32_t data)
{
    UNUSED(vector);
    UNUSED(data);

#ifdef BUILD_CORE_CORE0
    /* In this scenario, RTC timer belongs to dtop core and it is still in counting:
     *     Dtop core is wakeuped(wakeuped by non RTC timer) and bt is in sleeping.
     * Then, when the RTC timer target time is comming, it will produce wakeup signal
     * to BT core and produce an rtc interrupt to DTOP core. But BT core capture the wakeup
     * signal in next positive edge of 32K RTC clocks and DTOP core enter this isr handler
     * quickly, if the RTC timer isr happening instant is far away from next positive edge
     * of 32K RTC clocks, it may cause DTOP core clear RTC wakeup source before BT core
     * capture the wakeup signal. Then BT core will not be wakeup until next wakeup source.
     * So do delay here before clear RTC timer. Otherwise, it will cause BT core sleep timeout.
     */
    uint32_t delay_mcycles = IOT_LPM_PMM_RTC_ISR_DELAY_US * CLOCK_CORE_TO_MHZ(clock_get_core_clock(0));
    uint32_t c1;
    uint32_t c2;

    c1 = (uint32_t)cpu_get_mcycle();
    do {
        c2 = (uint32_t)cpu_get_mcycle();
    } while (c2 - c1 < delay_mcycles);
#endif

    uint32_t mask = dev_spinlock_lock(PMM_WAKEUP_RTC_LOCK);
    uint32_t s2 = pmm_get_scratch2_register();
    /*
     * Clear RTC timer only when there's no other owner.
     */
    if (IOT_LPM_PMM_RTC_LOCK_HAS_OTHER(s2)) {
        /*
         * Only clear itself's lock and ref bit, other owner's lock and ref bit should
         * be clear then other owner's RTC ISR interrupt handler
         */
        s2 &= ~(IOT_LPM_PMM_RTC_LOCK_SELF_BIT | IOT_LPM_PMM_RTC_REF_SELF_BIT);
    } else {
        rtc_timer_int_disable(IOT_LPM_WAKEUP_TIMER);
        rtc_timer_int_clear(IOT_LPM_WAKEUP_TIMER);
        rtc_timer_disable(IOT_LPM_WAKEUP_TIMER);

        g_clr_pmm_ts_rtc = iot_rtc_get_global_time();

        /* There's might be other ref bit, so clear all ref bits */
        s2 &= ~(IOT_LPM_PMM_RTC_LOCK_SELF_BIT | IOT_LPM_PMM_RTC_REF_MASK);
    }

    pmm_set_scratch2_register(s2);

    g_pmm_rtc_isr_cnt++;
    dev_spinlock_unlock(PMM_WAKEUP_RTC_LOCK, mask);
    iot_irq_mask(pmm_wakeup_rtc_irq);

    return 0;
}

#ifdef BUILD_CORE_CORE0
static uint32_t iot_lpm_pmm_early_wakeup_by_bt_isr_handler(uint32_t vector, uint32_t data)
    IRAM_TEXT(iot_lpm_pmm_early_wakeup_by_bt_isr_handler);
static uint32_t iot_lpm_pmm_early_wakeup_by_bt_isr_handler(uint32_t vector, uint32_t data)
{
    UNUSED(vector);
    UNUSED(data);

    uint32_t wakeup_src = intc_get_wakeup_state();
    if ((wakeup_src & BIT(WAKEUP_BT_PD_EARLY_BY_BT_TMR))) {
        pmm_bt_pd_wakeup_by_bt_tmr_clr(true);
    }
    return 0;
}

/* let the cpu exit wfi for the DTOP hardware not shutdown case */
static void iot_lpm_early_wakeup_by_bt_init(void)
{
    //TODO: a int handler could for below wakeup source
    intc_set_wakeup_enable(WAKEUP_BT_PD_EARLY_BY_BT_TMR);
    pmm_wakeup_early_irq =
        iot_irq_create(BT_PD_EARLY_WAKEUP_BY_BT_TMR, 0, iot_lpm_pmm_early_wakeup_by_bt_isr_handler);
    iot_irq_unmask(pmm_wakeup_early_irq);
}
#endif

uint32_t g_wakeup_ts_rtc = 0;
uint32_t wakeup_isr_count = 0;
static uint32_t iot_lpm_wakeup_handler(uint32_t vector, uint32_t data)
    IRAM_TEXT(iot_lpm_wakeup_handler);
static uint32_t iot_lpm_wakeup_handler(uint32_t vector, uint32_t data)
{
    UNUSED(vector);
    UNUSED(data);

    uint32_t wakeup_src = intc_get_wakeup_state();

#ifdef BUILD_CORE_CORE0
    if (!(wakeup_src & BIT(WAKEUP_RTC_TMR0_INT0)))
#elif BUILD_CORE_CORE1
    if (!(wakeup_src & BIT(WAKEUP_RTC_TMR1_INT0)))
#elif BUILD_CORE_DSP
    if (!(wakeup_src & BIT(WAKEUP_RTC_TMR2_INT0)))
#endif
    {
        iot_rtc_delete_timer(light_sleep_timer_id);
    }

    g_wakeup_ts_rtc = iot_rtc_get_global_time();
    pmm_gpio_wakeup_clear_all();

    apb_wakeup_int_process_done(apb_get_sub_sys(cpu_get_mhartid()));

    wakeup_isr_count++;

    return 0;
}

#ifdef BUILD_CORE_CORE0
static uint32_t iot_lpm_bod_int_handler(uint32_t vector, uint32_t data)
    IRAM_TEXT(iot_lpm_bod_int_handler);
static uint32_t iot_lpm_bod_int_handler(uint32_t vector, uint32_t data)
{
    UNUSED(vector);
    UNUSED(data);

    pmm_bod_int_en(false);
    pmm_bod_int_clear();
    DBGLOG_DRIVER_ERROR("[UVP]int detected\n");
    return 0;
}
#endif

uint32_t iot_lpm_get_wakeup_count(void)
{
    return wakeup_isr_count;
}

static void iot_timer_wakeup_handler(void *param) IRAM_TEXT(iot_timer_wakeup_handler);
static void iot_timer_wakeup_handler(void *param)
{
    UNUSED(param);
}

void iot_lpm_pmm_wakeup_rtc_init(void)
{
#ifdef BUILD_CORE_CORE0
    // only if core0 init, need to disable the timer
    rtc_timer_disable(IOT_LPM_WAKEUP_TIMER);

    // clear scratch2
    pmm_set_scratch2_register(0);
#endif

    uint32_t cpu = cpu_get_mhartid();
    PMM_PWR_DOMAIN pd = pmm_get_power_domain(cpu);
    INTC_WAKEUP_SRC intc_wake_rtc =
        IOT_LPM_WAKEUP_TIMER == RTC_TIMER_6 ? WAKEUP_PMM_RTC_TIMER_INT0 : WAKEUP_PMM_RTC_TIMER_INT1;
    PMM_DS_WAKEUP_SRC wakeup_rtc =
        IOT_LPM_WAKEUP_TIMER == RTC_TIMER_6 ? PMM_DS_WAKEUP_RTC0 : PMM_DS_WAKEUP_RTC1;

    intc_set_wakeup_enable(intc_wake_rtc);
    pmm_set_pd_wakeup_src_req(pd, wakeup_rtc, true);
    pmm_set_rtc_wakeup(true);

    pmm_wakeup_rtc_irq = iot_irq_create(DIG_RTC_TIMER_INT0_VLD, 0, iot_lpm_pmm_rtc_isr_handler);
}

void iot_lpm_enter_dozesleep(void) IRAM_TEXT(iot_lpm_enter_dozesleep);
void iot_lpm_enter_dozesleep(void)
{
    uint32_t cpu = cpu_get_mhartid();

#ifdef BUILD_CORE_CORE0
    // enable cpu clk force on bit
    apb_m_s_enable(APB_M_S_RV5_CORE0_CLK);
    pmm_set_chip_sleep(PMM_CHIP_SLEEP_NONE);
#elif BUILD_CORE_CORE1
    apb_clk_enable(APB_CLK_RV5_CORE1_CLK);
#endif

    // unMask cpu interrupt
    apb_cpu_int_mask((uint8_t)cpu, false, false, false);

    // Apb Disable sleep mode
    apb_sub_sys_sleep_enable(apb_get_sub_sys(cpu), APB_SLEEP_NONE);
}

void iot_lpm_enter_lightsleep(void) IRAM_TEXT(iot_lpm_enter_lightsleep);
void iot_lpm_enter_lightsleep(void)
{
    uint32_t cpu = cpu_get_mhartid();

    // Disable cpu interrupt for context safe
    uint32_t mask = cpu_disable_irq();

#ifdef BUILD_CORE_CORE0
    // Disable cpu clk force on bit
    apb_m_s_disable(APB_M_S_RV5_CORE0_CLK);
#elif BUILD_CORE_CORE1
    apb_clk_disable(APB_CLK_RV5_CORE1_CLK);
#endif

    // Mask cpu interrupt
    apb_cpu_int_mask((uint8_t)cpu, true, true, true);

    // Apb Enable sleep mode
    apb_sub_sys_sleep_enable(apb_get_sub_sys(cpu), APB_SLEEP_LIGHT);

    // Pmm enable sleep mode
    pmm_sleep_control(pmm_get_power_domain(cpu), PMM_SLEEP_LIGHT, true);

    pmm_gpio_wakeup_clear_all();

    light_sleep_cnt++;

    // Cpu enter wfi
    cpu_enter_wfi();

    /**
     * wakeup apb int mask will clear by hardware
     * we just need enable cpu irq here
     */
    cpu_restore_irq(mask);
}

void iot_lpm_xtal_lowpower_enable(bool_t enable)
{
#ifdef BUILD_CORE_CORE0
    bingo_pmm_xtal_lowpower_config(enable);
    if (enable) {
        pmm_xtal_intf_config(enable);
    }
#else
    UNUSED(enable);
#endif
}

void iot_lpm_xtal_enable(bool_t enable)
{
#ifdef BUILD_CORE_CORE0
    bingo_pmm_xtal_enable(enable);
    pmm_xtal_intf_config(enable);
#else
    UNUSED(enable);
#endif
}

void iot_lpm_enter_deepsleep(void)
{
    uint32_t cpu = cpu_get_mhartid();

#ifdef BUILD_CORE_CORE0
    uint8_t flag;
    flag = pmm_charger_flag_get();
     if (flag) {
         pmm_charger_flag_int_type(PMM_CHG_INT_LEVEL_LOW);
     } else {
         pmm_charger_flag_int_type(PMM_CHG_INT_LEVEL_HIGH);
     }
    // Reduce dcdc pulse width
    iot_lpm_set_dcdc_pulse_width(0);

    // Disable cpu clk force on bit
    apb_m_s_disable(APB_M_S_RV5_CORE0_CLK);

    // enable xtal for deepsleep mode enter
    iot_lpm_xtal_lowpower_enable(true);

    // pmm ldo stages
    pmm_ldo_mode_entry_config(PMM_WORK_LINK_MODE);
    // DCDC ldo stages
    pmm_dcdc_ldo_mode_entry_config(PMM_WORK_LINK_MODE);

    pmm_set_chip_sleep(PMM_CHIP_SLEEP_DEEP);

    pmm_gpio_wakeup_clear_all();
#elif BUILD_CORE_CORE1
    apb_clk_disable(APB_CLK_RV5_CORE1_CLK);
#endif

    // Mask cpu interrupt
    apb_cpu_int_mask((uint8_t)cpu, true, true, true);

    // Apb Enable sleep mode
    apb_sub_sys_sleep_enable(apb_get_sub_sys(cpu), APB_SLEEP_DEEP);

    // Pmm enable sleep mode
    pmm_sleep_control(pmm_get_power_domain(cpu), PMM_SLEEP_DEEP, true);

    /* deep cnt after INT masked */
    deep_sleep_cnt++;
}

void iot_lpm_exit_deepsleep(void) IRAM_TEXT(iot_lpm_exit_deepsleep);
void iot_lpm_exit_deepsleep(void)
{
    uint32_t cpu = cpu_get_mhartid();

    // Apb Disable sleep mode
    apb_sub_sys_sleep_enable(apb_get_sub_sys(cpu), APB_SLEEP_NONE);

    // unMask cpu interrupt
    apb_cpu_int_mask((uint8_t)cpu, false, false, false);

#if defined(BUILD_CORE_CORE0)
    pmm_set_chip_sleep(PMM_CHIP_SLEEP_NONE);
    pmm_charger_flag_int_type(PMM_CHG_INT_EDGE_BOTH);

    // Disable xtal lowpower mode
    iot_lpm_xtal_lowpower_enable(false);

    // enable cpu clk force on bit
    apb_m_s_enable(APB_M_S_RV5_CORE0_CLK);

    // Increase dcdc pulse width
    iot_lpm_set_dcdc_pulse_width(0xF);

    //set lower uvp to avoid uvp reset
    pmm_charger_uvp_mv_config(PMM_CHG_UVP_2900);
#elif defined(BUILD_CORE_CORE1)
    apb_clk_enable(APB_CLK_RV5_CORE1_CLK);
#endif
}

void iot_lpm_enter_shutdown(void)
{
#ifdef BUILD_CORE_CORE0
    uint8_t flag;

    flag = pmm_charger_flag_get();
    if (flag) {
        pmm_charger_flag_int_type(PMM_CHG_INT_LEVEL_LOW);
    } else {
        pmm_charger_flag_int_type(PMM_CHG_INT_LEVEL_HIGH);
    }

    // Disable charger vbus
    pmm_charger_vbus_gpio_enable(false);

    pmm_power_domain_force_powerdown(PMM_PD_AUDIO, true);
    //    pmm_power_domain_force_powerdown(PMM_PD_BT, 1);
    pmm_power_domain_force_powerdown(PMM_PD_AUDIO_INTF, true);

    pmm_clear_all_pd_wakeup_src_req(PMM_PD_AUDIO);
    pmm_clear_all_pd_wakeup_src_req(PMM_PD_AUDIO_INTF);
    // FIXME force down bt & dsp
#elif defined(BUILD_CORE_CORE1)
    pmm_bt_sleep_stat_ignore(true);
    pmm_bt_wakeup_lp_en(false);
    pmm_clear_all_pd_wakeup_src_req(PMM_PD_BT);
#endif

    // FIXME remove bt & dsp force down
    // pmm_bt_sleep_stat_ignore(true);
    pmm_bt_wakeup_lp_en(false);
    pmm_bt_timing_gen_soft_rst();

    uint32_t cpu = cpu_get_mhartid();
    PMM_PWR_DOMAIN pd = pmm_get_power_domain(cpu);

#ifdef BUILD_CORE_CORE0
    // Disable cpu clk force on bit
    apb_m_s_disable(APB_M_S_RV5_CORE0_CLK);

    // disable xtal for shutdown mode enter
    iot_lpm_xtal_enable(false);
#elif BUILD_CORE_CORE1
    apb_clk_disable(APB_CLK_RV5_CORE1_CLK);
#endif
    // Mask cpu interrupt
    apb_cpu_int_mask((uint8_t)cpu, true, true, true);

#ifdef BUILD_CORE_CORE0
    // pmm ldo stages
    pmm_ldo_mode_entry_config(PMM_WORK_SHIP_MODE);
    // DCDC ldo stages
    pmm_dcdc_ldo_mode_entry_config(PMM_WORK_SHIP_MODE);

    pmm_set_32k_clk_src(PMM_32K_SRC_RCO);

    pmm_set_chip_sleep(PMM_CHIP_SLEEP_DEEP);
    pmm_set_sleep_ram_full_state(PMM_PD_DCORE, PMM_SLEEP_DEEP_SLEEP_ENTRY, PMM_MEM_OFF);
    pmm_set_sleep_ram_full_state(PMM_PD_DCORE, PMM_SLEEP_DEEP_SLEEP_EXIT, PMM_MEM_ON);

    pmm_set_sleep_ram_full_state(PMM_PD_BT, PMM_SLEEP_DEEP_SLEEP_ENTRY, PMM_MEM_OFF);
    pmm_set_sleep_ram_full_state(PMM_PD_BT, PMM_SLEEP_DEEP_SLEEP_EXIT, PMM_MEM_ON);

    pmm_set_sleep_ram_full_state(PMM_PD_AUDIO, PMM_SLEEP_DEEP_SLEEP_ENTRY, PMM_MEM_OFF);
    pmm_set_sleep_ram_full_state(PMM_PD_AUDIO, PMM_SLEEP_DEEP_SLEEP_EXIT, PMM_MEM_ON);

    pmm_set_cpu_boot_addr(cpu, CPU_BOOT_ADDR);
#endif
    // Apb Enable sleep mode
    apb_sub_sys_sleep_enable(apb_get_sub_sys(cpu), APB_SLEEP_DEEP);

    // Pmm enable sleep mode
    pmm_sleep_control(pd, PMM_SLEEP_DEEP, true);

#ifdef BUILD_CORE_CORE0
    pmm_gpio_wakeup_clear_all();
#endif
    cpu_enter_wfi();

    // Shutdown, Re-enable charger vbus for log output
    pmm_charger_vbus_gpio_enable(true);
}

void iot_lpm_set_dig_rtc_wake_src(uint32_t time)
{
    light_sleep_timer_id = iot_rtc_add_timer(time, iot_timer_wakeup_handler, NULL);

#ifdef BUILD_CORE_CORE0
    intc_set_wakeup_enable(WAKEUP_RTC_TMR0_INT0);
#elif BUILD_CORE_CORE1
    intc_set_wakeup_enable(WAKEUP_RTC_TMR1_INT0);
#elif BUILD_CORE_DSP
    intc_set_wakeup_enable(WAKEUP_RTC_TMR2_INT0);
#endif
}

uint32_t g_need_wakeup_rtc;
uint32_t g_cur_rtc;

uint8_t iot_lpm_set_pmm_rtc_wakeup_src(uint32_t slp_st_rtc, uint32_t time_rtc)
{
    /**
     * 1. acquire pmm rtc spinlock
     * 2. calc sleep time
     * 3. write self lock state to scratch2
     * 4. wtite wake time to scrath3
     * 5. unmask wakeup rtc int
     * 6. enable wakeup rtc
     * 7. release pmm rtc spinlock
     */
    uint8_t ret = RET_NOT_READY;
    uint32_t now = slp_st_rtc;

    uint32_t mask = dev_spinlock_lock(PMM_WAKEUP_RTC_LOCK);

    uint32_t wakeup = now + time_rtc;

    uint32_t s2 = pmm_get_scratch2_register();
    if (IOT_LPM_PMM_RTC_REF_HAS_OTHER(s2)) {
        /*
         * if first time enter with inited value or
         * locked and locked with other core, then
         * we need to check if we could go to sleep
         * with rtc wakeup src set
         */
        uint32_t s3 = pmm_get_scratch3_register();
        uint32_t safe_gap = MS_TO_INT_RTC(IOT_LPM_PMM_RTC_WAKEUP_EARLY_MS);
        /*
         * if other core is using the rtc timer
         */
        if (s3 <= now) {
            /** the other core's last wakeup time already set,
             * but not fully wakeup yet, do not touch the timer
             * wait it clear done by other core
             */
            dev_spinlock_unlock(PMM_WAKEUP_RTC_LOCK, mask);
            return ret;
        }

        if (s3 <= wakeup) {
            // the other core is wakeup earlier
            if (wakeup - s3 > safe_gap && s3 - now > safe_gap) {
                /* only enough space to wakeup then go to sleep */
                /* and not wake up too quickly */
                g_need_wakeup_rtc = wakeup;
                g_cur_rtc = s3;
                s2 |= IOT_LPM_PMM_RTC_REF_SELF_BIT;
                pmm_set_scratch2_register(s2);
                ret = RET_OK;
            }
            dev_spinlock_unlock(PMM_WAKEUP_RTC_LOCK, mask);
            return ret;
        } else if (s3 - wakeup <= safe_gap) {
            // not early enough, just return and don't sleep
            dev_spinlock_unlock(PMM_WAKEUP_RTC_LOCK, mask);
            return ret;
        }
    }

    /*
     * if we can reset the rtc timer
     * then do it now
     */
    ret = RET_OK;
    s2 |= (IOT_LPM_PMM_RTC_LOCK_SELF_BIT | IOT_LPM_PMM_RTC_REF_SELF_BIT);
    pmm_set_scratch2_register(s2);
    pmm_set_scratch3_register(wakeup);

    iot_irq_unmask(pmm_wakeup_rtc_irq);

    rtc_timer_set_value(IOT_LPM_WAKEUP_TIMER, time_rtc);
    rtc_timer_int_enable(IOT_LPM_WAKEUP_TIMER);
    rtc_timer_enable(IOT_LPM_WAKEUP_TIMER);

    dev_spinlock_unlock(PMM_WAKEUP_RTC_LOCK, mask);

    return ret;
}

void iot_lpm_set_pmm_gpio_wakeup_src(uint16_t gpio)
{
#ifdef BUILD_CORE_CORE0
    gpio_wakeup_enable(gpio, true);
#else
    UNUSED(gpio);
#endif

    intc_set_wakeup_enable(WAKEUP_PMM_GPIO_INT0);
    intc_set_wakeup_enable(WAKEUP_PMM_GPIO_INT1);
    intc_set_wakeup_enable(WAKEUP_DIG_GPIO);
    intc_set_wakeup_enable(WAKEUP_GPIO_WAKEUP);

    pmm_set_gpio_wakeup(true);
    pmm_set_pd_wakeup_src_req(PMM_PD_DCORE, PMM_DS_WAKEUP_GPIO, true);
}

static void iot_lpm_interrupt_init(void)
{
    iot_irq_t irq = iot_irq_create(WAKEUP_INT, 0, iot_lpm_wakeup_handler);
    iot_irq_unmask(irq);
}

uint32_t iot_lpm_get_light_slp_cnt(void) IRAM_TEXT(iot_lpm_get_light_slp_cnt);
uint32_t iot_lpm_get_light_slp_cnt(void)
{
    return light_sleep_cnt;
}

uint32_t iot_lpm_get_deep_slp_cnt(void)
{
    return deep_sleep_cnt;
}

uint32_t iot_lpm_get_perf_mon_cnt(uint8_t value)
{
    if (value == 0) {
        return pmm_perf_mon_val0_get();
    } else if (value == 1) {
        return pmm_perf_mon_val1_get();
    }
    return 0;
}

void iot_lpm_init(void)
{
    uint32_t cpu = cpu_get_mhartid();
    PMM_PWR_DOMAIN pd = pmm_get_power_domain(cpu);

    pmm_set_cpu_boot_addr(cpu, (uint32_t)iot_lpm_warm_entry);

    /* disable auto gate mode */
#if defined(BUILD_CORE_CORE0)
    apb_sys_root_clk_enable(APB_CLK_SYS_DTOP, APB_TOP_CLK_SLP_AUTO_GATE, false);
#elif defined(BUILD_CORE_CORE1)
    apb_sys_root_clk_enable(APB_CLK_SYS_BCP, APB_TOP_CLK_SLP_AUTO_GATE, false);
#elif defined(BUILD_CORE_DSP)
    apb_sys_root_clk_enable(APB_CLK_SYS_ACP, APB_TOP_CLK_SLP_AUTO_GATE, false);
#endif

    /* Set sleep mem config */
    pmm_set_sleep_ram_full_state(pd, PMM_SLEEP_DEEP_SLEEP_ENTRY, PMM_MEM_RETENTION_2);
    pmm_set_sleep_ram_full_state(pd, PMM_SLEEP_DEEP_SLEEP_EXIT, PMM_MEM_ON);

    pmm_set_sleep_rom_full_state(pd, PMM_SLEEP_DEEP_SLEEP_ENTRY, PMM_MEM_OFF);
    pmm_set_sleep_rom_full_state(pd, PMM_SLEEP_DEEP_SLEEP_EXIT, PMM_MEM_ON);

    pmm_set_sleep_ram_full_state(pd, PMM_SLEEP_LIGHT_SLEEP_ENTRY, PMM_MEM_RETENTION_2);
    pmm_set_sleep_ram_full_state(pd, PMM_SLEEP_LIGHT_SLEEP_EXIT, PMM_MEM_ON);

    pmm_set_sleep_rom_full_state(pd, PMM_SLEEP_LIGHT_SLEEP_ENTRY, PMM_MEM_OFF);
    pmm_set_sleep_rom_full_state(pd, PMM_SLEEP_LIGHT_SLEEP_EXIT, PMM_MEM_ON);

    pmm_set_mem_timing_control();

    pmm_set_domain_fsm(pd, CGM_ON_DLY, 2);
    pmm_set_domain_fsm(pd, CGM_OFF_DLY, 2);
    pmm_set_domain_fsm(pd, RST_ASSERT_DLY, 2);
    pmm_set_domain_fsm(pd, SHUTDOWN_DLY, 1);
    pmm_set_domain_fsm(pd, PWR_ON_SEQ_DLY, 2);
    pmm_set_domain_fsm(pd, PWR_ON_DLY, 2);
    pmm_set_domain_fsm(pd, RST_DEASSERT_DLY, 2);
    pmm_set_domain_fsm(pd, ISO_OFF_DLY, 1);
    pmm_set_domain_fsm(pd, ISO_ON_DLY, 1);

    /* There is risk to power on BT domain due to the de-assertion of isolation follows
     * the bt async reset de-assertion
     * and this may cause the BT command error: BT domain sends out the command req
     * but it is isolated and doesn't get the response from dtop.
     * Programm the following register to 0 to de-assert the isolation
     * and bt async reset at the same time so that
     * it avoids the above situation.
     */
    pmm_bt_rst_deassert_dly_set(0);

#ifdef BUILD_CORE_CORE0
    pmm_ldo_dcdc_reg_init();

#ifdef VOLTAGE_COMPATIBLE
    pmm_use_compatible_voltage(2);
#endif

    pmm_set_dcore_wakeup_by_bt(true);

    /* enable the pmm performance monitor counter chip */
    pmm_perf_mon_cfg_in_chip_init();

    /* uvp in interrupts */
    iot_irq_t irq = iot_irq_create(PMM_BOD_INT, 0, iot_lpm_bod_int_handler);
    iot_irq_unmask(irq);
    pmm_bod_int_clear();
    pmm_bod_int_en(true);

    pmm_perf_mon_cfg_val0_source(PMM_PERF_COUNTER_AUD_PD_IN_DEEP_SLEEP);
    pmm_perf_mon_cfg_val1_source(PMM_PERF_COUNTER_DCORE_PD_IN_DEEP_SLEEP);
    /* enable early wakeup intterupt on DTOP */
    iot_lpm_early_wakeup_by_bt_init();

    intc_set_wakeup_enable(WAKEUP_CHARGER_ON_FLAG_INT);
    pmm_set_pd_wakeup_src_req(pd, PMM_DS_WAKEUP_GPI_DEB, true);
    pmm_set_pd_wakeup_src_req(pd, PMM_DS_WAKEUP_TOUCH_KEY, true);

    // only be effective when enter deep sleep
    // pmm ldo stages
    pmm_ldo_mode_entry_config(PMM_WORK_LINK_MODE);
    // DCDC ldo stages
    pmm_dcdc_ldo_mode_entry_config(PMM_WORK_LINK_MODE);

    pmm_xtal_intf_config(true);
#endif

    // All core need wakeup by pmm rtc
    iot_lpm_pmm_wakeup_rtc_init();

    pmm_set_pd_wakeup_src_req(pd, PMM_DS_WAKEUP_WIC0, true);
    pmm_set_pd_wakeup_src_req(pd, PMM_DS_WAKEUP_WIC1, true);

    iot_lpm_interrupt_init();

    // default doze sleep
    iot_lpm_enter_dozesleep();
}

bool_t iot_lpm_wakeup_from_charger(void)
{
    return pmm_wakeup_source_charger();
}

void iot_lpm_get_power_domain_status(uint8_t *dcore_status, uint8_t *bt_status, uint8_t *dsp_status,
                                     uint8_t *audif_status)
{
    *dcore_status = pmm_get_power_domain_status(PMM_PD_DCORE);
    *bt_status = pmm_get_power_domain_status(PMM_PD_BT);
    *dsp_status = pmm_get_power_domain_status(PMM_PD_AUDIO);
    *audif_status = pmm_get_power_domain_status(PMM_PD_AUDIO_INTF);
}

void iot_lpm_get_wakeup_req_src(uint16_t *dcore_src, uint16_t *bt_src, uint16_t *dsp_src)
{
    *dcore_src = pmm_get_pd_wakeup_src_req(PMM_PD_DCORE);
    *bt_src = pmm_get_pd_wakeup_src_req(PMM_PD_BT);
    *dsp_src = pmm_get_pd_wakeup_src_req(PMM_PD_AUDIO);
}

void iot_lpm_set_pd_wakeup_src_req(IOT_LPM_DS_WAKEUP_SRC src, bool_t enable)
{
    uint32_t cpu = cpu_get_mhartid();
    PMM_PWR_DOMAIN pd = pmm_get_power_domain(cpu);

    pmm_set_pd_wakeup_src_req(pd, (PMM_DS_WAKEUP_SRC)src, enable);
}

void iot_lpm_clear_all_pd_wakeup_src_req(void)
{
    uint32_t cpu = cpu_get_mhartid();
    PMM_PWR_DOMAIN pd = pmm_get_power_domain(cpu);

    pmm_clear_all_pd_wakeup_src_req(pd);
}

void iot_lpm_set_dcdc_pulse_width(uint8_t pulse_width)
{
    uint8_t width = (uint8_t)MAX(pulse_width, 15);

    pmm_ana_dcdc_enable(true);
#ifdef USE_DCDC08_HIGH_PULSE_WIDTH
    pmm_ana_dcdc_on_cfg(PMM_ANA_DCDC0P8, width);
#endif
#ifdef USE_DCDC12_HIGH_PULSE_WIDTH
    pmm_ana_dcdc_on_cfg(PMM_ANA_DCDC1P2, width);
#endif
#ifdef USE_DCDC18_HIGH_PULSE_WIDTH
    pmm_ana_dcdc_on_cfg(PMM_ANA_DCDC1P8, width);
#endif
    UNUSED(width);
}
