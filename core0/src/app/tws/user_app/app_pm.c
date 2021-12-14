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
#include "os_utils.h"
#include "boot_reason.h"
#include "power_mgnt.h"
#include "app_pm.h"
#include "app_main.h"
#include "app_bt.h"
#include "app_bat.h"
#include "ro_cfg.h"
#include "usr_cfg.h"
#include "app_btn.h"
#include "battery_charger.h"
#include "app_charger.h"
#include "boot_reason.h"
#include "app_tone.h"
#include "ota_task.h"
#include "app_led.h"
#include "app_evt.h"
#include "app_wws.h"
#include "dfs.h"

#define APP_PM_CRASH_RESET_FLAG_POWER_ON  0x70
#define APP_PM_CRASH_RESET_FLAG_POWER_OFF 0x33

#define APP_PM_REBOOT_FLAG_USER            0
#define APP_PM_REBOOT_FLAG_CHARGER         1
#define APP_PM_REBOOT_FLAG_SHUTDOWN_FAILED 2
#define APP_PM_REBOOT_FLAG_FACTORY_RESET   3
#define APP_PM_REBOOT_FLAG_OTA             4

#define PM_MSG_ID_POWER_OFF              1
#define PM_MSG_ID_REBOOT                 2
#define PM_MSG_ID_ENABLE_POWER_OFF       3
#define PM_MSG_ID_FORCE_SHUTDOWN         4
#define PM_MSG_ID_FORCE_REBOOT           5
#define PM_MSG_ID_CORE_DUMP              6
#define PM_MSG_ID_BT_POWER_OFF_DONE      7
#define PM_MSG_ID_AUTO_POWER_OFF         8
#define PM_REMOTE_MSG_ID_SYNC_POWER_OFF  9
#define PM_REMOTE_MSG_ID_START_POWER_OFF 10

#ifndef MAX_BT_DISABLE_WAIT_TIME_MS
#define MAX_BT_DISABLE_WAIT_TIME_MS 3000
#endif

#ifndef MAX_BTN_RELEASE_WAIT_TIME_MS
#define MAX_BTN_RELEASE_WAIT_TIME_MS 20000
#endif

#ifndef MAX_TONE_DONE_WAIT_TIME_MS
#define MAX_TONE_DONE_WAIT_TIME_MS 2000
#endif

#ifndef SHUTDOWN_FAIL_TIME_MS
#define SHUTDOWN_FAIL_TIME_MS 5000
#endif

#ifndef WAKEUP_POWER_ON_TIMEOUT_MS
#define WAKEUP_POWER_ON_TIMEOUT_MS 15000
#endif

#ifndef BATTERY_FULL_SHUTDOWN_WAIT_TIME_MS
#define BATTERY_FULL_SHUTDOWN_WAIT_TIME_MS 15000
#endif

#ifndef AUTO_POWER_OFF_SYNC_WAIT_TIME_MS
#define AUTO_POWER_OFF_SYNC_WAIT_TIME_MS 3000
#endif

typedef struct {
    pm_power_on_reason_t power_on_reason;
    pm_reboot_reason_t reboot_reason;
    bool_t power_off_disabled;
    bool_t shutdown_pending;
    bool_t reboot_pending;
    bool_t auto_power_off_disabled;
    uint16_t auto_power_off_timeout;
    bool_t shutdown_ending;
    bool_t reboot_ending;
} app_pm_context_t;

static app_pm_context_t _context = {0};
static app_pm_context_t *context = &_context;

static uint8_t soft_reset_flag;
static BOOT_REASON_SOFT_REASON soft_reset_reason;

static void do_init(void);

static inline void start_auto_power_off(uint32_t timeout_ms)
{
    if (timeout_ms == 0) {
        DBGLOG_PM_DBG("start_auto_power_off timeout==0\n");
        return;
    }

    app_cancel_msg(MSG_TYPE_PM, PM_MSG_ID_POWER_OFF);
    app_cancel_msg(MSG_TYPE_PM, PM_MSG_ID_AUTO_POWER_OFF);

    if (AUTO_POWER_OFF_SYNC_WAIT_TIME_MS && (app_bt_get_sys_state() > STATE_DISABLED)) {
        app_send_msg_delay(MSG_TYPE_PM, PM_MSG_ID_AUTO_POWER_OFF, NULL, 0, timeout_ms);
    } else {
        app_send_msg_delay(MSG_TYPE_PM, PM_MSG_ID_POWER_OFF, NULL, 0, timeout_ms);
    }
}

