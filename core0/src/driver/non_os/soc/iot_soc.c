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

#include "iot_soc.h"
#include "iot_dsp.h"
#include "iot_timer.h"
#include "iot_resource.h"
#include "iot_clock.h"
#include "iot_rtc.h"
#include "iot_lpm.h"

/* hw includes */
#include "apb.h"
#include "pmm.h"
#include "adi_master.h"
#include "adi_slave.h"
#include "bingo_pmm.h"
#include "bingo_ana.h"
#include "pll_cal.h"
#include "spinlock.h"
#include "rtc.h"
#include "pin.h"
#include "dtop_ana.h"
#include "aud_intf_pwr.h"
#include "clock.h"

#ifdef LOW_POWER_ENABLE
#include "dev_pm.h"
#endif

#if defined(BUILD_CORE_CORE1)
#include "wdt.h"
#endif

#define IOT_SOC_BOOT_MAGIC   0x12345678

static int soc_initialed = 0;

#ifdef LOW_POWER_ENABLE
static uint32_t iot_soc_chip_restore(uint32_t data);
static struct pm_operation soc_pm;
#endif

#ifdef LOW_POWER_ENABLE
static uint32_t iot_soc_chip_restore(uint32_t data) IRAM_TEXT(iot_soc_chip_restore);
static uint32_t iot_soc_chip_restore(uint32_t data)
{
    UNUSED(data);
#if defined(BUILD_CORE_CORE0)
    iot_soc_chip_init(false);
#elif defined(BUILD_CORE_CORE1)
    apb_brg_switch_mode_cfg(APB_SYS_BRG_AON2BT, APB_BRG_SWITCH_BYPASS_MODE);
    if (clock_get_core_clock(1) == CLOCK_CORE_16M) {
        apb_brg_switch_mode_cfg(APB_BT_AHB_BRG, APB_BRG_SWITCH_BYPASS_MODE);
    }
#endif
    return RET_OK;
}
#endif

void iot_soc_madc_config(void) IRAM_TEXT(iot_soc_madc_config);
void iot_soc_madc_config(void)
{
    bingo_ana_bg_tune_config();
    bingo_pmm_crystl_current_config();
    bingo_ana_ic25ua_out_tune_config();
}

/* the configuration sequence of adi wire mode
 * a): configure the bingo pmm adr ctr
 * b): configure the pmm general cfg2
 * c): configure the audio adi tx/rx
 * d): configure the adi slave sync
 *  NOTE:
 *    1): never try to read adie reg between a) and b)
 *    2): delay 4us after configured wire mode, for there is a entry fifo there
 */
void iot_soc_adi_wire_mode_config(ADI_WIRE_MODE wire_mode)
{
    cpu_critical_enter();
    bingo_pmm_bingo_adr_ctrl_wire(wire_mode);
    pmm_set_pmm_general_cfg2_wire(wire_mode);
    //TODO need delay 4us for the worst case: fifo is 12
    adi_master_audio_channel_sync(wire_mode);
    adi_slave_sync();
    cpu_critical_exit();
}

void iot_soc_ldo_config(void)
{
    // bypass to normal pmm ldo
    pmm_dcdc_ldo_init();

    pmm_pmm_ldo_config(PMM_LDO_NORMAL);
    pmm_flash_ldo_config(PMM_FLASHLDO_NORMAL);
    pmm_dcdc_1p2_set(PMM_DCDC1P2_1200);
    pmm_dcdc_0p8_config(PMM_DCDC0P8_NORMAL);
    bingo_pmm_crystl_current_config();
    pmm_aon3p1_config();
    bingo_pmm_xtal_ldo_config();

    // set dcdc ldo to 0.6
    apb_memory_dfs_config(APB_MEM_DVS_0_6);
    pmm_memory_dfs_config(APB_MEM_DVS_0_6);
    pmm_dldo_config(PMM_DLDO_VOL_0_6);
}

