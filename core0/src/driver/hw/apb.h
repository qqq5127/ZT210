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
#ifndef _DRIVER_HW_APB_H
#define _DRIVER_HW_APB_H

#include "types.h"
#include "uart.h"

enum {
    APB_CPU_0,  // DTOP cpu
    APB_CPU_1,  // BT cpu0
    APB_CPU_2,  // BT cpu1
    APB_CPU_3,  // DSP
};

/*
* APB modules id
*/
typedef enum {
    // DTOP
    APB_CLK_GLB0 = 0,
    APB_CLK_UART0,
    APB_CLK_UART1,
    APB_CLK_WIC,
    APB_CLK_PMON,
    APB_CLK_GTMR0,
    APB_CLK_RESERVED_0,
    APB_CLK_INTC0,
    APB_CLK_WDG0,
    APB_CLK_GPIO,
    APB_CLK_SPI0_M,
    APB_CLK_SPI1_M,
    APB_CLK_SPI2_M,
    APB_CLK_SPI0_S,
    APB_CLK_ADA0,
    APB_CLK_VAD,
    APB_CLK_MADC,
    APB_CLK_IIC0,
    APB_CLK_IIC1,
    APB_CLK_LEDC,
    APB_CLK_RTC_TMR0,
    APB_CLK_RESERVED_1,
    APB_CLK_RESERVED_2,
    APB_CLK_PIN_REG,
    APB_CLK_DTOP_MAX = 31,

    // BT
    APB_CLK_GLBREG1 = 32,
    APB_CLK_GTMR1,
    APB_CLK_INTC1,
    APB_CLK_WDG1,
    APB_CLK_RTC_TIMER1,
    APB_CLK_SPINLOCK1,
    APB_CLK_FIREWALL1,
    APB_CLK_MAILBOX1,
    APB_CLK_RV5_CORE1_CLK,
    APB_CLK_RTC_MON,
    APB_CLK_FMST,
    APB_CLK_RV5_CORE2_CLK,
    APB_CLK_INTC2,
    APB_CLK_BT_ADR_CLK,
    APB_CLK_SW_DMA1,
    APB_CLK_WDG2,
    APB_CLK_BCP_WIC,
    APB_CLK_BT_MAX = 63,

    // AUDIO
    APB_CLK_GLBREG2 = 64,
    APB_CLK_GTMR2,
    APB_CLK_DSP_INTC,
    APB_CLK_WDG3,
    APB_CLK_RTC_TIMER2,
    APB_CLK_SPINLOCK2,
    APB_CLK_FIREWALL2,
    APB_CLK_SW_DMA2,
    APB_CLK_MINI_CACHE,
    APB_CLK_ACP_WIC,
    APB_CLK_RESERVED_3,
    APB_CLK_SMC,
    APB_CLK_SMC_CACHE,
    APB_CLK_AUD_ADI,
    APB_CLK_DSP,
    APB_CLK_AUDIO_MAX = 95,
} APB_CLK;

typedef enum {
    APB_AHB_ROM,
    APB_AHB_DBG_TOP,
    APB_AHB_RESERVED_0,
    APB_AHB_SW_DMA0,
    APB_AHB_AUD_ACC,
    APB_AHB_SFC_REG,

    APB_AHB_GROUP0_HCLK_MAX = 31,

    APB_AHB_SPINLOCK,
    APB_AHB_CACHE,
    APB_AHB_MAILBOX0,
    APB_AHB_FIREWALL,
    APB_AHB_CLK_REG,
    APB_AHB_PMU,
    APB_AHB_ADI,
    APB_AHB_HCLK_MAX = 64,
} APB_AHB_HCLK;

typedef enum {
    APB_M_S_RV5_CORE0_CLK,
    APB_M_S_HW_DMA_CLK,
    APB_M_S_HCLK_FUNC_DMA,
    APB_M_S_VAD_CLK,
    APB_M_S_MADC_CLK,

    APB_M_S_CACHE0_CLK = 17,
    APB_M_S_EMC_CLK,
    APB_M_S_JTAG_TOP_CLK,
    APB_M_S_UART_MEM,

    APB_M_S_CLK_MAX = 32,
} APB_M_S_CLK;