static inline void stop_auto_power_off(void)
{
    app_cancel_msg(MSG_TYPE_PM, PM_MSG_ID_POWER_OFF);
    app_cancel_msg(MSG_TYPE_PM, PM_MSG_ID_AUTO_POWER_OFF);
}

static void wait_tone_led_anc_done(void)
{
    bool_t tone_playing;
    bool_t led_playing;
    bool_t anc_switching;

    for (int i = 1; i <= MAX_TONE_DONE_WAIT_TIME_MS / 100; i++) {
        app_handle_pending_message();

        tone_playing = app_tone_is_playing();
        led_playing = app_led_is_event_playing();
        anc_switching = app_audio_is_anc_switching();

        if ((!tone_playing) && (!led_playing) && (!anc_switching)) {
            DBGLOG_PM_DBG("wait (tone %d, led %d, anc %d) done\n", tone_playing, led_playing,
                          anc_switching);
            break;
        }
        if (i % 5 == 0) {
            DBGLOG_PM_ERR("wait (tone %d, led %d, anc %d)\n", tone_playing, led_playing,
                          anc_switching);
        }
        os_delay(100);
        if (i == MAX_TONE_DONE_WAIT_TIME_MS / 100) {
            DBGLOG_PM_ERR("wait (tone %d, led %d, anc %d) timeout!\n", tone_playing, led_playing,
                          anc_switching);
            app_tone_cancel_all(true);
        }
    }
}

static void do_shutdown(void)
{
    if ((context->reboot_ending) || (context->shutdown_ending)) {
        DBGLOG_PM_DBG("duplicate shutdown ignored\n");
        return;
    }

    DBGLOG_PM_DBG("[auto]do_shutdown\n");
    context->shutdown_ending = true;

    wait_tone_led_anc_done();

    app_deinit();
    os_delay(100);   //wait flash write done

    /* dfs deinit must before bt/dsp shutdown */
    dfs_deinit();

    bt_handle_user_shutdown();

    if (app_btn_has_btn_power_on()) {
        for (int i = 1; i <= MAX_BTN_RELEASE_WAIT_TIME_MS / 100; i++) {
            if (app_btn_all_released()) {
                break;
            }
            if (i % 5 == 0) {
                DBGLOG_PM_DBG("please release the button to power off!\n");
            }
            os_delay(100);
            if (i == MAX_BTN_RELEASE_WAIT_TIME_MS / 100) {
                DBGLOG_PM_ERR("wait button release timeout, force shutdown!\n");
            }
        }
    }

    DBGLOG_PM_DBG("[auto]power_mgnt_shut_down\n");
    power_mgnt_shut_down();

    os_delay(SHUTDOWN_FAIL_TIME_MS);
    DBGLOG_PM_ERR("shutdown failed, force reboot\n");
    boot_reason_system_reset(BOOT_REASON_SOFT_REASON_APP, APP_PM_REBOOT_FLAG_SHUTDOWN_FAILED);
    while (1) {
        os_delay(10000);
    }
}

static void pm_power_off(void)
{
    DBGLOG_PM_DBG("pm_power_off\n");

    boot_reason_set_crash_flag(APP_PM_CRASH_RESET_FLAG_POWER_OFF);

    stop_auto_power_off();
    app_tone_cancel_all(true);
    app_bt_power_off();

    if (app_charger_is_charging() && (!app_bat_is_full())
        && (battery_charger_get_cur_limit() != 0)) {
        DBGLOG_PM_DBG("power off when charging, ignored\n");
        return;
    }

    if (app_bt_get_sys_state() <= STATE_DISABLED) {
        do_shutdown();
    } else {
        context->shutdown_pending = true;
        app_send_msg_delay(MSG_TYPE_PM, PM_MSG_ID_FORCE_SHUTDOWN, NULL, 0,
                           MAX_BT_DISABLE_WAIT_TIME_MS);
    }
}

