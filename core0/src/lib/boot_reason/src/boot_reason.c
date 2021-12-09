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
#include "riscv_cpu.h"
#include "iot_gpio.h"
#include "iot_soc.h"
#include "iot_wdt.h"
#include "iot_debounce.h"
#include "iot_charger.h"
#include "boot_reason.h"

#if !defined(BUILD_OS_NON_OS)
#include "os_hook.h"
#endif

#ifdef LIB_GENERIC_TRANSMISSION_ENABLE
#include "generic_transmission_api.h"
#endif

#define BOOT_REASON_SOFT_RESET_MAGIC 0x5352   //SR
#define BOOT_REASON_CPU_RESET_MAGIC  0x12345678

#define BOOT_REASON_SOFT_RESET_MAGIC_OFFSET  16
#define BOOT_REASON_SOFT_RESET_MAGIC_MASK    0xFFFF0000
#define BOOT_REASON_SOFT_RESET_REASON_OFFSET 8
#define BOOT_REASON_SOFT_RESET_REASON_MASK   0x0000FF00
#define BOOT_REASON_SOFT_RESET_FLAG_OFFSET   0
#define BOOT_REASON_SOFT_RESET_FLAG_MASK     0xFF

#define BOOT_REASON_SYNCED_MAGIC              0xAA
#define BOOT_REASON_SYNCED_MAGIC_OFFSET       24
#define BOOT_REASON_SYNCED_MAGIC_MASK         0xFF000000
#define BOOT_REASON_SYNCED_SOFT_REASON_OFFSET 16
#define BOOT_REASON_SYNCED_SOFT_REASON_MASK   0x00FF0000
#define BOOT_REASON_SYNCED_SOFT_FLAG_OFFSET   8
#define BOOT_REASON_SYNCED_SOFT_FLAG_MASK     0x0000FF00
#define BOOT_REASON_SYNCED_WAKEUP_SRC_OFFSET  3
#define BOOT_REASON_SYNCED_WAKEUP_SRC_MASK    0xF8
#define BOOT_REASON_SYNCED_REASON_OFFSET      0
#define BOOT_REASON_SYNCED_REASON_MASK        0x7

static BOOT_REASON_SOFT_REASON boot_reason_reset_reason = BOOT_REASON_SOFT_REASON_UNKNOWN;
static uint8_t boot_reason_reset_flag = 0;

static void boot_reason_clear_reason(BOOT_REASON_TYPE reason)
{
    if (reason == BOOT_REASON_SOFT) {
        iot_soc_clear_reset_cause(IOT_SOC_RESET_WATCHDOG);
    } else if (reason == BOOT_REASON_SLEEP) {
        iot_soc_clear_reset_cause(IOT_SOC_RESET_WARM);
    } else if (reason == BOOT_REASON_WDT) {
        iot_soc_clear_reset_cause(IOT_SOC_RESET_WATCHDOG);
    } else {
        iot_soc_clear_reset_cause(IOT_SOC_RESET_COLD);
    }

    iot_soc_set_soft_reset_flag(iot_soc_get_soft_reset_flag() & BOOT_REASON_SOFT_RESET_FLAG_MASK);
}

static bool_t boot_reason_synced_reason(void)
{
    /* check boot reason synced from hardware */
    uint32_t val = iot_soc_restore_boot_reason();

    uint16_t magic = ((val & BOOT_REASON_SYNCED_MAGIC_MASK) >> BOOT_REASON_SYNCED_MAGIC_OFFSET);

    if (magic == BOOT_REASON_SYNCED_MAGIC) {
        return true;
    } else {
        return false;
    }
}

static BOOT_REASON_TYPE boot_reason_restore_reason(void)
{
    /* check boot reason synced from hardware */
    uint32_t val = iot_soc_restore_boot_reason();

    return (BOOT_REASON_TYPE)((val & BOOT_REASON_SYNCED_REASON_MASK)
                              >> BOOT_REASON_SYNCED_REASON_OFFSET);   //lint !e835 ">> 0"
}

