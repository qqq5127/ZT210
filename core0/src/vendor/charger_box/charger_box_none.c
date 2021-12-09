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
#define CHARGER_BOX_NONE 1

#pragma GCC diagnostic ignored "-Wundef"

#if CHARGER_BOX == CHARGER_BOX_NONE
#include "os_timer.h"
#include "os_utils.h"
#include "battery_charger.h"
#include "lib_dbglog.h"
#include "boot_reason.h"
#include "charger_box.h"
#include "iot_debounce.h"

#define CHARGER_MSG_DEBUG 1

#undef DBGLOG_CHARGER_DBG
#undef DBGLOG_CHARGER_ERR

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

#define OP_TIMEOUT_MS            100
#define OP_BT_DISABLE_TIMEOUT_MS 3000   //3s

static charger_callback_t callback = NULL;
static timer_id_t timer = 0;
static timer_id_t timer_bt_disable = 0;
static bool_t in_box = false;
static bool_t btn_wakeup = false;
static bool_t is_hw_reset_enabled = false;

static void timer_func(timer_id_t timer_id, void *arg)
{
    UNUSED(timer_id);
    UNUSED(arg);

    bool_t charge_on = battery_charger_get_flag();

    DBGLOG_CHARGER_DBG("charger level change timeout flag : %d\n", charge_on);

    if (charge_on) {
        os_start_timer(timer_bt_disable, OP_BT_DISABLE_TIMEOUT_MS);
    } else {
        callback(CHARGER_EVT_TAKE_OUT, NULL, 0);
        if (in_box) {
            callback(CHARGER_EVT_POWER_ON, NULL, 0);
        }
        in_box = false;
    }
}

static void handle_charge_flag(uint8_t flag, uint32_t peried)
{
    UNUSED(peried);
    bool_t charge_on = flag;

    iot_debounce_set_clr_counter_flag(1);
    DBGLOG_CHARGER_DBG("charger disable hw reset\n");

    if (!callback) {
        return;
    }

    if (charge_on) {
        if (!in_box) {
            in_box = true;
            callback(CHARGER_EVT_PUT_IN, NULL, 0);
        }
    } else {
        is_hw_reset_enabled = true;
    }

    if (os_is_timer_active(timer)) {
        os_stop_timer(timer);
    }

    if (os_is_timer_active(timer_bt_disable)) {
        os_stop_timer(timer_bt_disable);
    }

    os_start_timer(timer, OP_TIMEOUT_MS);
}

static void timer_bt_disable_func(timer_id_t timer_id, void *arg)
{
    UNUSED(timer_id);
    UNUSED(arg);

    callback(CHARGER_EVT_BT_DISABLE, NULL, 0);
    if (is_hw_reset_enabled) {
        iot_debounce_set_clr_counter_flag(0);
        DBGLOG_CHARGER_DBG("charger enable hw reset\n");
    }
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
        if (!btn_wakeup) {
            callback(CHARGER_EVT_POWER_ON, NULL, 0);
        }
        is_hw_reset_enabled = true;
    } else {
        in_box = true;
        is_hw_reset_enabled = false;
        callback(CHARGER_EVT_PUT_IN, NULL, 0);
    }

    battery_charger_change_register_callback(handle_charge_flag);
}
#endif
