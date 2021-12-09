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
#include "os_timer.h"
#include "string.h"

#include "iot_charger.h"
#include "iot_timer.h"
#include "power_mgnt.h"
#include "iot_debounce.h"
#include "iot_gpio.h"

#include "iot_share_task.h"
#include "lib_dbglog.h"
#include "storage_controller.h"

#include "battery.h"

#define BATTERY_CHARGEAR_DEBUG 1

#define BAT_CHG_FLAG_FIFO_LEN        32
#define BAT_CHG_FLAG_FIFO_IS_EMPTY() (bat_chg_flag_fifo_in == bat_chg_flag_fifo_out)
#define BAT_CHG_FLAG_FIFO_IS_FULL() \
    (((bat_chg_flag_fifo_in + 1) & (BAT_CHG_FLAG_FIFO_LEN - 1)) == bat_chg_flag_fifo_out)

//modify1:if change this,notify modify2
#define BATTERY_CHARGER_TIMER_INTERVAL_MS    1000
#define BATTERY_CHARGER_DEFAULT_TIME_S       10
#define BATTERY_CHARGER_FORCE_CHARGER_TIME_S 2
#define BATTERY_CV_TIMEOUT_S                 5400
//judge cv mode time
#define BATTERY_CV_JUDGE_TIMERS         60
#define BATTERY_ADC_ACCURACY_MV         60
#define BATTERY_CHARGER_FLAG_TIMEOUT_MS 1000

#define BATTERY_CHARGER_FORCE_TARGERT_VOL 4260
#define BATTERY_CHARGER_RECHARGE_VOL_MV   4000

/*lint -esym(749, LIB_BATTERT_CFG_ID_MAX) not referenced */
enum {
    LIB_BATTERY_CFG_ID = LIB_BATTERY_BASE_ID,
    LIB_BATTERT_CFG_ID_MAX = LIB_BATTERY_END_ID,
};

typedef struct {
    timer_id_t charger_timer;
    /* cc 2 cv judge counter */
    uint16_t cc_cv_counter;
    /* voltage not change counter */
    uint16_t cv_counter;
    /* charger counter in current current */
    uint16_t counter;
    /* monitor delay and continue charger counter after bat full */
    uint16_t mon_full_counter;
    /* current_now */
    uint16_t current_now;
    BAT_CHARGER_MODE mode;
} battery_charger_state_t;

typedef struct {
    uint32_t last_tick;
    bool_t open_charger_flag;
} battery_charger_share_task_msg_t;

/**
 *  vol ->                   fast          cv        full      end
 *         +-----------------+-------------+----------+----------+
 *  cur -> | uvp_charger_cur |             | cur-mon  |    420s  |
 *         +-----------------+-------------+----------+----------+
 *  vol -> 0                3.4        cc-cv-flag  cur-mon-flag
 */

static battery_charger_state_t bat_charger_info;
static battery_charger_cfg_t bat_charger_cfg = {.bat_chg_full_vol = BAT_CHARGER_FULL_VOL_DEFAULT,
                                                .bat_capacity = BAT_CAPACITY_DEFAULT,
                                                .bat_chg_total_timeout =
                                                    BAT_CHG_TOTAL_TIMEOUT_DEFAULT,
                                                .bat_full_timeout = BAT_FULL_TIMEOUT_DEFAULT,
                                                .fast_charger_cur = BAT_FAST_CHARGER_CUR_DEFAULT,
                                                .dp_charger_cur = BAT_DP_CHARGER_CUR_DEFAULT,
                                                .cut_off_cur = BAT_CUT_OFF_CUR_DEFAULT};

//for detect charger flag continuous high or low
static timer_id_t battery_charger_ft_timeout_timer = 0;
static battery_charger_change_cb battery_charger_flag_cb = NULL;

static battery_charger_share_task_msg_t bat_charger_task_msg_data = {0};
static uint16_t bat_charger_cur_limit = BAT_FAST_CHARGER_CUR_DEFAULT;

static battery_state_cb bat_state_cb = NULL;

static uint32_t bat_chg_flag_fifo_in = 0;
static uint32_t bat_chg_flag_fifo_out = 0;
static uint32_t bat_chg_flag_fifo[BAT_CHG_FLAG_FIFO_LEN];
static bool_t bat_chg_last_flag;