static void do_reboot(void)
{
    uint8_t flag;

    if ((context->reboot_ending) || (context->shutdown_ending)) {
        DBGLOG_PM_DBG("duplicate reboot ignored\n");
        return;
    }

    if (app_charger_get_box_state() == BOX_STATE_CLOSED) {
        if ((context->reboot_reason == PM_REBOOT_REASON_FACTORY_RESET)
            || (context->reboot_reason == PM_REBOOT_REASON_OTA)) {
            context->reboot_reason = PM_REBOOT_REASON_CHARGER;
            DBGLOG_PM_DBG("box close, do not enable bt after reboot\n");
        }
    }

    context->reboot_ending = true;

    DBGLOG_PM_DBG("do_reboot\n");

    wait_tone_led_anc_done();

    app_deinit();
    DBGLOG_PM_DBG("boot_reason_system_reset\n");
    os_delay(100);   //wait flash write done

    switch (context->reboot_reason) {
        case PM_REBOOT_REASON_USER:
            flag = APP_PM_REBOOT_FLAG_USER;
            break;
        case PM_REBOOT_REASON_CHARGER:
            flag = APP_PM_REBOOT_FLAG_CHARGER;
            break;
        case PM_REBOOT_REASON_FACTORY_RESET:
            flag = APP_PM_REBOOT_FLAG_FACTORY_RESET;
            break;
        case PM_REBOOT_REASON_OTA:
            flag = APP_PM_REBOOT_FLAG_OTA;
            break;
        default:
            flag = APP_PM_REBOOT_FLAG_CHARGER;
            break;
    }

    boot_reason_system_reset(BOOT_REASON_SOFT_REASON_APP, flag);
    while (1) {
        os_delay(10000);
    }
}

