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
#include "app_bat.h"
#include "app_pm.h"
#include "app_main.h"
#include "app_evt.h"
#include "app_charger.h"
#include "app_led.h"
#include "battery.h"
#include "battery_charger.h"
#include "ro_cfg.h"
#include "usr_cfg.h"
#include "app_bt.h"
#include "app_econn.h"
#include "app_wws.h"
#include "ntc.h"
#include "app_user_battery.h"

#ifndef CHECK_BATTERY_LEVEL_PERIOD_MS
#define CHECK_BATTERY_LEVEL_PERIOD_MS (10 * 1000)
#endif

#ifndef CHARGING_CHECK_BATTERY_LEVEL_PERIOD_MS
#define CHARGING_CHECK_BATTERY_LEVEL_PERIOD_MS (5 * 1000)
#endif

typedef enum {
    BAT_MSG_ID_LOW,
    BAT_MSG_ID_FULL,
    BAT_MSG_ID_LEVEL,
} battery_msg_id_t;

typedef struct {
    bool_t is_full;
    bool_t is_low;
    bool_t critical_low;
    uint8_t level;
    uint8_t base_level;
    uint16_t virtual_volt;
    uint16_t volt_mv;
    bool_t en_battery_report;
    bool_t charge_started;
    uint8_t last_charged_capacity;
} battery_context_t;

static battery_context_t _context;
static battery_context_t *context = &_context;

static void battery_low_check(uint16_t volt_mv)
{
    if (app_charger_is_charging()) {
        DBGLOG_BAT_DBG("battery_low_check ignored for in box\n");
        context->is_low = false;
        app_cancel_msg(MSG_TYPE_BAT, BAT_MSG_ID_LOW);

        if (battery_charger_get_cur_limit() != 0) {
            context->critical_low = false;
        } else {
            if (volt_mv < ro_bat_cfg()->battery_low_power_off_voltage) {
                DBGLOG_BAT_DBG("bat low power off when cur limit is 0\n");
                context->critical_low = true;
                app_pm_handle_battery_critical_low();
            }
        }
    } else {
        if (volt_mv < ro_bat_cfg()->battery_low_power_off_voltage) {
            context->critical_low = true;
            app_pm_handle_battery_critical_low();
        } else if (volt_mv < ro_bat_cfg()->battery_low_event_voltage) {
            if (!context->is_low) {
                app_send_msg(MSG_TYPE_BAT, BAT_MSG_ID_LOW, NULL, 0);
            }
            context->is_low = true;
        } else if (context->is_low) {
            DBGLOG_BAT_DBG("battery low shake ignored when discharging\n");
        }
    }
}

static uint8_t volt_2_level(uint16_t volt_mv)
{
    const ro_cfg_battery_t *bat_cfg = ro_bat_cfg();
    uint16_t level_volt;
    uint16_t next_level_volt;
    uint16_t level_0_volt;

    level_0_volt = bat_cfg->battery_low_power_off_voltage;

    if (level_0_volt > bat_cfg->battery_level_voltages[0]) {
        level_0_volt = bat_cfg->battery_level_voltages[0] - 500;
    }

    if (volt_mv >= bat_cfg->battery_level_voltages[9]) {
        return 100;
    } else if (volt_mv <= level_0_volt) {
        return 0;
    }

    for (int i = 8; i >= 0; i--) {
        level_volt = bat_cfg->battery_level_voltages[i];
        if (volt_mv >= level_volt) {
            next_level_volt = bat_cfg->battery_level_voltages[i + 1];
            return i * 10 + 10
                + ((uint32_t)(volt_mv - level_volt)) * 10 / (next_level_volt - level_volt);
        }
    }

    return ((uint32_t)(volt_mv - level_0_volt)) * 10
        / (bat_cfg->battery_level_voltages[0] - level_0_volt);
}

