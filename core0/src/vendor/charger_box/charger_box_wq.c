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
#ifndef CHARGER_BOX
#define CHARGER_BOX CHARGER_BOX_WQ
#endif

#define CHARGER_BOX_WQ 1

#pragma GCC diagnostic ignored "-Wundef"

#if CHARGER_BOX == CHARGER_BOX_WQ
#include "os_timer.h"
#include "os_utils.h"
#include "battery_charger.h"
#include "lib_dbglog.h"
#include "boot_reason.h"
#include "charger_box.h"
#include "iot_debounce.h"

#define CHARGER_MSG_DEBUG 0

#if CHARGER_MSG_DEBUG
#define DBGLOG_CHARGER_DBG DBGLOG_LIB_INFO
#define DBGLOG_CHARGER_ERR DBGLOG_LIB_ERROR
#else
#define DBGLOG_CHARGER_DBG(...) \
    do {                        \
    } while (0)
#define DBGLOG_CHARGER_ERR(...) \
    do {                        \
    } while (0)
#endif

#define BIT_TIME_MS              10
#define START_HIGH_BIT_COUNT     10   //100ms
#define OP_TIMEOUT_MS            300
#define OP_BT_DISABLE_TIMEOUT_MS 3000   //3s

#define BOX_MSG_OPT_CODE_BOX_OPEN   0x50
#define BOX_MSG_OPT_CODE_BOX_CLOSE  0x51
#define BOX_MSG_OPT_CODE_LONG_3S    0x52
#define BOX_MSG_OPT_CODE_LONG_10S   0x53
#define BOX_MSG_OPT_CODE_RECONFIG   0x54
#define BOX_MSG_OPT_CODE_ENTER_DUT  0x55
#define BOX_MSG_OPT_CODE_ENTER_OTA  0x56
#define BOX_MSG_OPT_CODE_ENTER_PAIR 0x57
#define BOX_MSG_OPT_CODE_SHUT_DOWN  0x58
#define BOX_MSG_OPT_CODE_LONG_20S   0x59

//-HIGH_100MS-|S |---------OPCODE--------|P |S |--------PARAM----------|P |
//____________    __    __    __    __    __    __    __    __    __    __
//            |__|  |__|  |__|  |__|  |__|  |__|  |__|  |__|  |__|  |__|
typedef enum {
    STAGE_IDLE,
    STAGE_HIGH_100MS,
    STAGE_OPCODE_START,
    STAGE_OPCODE,
    STAGE_OPCODE_STOP,
    STAGE_PARAM_START,
    STAGE_PARAM,
} stage_t;

static charger_callback_t callback = NULL;
static timer_id_t timer = 0;
static timer_id_t timer_bt_disable = 0;
static bool_t charging = false;
static bool_t in_box = false;
static bool_t first_charging = true;
static bool_t btn_wakeup = false;

static stage_t stage = STAGE_IDLE;
static bool_t last_charge_on = false;
static uint8_t opcode = 0;
static uint8_t param = 0;
static uint8_t cur_bit_count = 0;
static uint32_t last_time = 0;
static uint8_t last_opcode = 0;
static uint8_t last_param = 0;
static bool_t shutdown_sent = false;

static void sm_opcode(bool_t high, uint32_t count);
static void sm_param(bool_t high, uint32_t count);

static void handle_wq_opcode(uint8_t wq_opcode, uint8_t param)
{
    switch (wq_opcode) {
        case BOX_MSG_OPT_CODE_BOX_OPEN:
            callback(CHARGER_EVT_BOX_BATTERY, &param, sizeof(param));
            callback(CHARGER_EVT_BOX_OPEN, NULL, 0);
            break;
        case BOX_MSG_OPT_CODE_BOX_CLOSE:
            callback(CHARGER_EVT_BOX_BATTERY, &param, sizeof(param));
            callback(CHARGER_EVT_BOX_CLOSE, NULL, 0);
            break;
        case BOX_MSG_OPT_CODE_LONG_3S:
            callback(CHARGER_EVT_BOX_BATTERY, &param, sizeof(param));
            callback(CHARGER_EVT_AG_PAIR, NULL, 0);
            break;
        case BOX_MSG_OPT_CODE_LONG_10S:
            callback(CHARGER_EVT_BOX_BATTERY, &param, sizeof(param));
            callback(CHARGER_EVT_RESET, NULL, 0);
            break;
        case BOX_MSG_OPT_CODE_RECONFIG:
            callback(CHARGER_EVT_BOX_BATTERY, &param, sizeof(param));
            callback(CHARGER_EVT_RESET, NULL, 0);
            break;
        case BOX_MSG_OPT_CODE_ENTER_DUT:
            callback(CHARGER_EVT_ENTER_DUT, NULL, 0);
            break;
        case BOX_MSG_OPT_CODE_ENTER_OTA:
            callback(CHARGER_EVT_ENTER_OTA, NULL, 0);
            break;
        case BOX_MSG_OPT_CODE_ENTER_PAIR:
            callback(CHARGER_EVT_TWS_MAGIC, &param, sizeof(param));
            break;
        case BOX_MSG_OPT_CODE_SHUT_DOWN:
            callback(CHARGER_EVT_BT_DISABLE, NULL, 0);
            break;
        case BOX_MSG_OPT_CODE_LONG_20S:
            callback(CHARGER_EVT_POPUP_TOGGLE, NULL, 0);
            break;
        default:
            callback((charger_evt_t)(0x80 | wq_opcode), NULL, 0);
            break;
    }
}