static void app_pm_handle_msg(uint16_t msg_id, void *param)
{
    UNUSED(param);

    switch (msg_id) {
        case PM_MSG_ID_POWER_OFF:
            if (app_bt_is_in_audio_test_mode()) {
                DBGLOG_PM_DBG("PM_MSG_ID_POWER_OFF ignored by dut mode\n");
            } else if (ota_task_is_running()) {
                start_auto_power_off(((uint32_t)context->auto_power_off_timeout) * 1000);
            } else {
                DBGLOG_PM_DBG("power off!\n");
                pm_power_off();
            }
            break;
        case PM_MSG_ID_AUTO_POWER_OFF:
            app_evt_send(EVTSYS_AUTO_POWER_OFF);
            if (app_wws_is_connected_master()) {
                app_wws_send_remote_msg(MSG_TYPE_PM, PM_REMOTE_MSG_ID_SYNC_POWER_OFF, 0, NULL);
            }
            stop_auto_power_off();
            app_send_msg_delay(MSG_TYPE_PM, PM_MSG_ID_POWER_OFF, NULL, 0,
                               AUTO_POWER_OFF_SYNC_WAIT_TIME_MS);
            break;
        case PM_MSG_ID_REBOOT:
            app_pm_reboot(PM_REBOOT_REASON_USER);
            break;
        case PM_MSG_ID_ENABLE_POWER_OFF:
            context->power_off_disabled = false;
            DBGLOG_PM_DBG("power off enabled!\n");
            break;
        case PM_MSG_ID_FORCE_SHUTDOWN:
            DBGLOG_PM_ERR("wait for bt_off timeout, force reset\n");
            //do_shutdown(); //force shutdown may not work when bt off failed
            boot_reason_system_reset(BOOT_REASON_SOFT_REASON_APP,
                                     APP_PM_REBOOT_FLAG_SHUTDOWN_FAILED);
            break;
        case PM_MSG_ID_FORCE_REBOOT:
            DBGLOG_PM_ERR("wait for bt_off timeout, force reboot\n");
            do_reboot();
            break;
        case PM_MSG_ID_CORE_DUMP:
            assert(0);
            break;
        case PM_MSG_ID_BT_POWER_OFF_DONE:
            DBGLOG_PM_DBG("PM_MSG_ID_BT_POWER_OFF_DONE\n");
            if (context->shutdown_pending) {
                context->shutdown_pending = false;
                app_cancel_msg(MSG_TYPE_PM, PM_MSG_ID_FORCE_SHUTDOWN);
                do_shutdown();
            } else if (context->reboot_pending) {
                context->reboot_pending = false;
                app_cancel_msg(MSG_TYPE_PM, PM_MSG_ID_FORCE_REBOOT);
                do_reboot();
            } else if (((!app_charger_is_charging()) || (battery_charger_get_cur_limit() == 0))
                       && (app_bt_get_sys_state() <= STATE_DISABLED)) {
                if (app_bat_get_level() == 100) {
                    start_auto_power_off(BATTERY_FULL_SHUTDOWN_WAIT_TIME_MS);
                } else {
                    start_auto_power_off(WAKEUP_POWER_ON_TIMEOUT_MS);
                }
            } else {
                DBGLOG_PM_DBG("PM_MSG_ID_BT_POWER_OFF_DONE charging:%d state:0x%x\n",
                              app_charger_is_charging(), app_bt_get_sys_state());
            }
            break;
        case PM_REMOTE_MSG_ID_SYNC_POWER_OFF:
            if (context->auto_power_off_disabled) {
                DBGLOG_PM_ERR("PM_REMOTE_MSG_ID_SYNC_POWER_OFF when off disabled, ignore\n");
            } else if (app_bt_get_sys_state() >= STATE_CONNECTED) {
                DBGLOG_PM_ERR("PM_REMOTE_MSG_ID_SYNC_POWER_OFF when connected, ignore\n");
            } else if (app_bt_get_sys_state() <= STATE_DISABLED) {
                DBGLOG_PM_ERR("PM_REMOTE_MSG_ID_SYNC_POWER_OFF when bt disabled, ignore\n");
            } else {
                stop_auto_power_off();
                app_send_msg_delay(MSG_TYPE_PM, PM_MSG_ID_POWER_OFF, NULL, 0,
                                   AUTO_POWER_OFF_SYNC_WAIT_TIME_MS);
            }
            break;
        case PM_REMOTE_MSG_ID_START_POWER_OFF:
            if (context->auto_power_off_disabled) {
                DBGLOG_PM_ERR("PM_REMOTE_MSG_ID_START_POWER_OFF when off disabled, ignore\n");
            } else if (app_bt_get_sys_state() >= STATE_CONNECTED) {
                DBGLOG_PM_ERR("PM_REMOTE_MSG_ID_START_POWER_OFF when connected, ignore\n");
            } else if (app_bt_get_sys_state() <= STATE_DISABLED) {
                DBGLOG_PM_ERR("PM_REMOTE_MSG_ID_START_POWER_OFF when bt disabled, ignore\n");
            } else {
                uint32_t timeout = *((uint32_t *)param);
                stop_auto_power_off();
                app_send_msg_delay(MSG_TYPE_PM, PM_MSG_ID_AUTO_POWER_OFF, NULL, 0, timeout);
            }
            break;
        default:
            break;
    }
}