#if BATTERY_CHARGEAR_DEBUG
static uint32_t bat_chg_shake_int_count = 0;
#endif

static void battery_charger_set_voltage(uint32_t mv)
{
    IOT_CHARGER_VOL dlta;

    //3734 is IOT_CHARGER_VOL_3734 = 0, 3772 is IOT_CHARGER_VOL_3772 = 1...
    dlta = (IOT_CHARGER_VOL)((mv - 3719) / 30);
    iot_charger_set_voltage(dlta);
#if BATTERY_CHARGEAR_DEBUG
    DBGLOG_LIB_BATTERY_INFO("[CHARGER] bat full vol:%d dlta: %d\n", mv, dlta);
#endif
}

static void battery_charger_set_charger_mon_current(uint16_t cur)
{
    //the same as set charger current
    battery_charger_set_current(cur);
}

static void check_bat_bad(void)
{
    bool_t bat_bad = false;
    uint8_t param = 0;
    uint16_t vol = iot_charger_get_vbat_mv();

    if (vol < (bat_charger_cfg.bat_chg_full_vol - BATTERY_ADC_ACCURACY_MV)) {
        bat_bad = true;
        param = BATTERY_STATE_BAD;
    } else {
        bat_bad = false;
        param = BATTERY_STATE_FULL;
    }

    battery_set_bat_bad(bat_bad);
    if (bat_state_cb) {
        bat_state_cb((BATTERY_STATE)param);
    }
}

static void battery_charger_set_mode(BAT_CHARGER_MODE mode)
{
    if (mode == bat_charger_info.mode) {
        return;
    }

    if (BAT_CHARGER_MODE_FAST == mode) {
        bat_charger_info.counter = 0;
    } else if (BAT_CHARGER_MODE_CV == mode) {
        bat_charger_info.cv_counter = 0;
    } else if (BAT_CHARGER_MODE_FULL == mode) {
        bat_charger_info.mon_full_counter = 0;
    } else if (BAT_CHARGER_MODE_STOP == mode) {
        bat_charger_info.cc_cv_counter = 0;
        bat_charger_info.cv_counter = 0;
        bat_charger_info.counter = 0;
        bat_charger_info.mon_full_counter = 0;
    } else {
    }

    bat_charger_info.mode = mode;
#if BATTERY_CHARGEAR_DEBUG
    DBGLOG_LIB_BATTERY_INFO("[CHARGER] set mode:%d\n", mode);
#endif
}

static void battery_charger_time_out(void)
{
#if BATTERY_CHARGEAR_DEBUG
    DBGLOG_LIB_BATTERY_INFO("[CHARGER] timeout,mode:%d\n", bat_charger_info.mode);
#endif
    iot_charger_clear_charger_mon_flag();
    assert(!iot_charger_get_charger_mon_flag());
    check_bat_bad();
    battery_charger_set_current(0);
    battery_charger_set_mode(BAT_CHARGER_MODE_STOP);
    if (bat_state_cb) {
        bat_state_cb(BATTERY_STATE_CHARGE_STOP);
    }
}

static bool_t battery_charger_check_cv(void)
{
    if (iot_charger_get_charger_state() == IOT_CHG_CV) {
        bat_charger_info.cc_cv_counter++;
    } else {
        bat_charger_info.cc_cv_counter = 0;
    }

    //modify2:cv flag always 1 during a peroid
    if (bat_charger_info.cc_cv_counter >= BATTERY_CV_JUDGE_TIMERS) {
        return true;
    }

    return false;
}

static void battery_charger_cmc(void)
{
    iot_charger_clear_charger_mon_flag();
    assert(!iot_charger_get_charger_mon_flag());

    if (os_is_timer_active(bat_charger_info.charger_timer)) {
        os_stop_timer(bat_charger_info.charger_timer);

        if (bat_state_cb && (bat_charger_info.mode != BAT_CHARGER_MODE_STOP)) {
            bat_state_cb(BATTERY_STATE_CHARGE_STOP);
        }
    }

    bat_charger_info.cc_cv_counter = 0;
    bat_charger_info.cv_counter = 0;
    bat_charger_info.counter = 0;
    bat_charger_info.mon_full_counter = 0;
    bat_charger_info.current_now = 0;
    bat_charger_info.mode = BAT_CHARGER_MODE_CMC;
}