static void timer_func(timer_id_t timer_id, void *arg)
{
    bool_t charge_on = battery_charger_get_flag();

    UNUSED(timer_id);
    UNUSED(arg);

    if (stage == STAGE_PARAM) {
        sm_param(last_charge_on, 8);
        stage = STAGE_IDLE;
        os_stop_timer(timer);
        os_start_timer(timer, OP_TIMEOUT_MS);
        return;
    }

    if (charge_on) {
        charging = true;
        if (!first_charging) {
            os_start_timer(timer_bt_disable, OP_BT_DISABLE_TIMEOUT_MS);
        }
        first_charging = false;
    } else {
        in_box = false;
        callback(CHARGER_EVT_TAKE_OUT, NULL, 0);
        if (last_opcode == BOX_MSG_OPT_CODE_SHUT_DOWN) {
            callback(CHARGER_EVT_POWER_OFF, NULL, 0);
        } else if (charging) {
            charging = false;
            callback(CHARGER_EVT_POWER_ON, NULL, 0);
        }
    }

    stage = STAGE_IDLE;
}

static void timer_bt_disable_func(timer_id_t timer_id, void *arg)
{
    UNUSED(timer_id);
    UNUSED(arg);

    callback(CHARGER_EVT_BT_DISABLE, NULL, 0);
}

static void sm_idle(bool_t high, uint32_t count)
{
    if (!high) {
        DBGLOG_CHARGER_ERR("sm_idle low count:%d\n", count);
        return;
    }

    if (count < START_HIGH_BIT_COUNT) {
        DBGLOG_CHARGER_ERR("sm_idle count:%d\n", count);
        return;
    }

    stage = STAGE_HIGH_100MS;
    cur_bit_count = 0;
    param = 0;
    opcode = 0;
}

static void sm_high_100ms(bool_t high, uint32_t count)
{
    if (high) {
        stage = STAGE_IDLE;
        DBGLOG_CHARGER_ERR("sm_high_100ms high count:%d\n", count);
        return;
    }

    if (count == 1) {
        stage = STAGE_OPCODE_START;
    } else {
        stage = STAGE_OPCODE;
        sm_opcode(high, count - 1);
    }
}

static void sm_opcode_start(bool_t high, uint32_t count)
{
    stage = STAGE_OPCODE;
    sm_opcode(high, count);
}

static void sm_opcode(bool_t high, uint32_t count)
{
    bool_t ext = false;

    if (cur_bit_count + count > 8) {
        count = 8 - cur_bit_count;
        ext = true;
    }

    if (high) {
        for (uint32_t i = 0; i < count; i++) {
            opcode = opcode | (1 << cur_bit_count);
            cur_bit_count += 1;
        }
        if (ext) {
            stage = STAGE_OPCODE_STOP;
            cur_bit_count = 0;
        }
    } else {
        cur_bit_count += count;
        if (ext) {
            DBGLOG_CHARGER_ERR("sm_opcode low count:%d\n", count);
            stage = STAGE_IDLE;
        }
    }
}

static void sm_opcode_stop(bool_t high, uint32_t count)
{
    if (high) {
        DBGLOG_CHARGER_ERR("sm_opcode_stop high count:%d\n", count);
        stage = STAGE_IDLE;
        return;
    }

    if (count == 1) {
        stage = STAGE_PARAM_START;
    } else {
        stage = STAGE_PARAM;
        sm_param(high, count - 1);
    }
}

static void sm_param_start(bool_t high, uint32_t count)
{
    stage = STAGE_PARAM;
    sm_param(high, count);
}

