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

#ifndef _DRIVER_NON_OS_SOC_H
#define _DRIVER_NON_OS_SOC_H

/**
 * @addtogroup HAL
 * @{
 * @addtogroup SOC
 * @{
 * This section introduces the SOC module's enum, functions and how to use this driver.
 */

#include "types.h"
#include "adi_common.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief SOC bluetooth sub module. */
typedef enum {
    IOT_SOC_BT_SUB_MODULE_OSC,
    IOT_SOC_BT_SUB_MODULE_LC,
    IOT_SOC_BT_SUB_MODULE_PHY,
    IOT_SOC_BT_SUB_MODULE_PHY_REG,
    IOT_SOC_BT_SUB_MODULE_IP_REG,
    IOT_SOC_BT_SUB_MODULE_CP,
    IOT_SOC_BT_SUB_MODULE_MAX,
} IOT_SOC_BT_SUB_MODULE;

typedef enum {
    IOT_SOC_RESET_COLD,
    IOT_SOC_RESET_WARM,
    IOT_SOC_RESET_WATCHDOG,
}IOT_SOC_RESET_CAUSE;

typedef enum {
    IOT_SOC_CPU_ACCESS_JTAG,
    IOT_SOC_CPU_ACCESS_DTOP_ROM,
    IOT_SOC_CPU_ACCESS_DTOP_PERI,
    IOT_SOC_CPU_ACCESS_DTOP_IRAM,
    IOT_SOC_CPU_ACCESS_DTOP_FLASH,
    IOT_SOC_CPU_ACCESS_DTOP_NULL,
    IOT_SOC_CPU_ACCESS_BT_ROM,
    IOT_SOC_CPU_ACCESS_BT_IRAM,
    IOT_SOC_CPU_ACCESS_BT_NULL,
    IOT_SOC_CPU_ACCESS_BT_PERI,
    IOT_SOC_CPU_ACCESS_DSP_RAM,
    IOT_SOC_CPU_ACCESS_DSP_PERI,
    IOT_SOC_CPU_ACCESS_DSP_DDR,
    IOT_SOC_CPU_ACCESS_DSP_NULL0,
    IOT_SOC_CPU_ACCESS_DSP_NULL1,
    IOT_SOC_CPU_ACCESS_ZERO,
}IOT_SOC_CPU_ACCESS_SLAVE_PORT;

/**
 * @brief This function is to init the soc chip.
 */
void iot_soc_chip_init(bool cold_boot);

/**
 * @brief This function is to init the soc chip PMM module.
 */
void iot_soc_chip_pmm_init(void);

/**
 * @brief This function is to set soc madc para.
 *
 */
void iot_soc_madc_config(void);

/**
 * @brief This function is to set soc ldo.
 */
void iot_soc_ldo_config(void);

/**
 * @brief This function is to enable soc bt clock.
 */
void iot_soc_bt_clk_enable(void);

/**
 * @brief This function is to disable soc bt clock.
 */
void iot_soc_bt_clk_disable(void);

/**
 * @brief This function is to enable soc bt phy clock.
 */
void iot_soc_bt_phy_clk_enable(void);

/**
 * @brief This function is to disable soc bt phy clock.
 */
void iot_soc_bt_phy_clk_disable(void);

/**
 * @brief This function is to enable soc ahb bt async clock.
 */
void iot_soc_ahb_bt_async_enable(void);

/**
 * @brief This function is to disable soc ahb bt async clock.
 */
void iot_soc_ahb_bt_async_disable(void);

/**
 * @brief This function is to enable soc bt cpu access.
 */
void iot_soc_bt_cpu_access_enable(void);

/**
 * @brief This function is to disable soc bt cpu access.
 */
void iot_soc_bt_cpu_access_disable(void);

/**
 * @brief This function is to enable soc clock bt anb frc.
 */
void iot_soc_clk_bt_ahb_frc_enable(void);

/**
 * @brief This function is to disable soc clock bt anb frc.
 */
void iot_soc_clk_bt_ahb_frc_disable(void);

/**
 * @brief This function is to enable soc bt osc.
 */
void iot_soc_bt_osc_enable(void);

/**
 * @brief This function is to disable soc bt osc.
 */
void iot_soc_bt_osc_disable(void);

/**
 * @brief This function is to reset bt soft.
 *
 * @param module
 */
void iot_soc_bt_soft_reset(IOT_SOC_BT_SUB_MODULE module);

/**
 * @brief Power on bt power domain
 */
void iot_soc_bt_power_up(void);

/**
 * @brief Power off bt power domain
 */
void iot_soc_bt_power_off(void);

/**
 * @brief Power on audio interface power domain
 */
void iot_soc_audio_intf_power_up(void);

/**
 * @brief Power off audio interface power domain
 */
void iot_soc_audio_intf_power_off(void);

/**
 * @brief This function is to start soc bt.
 *
 * @param start_pc is the starting point.
 */
void iot_soc_bt_start(uint32_t start_pc);