static void battery_charger_event_handler(void *arg)
{
    uint16_t bat_vol_mv;
    UNUSED(arg);

    //clear deb counter every seconds
    iot_debounce_reconfig_hard_reset_time(IOT_DEBOUNCE_NO_TIMER);

    if ((!iot_charger_flag_get()) && (bat_charger_info.mode != BAT_CHARGER_MODE_CMC)) {
        battery_charger_stop();
        battery_charger_set_current(bat_charger_cfg.dp_charger_cur);
        return;
    }

    //total timeout,bat bad,stop charger state machine
    if ((bat_charger_info.counter > bat_charger_cfg.bat_chg_total_timeout
         || bat_charger_info.mon_full_counter > bat_charger_cfg.bat_full_timeout
         || bat_charger_info.cv_counter > BATTERY_CV_TIMEOUT_S)
        && (bat_charger_info.mode != BAT_CHARGER_MODE_STOP)) {
        battery_charger_time_out();
        return;
    }

    //force charger end
    if (bat_charger_info.counter == BATTERY_CHARGER_FORCE_CHARGER_TIME_S) {
        //set charger vol cur back
        battery_charger_set_voltage(bat_charger_cfg.bat_chg_full_vol);
        battery_charger_set_current(bat_charger_cfg.fast_charger_cur);
    }

    bat_charger_info.counter++;
    bat_vol_mv = battery_get_voltage_mv();

#if BATTERY_CHARGEAR_DEBUG
    if (bat_charger_info.counter % BATTERY_CHARGER_DEFAULT_TIME_S == 0) {
        DBGLOG_LIB_BATTERY_INFO("[CHARGER] mode: %d, cur:%d, vol:%d, time:%d\n",
                                bat_charger_info.mode, bat_charger_info.current_now, bat_vol_mv,
                                bat_charger_info.counter);
    }
#endif

    switch (bat_charger_info.mode) {
        case BAT_CHARGER_MODE_FAST:
            if (battery_charger_check_cv()) {
                battery_charger_set_voltage(bat_charger_cfg.bat_chg_full_vol);
                battery_charger_set_mode(BAT_CHARGER_MODE_SET_MONITOR);
                iot_charger_mon_enable();
                return;
            }
            break;
        case BAT_CHARGER_MODE_SET_MONITOR:
            battery_charger_set_charger_mon_current(bat_charger_cfg.cut_off_cur);
            battery_charger_set_mode(BAT_CHARGER_MODE_CV);
            return;
        case BAT_CHARGER_MODE_CV:
            bat_charger_info.cv_counter++;
            if (iot_charger_get_charger_mon_flag()) {
                battery_charger_set_mode(BAT_CHARGER_MODE_FULL);
                iot_charger_clear_charger_mon_flag();
                return;
            }
            break;
        case BAT_CHARGER_MODE_FULL:
            bat_charger_info.mon_full_counter++;
            break;

        case BAT_CHARGER_MODE_STOP:
            if (bat_vol_mv < BATTERY_CHARGER_RECHARGE_VOL_MV) {
                battery_charger_set_mode(BAT_CHARGER_MODE_FAST);
            }

            // Battery full, but charger flag is on, do not stop the scheduler and do nothing here.
            bat_charger_info.counter = 0;
            break;

        case BAT_CHARGER_MODE_CMC:
            battery_charger_cmc();
            break;

        case BAT_CHARGER_MODE_MAX:
            break;

        default:
            assert(0);
    }
}

static void battery_charger_timer_handler(timer_id_t timer_id, void *arg)
{
    UNUSED(timer_id);
    UNUSED(arg);

    ///* post event to share task to show CPU usage */
    iot_share_task_post_event(IOT_SHARE_TASK_QUEUE_LP, IOT_SHARE_EVENT_CHARGER_EVENT);
}