void iot_soc_chip_init(bool cold_boot) IRAM_TEXT(iot_soc_chip_init);
void iot_soc_chip_init(bool cold_boot)
{
    UNUSED(cold_boot);
#ifdef LOW_POWER_ENABLE
    if (soc_initialed == 0) {
        list_init(&soc_pm.node);
        soc_pm.restore = iot_soc_chip_restore;
        iot_dev_pm_register(&soc_pm);
    }
#endif
    // Branch prediction function enable
    apb_btb_enable((uint8_t)cpu_get_mhartid(), true);

    apb_set_cpu_boot_magic((uint8_t)cpu_get_mhartid(), IOT_SOC_BOOT_MAGIC);

    // Data abort exception enable now.
    iot_soc_cpu_exception_enable(true);

#if defined(BUILD_CORE_CORE0)
#ifdef BUILD_COMMON_ADI2PMM_PATH
    apb_pmm_access_path_enable(false);
#endif

    if (soc_initialed == 0) {
#ifdef ENABLE_DCDC_1P8
        pmm_dcdc1p8_config(PMM_DCDC1P8_NORMAL);
#endif
        // Increase dcdc pulse width
        iot_lpm_set_dcdc_pulse_width(0xF);
    }

    iot_soc_madc_config();

    // Set adi fifo depth to 1
    adi_master_fifo_depth(0);

    //adi master slave sync
    iot_soc_adi_wire_mode_config(ADI_WIRE_MODE_2);

    // Disable debug mode
    pmm_debug_mode_enable(false);

    pll_sleep_auto_wake_en(true);

#ifndef AUD_INTF_PWR
    /* power on audio interface */
    iot_soc_audio_intf_power_up();
#endif
    dev_spinlock_init();

    apb_set_dws_wait_timing(0);
#elif defined(BUILD_CORE_CORE1)
    apb_brg_switch_mode_cfg(APB_SYS_BRG_AON2BT, APB_BRG_SWITCH_BYPASS_MODE);
    if (clock_get_core_clock(1) == CLOCK_CORE_16M) {
        apb_brg_switch_mode_cfg(APB_BT_AHB_BRG, APB_BRG_SWITCH_BYPASS_MODE);
    }

    /* disable core2's watchdog */
    wdt_deinit(WATCHDOG2);
#elif defined(BUILD_CORE_DSP)
#endif

    if (soc_initialed == 0) {
        /* timing gen in pmm should reset before irq init */
        pmm_bt_timing_gen_soft_rst();
        soc_initialed = 1;
    }

    /* Clear scratch1 register in any case,
       because if boot with scratch1 register(>0), will cause
       iot_suspend_sched task can't exit */
    pmm_set_scratch1_register(0x0);
}

void iot_soc_chip_pmm_init(void)
{
    // Revert the shutdown config to remove the influence towards the deep sleep.
    bingo_pmm_shutdown_reset();

    // Enable auto clock gating for PMM reg access
    pmm_clk_disable(PMM_CLK_PMM_REG);

    // Force shutdown BT_PD, AUD_PD and DUDIO_INTF_PD
    iot_dsp_power_off();
    // iot_soc_bt_power_off();
    iot_soc_audio_intf_power_off();

    // SOC clock switching to desired clock XTAL
    iot_clock_set_mode(IOT_CLOCK_MODE_0);

    // Disable clock of unused blocks
    apb_ahb_hclk_disable(APB_AHB_SW_DMA0);

    // Disable DIG_PAD function shared on TouchKey
    pmm_touch_key_init(true);
    for (uint8_t i = PIN_72; i <= PIN_75; i++) {
        pin_set_pull_mode(i, PIN_PULL_UP);
    }

    // Disable Meter ADC
    dtop_ana_gpio_init();
    bingo_ana_mic_adc_enable(BINGO_ANA_FIRST_MIC, false);

    // Disable MIC bias
    bingo_ana_micbias_enable(BINGO_ANA_MICBIAS_FIRST_STAGE, false);
    bingo_ana_micbias_enable(BINGO_ANA_MICBIAS_SECOND_STAGE, false);

    // Disable speakers nothing to do

    // Turn off SYSPLL LDO
    bingo_pmm_pll_enable(false);

    iot_soc_ldo_config();

#if (RTC_CLK_HZ == XTAL_32K_CLK_HZ)
    pmm_set_32k_clk_src(PMM_32K_SRC_XTAL);
#else
    pmm_set_32k_clk_src(PMM_32K_SRC_RCO);
#endif
}

void iot_soc_bt_clk_enable(void)
{
    apb_bt_clk_enable();
}

void iot_soc_bt_clk_disable(void)
{
    apb_bt_clk_disable();
}

void iot_soc_bt_phy_clk_enable(void)
{
    apb_bt_phy_clk_enable();
}

void iot_soc_bt_phy_clk_disable(void)
{
    apb_bt_phy_clk_disable();
}

void iot_soc_ahb_bt_async_enable(void)
{
    apb_ahb_bt_async_enable();
}

void iot_soc_ahb_bt_async_disable(void)
{
    apb_ahb_bt_async_disable();
}

void iot_soc_bt_cpu_access_enable(void)
{
    apb_bt_cpu_access_enable();
}