/**
 * @brief This function is to reset soc chip.
 *
 */
void iot_soc_chip_reset(void);

/**
 * @brief This function is to run bist.
 *
 * @return bool_t true if write SUCC otherwise false.
 */
bool_t iot_soc_bist_run(void);

/**
 * @brief This function is to set soft reset flag.
 *
 * @param [in] val is soft reset flag.
 */
void iot_soc_set_soft_reset_flag(uint32_t val);

/**
 * @brief This function is to get soft reset flag.
 *
 * @return uint32_t is soft reset flag.
 */
uint32_t iot_soc_get_soft_reset_flag(void);

/**
 * @brief This function is to get reset cause.
 *
 * @return IOT_SOC_RESET_CAUSE is reset cause.
 */
IOT_SOC_RESET_CAUSE iot_soc_get_reset_cause(void);

/**
 * @brief This function is to clear reset cause.
 *
 * @param reset is reset cause.
 */
void iot_soc_clear_reset_cause(IOT_SOC_RESET_CAUSE reset);

/**
 * @brief This function is to get wakeup source.
 *
 * @return uint32_t is wakeup source.
 */
uint32_t iot_soc_get_wakeup_source(void);

/**
 * @brief This function is to save boot reason into scratch register.
 *
 * @param boot_reason is saved.
 */
void iot_soc_save_boot_reason(uint32_t boot_reason);

/**
 * @brief This function is to restore boot reason from scratch register.
 *
 * @return uint32_t is restored boot reason.
 */
uint32_t iot_soc_restore_boot_reason(void);

/**
 * @brief This function is to get reset cause.
 *
 * @return uint32_t is reset cause.
 */
uint32_t iot_soc_reset_cause_get(void);

/**
 * @brief This function is to get the reset flag.
 *
 * @return uint32_t is the reset flag.
 */
uint32_t iot_soc_get_reset_flag(void);

/**
 * @brief This function is to enable or disable cpu exception.
 *
 * @param enable is whether enabler or disable.
 */
void iot_soc_cpu_exception_enable(bool_t enable);

/**
 * @brief This function is to enable or disable cpu access to ahb slave port.
 *
 * @param port is ahb slave port.
 * @param enable is whether enabler or disable.
 */
void iot_soc_cpu_access_enable(IOT_SOC_CPU_ACCESS_SLAVE_PORT port, bool_t enable);

/**
 * @brief This function is to get wakeup source reqeust.
 *
 * @return uint16_t is wakeup source request.
 */
uint16_t iot_soc_get_wakeup_source_req(void);

/**
 * @brief This function is to get audio intf's power mask.
 *
 * @return uint32_t is mask value, for debug purpose.
 */
uint32_t iot_soc_audio_intf_power_mask(void);

/**
 * @brief This function is used to read soc register(include general/special purpose register).
 *
 * @param addr is target register address.(Register address 4 byte alignmnet required.)
 * @return uint32_t return the data stored in the target register.
 *
 */
uint32_t iot_soc_register_read(uint32_t addr);
/**
 * @brief This function is write soc register(include general/special purpose register).
 *
 * @param addr is target register address.(Register address 4 byte alignmnet required.)
 * @param val is target register value.
 * @return uint32_t return the data stored in the target register.
 */
uint32_t iot_soc_register_write(uint32_t addr, uint32_t val);

/**
 * @brief This function is to get cpu reset flag.
 *
 * @return uint32_t is cpu reset flag, return 0x12345678 if this is CPU reset.
 */
uint32_t iot_soc_get_cpu_reset_flag(void);

/**
 * @brief This function is to clear cpu reset flag;
 */
void iot_soc_clear_cpu_reset_flag(void);

/**
 * @brief This function is to config adi interface wire mode.
 */
void iot_soc_adi_wire_mode_config(ADI_WIRE_MODE wire_mode);

/**
 * @brief This functions increase reference counter of suspend task
 *
 * @return uint32_t is reference counter.
 */
uint32_t iot_soc_inc_task_suspend_vote(void);

/**
 * @brief This function is to set task suspend vote
 *
 * @return uint32_t is reference counter.
 */
uint32_t iot_soc_set_task_suspend_vote(void);

/**
 * @brief This function is to get task suspend vote
 *
 * @return uint32_t is vote value.
 */
uint32_t iot_soc_get_task_suspend_vote(void);

/**
 * @brief This function is to clear task suspend vote.
 *
 * @return uint32_t is reference counter.
 */
uint32_t iot_soc_clear_task_suspend_vote(void);

/**
 * @brief This function is to set ppm registers.
 *
 * @param ppm is ppm value.
 */
void iot_soc_set_ppm(uint8_t ppm);

/**
 * @brief This functions is to check wakeup source is vbat resume
 */
bool iot_soc_wakeup_source_is_vbat_resume(void);

#ifdef __cplusplus
}
#endif

/**
 * @}
 * addtogroup SOC
 * @}
 * addtogroup HAL
 */

#endif /* _DRIVER_NON_OS_SOC_H */