typedef enum {
    BT_SUB_MODULE_OSC,
    BT_SUB_MODULE_LC,
    BT_SUB_MODULE_PHY,
    BT_SUB_MODULE_PHY_REG,
    BT_SUB_MODULE_IP_REG,
    BT_SUB_MODULE_CP,
    BT_SUB_MODULE_MAX,
} BT_SUB_MODULE;

typedef enum {
    /* IROM :
     * 0x26000000 ~ 0x2607ffff   512K
     * 0x260e0000 ~ 0x260fffff   128K
     */
    DSP_ROM_MEM_LAYOUT_MODE_0,

    /* IROM :
     * 0x26000000 ~ 0x2607ffff   512K
     * DROM :
     * 0x26500000 ~ 0x2651ffff   128K
     */
    DSP_ROM_MEM_LAYOUT_MODE_1,

    /* IROM :
     * 0x26000000 ~ 0x2607ffff   512K
     * 0x260e0000 ~ 0x260effff    64K
     * DROM :
     * 0x26500000 ~ 0x2650ffff    64K
     */
    DSP_ROM_MEM_LAYOUT_MODE_2,
    DSP_ROM_MEM_LAYOUT_MODE_MAX,
} DSP_ROM_MEM_LAYOUT_MODE;

typedef enum {
    /* IRAM :
     * 0x26080000 ~ 0x260bffff   256K
     * DRAM :
     * 0x263e0000 ~ 0x2641ffff   256K
     */
    DSP_RAM_MEM_LAYOUT_MODE_0,

    /* IRAM :
     * 0x26080000 ~ 0x260dffff   384K
     * DRAM :
     * 0x263e0000 ~ 0x263fffff   128K
     */
    DSP_RAM_MEM_LAYOUT_MODE_1,

    /* IRAM :
     * 0x26080000 ~ 0x2609ffff   128K
     * DRAM :
     * 0x263c0000 ~ 0x2641ffff   384K
     */
    DSP_RAM_MEM_LAYOUT_MODE_2,

    /* DRAM :
     * 0x263c0000 ~ 0x2643ffff   512K
     */
    DSP_RAM_MEM_LAYOUT_MODE_3,

    /* IRAM (from DTOP memory):
     * 0x26080000 ~ 0x2609ffff   128K
     * DRAM :
     * 0x263c0000 ~ 0x2643ffff   512K
     */
    DSP_RAM_MEM_LAYOUT_MODE_4,
    DSP_RAM_MEM_LAYOUT_MODE_MAX,
} DSP_RAM_MEM_LAYOUT_MODE;

typedef enum {
    APB_SLEEP_NONE,
    APB_SLEEP_LIGHT,
    APB_SLEEP_DEEP,
} APB_SLEEP_TYPE;

typedef enum {
    APB_SUB_SYS_DTOP,
    APB_SUB_SYS_BT,
    APB_SUB_SYS_AUD,
    APB_SUB_SYS_UNKONWN,
} APB_SUB_SYS;

typedef enum {
    APB_DTOP_ROM_PATCH,
    APB_BT_ROM_PATCH,
    APB_DSP_ROM_PATCH,
} APB_ROM_PATCH_GROUP;

typedef enum {
    APB_ROM_PATCH0,
    APB_ROM_PATCH1,
} APB_ROM_PATCH_ID;

typedef enum {
    APB_SYS_BRG_AON2BT,
    APB_SYS_BRG_AON2AUD,
    APB_SYS_BRG_BT2AUD,
    APB_BT_AHB_BRG,
    APB_BT_AHB_MAX,
} APB_SYS_BRG;

typedef enum {
    APB_BRG_SWITCH_FIFO_MODE,
    APB_BRG_SWITCH_BYPASS_MODE,
} APB_BRG_SWITCH_MODE;

typedef enum {
    APB_SYS_HW_SCORE_CPU0_WFI,
    APB_SYS_HW_SCORE_CPU1_WFI,
    APB_SYS_HW_SCORE_BT_EN,
    APB_SYS_HW_SCORE_BT_PHY_EN,
    APB_SYS_HW_SCORE_SW_DMA1_EN,
    APB_SYS_HW_SCORE_BT_DEEP_SLEEP_STAT,
    APB_SYS_HW_SCORE_BT_OSC_EN,
    APB_SYS_HW_SCORE_RESV,
} APB_SYS_HW_SCORE;