void iot_soc_bt_cpu_access_disable(void)
{
    apb_bt_cpu_access_disable();
}

void iot_soc_clk_bt_ahb_frc_enable(void)
{
    apb_clk_bt_ahb_frc_enable();
}

void iot_soc_clk_bt_ahb_frc_disable(void)
{
    apb_clk_bt_ahb_frc_disable();
}

void iot_soc_bt_osc_enable(void)
{
    apb_bt_osc_enable();
}

void iot_soc_bt_osc_disable(void)
{
    apb_bt_osc_disable();
}

void iot_soc_bt_soft_reset(IOT_SOC_BT_SUB_MODULE module)
{
    apb_bt_soft_reset((BT_SUB_MODULE)module);
}

void iot_soc_bt_power_up(void)
{
    pmm_power_domain_poweron(PMM_PD_BT);
    bingo_pmm_phy_dac_clock_sel();
}

void iot_soc_bt_power_off(void)
{
    pmm_power_domain_poweroff(PMM_PD_BT);
}

void iot_soc_audio_intf_power_up(void)
{
    pmm_power_domain_poweron(PMM_PD_AUDIO_INTF);
}

void iot_soc_audio_intf_power_off(void)
{
    pmm_power_domain_poweroff(PMM_PD_AUDIO_INTF);
}

uint32_t iot_soc_audio_intf_power_mask(void)
{
    return aud_intf_pwr_get_mask();
}

void iot_soc_bt_start(uint32_t start_pc)
{
    apb_disable_bt_rom_lp_mode();

    pmm_set_cpu_boot_addr(0x1, start_pc);

    apb_cpu1_enable();
}

void iot_soc_chip_reset(void)
{
    pmm_chip_reset();
}

bool_t iot_soc_bist_run(void)
{
    uint32_t test_vector[3] = {0x11111111, 0x22222222, 0x33333333};
    uint32_t mst_err_cnt;
    uint32_t slv_err_cnt;
    bool_t ret = false;

    adi_slave_bist_set_vector(test_vector);

    adi_master_bist_set_vector(test_vector);

    iot_timer_delay_us(100);

    adi_master_bist_start();

    iot_timer_delay_us(100);

    mst_err_cnt = adi_master_bist_get_error_count();
    slv_err_cnt = adi_slave_bist_get_error_count();

    if (mst_err_cnt == 0x0 && slv_err_cnt == 0x0) {
        ret = true;
    }

    return ret;
}

void iot_soc_set_soft_reset_flag(uint32_t val)
{
    pmm_set_scratch0_register(val);
}

uint32_t iot_soc_get_soft_reset_flag(void)
{
    return pmm_get_scratch0_register();
}

IOT_SOC_RESET_CAUSE iot_soc_get_reset_cause(void)
{
    return (IOT_SOC_RESET_CAUSE)pmm_get_reset_cause();
}

void iot_soc_clear_reset_cause(IOT_SOC_RESET_CAUSE reset)
{
    pmm_clear_reset_cause((PMM_RESET_CAUSE)reset);
}

uint32_t iot_soc_get_wakeup_source(void)
{
    return pmm_get_wakeup_source();
}

uint16_t iot_soc_get_wakeup_source_req(void)
{
    return pmm_get_pd_wakeup_src_req(PMM_PD_DCORE);
}

void iot_soc_save_boot_reason(uint32_t val)
{
    apb_set_scratch0_register(val);
}

uint32_t iot_soc_restore_boot_reason(void)
{
    return apb_get_scratch0_register();
}

uint32_t iot_soc_reset_cause_get(void)
{
    return pmm_get_reset_cause_register();
}

uint32_t iot_soc_get_reset_flag(void)
{
    return apb_get_reset_flag();
}

void iot_soc_cpu_exception_enable(bool_t enable)
{
    uint32_t cpu = cpu_get_mhartid();
    switch (cpu) {
        case 0:
            apb_cpu0_exception_enable(enable);
            break;
        case 1:
            apb_cpu1_exception_enable(enable);
            break;
        case 2:
            apb_cpu2_exception_enable(enable);
            break;
        default:
            break;
    }
}

void iot_soc_cpu_access_enable(IOT_SOC_CPU_ACCESS_SLAVE_PORT port, bool_t enable)
    IRAM_TEXT(iot_soc_cpu_access_enable);