static void generate_power_on_reason(void)
{
    BOOT_REASON_TYPE boot_reason = boot_reason_get_reason();
    DBGLOG_PM_DBG("boot_reason = %d\n ", boot_reason);

    switch (boot_reason) {
        case BOOT_REASON_WDT:
            context->power_on_reason = PM_POWER_ON_REASON_WDT_CRASH;
            DBGLOG_PM_DBG("BOOT_REASON_WDT\n");
            break;
        case BOOT_REASON_POR:
            context->power_on_reason = PM_POWER_ON_REASON_PWR_RESET;
            DBGLOG_PM_DBG("BOOT_REASON_POR\n");
            break;
        case BOOT_REASON_SLEEP: {
            DBGLOG_PM_DBG("BOOT_REASON_SLEEP\n");
            BOOT_REASON_WAKEUP_SOURCE wakeup_source = boot_reason_get_wakeup_source();
            if (wakeup_source == BOOT_REASON_WAKEUP_SRC_GPIO) {
                context->power_on_reason = PM_POWER_ON_REASON_GPIO;
                DBGLOG_PM_DBG("GPIO wakeup_source\n");
            } else if (wakeup_source == BOOT_REASON_WAKEUP_SRC_TK) {
                context->power_on_reason = PM_POWER_ON_REASON_GPIO;
                DBGLOG_PM_DBG("touchkey wakeup_source\n");
            } else if (wakeup_source == BOOT_REASON_WAKEUP_SRC_DEB) {
                context->power_on_reason = PM_POWER_ON_REASON_GPIO;
                DBGLOG_PM_DBG("debounce wakeup_source\n");
            } else if (wakeup_source == BOOT_REASON_WAKEUP_SRC_CHARGER_ON) {
                context->power_on_reason = PM_POWER_ON_REASON_CHARGER;
                DBGLOG_PM_DBG("Charger wakeup_source\n");
            } else {
                DBGLOG_PM_DBG("Other wakeup_source = %d\n", wakeup_source);
            }
        } break;
        case BOOT_REASON_SOFT: {
            soft_reset_reason = boot_reason_get_soft_reset_reason(&soft_reset_flag);
            DBGLOG_PM_DBG("BOOT_REASON_SOFT reason:%d flag:%d\n ", soft_reset_reason,
                          soft_reset_flag);

            switch (soft_reset_reason) {
                case BOOT_REASON_SOFT_REASON_UNKNOWN:
                    context->power_on_reason = PM_POWER_ON_REASON_REBOOT;
                    DBGLOG_PM_ERR("unknown soft reset reason:%d\n", soft_reset_reason);
                    break;
                case BOOT_REASON_SOFT_REASON_EXCEPTION:
                    context->power_on_reason = PM_POWER_ON_REASON_WDT_CRASH;
                    break;
                case BOOT_REASON_SOFT_REASON_SBL:
                    context->power_on_reason = PM_POWER_ON_REASON_WDT_CRASH;
                    break;
                case BOOT_REASON_SOFT_REASON_APP:
                    if (soft_reset_flag == APP_PM_REBOOT_FLAG_SHUTDOWN_FAILED) {
                        app_btn_open_sensor();   //enable key_sensor
                        os_delay(3000);          //delay sometime to wait bt core start
                        app_btn_deinit();
                        DBGLOG_PM_DBG("reboot after shutdown failed, shutdown again.\n");
                        bt_handle_user_shutdown();
                        power_mgnt_shut_down();
                        os_delay(3000);
                        assert(0);
                        while (1) {
                        }
                    } else if (soft_reset_flag == APP_PM_REBOOT_FLAG_FACTORY_RESET) {
                        context->power_on_reason = PM_POWER_ON_REASON_FACTORY_RESET;
                    } else if (soft_reset_flag == APP_PM_REBOOT_FLAG_OTA) {
                        context->power_on_reason = PM_POWER_ON_REASON_OTA;
                    } else {
                        context->power_on_reason = PM_POWER_ON_REASON_REBOOT;
                    }
                    break;
                case BOOT_REASON_SOFT_REASON_WDT_TIMEOUT:
                    context->power_on_reason = PM_POWER_ON_REASON_WDT_CRASH;
                    break;
                case BOOT_REASON_SOFT_REASON_SYS:
                    context->power_on_reason = PM_POWER_ON_REASON_REBOOT;
                    break;
                case BOOT_REASON_SOFT_REASON_OTA:
                    context->power_on_reason = PM_POWER_ON_REASON_OTA;
                    break;
                case BOOT_REASON_SOFT_REASON_UVP:
                    context->power_on_reason = PM_POWER_ON_REASON_PWR_RESET;
                    break;
                default:
                    context->power_on_reason = PM_POWER_ON_REASON_REBOOT;
                    DBGLOG_PM_ERR("unknown soft reset reason:%d\n", soft_reset_reason);
                    break;
            }
        } break;
        case BOOT_REASON_HARD:
            DBGLOG_PM_DBG("BOOT_REASON_HARD");
            context->power_on_reason = PM_POWER_ON_REASON_HW_RESET;
            break;
        default:
            break;
    }
}

static void handle_crash_power_on(void)
{
    uint8_t charger_flag = battery_charger_get_flag();
    uint8_t crash_flag = boot_reason_get_crash_flag();

    DBGLOG_PM_DBG("power on after wdt, charging:%d crash_flag:%d\n", charger_flag, crash_flag);

    if (crash_flag == APP_PM_CRASH_RESET_FLAG_POWER_ON) {
        app_pm_power_on();
    } else if (crash_flag == APP_PM_CRASH_RESET_FLAG_POWER_OFF) {
        start_auto_power_off(WAKEUP_POWER_ON_TIMEOUT_MS);
    } else {
        DBGLOG_PM_ERR("unknown crash soft reset flag:%d\n", crash_flag);
        start_auto_power_off(WAKEUP_POWER_ON_TIMEOUT_MS);
    }
}