typedef enum {
    APB_MEM_DVS_0_6,
    APB_MEM_DVS_0_7,
    APB_MEM_DVS_0_8,
    APB_MEM_DVS_MAX,
} APB_MEM_DVS;

typedef enum {
    APB_CLK_SYS_DTOP,
    APB_CLK_SYS_ACP,
    APB_CLK_SYS_BCP,
    APB_CLK_SYS_AUD_INF,
    APB_CLK_SYS_MAX,
} APB_ROOT_CLK_SYS;

typedef enum {
    APB_ROOT_CLK_FRC_EN,
    APB_ROOT_CLK_SLP_AUTO_GATE,
    APB_ROOT_CLK_RESET_AUTO_GATE,
    APB_TOP_CLK_FRC_EN,
    APB_TOP_CLK_SLP_AUTO_GATE,
    APB_TOP_CLK_RESET_AUTO_GATE,
} APB_SYS_ROOT_CLK;

typedef enum {
    APB_CPU_ACCESS_JTAG,
    APB_CPU_ACCESS_DTOP_ROM,
    APB_CPU_ACCESS_DTOP_PERI,
    APB_CPU_ACCESS_DTOP_IRAM,
    APB_CPU_ACCESS_DTOP_FLASH,
    APB_CPU_ACCESS_DTOP_NULL,
    APB_CPU_ACCESS_BT_ROM,
    APB_CPU_ACCESS_BT_IRAM,
    APB_CPU_ACCESS_BT_NULL,
    APB_CPU_ACCESS_BT_PERI,
    APB_CPU_ACCESS_DSP_RAM,
    APB_CPU_ACCESS_DSP_PERI,
    APB_CPU_ACCESS_DSP_DDR,
    APB_CPU_ACCESS_DSP_NULL0,
    APB_CPU_ACCESS_DSP_NULL1,
    APB_CPU_ACCESS_ZERO,
}APB_CPU_ACCESS_SLAVE_PORT;

APB_SUB_SYS apb_get_sub_sys(uint32_t cpu);
void apb_clk_enable(APB_CLK module);
void apb_clk_disable(APB_CLK module);
void apb_clk_reset(APB_CLK module);
void apb_m_s_enable(APB_M_S_CLK module);
void apb_m_s_disable(APB_M_S_CLK module);
void apb_m_s_reset(APB_M_S_CLK module);
void apb_ahb_hclk_enable(APB_AHB_HCLK module);
void apb_ahb_hclk_disable(APB_AHB_HCLK module);
void apb_ahb_hclk_reset(APB_AHB_HCLK module);
void apb_misc_clk_enable(uint8_t module);
void apb_misc_clk_disable(uint8_t module);
void apb_mclk_out_enable(void);
void apb_mclk_out_disable(void);
void apb_bcp_enable(void);
void apb_bcp_disable(void);
void apb_bcp_soft_reset(void);
void apb_acp_enable(void);
void apb_acp_disable(void);
void apb_dsp_enable(void);
void apb_dsp_disable(void);
void apb_acp_soft_reset(void);
void apb_i2s_soft_start(int direct);
void apb_i2s_soft_stop(int direct);
void apb_bt_clk_enable(void);
void apb_bt_clk_disable(void);
void apb_bt_phy_clk_enable(void);
void apb_bt_phy_clk_disable(void);
void apb_ahb_bt_async_enable(void);
void apb_ahb_bt_async_disable(void);
void apb_bt_cpu_access_enable(void);
void apb_bt_cpu_access_disable(void);
void apb_clk_bt_ahb_frc_enable(void);
void apb_clk_bt_ahb_frc_disable(void);
void apb_bt_osc_enable(void);
void apb_bt_osc_disable(void);
void apb_bt_soft_reset(BT_SUB_MODULE module);
void apb_dsp_stall_enable(void);
void apb_dsp_stall_disable(void);
void apb_dsp_misc_ctl_in(uint32_t value);
void apb_cpu1_enable(void);
void apb_cpu2_enable(void);
void apb_cpu2_reset(void);
void apb_set_cpu1_pcaddr(uint32_t pc);
void apb_set_cpu2_pcaddr(uint32_t pc);
void apb_dsp_set_pcaddr(uint32_t pc);
void apb_dsp_switch_rom_layout(DSP_ROM_MEM_LAYOUT_MODE mode);
void apb_dsp_switch_ram_layout(DSP_RAM_MEM_LAYOUT_MODE mode);
void apb_dsp_enable_wfi_mode_sel(bool_t enable);
void apb_cpu_int_mask(uint8_t cpu, bool_t meip, bool_t msip, bool_t mtip);
void apb_sys_root_clk_enable(APB_ROOT_CLK_SYS sys, APB_SYS_ROOT_CLK clk, bool_t enable);
void apb_sub_sys_sleep_enable(APB_SUB_SYS sys, APB_SLEEP_TYPE sleep);
void apb_wakeup_int_process_done(APB_SUB_SYS sys);
void apb_pmm_access_path_enable(bool_t enable);
void apb_dbus_config(uint8_t group, uint8_t sel);
void apb_deb_clk_div_sel_set(uint8_t deb_div_sel);
void apb_misc_remapsfc_enable(bool_t enable);
void apb_brg_switch_mode_cfg(APB_SYS_BRG brg, APB_BRG_SWITCH_MODE mode);
void apb_rom_patch_enable(APB_ROM_PATCH_GROUP group, APB_ROM_PATCH_ID patch_id);
void apb_rom_patch_disable(APB_ROM_PATCH_GROUP group,
                           APB_ROM_PATCH_ID patch_id);