static void battery_check(void)
{
    uint16_t volt_mv;
    uint8_t level;
    uint8_t charged_capacity;

    level = app_econn_get_bat_level();
    if (level != BAT_LEVEL_INVALID) {
        DBGLOG_EVT_DBG("battery_check received %d from econn\n", level);
        goto out;
    }

    volt_mv = battery_get_voltage_mv();

    if ((!context->charge_started) && (context->volt_mv != 0)) {
        if ((volt_mv >= context->volt_mv + 50) || (volt_mv <= context->volt_mv - 50)) {
            DBGLOG_BAT_ERR("invalid volt, charging:%d volt:%d last_volt:%d",
                           context->charge_started, volt_mv, context->volt_mv);
            volt_mv = battery_get_voltage_mv();
        }
    }

    if (volt_mv < UVP_VOLT_MV) {
        DBGLOG_BAT_ERR("invalid battery volt %d\n", volt_mv);
        volt_mv = battery_get_voltage_mv();
    }

    context->volt_mv = volt_mv;

    if (context->virtual_volt != BAT_VOLT_INVALID) {
        DBGLOG_BAT_DBG("volt_mv:%d => virtual_volt:%d\n", volt_mv, context->virtual_volt);
        volt_mv = context->virtual_volt;
        context->level = BAT_LEVEL_INVALID;
    }

    level = volt_2_level(volt_mv);
    DBGLOG_BAT_DBG("volt:%d lvl:%d charging:%d charge_flag:%d full:%d\n", volt_mv, level,
                   app_charger_is_charging(), battery_charger_get_flag(), context->is_full);

    battery_low_check(volt_mv);

    if (context->level == BAT_LEVEL_INVALID) {
        pm_power_on_reason_t reason = app_pm_get_power_on_reason();
        if ((reason == PM_POWER_ON_REASON_HW_RESET) || (reason == PM_POWER_ON_REASON_FACTORY_RESET)
            || (reason == PM_POWER_ON_REASON_OTA)) {
            if (level > 95) {
                level = 95;
            }
        }
        context->base_level = level;
    }

    charged_capacity = battery_charger_get_charged_capacity(context->base_level);

    if (context->level == BAT_LEVEL_INVALID) {
        DBGLOG_BAT_DBG("first battery level\n");
    } else if (app_charger_is_charging() && context->is_full) {
        level = 100;
    } else if (app_charger_is_charging() || battery_charger_get_flag()) {
        uint8_t new_level;
        if ((charged_capacity >= context->last_charged_capacity)
            && (battery_charger_get_cur_limit() != 0)) {
            new_level = context->level + charged_capacity - context->last_charged_capacity;
            if ((context->level <= 95) && (new_level > 95)) {
                new_level = 95;
            }
            if (new_level > 100) {
                new_level = 100;
            }
            DBGLOG_BAT_DBG("charging lvl:%d => %d\n", level, new_level);
        } else {
            new_level = context->level;
            DBGLOG_BAT_DBG("charging keep lvl:%d => %d\n", level, new_level);
        }

        level = new_level;
    }
    context->last_charged_capacity = charged_capacity;

    if (context->virtual_volt != BAT_VOLT_INVALID) {
        DBGLOG_BAT_DBG("bat lvl virtual\n");
    } else if (context->level == BAT_LEVEL_INVALID) {
    } else if (app_charger_is_charging()) {
        if (level < context->level) {
            DBGLOG_BAT_DBG("bat lvl shake revise when charging: %d=>%d\n", level, context->level);
            level = context->level;
        }
    } else if (level > context->level) {
        DBGLOG_BAT_DBG("bat lvl shake revise when discharging: %d=>%d\n", level, context->level);
        level = context->level;
    }

out:
    if (level != context->level) {
        context->level = level;
        app_evt_send(EVTSYS_BATTERY_LEVEL_CHANGED);
        app_econn_handle_battery_level_changed(level);
        if (context->en_battery_report) {
            app_wws_send_battery(level);
            app_bt_report_battery_level(level);
        }
    }

    if (!app_charger_is_charging()) {
        context->base_level = level;
    }
}