static void boot_reason_save_reason(BOOT_REASON_TYPE boot_reason,
                                    BOOT_REASON_SOFT_REASON soft_boot_reason, uint8_t flag,
                                    BOOT_REASON_WAKEUP_SOURCE wakeup_src)
{
    uint32_t val =
        (BOOT_REASON_SYNCED_MAGIC << BOOT_REASON_SYNCED_MAGIC_OFFSET)   //lint !e648 not overflow
        | (soft_boot_reason << BOOT_REASON_SYNCED_SOFT_REASON_OFFSET)
        | (flag << BOOT_REASON_SYNCED_SOFT_FLAG_OFFSET)
        | (wakeup_src << BOOT_REASON_SYNCED_WAKEUP_SRC_OFFSET) | boot_reason;
    iot_soc_save_boot_reason(val);
}

static BOOT_REASON_SOFT_REASON boot_reason_restore_soft_reset_reason(uint8_t *flag)
{
    /* check boot reason synced from hardware */
    uint32_t val = iot_soc_restore_boot_reason();

    *flag = ((val & BOOT_REASON_SYNCED_SOFT_FLAG_MASK) >> BOOT_REASON_SYNCED_SOFT_FLAG_OFFSET);

    BOOT_REASON_SOFT_REASON reason = (BOOT_REASON_SOFT_REASON)(
        (val & BOOT_REASON_SYNCED_SOFT_REASON_MASK) >> BOOT_REASON_SYNCED_SOFT_REASON_OFFSET);

    return reason;
}

static BOOT_REASON_WAKEUP_SOURCE boot_reason_restore_wakeup_source(void)
{
    /* check wakeup source synced from hardware */
    uint32_t val = iot_soc_restore_boot_reason();

    return (BOOT_REASON_WAKEUP_SOURCE)((val & BOOT_REASON_SYNCED_WAKEUP_SRC_MASK)
                                       >> BOOT_REASON_SYNCED_WAKEUP_SRC_OFFSET);
}

static void boot_reason_set_soft_reset_reason(BOOT_REASON_SOFT_REASON reason, uint8_t flag)
{
    uint32_t val = (BOOT_REASON_SOFT_RESET_MAGIC << BOOT_REASON_SOFT_RESET_MAGIC_OFFSET)
        | (reason << BOOT_REASON_SOFT_RESET_REASON_OFFSET);

    if (reason == BOOT_REASON_SOFT_REASON_EXCEPTION
        || reason == BOOT_REASON_SOFT_REASON_WDT_TIMEOUT) {
        val |= (iot_soc_get_soft_reset_flag() & BOOT_REASON_SOFT_RESET_FLAG_MASK);
    } else {
        val |= flag;
    }

    iot_soc_set_soft_reset_flag(val);
}

BOOT_REASON_SOFT_REASON boot_reason_get_soft_reset_reason(uint8_t *flag)
{
    uint32_t val;
    uint16_t magic;
    BOOT_REASON_SOFT_REASON reason = BOOT_REASON_SOFT_REASON_UNKNOWN;

    /* synced with hardware */
    if (boot_reason_synced_reason()) {
        return boot_reason_restore_soft_reset_reason(flag);
    }

    /* check from hardware */
    val = iot_soc_get_soft_reset_flag();

    magic = ((val & BOOT_REASON_SOFT_RESET_MAGIC_MASK) >> BOOT_REASON_SOFT_RESET_MAGIC_OFFSET);

    if (magic == BOOT_REASON_SOFT_RESET_MAGIC) {
        *flag = ((val & BOOT_REASON_SOFT_RESET_FLAG_MASK)
                 >> BOOT_REASON_SOFT_RESET_FLAG_OFFSET);   //lint !e835 ">> 0"
        reason = (BOOT_REASON_SOFT_REASON)((val & BOOT_REASON_SOFT_RESET_REASON_MASK)
                                           >> BOOT_REASON_SOFT_RESET_REASON_OFFSET);
    }

    return reason;
}

BOOT_REASON_WAKEUP_SOURCE boot_reason_get_wakeup_source(void)
{
    uint32_t source = iot_soc_get_wakeup_source();
    BOOT_REASON_WAKEUP_SOURCE reason;
    uint16_t source_req = iot_soc_get_wakeup_source_req();

    /* synced with hardware */
    if (boot_reason_synced_reason()) {
        return boot_reason_restore_wakeup_source();
    }

    for (reason = BOOT_REASON_WAKEUP_SRC_GPIO; reason < BOOT_REASON_WAKEUP_SRC_MAX; reason++) {
        if (source & BIT(reason)) {
            if (reason > BOOT_REASON_WAKEUP_SRC_DEB) {
                break;
            } else {
                /* need check with wakeup request  */
                if (source_req & BIT(reason)) {
                    break;
                }
            }
        }
    }

    return (BOOT_REASON_WAKEUP_SOURCE)reason;
}

