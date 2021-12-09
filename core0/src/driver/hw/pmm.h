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

#ifndef DRIVER_HW_PMM_H
#define DRIVER_HW_PMM_H
#ifdef __cplusplus
extern "C" {
#endif

#include "iot_memory_config.h"
#include "adi_common.h"

#if defined BUILD_CORE_CORE0
#define CPU_BOOT_ADDR DCP_ROM_START
#elif defined BUILD_CORE_CORE1
#define CPU_BOOT_ADDR BT_IRAM_START
#endif

#define   CHG_DEFAULT_RES_CODE         8
#define   DLDO06_MIN_CODE              0
#define   DLDO06_MAX_CODE              31
#define   DCDC08_MIN_CODE              0
#define   DCDC08_MAX_CODE              31
#define   DCDC18_MIN_CODE              0
#define   FLASHLDO_MIN_CODE            0
#define   PMMLDO_MIN_CODE              0
#define   DCDC_1P2_DEFAULT_VOL         1200
#define   UVP_DEFAULT_STEP_MV          100
#define   UVP_DEFAULT_CODE             5
#define   CHG_UVP_MIN_CODE             0
#define   CHG_UVP_MAX_CODE             7
#define   CHG_UVP_HYST                 200

/*dcdc1p8 deep sleep*/
#define DCDC1P8_DELTA_MV         (100) //1.8V->1.7V
#define DCDC1P8_STEP_MV          (20)

/*dcdc0p8 deep sleep*/
#define DCDC0P8_DELTA_MV         (60)  //0.8->0.74
#define DCDC0P8_STEP_MV          (8.66f)

/*flash_ldo deep sleep*/
#define FLASH_LDO_DELTA_MV        (100)  //1.8->1.7
#define FLASH_LDO_STEP_MV         (40)

/*pmm_ldo deep sleep*/
#define PMM_LDO_DELTA_MV         (70)  //0.9->0.83
#define PMM_LDO_STEP_MV          (20)

typedef enum {
    PMM_CLK_LEDC,
    PMM_CLK_PMAGIC,
    PMM_CLK_PMM_REG,
    PMM_CLK_RTC_TMR,
    PMM_CLK_IO_MUX,
    PMM_CLK_GPIO_TOP,
    PMM_CLK_WDG,
    PMM_CLK_ANAREG,
    PMM_CLK_TK,
    PMM_CLK_GPI_DEB,

    PMM_CLK_WDG_DTOP = 30,
} PMM_CLK;

typedef enum {
    PMM_DS_WAKEUP_GPIO,
    PMM_DS_WAKEUP_RTC1,
    PMM_DS_WAKEUP_RTC0,
    PMM_DS_WAKEUP_BAT_RESUME,
    PMM_DS_WAKEUP_CHARGER_ON,
    PMM_DS_WAKEUP_CHARGER_2P5,
    PMM_DS_WAKEUP_TOUCH_KEY,
    PMM_DS_WAKEUP_GPI_DEB,

#if defined(BUILD_CORE_CORE0)
    /* Dtop domain */
    PMM_DS_WAKEUP_AUDIC_WIC,
    PMM_DS_WAKEUP_BT_WIC,

    PMM_DS_WAKEUP_WIC1 = PMM_DS_WAKEUP_AUDIC_WIC,
    PMM_DS_WAKEUP_WIC0 = PMM_DS_WAKEUP_BT_WIC,

#elif defined(BUILD_CORE_CORE1)
    /* BT domain */
    PMM_DS_WAKEUP_AUDIC_WIC,
    PMM_DS_WAKEUP_DTOP_WIC,

    PMM_DS_WAKEUP_WIC1 = PMM_DS_WAKEUP_AUDIC_WIC,
    PMM_DS_WAKEUP_WIC0 = PMM_DS_WAKEUP_DTOP_WIC,

#else
    /* DSP domain */
    PMM_DS_WAKEUP_BT_WIC,
    PMM_DS_WAKEUP_DTOP_WIC,

    PMM_DS_WAKEUP_WIC1 = PMM_DS_WAKEUP_BT_WIC,
    PMM_DS_WAKEUP_WIC0 = PMM_DS_WAKEUP_DTOP_WIC,

#endif
} PMM_DS_WAKEUP_SRC;

typedef enum {
    PMM_PD_AUDIO,
    PMM_PD_BT,
    PMM_PD_DCORE,
    PMM_PD_AUDIO_INTF,

    PMM_PWR_DOMAIN_MAX,
} PMM_PWR_DOMAIN;

typedef enum {
    PMM_MEM_ON,
    PMM_MEM_RETENTION_1,
    PMM_MEM_RETENTION_2,
    PMM_MEM_OFF,
} PMM_MEM_MODE;

typedef enum {
    PMM_WORK_SHIP_MODE,
    PMM_WORK_LINK_MODE,
} PMM_WORK_MODE;

typedef enum {
    PMM_SLEEP_LIGHT_SLEEP_ENTRY,
    PMM_SLEEP_LIGHT_SLEEP_EXIT,
    PMM_SLEEP_DEEP_SLEEP_ENTRY,
    PMM_SLEEP_DEEP_SLEEP_EXIT,
} PMM_SLEEP_STATE;

typedef enum {
    PMM_SLEEP_LIGHT,
    PMM_SLEEP_FORCE_ON,
    PMM_SLEEP_DEEP,
} PMM_SLEEP_MODE;

typedef enum {
    PMM_CHIP_SLEEP_NONE,
    PMM_CHIP_SLEEP_LIGHT,
    PMM_CHIP_SLEEP_DEEP,
} PMM_CHIP_SLEEP;

typedef enum {
    SHUTDOWN_DLY,
    RST_ASSERT_DLY,
    CGM_OFF_DLY,
    CGM_ON_DLY,

    ISO_OFF_DLY,
    RST_DEASSERT_DLY,
    PWR_ON_DLY,
    PWR_ON_SEQ_DLY,

    ISO_ON_DLY,
} PMM_FSM;

/**
  * @brief Modes of interrupt. Only when gpio_mode set as GPIO_INTERRUPT,
  * int_mode is available.
  */
typedef enum {
    /**< Disable the interrupt. */
    PMM_CHG_INT_DISABLE,
    /**< Interrupt triggered when switchs from LOW to HIGH. */
    PMM_CHG_INT_EDGE_RAISING,
    /**< Interrupt triggered when switchs from HIGH to LOW. */
    PMM_CHG_INT_EDGE_FALLING,
    /**< Interrupt triggered when switchs to HIGH or LOW . */
    PMM_CHG_INT_EDGE_BOTH,
    /**< Interrupt triggered when stays in LOW. */
    PMM_CHG_INT_LEVEL_LOW,
    /**< Interrupt triggered when stays in HIGH. */
    PMM_CHG_INT_LEVEL_HIGH,
    /**< Invalid value */
    PMM_CHG_INT_MODE_MAX
} PMM_CHG_INT_MODE;

typedef enum {
    PMM_DLDO_VOL_0_6,
    PMM_DLDO_VOL_0_7,
    PMM_DLDO_VOL_0_8,
    PMM_DLDO_VOL_0_56,
    PMM_DLDO_VOL_MAX,
} PMM_DLDO_VOL;

typedef enum {
    PMM_DCDC1P8_NORMAL,
    PMM_DCDC1P8_LOWPOWER,
} PMM_DCDC1P8_VOL;

typedef enum {
    PMM_FLASHLDO_NORMAL,
    PMM_FLASHLDO_LOWPOWER,
} PMM_FLASHLDO_VOL;

typedef enum {
    PMM_DCDC1P2_1000 = 1000,
    PMM_DCDC1P2_1100 = 1100,
    PMM_DCDC1P2_1200 = 1200,
    PMM_DCDC1P2_1300 = 1300,
    PMM_DCDC1P2_1400 = 1400,
} PMM_DCDC1P2_VOL;

typedef enum {
    PMM_DCDC0P8_NORMAL,
    PMM_DCDC0P8_LOWPOWER,
} PMM_DCDC0P8_VOL;

typedef enum {
    PMM_LDO_NORMAL,
    PMM_LDO_LOWPOWER,
} PMM_LDO_VOL;

typedef enum {
    PMM_CHG_UVP_2600 = 2600,
    PMM_CHG_UVP_2700 = 2700,
    PMM_CHG_UVP_2800 = 2800,
    PMM_CHG_UVP_2900 = 2900,
    PMM_CHG_UVP_3000 = 3000,
    PMM_CHG_UVP_3100 = 3100,
    PMM_CHG_UVP_3200 = 3200,
    PMM_CHG_UVP_3300 = 3300,
} PMM_CHG_UVP_MV;

typedef enum {
    PMM_32K_SRC_RCO,
    PMM_32K_SRC_XTAL,
} PMM_32K_SRC;

typedef enum {
    PMM_PERF_COUNTER_CHIP_IN_SLEEP,
    PMM_PERF_COUNTER_CHIP_IN_DEEP_SLEEP,
    PMM_PERF_COUNTER_CHIP_IN_LIGHT_SLEEP,
    PMM_PERF_COUNTER_DCORE_PD_IN_DEEP_SLEEP,
    PMM_PERF_COUNTER_DCORE_PD_IN_LIGHT_SLEEP,
    PMM_PERF_COUNTER_BT_PD_IN_DEEP_SLEEP,
    PMM_PERF_COUNTER_BT_PD_IN_LIGHT_SLEEP,
    PMM_PERF_COUNTER_AUD_PD_IN_DEEP_SLEEP,
    PMM_PERF_COUNTER_AUD_PD_IN_LIGHT_SLEEP,
    PMM_PERF_COUNTER_AUD_INTF_PD_IN_DEEP_SLEEP,
    PMM_PERF_COUNTER_AUD_INTF_PD_IN_LIGHT_SLEEP,
    PMM_PERF_COUNTER_PMM_PERF_COUNTER_MAX,
} PMM_PERF_COUNTER;

typedef enum {
    PMM_DCDC_1P2_FW_RAMPUP,
    PMM_DCDC_1P2_FW_RAMPDOWN,
    PMM_DCDC_1P2_SLEEP_EXIT,
    PMM_DCDC_1P2_SLEEP_ENTRY,
    PMM_DCDC_1P2_BT_PA_RAMPUP,
    PMM_DCDC_1P2_BT_PA_RAMPDOWN,
} PMM_DCDC_1P2_OP;

typedef enum {
    PMM_RESET_COLD,
    PMM_RESET_WARM,
    PMM_RESET_WATCHDOG,
}PMM_RESET_CAUSE;

typedef enum {
    PMM_TOUCH_KEY_DIV_32K_FREQ,
    PMM_TOUCH_KEY_DIV_16K_FREQ,
    PMM_TOUCH_KEY_DIV_10K_FREQ,
    PMM_TOUCH_KEY_DIV_8K_FREQ
} PMM_TOUCH_KEY_DIV_FREQ;

PMM_PWR_DOMAIN pmm_get_power_domain(uint32_t cpu);
void pmm_gpio_wakeup_clear_all(void);
void pmm_chip_reset(void);
void pmm_touch_key_reset(void);
void pmm_clk_enable(PMM_CLK module);
void pmm_clk_disable(PMM_CLK module);
void pmm_clk_reset(PMM_CLK module);
void pmm_ledc_reset(void);
void pmm_ledc_clk_32k_en(bool_t enable);
void pmm_set_32k_clk_src(PMM_32K_SRC src);
void pmm_chip_force_shutdown(bool_t enable);
uint8_t pmm_get_power_domain_status(PMM_PWR_DOMAIN pd);
void pmm_set_rtc_wakeup(bool_t enable);
bool_t pmm_read_touch_key_clk_enable(void);
void pmm_set_touch_key_clk_enable(bool_t enable);
void pmm_set_touch_key_clk_freq(PMM_TOUCH_KEY_DIV_FREQ div);
void pmm_set_gpio_wakeup(bool_t enable);
void pmm_clear_gpio_wakeup(void);
void pmm_xtal_16m_force_on(bool_t enable);
void pmm_set_xtal_16m_lp_settle_time(uint8_t time);
void pmm_set_xtal_16m_off_settle_time(uint8_t time);

void pmm_power_domain_force_poweron(PMM_PWR_DOMAIN pd, bool_t enable);
void pmm_power_domain_force_powerdown(PMM_PWR_DOMAIN pd, bool_t enable);

void pmm_power_domain_poweron(PMM_PWR_DOMAIN pd);
void pmm_power_domain_poweroff(PMM_PWR_DOMAIN pd);

void pmm_power_domain_force_light_sleep(PMM_PWR_DOMAIN pd, bool_t enable);
void pmm_force_chip_light_sleep(bool_t enable);
void pmm_set_cpu_boot_addr(uint32_t cpu, uint32_t addr);
void pmm_set_pd_wakeup_src_req(PMM_PWR_DOMAIN pd, PMM_DS_WAKEUP_SRC src,
                               bool_t enable);
void pmm_clear_all_pd_wakeup_src_req(PMM_PWR_DOMAIN pd);
uint16_t pmm_get_pd_wakeup_src_req(PMM_PWR_DOMAIN pd);
void pmm_sleep_control(PMM_PWR_DOMAIN domain, PMM_SLEEP_MODE mode,
                       bool_t enable);
void pmm_set_chip_sleep(PMM_CHIP_SLEEP sleep);
void pmm_set_dcore_wakeup_by_bt(bool_t enable);
void pmm_set_sleep_ram_full_state(PMM_PWR_DOMAIN pd, PMM_SLEEP_STATE sleep,
                                  PMM_MEM_MODE mode);
void pmm_set_sleep_rom_full_state(PMM_PWR_DOMAIN pd, PMM_SLEEP_STATE sleep,
                                  PMM_MEM_MODE mode);
void pmm_set_sleep_ram_state(PMM_PWR_DOMAIN pd, PMM_SLEEP_STATE sleep,
                             uint8_t bank, PMM_MEM_MODE mode);
void pmm_power_debug_enable(bool_t enable);
void pmm_power_debug_sig_sel(uint8_t sig_sel);
void pmm_sw_wakeup_bt(void);
void pmm_sw_wakeup_bt_en(bool_t enable);
void pmm_bt_sleep_stat_ignore(bool_t enable);
void pmm_set_domain_fsm(PMM_PWR_DOMAIN domain, PMM_FSM fsm, uint8_t value);

void pmm_bt_pd_wakeup_by_bt_tmr_eb(bool_t enable);

void pmm_bt_wakeup_lp_en(bool_t enable);
void pmm_bt_timing_gen_soft_rst(void);

void pmm_set_bootstrap_cfg(uint32_t cfg);

void pmm_vbatt_bod_force_sleep_en(bool_t enable);

/* charger api */
void pmm_charger_flag_en(bool_t enable);
uint8_t pmm_charger_flag_get(void);
void pmm_charger_flag_int_en(bool_t enable);
void pmm_charger_flag_int_type(PMM_CHG_INT_MODE type);
void pmm_charger_flag_int_clear(void);
void pmm_charger_flag_wakeup_en(bool_t enable);
void pmm_charger_uvp_sel(void);
void pmm_charger_set_uvp(uint8_t chg_uvp_code);
void pmm_charger_uvp_mv_config(PMM_CHG_UVP_MV uvp_mv);
void pmm_charger_vbus_en(bool_t enable);
bool_t pmm_charger_vbus_get(void);
void pmm_charger_vbus_int_en(bool_t enable);
void pmm_charger_vbus_int_type(PMM_CHG_INT_MODE type);
void pmm_charger_vbus_int_clear(void);
void pmm_charger_vbus_wakeup_en(bool_t enable);
void pmm_charger_vbus_gpio_enable(bool_t enable);

void pmm_debug_mode_enable(bool_t enable);

void pmm_touch_key_init(bool_t enable);
void pmm_set_tk_pad_to_gpio(uint8_t pin);
void pmm_set_tk_pad_to_tk(uint8_t pin);
uint32_t pmm_read_tk_digpad_cfg(void);
uint32_t pmm_read_clk_cfg(void);
PMM_32K_SRC pmm_get_32k_clk_src(void);

void pmm_ldo_mode_entry_config(PMM_WORK_MODE mode);
void pmm_dcdc_ldo_mode_entry_config(PMM_WORK_MODE mode);

int8_t pmm_use_compatible_voltage(int8_t value);
void pmm_dldo_set_value(uint8_t vref);
void pmm_dldo_set_bypass(void);
void pmm_dldo_config(PMM_DLDO_VOL vol);
void pmm_dcdc_ldo_init(void);
void pmm_dcdc1p8_config(PMM_DCDC1P8_VOL vol);
void pmm_flash_ldo_config(PMM_FLASHLDO_VOL vol);
void pmm_set_dcdc_0p8(uint8_t vref);
void pmm_dcdc_0p8_config(PMM_DCDC0P8_VOL vol);
void pmm_get_pmm_ldo_config(uint32_t value[4]);

void pmm_set_charger_current(uint16_t cur_0_1ma);
void pmm_set_charger_current_config(uint8_t seg, uint8_t code);

/**
 * @brief This function is used to deep sleep charger current set.
 *
 * @param cur_0_1ma is current to set, unit 0.1mA
 */
void pmm_set_dp_charger_current(uint16_t cur_0_1ma);
void pmm_perf_mon_cfg_in_chip_init(void);
void pmm_perf_mon_cfg_val0_source(PMM_PERF_COUNTER value);
void pmm_perf_mon_cfg_val1_source(PMM_PERF_COUNTER value);
uint32_t pmm_perf_mon_val0_get(void);
uint32_t pmm_perf_mon_val1_get(void);
void pmm_set_mem_timing_control(void);
void pmm_set_dcdc_1p2(PMM_DCDC_1P2_OP op, uint8_t stage, const uint8_t *value);
int32_t pmm_dcdc_1p2_get(PMM_DCDC1P2_VOL vol);
void pmm_dcdc_1p2_set(PMM_DCDC1P2_VOL vol);
void pmm_set_pmm_ldo(uint8_t vsel);
void pmm_pmm_ldo_config(PMM_LDO_VOL vol);
void pmm_ldo_dcdc_reg_init(void);
void pmm_set_aon3p1_ldo(uint8_t vsel);
void pmm_aon3p1_config(void);
void pmm_set_push_button_io_enable(bool_t en);

void pmm_dbg_sig_sel_set(uint8_t value);
void pmm_dbg_ctrl_eb_set(uint8_t value);
void pmm_obs_en_set(uint8_t value);
void pmm_obs_frc_set(uint8_t value);
void pmm_bt_rst_deassert_dly_set(uint8_t value);

void pmm_xtal_intf_config(bool_t enable);

bool_t pmm_wakeup_source_charger(void);

void pmm_set_scratch0_register(uint32_t val);
uint32_t pmm_get_scratch0_register(void);

void pmm_set_scratch1_register(uint32_t val);
uint32_t pmm_get_scratch1_register(void);

void pmm_set_scratch2_register(uint32_t val);
uint32_t pmm_get_scratch2_register(void);

void pmm_set_scratch3_register(uint32_t val);
uint32_t pmm_get_scratch3_register(void);

PMM_RESET_CAUSE pmm_get_reset_cause(void);
void pmm_clear_reset_cause(PMM_RESET_CAUSE reset);

uint32_t pmm_get_wakeup_source(void);
PMM_CHG_INT_MODE pmm_get_charger_flag_int_type(void);

void pmm_set_wdg_reset_mode(bool_t chip_reset);

void pmm_dcdc_1p2_vref_soft_reset(void);

uint32_t pmm_get_reset_cause_register(void);

void pmm_bt_pd_wakeup_by_bt_tmr_clr(bool_t enable);

void pmm_set_pmm_general_cfg2_wire(ADI_WIRE_MODE wire_mode);

//just use in sbl
void pmm_set_charger_cur_default(void);

void pmm_enable_gpio01_debug_mode(void);

//rtc_cycle max is 0xf
void pmm_memory_dfs_config(uint8_t vol);
void pmm_bod_int_en(bool_t en);
void pmm_bod_int_clear(void);

#ifdef __cplusplus
}
#endif

#endif /* DRIVER_HW_PMM_H */