void iot_soc_cpu_access_enable(IOT_SOC_CPU_ACCESS_SLAVE_PORT port, bool_t enable)
{
    uint32_t cpu = cpu_get_mhartid();
    switch (cpu) {
        case 0:
            apb_cpu0_access_enable((APB_CPU_ACCESS_SLAVE_PORT)port, enable);
            break;
        case 1:
            apb_cpu1_access_enable((APB_CPU_ACCESS_SLAVE_PORT)port, enable);
            break;
        case 2:
            apb_cpu2_access_enable((APB_CPU_ACCESS_SLAVE_PORT)port, enable);
            break;
        default:
            break;
    }
}

uint32_t iot_soc_register_read(uint32_t addr)
{
    assert(!(addr & 0x03));
    uint32_t ret;
    if (bingo_pmm_regaddr_is_in_range(addr)) {
        ret = bingo_pmm_read(addr);
    } else {
        /*default for general purpose register*/
        ret = *(volatile uint32_t *)(addr);
    }
    return ret;
}

uint32_t iot_soc_register_write(uint32_t addr, uint32_t val)
{
    assert(!(addr & 0x03));
    uint32_t ret;
    if (bingo_pmm_regaddr_is_in_range(addr)) {
        *(volatile uint32_t *)(addr) = val;
        iot_timer_delay_us(10);
        /*used for double check*/
        ret = bingo_pmm_read(addr);
    } else {
        /*default for core0 general purpose register*/
        *(volatile uint32_t *)(addr) = val;
        /*used for double check*/
        ret = *(volatile uint32_t *)(addr);
    }
    return ret;
}

uint32_t iot_soc_get_cpu_reset_flag(void)
{
    return pmm_get_scratch1_register();
}

void iot_soc_clear_cpu_reset_flag(void)
{
    pmm_set_scratch1_register(0x0);
}

union suspend_vote {
    uint32_t w;
    struct {
        uint32_t ref_count:31;
        uint32_t suspended:1;
    } b;
};

uint32_t iot_soc_inc_task_suspend_vote(void) IRAM_TEXT(iot_soc_inc_task_suspend_vote);
uint32_t iot_soc_inc_task_suspend_vote(void)
{
    uint32_t mask = dev_spinlock_lock(SUSPEND_TASK_LOCK);

    union suspend_vote ref;
    ref.w = pmm_get_scratch1_register();
    ref.b.ref_count++;
    pmm_set_scratch1_register(ref.w);

    dev_spinlock_unlock(SUSPEND_TASK_LOCK, mask);

    return ref.b.ref_count;
}

uint32_t iot_soc_set_task_suspend_vote(void) IRAM_TEXT(iot_soc_set_task_suspend_vote);
uint32_t iot_soc_set_task_suspend_vote(void)
{
    uint32_t mask = dev_spinlock_lock(SUSPEND_TASK_LOCK);

    union suspend_vote ref;
    ref.w = pmm_get_scratch1_register();
    if (ref.b.ref_count) {
        ref.b.suspended = 1;
    }
    pmm_set_scratch1_register((uint32_t)ref.w);

    dev_spinlock_unlock(SUSPEND_TASK_LOCK, mask);

    return ref.b.suspended;
}

uint32_t iot_soc_get_task_suspend_vote(void) IRAM_TEXT(iot_soc_get_task_suspend_vote);
uint32_t iot_soc_get_task_suspend_vote(void)
{
    uint32_t mask = dev_spinlock_lock(SUSPEND_TASK_LOCK);

    union suspend_vote ref;
    ref.w = pmm_get_scratch1_register();
    dev_spinlock_unlock(SUSPEND_TASK_LOCK, mask);

    return ref.b.suspended;
}

uint32_t iot_soc_clear_task_suspend_vote(void) IRAM_TEXT(iot_soc_clear_task_suspend_vote);
uint32_t iot_soc_clear_task_suspend_vote(void)
{
    uint32_t mask = dev_spinlock_lock(SUSPEND_TASK_LOCK);

    union suspend_vote ref;
    ref.w = pmm_get_scratch1_register();
    ref.b.ref_count--;
    if (ref.b.ref_count == 0) {
        ref.b.suspended = 0;
    }
    pmm_set_scratch1_register(ref.w);

    dev_spinlock_unlock(SUSPEND_TASK_LOCK, mask);

    return ref.b.ref_count;
}

void iot_soc_set_ppm(uint8_t ppm)
{
    bingo_pmm_crystl_tn_rng_cap_arry_set(ppm);
    bingo_pmm_l2n_crystl_tn_rng_cap_arry_set(ppm);
}

bool iot_soc_wakeup_source_is_vbat_resume(void)
{
    return !!(pmm_get_wakeup_source() & (1<<PMM_DS_WAKEUP_BAT_RESUME));
}