static void sm_param(bool_t high, uint32_t count)
{
    bool_t ext = false;

    if (cur_bit_count + count > 8) {
        count = 8 - cur_bit_count;
        ext = true;
    }

    if (high) {
        for (uint32_t i = 0; i < count; i++) {
            param = param | (1 << cur_bit_count);
            cur_bit_count += 1;
        }
        if (ext) {
            stage = STAGE_IDLE;
            cur_bit_count = 0;

            if (BOX_MSG_OPT_CODE_ENTER_PAIR == opcode) {
                //unused currently
                param = last_param;
            }

            if (opcode == BOX_MSG_OPT_CODE_SHUT_DOWN) {
                if ((last_opcode == BOX_MSG_OPT_CODE_SHUT_DOWN) && (!shutdown_sent)) {
                    /* double check */
                    handle_wq_opcode(opcode, param);
                    shutdown_sent = true;
                }
                last_opcode = opcode;
                last_param = param;
            } else {
                shutdown_sent = false;
                if ((opcode != last_opcode) || (param != last_param)) {
                    handle_wq_opcode(opcode, param);

                    last_opcode = opcode;
                    last_param = param;
                }
            }
            return;
        }
    } else {
        cur_bit_count += count;
        if (ext) {
            DBGLOG_CHARGER_ERR("sm_param low count:%d\n", count);
            stage = STAGE_IDLE;
            return;
        }
    }

    os_stop_timer(timer);
    os_start_timer(timer, BIT_TIME_MS * 8);
}

static void handle_bits(bool_t high, uint32_t count)
{
    switch (stage) {
        case STAGE_IDLE:
            sm_idle(high, count);
            break;
        case STAGE_HIGH_100MS:
            sm_high_100ms(high, count);
            break;
        case STAGE_OPCODE_START:
            sm_opcode_start(high, count);
            break;
        case STAGE_OPCODE:
            sm_opcode(high, count);
            break;
        case STAGE_OPCODE_STOP:
            sm_opcode_stop(high, count);
            break;
        case STAGE_PARAM_START:
            sm_param_start(high, count);
            break;
        case STAGE_PARAM:
            sm_param(high, count);
            break;
        default:
            break;
    }
}

static void handle_charge_flag(uint8_t flag, uint32_t period)
{
    uint32_t bit_count;
    bool_t charge_on = flag;

    //DBGLOG_CHARGER_DBG("handle_charge_flag flag:%d period:%d\n", flag, period);

    if (!flag) {
        iot_debounce_set_clr_counter_flag(0);
    }

    if (!callback) {
        return;
    }

    if (charge_on) {
        if (!in_box) {
            in_box = true;
            callback(CHARGER_EVT_PUT_IN, NULL, 0);
        }
    } else {
        if (charging || first_charging) {
            charging = false;
            first_charging = false;
            if ((!first_charging) || (!btn_wakeup)) {
                callback(CHARGER_EVT_POWER_ON, NULL, 0);
            }
        }
    }

    if (os_is_timer_active(timer_bt_disable)) {
        os_stop_timer(timer_bt_disable);
    }

    if (os_is_timer_active(timer)) {
        os_stop_timer(timer);
    }
    os_start_timer(timer, OP_TIMEOUT_MS);

    if (os_boot_time32() - last_time > 200) {
        period += 200;
    }

    bit_count = (period + BIT_TIME_MS / 2) / BIT_TIME_MS;

    if (bit_count == 0) {
        last_charge_on = charge_on;
        last_time = os_boot_time32();
        DBGLOG_CHARGER_ERR("handle_charge_flag period:%d\n", period);
        return;
    } else if (bit_count >= START_HIGH_BIT_COUNT) {
        stage = STAGE_IDLE;
    }

    handle_bits(last_charge_on, bit_count);

    last_charge_on = charge_on;
    last_time = os_boot_time32();
}

void charger_box_init(charger_callback_t _callback)
{
    callback = _callback;

    timer_bt_disable =
        os_create_timer(IOT_APP_BATTERY_CASE_MID, false, timer_bt_disable_func, NULL);
    timer = os_create_timer(IOT_APP_BATTERY_CASE_MID, false, timer_func, NULL);
    os_start_timer(timer, OP_TIMEOUT_MS);

    BOOT_REASON_TYPE boot_reason = boot_reason_get_reason();
    if (boot_reason == BOOT_REASON_SLEEP) {
        BOOT_REASON_WAKEUP_SOURCE wakeup_source = boot_reason_get_wakeup_source();
        if ((wakeup_source == BOOT_REASON_WAKEUP_SRC_GPIO)
            || (wakeup_source == BOOT_REASON_WAKEUP_SRC_TK)
            || (wakeup_source == BOOT_REASON_WAKEUP_SRC_DEB)) {
            btn_wakeup = true;
        }
    }

    bool_t charge_on = battery_charger_get_flag();

    if (!charge_on) {
        iot_debounce_set_clr_counter_flag(0);
    }

    if (!charge_on) {
        charging = false;
        first_charging = false;
        if (!btn_wakeup) {
            callback(CHARGER_EVT_POWER_ON, NULL, 0);
        }
    } else {
        in_box = true;
        callback(CHARGER_EVT_PUT_IN, NULL, 0);
    }

    last_charge_on = charge_on;
    stage = STAGE_IDLE;
    last_time = os_boot_time32();

    battery_charger_change_register_callback(handle_charge_flag);
}

#endif   //CHARGER_BOX == CHARGER_BOX_WQ