static void app_bat_handle_msg(uint16_t msg_id, void *param)
{
    int8_t ntc_value;

    UNUSED(param);
    if (msg_id == BAT_MSG_ID_LEVEL) {
        if (app_bt_is_in_audio_test_mode() || app_bt_is_in_dut_mode()) {
            return;
        }
        battery_check();

        ntc_value = ntc_read();
        if (ntc_value != NTC_INVALID) {
            app_econn_handle_ntc_value(ntc_value);
						//app_user_battery_ntc_check(ntc_value);	//cuixu add
        }

        app_cancel_msg(MSG_TYPE_BAT, BAT_MSG_ID_LEVEL);
        if (context->charge_started) {
            app_send_msg_delay(MSG_TYPE_BAT, BAT_MSG_ID_LEVEL, NULL, 0,
                               CHARGING_CHECK_BATTERY_LEVEL_PERIOD_MS);
        } else {
            app_send_msg_delay(MSG_TYPE_BAT, BAT_MSG_ID_LEVEL, NULL, 0,
                               CHECK_BATTERY_LEVEL_PERIOD_MS);
        }
    } else if (msg_id == BAT_MSG_ID_LOW) {
        if (context->is_low) {
            app_cancel_msg(MSG_TYPE_BAT, BAT_MSG_ID_LOW);
            if (usr_cfg_get_battery_low_prompt_interval()) {
                app_send_msg_delay(MSG_TYPE_BAT, BAT_MSG_ID_LOW, NULL, 0,
                                   usr_cfg_get_battery_low_prompt_interval() * 1000);
            } else {
                DBGLOG_BAT_DBG("battery_low_prompt_interval is 0\n");
            }

            if (app_bt_get_sys_state() > STATE_DISABLED) {
                app_evt_send(EVTSYS_BATTERY_LOW);
            }
        }
    } else if (msg_id == BAT_MSG_ID_FULL) {
        DBGLOG_BAT_DBG("battery full\n");
        app_evt_send(EVTSYS_CHARGE_COMPLETE);
        app_pm_handle_battery_full();
    }
}

static void battery_state_callback(BATTERY_STATE state)
{
    switch (state) {
        case BATTERY_STATE_UNKNOWN:
            break;
        case BATTERY_STATE_FULL:
        case BATTERY_STATE_BAD:
            context->is_full = true;
            app_send_msg(MSG_TYPE_BAT, BAT_MSG_ID_FULL, NULL, 0);
            break;
        case BATTERY_STATE_CHARGE_START:
            context->is_full = false;
            context->charge_started = true;
            break;
        case BATTERY_STATE_CHARGE_STOP:
            context->charge_started = false;
            break;
        default:
            break;
    }
}

void app_bat_init(void)
{
    DBGLOG_BAT_DBG("app_bat_init\n");
    context->level = BAT_LEVEL_INVALID;
    context->is_low = false;
    context->is_full = false;
    context->critical_low = false;
    context->virtual_volt = BAT_VOLT_INVALID;
    context->en_battery_report = true;

    app_register_msg_handler(MSG_TYPE_BAT, app_bat_handle_msg);
    battery_state_register_callback(battery_state_callback);
    app_send_msg_delay(MSG_TYPE_BAT, BAT_MSG_ID_LEVEL, NULL, 0, 100);

    ntc_init();
}

void app_bat_deinit(void)
{
    ntc_deinit();
}

uint8_t app_bat_get_level(void)
{
    if (context->virtual_volt != BAT_VOLT_INVALID) {
        return volt_2_level(app_bat_get_virtual_volt());
    } else {
        return context->level;
    }
}

bool_t app_bat_is_low(void)
{
    return context->is_low;
}

bool_t app_bat_is_critical_low(void)
{
    return context->critical_low;
}

bool_t app_bat_is_full(void)
{
    return context->is_full;
}

void app_bat_set_virtual_volt(uint16_t volt)
{
    context->virtual_volt = volt;
    context->level = BAT_LEVEL_INVALID;

    if (context->virtual_volt != BAT_VOLT_INVALID) {
        if (context->virtual_volt > ro_bat_cfg()->battery_low_event_voltage) {
            context->is_low = false;
        }
    } else {
        context->is_low = false;
        app_cancel_msg(MSG_TYPE_BAT, BAT_MSG_ID_LOW);
    }

    battery_check();
}

uint16_t app_bat_get_virtual_volt(void)
{
    return context->virtual_volt;
}

void app_bat_enable_report(bool_t en)
{
    context->en_battery_report = en;
}

void app_bat_handle_charge_changed(bool_t charging)
{
    UNUSED(charging);

    if (charging) {
        context->is_low = false;
        app_cancel_msg(MSG_TYPE_BAT, BAT_MSG_ID_LOW);
    }

    if (context->level == BAT_LEVEL_INVALID) {
        DBGLOG_BAT_DBG("app_bat_handle_charge_changed first bat level\n");
        return;
    }

    app_cancel_msg(MSG_TYPE_BAT, BAT_MSG_ID_LEVEL);
    if (charging) {
        app_send_msg_delay(MSG_TYPE_BAT, BAT_MSG_ID_LEVEL, NULL, 0, 1000);
    } else {
        app_send_msg_delay(MSG_TYPE_BAT, BAT_MSG_ID_LEVEL, NULL, 0, 5000);
    }
}
