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
#include "string.h"
#include "os_mem.h"

#include "lib_dbglog.h"
#include "cli.h"
#include "iot_lpm.h"
#include "os_syscall.h"
#include "cli_lowpower.h"
#include "cli_lowpower_definition.h"
#include "intc.h"
#include "pmm.h"
#include "iot_rtc_mon.h"
#include "iot_gpio.h"
#include "power_mgnt.h"
#include "rpc_caller.h"
#include "iot_rtc.h"

struct wakeup_src_arg {
    uint32_t src;
    uint32_t arg1;
} __attribute__((packed));

struct sleep_threshold {
    uint32_t deep;
    uint32_t light;
    /** 0: dtop, 1:bt, 2:dsp (not support) */
    uint8_t core;
} __attribute__((packed));

void cli_lowpower_enter_light_sleep(uint8_t *buffer, uint32_t bufferlen)
{
    UNUSED(buffer);
    UNUSED(bufferlen);

    uint32_t result = RET_OK;

    cli_interface_msg_response(
        CLI_MODULEID_LOWPOWER, CLI_MSGID_LOW_POWER_ENTER_LIGHT_SLEEP,
        (void *)&result, sizeof(result), 0, (uint8_t)result);

    DBGLOG_LIB_CLI_INFO("cli enter light sleep\n");
    iot_lpm_enter_lightsleep();
    DBGLOG_LIB_CLI_INFO("cli exit light sleep\n");
}

void cli_lowpower_enter_deep_sleep(uint8_t *buffer, uint32_t bufferlen)
{
    UNUSED(buffer);
    UNUSED(bufferlen);

    uint32_t result = RET_OK;

    cli_interface_msg_response(
        CLI_MODULEID_LOWPOWER, CLI_MSGID_LOW_POWER_ENTER_DEEP_SLEEP,
        (void *)&result, sizeof(result), 0, (uint8_t)result);

    DBGLOG_LIB_CLI_INFO("cli enter deep sleep\n");
    iot_lpm_enter_deepsleep();
    os_suspend();
    DBGLOG_LIB_CLI_INFO("cli exit deep sleep\n");
}

void cli_lowpower_enter_shutdown(uint8_t *buffer, uint32_t bufferlen)
{
    UNUSED(buffer);
    UNUSED(bufferlen);

    uint32_t result = RET_OK;

    cli_interface_msg_response(
        CLI_MODULEID_LOWPOWER, CLI_MSGID_LOW_POWER_ENTER_DEEP_SLEEP,
        (void *)&result, sizeof(result), 0, (uint8_t)result);

    DBGLOG_LIB_CLI_INFO("cli enter shutdown\n");

    // make sure chip level shutdown
    pmm_power_domain_force_powerdown(PMM_PD_BT, 1);
    pmm_clear_all_pd_wakeup_src_req(PMM_PD_BT);
    // avoid to wakeup by RTC timer which set by BT domain
    intc_set_wakeup_disable(WAKEUP_PMM_RTC_TIMER_INT0);
    pmm_set_pd_wakeup_src_req(PMM_PD_DCORE, PMM_DS_WAKEUP_RTC0, false);
    pmm_set_rtc_wakeup(false);

    iot_lpm_enter_shutdown();
}

static void gpio_int_callback(void)
{
    pmm_gpio_wakeup_clear_all();
}