void battery_charger_start(void)
{
    power_mgnt_clear_sleep_vote(POWER_SLEEP_CHARGER);
    iot_charger_clear_charger_mon_flag();
    assert(!iot_charger_get_charger_mon_flag());
#if BATTERY_CHARGEAR_DEBUG
    DBGLOG_LIB_BATTERY_INFO(
        "[CHARGER] fast charger cur:%d\n[CHARGER] bat full vol:%d\n[CHARGER] cut off cur:%d\n[CHARGER] cmc cur:%d\n[CHARGER] continue charger timout:%d\n",
        bat_charger_cfg.fast_charger_cur, bat_charger_cfg.bat_chg_full_vol,
        bat_charger_cfg.cut_off_cur, bat_charger_cfg.dp_charger_cur,
        bat_charger_cfg.bat_full_timeout);
#endif
    battery_charger_set_mode(BAT_CHARGER_MODE_FAST);

    //close charger if charger int happen
    bat_charger_task_msg_data.open_charger_flag = false;

    if (!os_is_timer_active(bat_charger_info.charger_timer)) {
        if (bat_state_cb) {
            bat_state_cb(BATTERY_STATE_CHARGE_START);
        }

        /* start timer to run charger scheduler */
        os_start_timer(bat_charger_info.charger_timer, BATTERY_CHARGER_TIMER_INTERVAL_MS);
    }
}

void battery_charger_stop(void)
{
    iot_charger_clear_charger_mon_flag();
    if (os_is_timer_active(bat_charger_info.charger_timer)) {
        os_stop_timer(bat_charger_info.charger_timer);

        if (bat_state_cb && (bat_charger_info.mode != BAT_CHARGER_MODE_STOP)) {
            bat_state_cb(BATTERY_STATE_CHARGE_STOP);
        }
    }

    bat_charger_info.current_now = 0;
    battery_charger_set_mode(BAT_CHARGER_MODE_STOP);
    power_mgnt_set_sleep_vote(POWER_SLEEP_CHARGER);
}

static void battery_charger_read_config(void)
{
    battery_charger_cfg_t tmp = {0};
    battery_charger_cfg_t *p = (battery_charger_cfg_t *)&tmp;
    uint32_t len = sizeof(tmp);

    uint32_t ret = storage_read(LIB_BATTERY_BASE_ID, LIB_BATTERY_CFG_ID, (void *)p, &len);

    if (ret == RET_OK) {
        if (p->bat_chg_full_vol > BAT_CHARGER_FULL_VOL_MIN
            && p->bat_chg_full_vol < BAT_CHARGER_FULL_VOL_MAX) {
            memcpy(&bat_charger_cfg, p, sizeof(bat_charger_cfg));
        }
    }
    bat_charger_cur_limit = bat_charger_cfg.fast_charger_cur;
}

static void battery_charger_cmc_event_handler(void *arg)
{
    UNUSED(arg);
    uint32_t us = 0;
    uint32_t cur_tick = 0;
    uint8_t flag = 0;
    os_stop_timer(battery_charger_ft_timeout_timer);
    iot_debounce_reconfig_hard_reset_time(IOT_DEBOUNCE_ONE_SECOND);

    //if box open when in charging peroid
    if (!bat_charger_task_msg_data.open_charger_flag) {
        battery_charger_cmc();
        battery_charger_set_voltage(BATTERY_CHARGER_FORCE_TARGERT_VOL);
        battery_charger_set_current(bat_charger_cfg.dp_charger_cur);
        bat_charger_task_msg_data.open_charger_flag = true;
    }

    if (battery_charger_flag_cb) {
        while (!BAT_CHG_FLAG_FIFO_IS_EMPTY()) {
            cur_tick = bat_chg_flag_fifo[bat_chg_flag_fifo_out];
            bat_chg_flag_fifo_out = (bat_chg_flag_fifo_out + 1) & (BAT_CHG_FLAG_FIFO_LEN - 1);
            flag = cur_tick & 0x01;
            cur_tick = cur_tick & 0xFFFFFFFE;

            us = cur_tick - bat_charger_task_msg_data.last_tick;
            //software debounce shake int
            if (us < 1000) {
                os_start_timer(battery_charger_ft_timeout_timer, BATTERY_CHARGER_FLAG_TIMEOUT_MS);
                return;

            } else if (us < 100 * 1000) {
                us += 500;
            }

            bat_charger_task_msg_data.last_tick = cur_tick;
            battery_charger_flag_cb(flag, us / 1000);
            bat_chg_last_flag = (bool_t)flag;
        }
    } else {
        if (BAT_CHG_FLAG_FIFO_IS_FULL()) {
            bat_chg_flag_fifo_out = (bat_chg_flag_fifo_out + 1) & (BAT_CHG_FLAG_FIFO_LEN - 1);
        }
    }

    os_start_timer(battery_charger_ft_timeout_timer, BATTERY_CHARGER_FLAG_TIMEOUT_MS);
}