static void handle_reset_power_on(void)
{
    uint8_t charger_flag = battery_charger_get_flag();

    DBGLOG_PM_DBG("power on after reset, charging:%d\n", charger_flag);
    if (!charger_flag) {   //filter out UVP POR
        app_pm_power_on();
    } else {
        start_auto_power_off(WAKEUP_POWER_ON_TIMEOUT_MS);
    }
}

static void handle_gpio_power_on(void)
{
    DBGLOG_PM_DBG("power on by gpio\n");
    start_auto_power_off(WAKEUP_POWER_ON_TIMEOUT_MS);
}

static void handle_charger_power_on(void)
{
    DBGLOG_PM_DBG("power on by charger, wait for box open\n");
    start_auto_power_off(WAKEUP_POWER_ON_TIMEOUT_MS);
}

static void handle_reboot_power_on(void)
{
    DBGLOG_PM_DBG("power on after reboot, reason:%d, flag:%d\n", soft_reset_reason,
                  soft_reset_flag);
    if ((soft_reset_flag == APP_PM_REBOOT_FLAG_FACTORY_RESET)
        || (soft_reset_flag == APP_PM_REBOOT_FLAG_OTA)
        || (soft_reset_flag == APP_PM_REBOOT_FLAG_USER)
        || (soft_reset_reason == BOOT_REASON_SOFT_REASON_OTA)) {
        app_pm_power_on();
    } else {
        start_auto_power_off(WAKEUP_POWER_ON_TIMEOUT_MS);
    }
}

static void do_init(void)
{
    DBGLOG_PM_DBG("do_init\n");

    if (context->power_on_reason == PM_POWER_ON_REASON_UNKNOWN) {
        generate_power_on_reason();
    }

    switch (context->power_on_reason) {
        case PM_POWER_ON_REASON_WDT_CRASH:
            handle_crash_power_on();
            break;
        case PM_POWER_ON_REASON_PWR_RESET:
            handle_reset_power_on();
            break;
        case PM_POWER_ON_REASON_GPIO:
            handle_gpio_power_on();
            break;
        case PM_POWER_ON_REASON_CHARGER:
            handle_charger_power_on();
            break;
        case PM_POWER_ON_REASON_REBOOT:
        case PM_POWER_ON_REASON_FACTORY_RESET:
        case PM_POWER_ON_REASON_OTA:
            handle_reboot_power_on();
            break;
        case PM_POWER_ON_REASON_HW_RESET:
            start_auto_power_off(WAKEUP_POWER_ON_TIMEOUT_MS);
            break;
        case PM_POWER_ON_REASON_UNKNOWN:
        default:
            DBGLOG_PM_ERR("pm unknown power on reason\n");
            start_auto_power_off(WAKEUP_POWER_ON_TIMEOUT_MS);
            break;
    }
}

void app_pm_power_on(void)
{
    if (app_bt_is_in_audio_test_mode() || app_bt_is_in_dut_mode()) {
        return;
    }

    boot_reason_set_crash_flag(APP_PM_CRASH_RESET_FLAG_POWER_ON);

    if (context->shutdown_ending) {
        DBGLOG_PM_DBG("app_pm_power_on when shutdown ending, reboot\n");
        boot_reason_system_reset(BOOT_REASON_SOFT_REASON_APP, APP_PM_REBOOT_FLAG_USER);
        while (1) {
            os_delay(10000);
        }
        return;
    }

    DBGLOG_PM_DBG("app_pm_power_on\n");

    if (ro_feat_cfg()->auto_power_off) {
        if (app_bt_get_sys_state() <= STATE_DISABLED) {
            start_auto_power_off(WAKEUP_POWER_ON_TIMEOUT_MS);
        } else if (app_bt_get_sys_state() < STATE_CONNECTED) {
            if (context->auto_power_off_disabled) {
                DBGLOG_PM_DBG("app_pm_power_on, auto power off disabled\n");
            } else {
                start_auto_power_off(((uint32_t)context->auto_power_off_timeout) * 1000);
            }
        } else {
            DBGLOG_PM_DBG("app_pm_power_on when connected\n");
        }
    } else {
        stop_auto_power_off();
    }

    app_bt_power_on();

    if (ro_to_cfg()->disable_power_off_after_power_on
        && context->power_on_reason != PM_POWER_ON_REASON_PWR_RESET) {
        DBGLOG_PM_DBG("disable power off %d seconds\n",
                      ro_to_cfg()->disable_power_off_after_power_on);
        context->power_off_disabled = true;
        app_send_msg_delay(MSG_TYPE_PM, PM_MSG_ID_ENABLE_POWER_OFF, NULL, 0,
                           ro_to_cfg()->disable_power_off_after_power_on * 1000);
    }
}