void apb_set_rom_patch_addr(APB_ROM_PATCH_GROUP group,
                            APB_ROM_PATCH_ID patch_id, uint8_t member,
                            uint32_t addr);
void apb_set_rom_patch_data(APB_ROM_PATCH_GROUP group,
                            APB_ROM_PATCH_ID patch_id, uint8_t member,
                            uint32_t data);
void apb_ada_src_data_sel(uint8_t data_sel);
uint32_t apb_get_chip_io_cfg(void);
void apb_disable_bt_rom_lp_mode(void);
void apb_bcp_set_slp_hw_vote(uint32_t hw_componet_id);
void apb_bcp_clr_slp_hw_vote(uint32_t hw_componet_id);
void apb_cpu0_exception_enable(bool_t enable);
void apb_cpu1_exception_enable(bool_t enable);
void apb_cpu2_exception_enable(bool_t enable);
void apb_cpu0_access_enable(APB_CPU_ACCESS_SLAVE_PORT port, bool_t enable);
void apb_cpu1_access_enable(APB_CPU_ACCESS_SLAVE_PORT port, bool_t enable);
void apb_cpu2_access_enable(APB_CPU_ACCESS_SLAVE_PORT port, bool_t enable);

void apb_memory_dfs_config(APB_MEM_DVS cfg);
uint32_t apb_rom_patch_get_status(APB_ROM_PATCH_GROUP group,
                                 APB_ROM_PATCH_ID patch_id);

uint32_t apb_dbus_status0_get(void);
void apb_bcp_dbus_sel_set(uint8_t value);
void apb_acp_dbus_sel_set(uint8_t value);

void apb_btb_enable(uint8_t cpu, bool_t enable);

void apb_set_scratch0_register(uint32_t val);
uint32_t apb_get_scratch0_register(void);
void apb_set_scratch1_register(uint32_t val);
uint32_t apb_get_scratch1_register(void);

void apb_set_dws_wait_timing(uint8_t val);
uint8_t apb_get_dws_wait_timing(void);

uint32_t apb_get_reset_flag(void);
uint32_t apb_get_cpu_int_mask(void);

void apb_set_cpu_boot_magic(uint32_t cpu, uint32_t magic);

uint32_t apb_get_clock_backup(void);
void apb_set_clock_backup(uint32_t val);

#endif /* _DRIVER_HW_APB_H */