static void battery_charger_flag_timerout_handler(timer_id_t timer_id, void *arg)
{
    UNUSED(timer_id);
    UNUSED(arg);
    bool_t flag = iot_charger_flag_get();

    os_stop_timer(battery_charger_ft_timeout_timer);
    bat_charger_task_msg_data.open_charger_flag = false;
    bat_charger_task_msg_data.last_tick = iot_timer_get_time();

    if (flag) {
        iot_charger_gpio_enable(false);
        battery_charger_start();
    } else {
        iot_charger_gpio_enable(true);
        battery_charger_stop();
        battery_charger_set_current(bat_charger_cfg.dp_charger_cur);
    }
    iot_charger_int_enable();

#if BATTERY_CHARGEAR_DEBUG
    DBGLOG_LIB_BATTERY_INFO("[CHARGER] charger int count:%d,fifo_in:%d,fifo_out:%d\n",
                            bat_chg_shake_int_count, bat_chg_flag_fifo_in, bat_chg_flag_fifo_out);
    bat_chg_shake_int_count = 0;
#endif
    if(bat_chg_flag_fifo_out != bat_chg_flag_fifo_in) {
        iot_share_task_post_event(IOT_SHARE_TASK_QUEUE_HP, IOT_SHARE_EVENT_CHARGER_CMC_EVENT);
        return;
    }

    if(bat_chg_last_flag != flag) {
         if (battery_charger_flag_cb) {
             battery_charger_flag_cb((uint8_t)flag, 1000);
             bat_chg_last_flag = (bool_t)flag;
         }
    }
}

static void battery_charger_int_handler(uint8_t flag) IRAM_TEXT(battery_charger_int_handler);
static void battery_charger_int_handler(uint8_t flag)
{
    uint32_t tick;

    power_mgnt_clear_sleep_vote(POWER_SLEEP_CHARGER);
    iot_charger_clear_charger_mon_flag();

    tick = iot_timer_get_time();

    if (flag == 1) {
        iot_charger_gpio_enable(false);
    }

    tick = (tick & 0xFFFFFFFE) | (flag ? 1 : 0);

    if (BAT_CHG_FLAG_FIFO_IS_FULL()) {
        bat_chg_flag_fifo[(bat_chg_flag_fifo_in - 1) & (BAT_CHG_FLAG_FIFO_LEN - 1)] = tick;
    } else {
        bat_chg_flag_fifo[bat_chg_flag_fifo_in] = tick;
        bat_chg_flag_fifo_in = (bat_chg_flag_fifo_in + 1) & (BAT_CHG_FLAG_FIFO_LEN - 1);
    }
#if BATTERY_CHARGEAR_DEBUG
    bat_chg_shake_int_count++;
#endif
    iot_share_task_post_event_from_isr(IOT_SHARE_TASK_QUEUE_HP, IOT_SHARE_EVENT_CHARGER_CMC_EVENT);
}

void battery_charger_init(void)
{
    // Already initialized
    if (bat_charger_info.charger_timer) {
        assert(0);
    }
    iot_charger_clear_charger_mon_flag();
    battery_charger_read_config();
    battery_charger_set_voltage(BATTERY_CHARGER_FORCE_TARGERT_VOL);
    battery_charger_set_current(bat_charger_cfg.dp_charger_cur);

    //charger state timer
    bat_charger_info.charger_timer =
        os_create_timer(IOT_BATTERY_MID, true, battery_charger_timer_handler, NULL);
    assert(bat_charger_info.charger_timer);
    iot_share_task_event_register(IOT_SHARE_TASK_QUEUE_LP, IOT_SHARE_EVENT_CHARGER_EVENT,
                                  battery_charger_event_handler, NULL);

    //flag continuous high or low timer
    battery_charger_ft_timeout_timer =
        os_create_timer(IOT_BATTERY_MID, true, battery_charger_flag_timerout_handler, NULL);
    assert(battery_charger_ft_timeout_timer);

    //for flag and tick send
    iot_share_task_event_register(IOT_SHARE_TASK_QUEUE_HP, IOT_SHARE_EVENT_CHARGER_CMC_EVENT,
                                  battery_charger_cmc_event_handler, NULL);

    bat_charger_task_msg_data.open_charger_flag = false;
    bat_charger_task_msg_data.last_tick = iot_timer_get_time();
    bat_chg_last_flag = iot_charger_flag_get();
    iot_charger_register_int_cb(IOT_CHG_INT_EDGE_BOTH, battery_charger_int_handler);
    iot_charger_int_enable();


    os_start_timer(battery_charger_ft_timeout_timer, BATTERY_CHARGER_FLAG_TIMEOUT_MS);
}