/* user power off*/
void app_pm_power_off(void)
{
    if (context->power_off_disabled) {
        DBGLOG_PM_DBG("app_pm_power_off, not enabled\n");
        return;
    }

    pm_power_off();
}

void app_pm_reboot(pm_reboot_reason_t reason)
{
    DBGLOG_PM_DBG("app_pm_reboot\n");

    if (app_pm_is_shuting_down()) {
        uint8_t flag = APP_PM_REBOOT_FLAG_USER;

        switch (reason) {
            case PM_REBOOT_REASON_USER:
                flag = APP_PM_REBOOT_FLAG_USER;
                boot_reason_set_crash_flag(APP_PM_CRASH_RESET_FLAG_POWER_ON);
                break;
            case PM_REBOOT_REASON_CHARGER:
                flag = APP_PM_REBOOT_FLAG_CHARGER;
                boot_reason_set_crash_flag(APP_PM_CRASH_RESET_FLAG_POWER_OFF);
                break;
            case PM_REBOOT_REASON_FACTORY_RESET:
                flag = APP_PM_REBOOT_FLAG_FACTORY_RESET;
                boot_reason_set_crash_flag(APP_PM_CRASH_RESET_FLAG_POWER_ON);
                break;
            case PM_REBOOT_REASON_OTA:
                flag = APP_PM_REBOOT_FLAG_OTA;
                boot_reason_set_crash_flag(APP_PM_CRASH_RESET_FLAG_POWER_ON);
                break;
            default:
                flag = APP_PM_REBOOT_FLAG_CHARGER;
                boot_reason_set_crash_flag(APP_PM_CRASH_RESET_FLAG_POWER_OFF);
                break;
        }

        DBGLOG_PM_ERR("app_pm_reboot when shuting down reason:%d\n", reason);
        boot_reason_system_reset(BOOT_REASON_SOFT_REASON_APP, flag);
        return;
    }

    context->reboot_reason = reason;
    stop_auto_power_off();
    app_bt_power_off();

    if (app_bt_get_sys_state() <= STATE_DISABLED) {
        do_reboot();
    } else {
        context->reboot_pending = true;
        app_send_msg_delay(MSG_TYPE_PM, PM_MSG_ID_FORCE_REBOOT, NULL, 0,
                           MAX_BT_DISABLE_WAIT_TIME_MS);
    }
}

pm_power_on_reason_t app_pm_get_power_on_reason(void)
{
    return context->power_on_reason;
}

bool_t app_pm_is_power_off_enabled(void)
{
    return !(context->power_off_disabled);
}

void app_pm_set_auto_power_off_timeout(uint16_t timeout_s)
{
    context->auto_power_off_timeout = timeout_s;
}

void app_pm_handle_bt_connected(void)
{
    stop_auto_power_off();
}

void app_pm_handle_bt_disconnected(void)
{
    if (ro_feat_cfg()->auto_power_off) {
        if (context->auto_power_off_disabled) {
            DBGLOG_PM_DBG("app_pm_handle_bt_disconnected, auto power off disabled\n");
        } else {
            start_auto_power_off(((uint32_t)context->auto_power_off_timeout) * 1000);
            if (app_wws_is_connected_master()) {
                uint32_t timeout = ((uint32_t)context->auto_power_off_timeout) * 1000;
                app_wws_send_remote_msg(MSG_TYPE_PM, PM_REMOTE_MSG_ID_START_POWER_OFF,
                                        sizeof(uint32_t), (uint8_t *)&timeout);
            }
        }
    }
}