void boot_reason_get_wakeup_gpio_source(uint16_t *gpio, uint8_t *level)
{
    iot_gpio_get_wakeup_source(gpio, level);
}

IOT_CHARGER_INT_TYPE boot_reason_get_wakeup_charger_flag(void)
{
    return iot_charger_get_int_type();
}

BOOT_REASON_TYPE boot_reason_get_reason(void)
{
    BOOT_REASON_TYPE boot_reason = BOOT_REASON_UNKNOWN;
    BOOT_REASON_SOFT_REASON soft_boot_reason;
    BOOT_REASON_WAKEUP_SOURCE wakeup_src = BOOT_REASON_WAKEUP_SRC_GPIO;
    uint8_t flag = 0;

    /* synced with hardware */
    if (boot_reason_synced_reason()) {
        return boot_reason_restore_reason();
    }

    /* hardware reset check*/
    IOT_SOC_RESET_CAUSE cause = iot_soc_get_reset_cause();

    if (cause == IOT_SOC_RESET_WARM) {
        boot_reason = BOOT_REASON_SLEEP;
    } else if (cause == IOT_SOC_RESET_WATCHDOG) {
        boot_reason = BOOT_REASON_WDT;
    } else {
        boot_reason = BOOT_REASON_POR;
    }

    /* charger hardware reset check */
    if (iot_debounce_get_hard_reset_flag()) {
        boot_reason = BOOT_REASON_HARD;
        iot_debounce_clear_hard_reset_flag();
    }

    /* soft reset check*/
    soft_boot_reason = boot_reason_get_soft_reset_reason(&flag);

    if (soft_boot_reason != BOOT_REASON_SOFT_REASON_UNKNOWN) {
        boot_reason = BOOT_REASON_SOFT;
    }

    /* CPU reset magic number check */
    if (iot_soc_get_cpu_reset_flag() == BOOT_REASON_CPU_RESET_MAGIC) {
        boot_reason = BOOT_REASON_CPU;
        iot_soc_clear_cpu_reset_flag();
    }

    if (boot_reason == BOOT_REASON_SLEEP) {
        wakeup_src = boot_reason_get_wakeup_source();
    }

    /* save boot reason and wakeup src */
    boot_reason_save_reason(boot_reason, soft_boot_reason, flag, wakeup_src);

    /* clear boot reason */
    boot_reason_clear_reason(boot_reason);

    return boot_reason;
}

static void boot_reason_chip_reset(void)
{
#ifdef GUARANTEE_COREDUMP_END
    // if BT core in panic status, wait for coredump end
    while (!in_irq() && cpu_get_int_enable()
           && generic_transmission_get_status(1) == GTB_ST_IN_PANIC) {
        os_delay(1);
    }
#endif

    /* diable interrupt */
    uint32_t mask = cpu_disable_irq();

    boot_reason_set_soft_reset_reason(boot_reason_reset_reason, boot_reason_reset_flag);
    /* soft reset */
    iot_wdt_global_do_reset(false);

    /* stay here till reset */
    volatile uint32_t forever = 1;

    while (forever) {
    }
    /* Should never be here */
    cpu_restore_irq(mask);
}

void boot_reason_do_soft_reset(BOOT_REASON_SOFT_REASON reason, uint8_t flag)
{
    boot_reason_reset_reason = reason;
    boot_reason_reset_flag = flag;

    boot_reason_chip_reset();
}

#if !defined(BUILD_OS_NON_OS)
void boot_reason_system_reset(BOOT_REASON_SOFT_REASON reason, uint8_t flag)
{
    boot_reason_reset_reason = reason;
    boot_reason_reset_flag = flag;
    os_hook_idle_register_callback(boot_reason_chip_reset);
}
#endif

void boot_reason_set_crash_flag(uint8_t flag)
{
    uint32_t value = iot_soc_get_soft_reset_flag();

    value = (value & ~BOOT_REASON_SOFT_RESET_FLAG_MASK) | flag;
    iot_soc_set_soft_reset_flag(value);
}

uint8_t boot_reason_get_crash_flag(void)
{
    return (uint8_t)(iot_soc_get_soft_reset_flag() & BOOT_REASON_SOFT_RESET_FLAG_MASK);
}