void battery_charger_set_current(uint16_t cur)
{
    if ((cur != bat_charger_info.current_now) || (cur != bat_charger_cur_limit)) {
        iot_charger_set_current(cur > bat_charger_cur_limit ? (uint16_t)(bat_charger_cur_limit * 10)
                                                            : (uint16_t)(cur * 10));
        bat_charger_info.current_now = cur;
    }
}

void battery_charger_change_register_callback(battery_charger_change_cb cb)
{
    battery_charger_flag_cb = cb;
    iot_share_task_post_event(IOT_SHARE_TASK_QUEUE_HP, IOT_SHARE_EVENT_CHARGER_CMC_EVENT);
}

uint8_t battery_charger_get_flag(void)
{
    return (uint8_t)iot_charger_flag_get();
}

void battery_state_register_callback(battery_state_cb cb)
{
    bat_state_cb = cb;
}

void battery_charger_disable_hard_reset(void)
{
    iot_debounce_disable_hard_reset();
}

void battery_charger_set_cur_limit(uint16_t cur_ma)
{
    bat_charger_cur_limit = cur_ma;

    if (bat_charger_info.mode == BAT_CHARGER_MODE_SET_MONITOR
        || bat_charger_info.mode == BAT_CHARGER_MODE_CV
        || bat_charger_info.mode == BAT_CHARGER_MODE_FULL) {
        iot_charger_clear_charger_mon_flag();
        assert(!iot_charger_get_charger_mon_flag());
        battery_charger_set_mode(BAT_CHARGER_MODE_FAST);
        bat_charger_info.current_now = bat_charger_cfg.fast_charger_cur;
    }

    battery_charger_set_current(bat_charger_info.current_now);
}

uint16_t battery_charger_get_cur_limit(void)
{
    return bat_charger_cur_limit;
}

uint8_t battery_charger_get_charged_capacity(uint8_t base_percent)
{
    uint8_t percentage = 0;
    uint16_t fast_cur = bat_charger_cfg.fast_charger_cur;
    uint16_t cut_off_cur = bat_charger_cfg.cut_off_cur;
    uint16_t cur = (uint16_t)((fast_cur - cut_off_cur) * (100 - base_percent) / 100 + cut_off_cur);
    uint16_t capacity = bat_charger_cfg.bat_capacity;
    uint16_t total_time = bat_charger_info.counter;
    uint16_t cv_time = bat_charger_info.cv_counter;
    uint16_t full_time = bat_charger_info.mon_full_counter;

    if (bat_charger_info.mode == BAT_CHARGER_MODE_FAST
        || bat_charger_info.mode == BAT_CHARGER_MODE_SET_MONITOR) {
        // percentage of power charged this time: currnet * time / capacity * 100
        percentage = (uint8_t)((cur * total_time / 36) / capacity);
    } else if (bat_charger_info.mode == BAT_CHARGER_MODE_CV) {
        // CV mode, all time - 1/2(CV time)
        percentage = (uint8_t)((cur * (total_time - cv_time / 2) / 36) / capacity);
    } else if (bat_charger_info.mode == BAT_CHARGER_MODE_FULL) {
        // FULL mode, all time - 1/2(CV time) - 1/4(FULL time)
        percentage =
            (uint8_t)((cur * ((total_time - cv_time / 2) - full_time / 4) / 36) / capacity);
    } else {
        // cmc, stop
    }

    return (uint8_t)MIN(percentage, 100);
}

BAT_CHARGER_MODE battery_charger_get_mode(void)
{
    return bat_charger_info.mode;
}