void app_pm_handle_bt_power_on(void)
{
    if (ro_feat_cfg()->auto_power_off) {
        if (context->auto_power_off_disabled) {
            DBGLOG_PM_DBG("app_pm_handle_bt_power_on, auto power off disabled\n");
            stop_auto_power_off();
        } else {
            start_auto_power_off(((uint32_t)context->auto_power_off_timeout) * 1000);
        }
    } else {
        stop_auto_power_off();
    }
}

void app_pm_handle_bt_power_off(void)
{
    app_send_msg(MSG_TYPE_PM, PM_MSG_ID_BT_POWER_OFF_DONE, NULL, 0);
}

void app_pm_handle_battery_full(void)
{
    if (app_charger_is_charging() && (app_bt_get_sys_state() <= STATE_DISABLED)) {
        DBGLOG_PM_DBG("battery full, power off\n");
        start_auto_power_off(BATTERY_FULL_SHUTDOWN_WAIT_TIME_MS);
    } else {
        DBGLOG_PM_DBG("app_pm_handle_battery_full charging:%d sys_state:0x%X\n",
                      app_charger_is_charging(), app_bt_get_sys_state());
    }
}

void app_pm_handle_battery_critical_low(void)
{
    if ((!app_charger_is_charging()) || (battery_charger_get_cur_limit() == 0)) {
        DBGLOG_PM_DBG("battery critical low, charging:%d cur_limit:%d power off\n",
                      app_charger_is_charging(), battery_charger_get_cur_limit());
        if (app_bt_get_sys_state() > STATE_DISABLED) {
            app_evt_send(EVTSYS_BAT_LOW_POWER_OFF);
        }
        pm_power_off();
    }
}

void app_pm_handle_charge_off(void)
{
    if (app_bt_get_sys_state() <= STATE_DISABLED) {
        DBGLOG_PM_DBG("charge off, start auto power off\n");
        if (app_bat_get_level() == 100) {
            start_auto_power_off(BATTERY_FULL_SHUTDOWN_WAIT_TIME_MS);
        } else {
            start_auto_power_off(WAKEUP_POWER_ON_TIMEOUT_MS);
        }
    }
}

void app_pm_handle_charger_disable_bt(void)
{
    boot_reason_set_crash_flag(APP_PM_CRASH_RESET_FLAG_POWER_OFF);

    app_cancel_msg(MSG_TYPE_PM, PM_MSG_ID_ENABLE_POWER_OFF);
    context->power_off_disabled = false;

    app_bt_power_off();
}

void app_pm_init(void)
{
    DBGLOG_PM_DBG("app_pm_init\n");
    app_register_msg_handler(MSG_TYPE_PM, app_pm_handle_msg);
    context->auto_power_off_timeout = ro_to_cfg()->auto_power_off;
    do_init();
}

void app_pm_deinit(void)
{
}

void app_pm_set_auto_power_off_enabled(bool_t enabled)
{
    context->auto_power_off_disabled = !enabled;

    DBGLOG_PM_DBG("app_pm_set_auto_power_off_enabled enabled:%d\n", enabled);

    if (app_bt_get_sys_state() <= STATE_DISABLED) {
        DBGLOG_PM_DBG("app_pm_set_auto_power_off_enabled STATE_DISABLED");
        return;
    }

    if (app_bt_get_sys_state() >= STATE_CONNECTED) {
        DBGLOG_PM_DBG("app_pm_set_auto_power_off_enabled STATE_CONNECTED");
        return;
    }

    if (!context->auto_power_off_disabled) {
        if (ro_feat_cfg()->auto_power_off) {
            start_auto_power_off(((uint32_t)context->auto_power_off_timeout) * 1000);
            if (app_wws_is_connected_master()) {
                uint32_t timeout = ((uint32_t)context->auto_power_off_timeout) * 1000;
                app_wws_send_remote_msg(MSG_TYPE_PM, PM_REMOTE_MSG_ID_START_POWER_OFF,
                                        sizeof(uint32_t), (uint8_t *)&timeout);
            }
        }
    } else {
        stop_auto_power_off();
    }
}

bool_t app_pm_is_shuting_down(void)
{
    return context->shutdown_ending;
}