void cli_lowpower_set_wakeup_source(uint8_t *buffer, uint32_t bufferlen)
{
    UNUSED(bufferlen);

    struct wakeup_src_arg *arg = (void *)buffer;
    uint32_t src = arg->src;
    uint32_t arg1 = arg->arg1;
    uint32_t result = RET_OK;

    DBGLOG_LIB_CLI_INFO("cli set wakeup source %d\n", src);

    switch (src) {
        case CLI_LP_WAKEUP_WIC:
            // wic default enable
            break;

        case CLI_LP_WAKEUP_RTC:
            DBGLOG_LIB_CLI_INFO("cli rtc interval %d ms\n", arg1);
            iot_lpm_set_pmm_rtc_wakeup_src(iot_rtc_get_global_time(),
                    MS_TO_INT_RTC(arg1));
            break;

        case CLI_LP_WAKEUP_GPIO:
            DBGLOG_LIB_CLI_INFO("cli gpio %d enable\n", arg1);
            iot_gpio_open_as_interrupt(IOT_AONGPIO_00 + arg1,
                                        IOT_GPIO_INT_LEVEL_LOW,
                                        gpio_int_callback);
            iot_lpm_set_pmm_gpio_wakeup_src(IOT_AONGPIO_00 + arg1);
            break;

        default:
            break;
    }

    cli_interface_msg_response(
        CLI_MODULEID_LOWPOWER, CLI_MSGID_LOW_POWER_SET_WAKEUP_SRC,
        (void *)&result, sizeof(result), 0, (uint8_t)result);
}

void cli_lowpower_set_sleep_thr(uint8_t *buffer, uint32_t bufferlen)
{
    UNUSED(bufferlen);

    struct sleep_threshold *arg = (void *)buffer;
    uint32_t result = RET_OK;

    DBGLOG_LIB_CLI_INFO("cli set sleep threshold deep:%u,light:%u,core:%d\n", arg->deep, arg->light, arg->core);

    if (arg->core == 0) {
        /* dtop */
        result = power_mgnt_set_sleep_thr(arg->deep, arg->light);
    } else if (arg->core == 1) {
        result = rpc_power_mgnt_set_sleep_thr(arg->deep, arg->light);
        /* bt */
    } else if (arg->core == 2) {
        /* dsp not support for now*/
        result = RET_NOSUPP;
    }

    cli_interface_msg_response(
        CLI_MODULEID_LOWPOWER, CLI_MSGID_LOW_POWER_SET_SLEEP_THR,
        (void *)&result, sizeof(result), 0, (uint8_t)result);
}

void cli_lowpower_disable_voltage_compatible(uint8_t *buffer, uint32_t bufferlen)
{
    UNUSED(buffer);
    UNUSED(bufferlen);
    uint32_t result = RET_OK;

#ifdef VOLTAGE_COMPATIBLE
    pmm_use_compatible_voltage(0);
#endif

    cli_interface_msg_response(
        CLI_MODULEID_LOWPOWER, CLI_MSGID_LOW_POWER_DISABLE_VOLTAGE_COMPATIBLE,
        (void *)&result, sizeof(result), 0, (uint8_t)result);
}

CLI_ADD_COMMAND(CLI_MODULEID_LOWPOWER, CLI_MSGID_LOW_POWER_ENTER_LIGHT_SLEEP,
                cli_lowpower_enter_light_sleep);
CLI_ADD_COMMAND(CLI_MODULEID_LOWPOWER, CLI_MSGID_LOW_POWER_ENTER_DEEP_SLEEP,
                cli_lowpower_enter_deep_sleep);
CLI_ADD_COMMAND(CLI_MODULEID_LOWPOWER, CLI_MSGID_LOW_POWER_ENTER_SHUTDOWN,
                cli_lowpower_enter_shutdown);
CLI_ADD_COMMAND(CLI_MODULEID_LOWPOWER, CLI_MSGID_LOW_POWER_SET_WAKEUP_SRC,
                cli_lowpower_set_wakeup_source);
CLI_ADD_COMMAND(CLI_MODULEID_LOWPOWER, CLI_MSGID_LOW_POWER_SET_SLEEP_THR,
                cli_lowpower_set_sleep_thr);
CLI_ADD_COMMAND(CLI_MODULEID_LOWPOWER, CLI_MSGID_LOW_POWER_DISABLE_VOLTAGE_COMPATIBLE,
                cli_lowpower_disable_voltage_compatible);
