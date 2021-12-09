#include "types.h"
#include "os_mem.h"
#include "iot_rtc.h"
#include "app_wws.h"
#include "led_manager.h"
#include "app_main.h"
#include "app_led.h"
#include "app_bt.h"
#include "app_evt.h"
#include "ro_cfg.h"
#include "play_controller.h"
#include "app_econn.h"

#define MAX_LED_NUM 3

#define SYNC_LED_STATE_FLAG 0x80000000

#ifndef SYNC_LED_DELAY_MS
#define SYNC_LED_DELAY_MS 600
#endif

#ifndef SYNC_STATE_LED_ANTI_SHAKE_MS
#define SYNC_STATE_LED_ANTI_SHAKE_MS 300
#endif

// clang-format off
#ifndef SYNC_EVENT_LED_LIST
#define SYNC_EVENT_LED_LIST     {EVTSYS_CONNECTED,EVTSYS_DISCONNECTED}
#endif
// clang-format on

static const uint32_t sync_event_list[] = SYNC_EVENT_LED_LIST;

static void led_action_end_callback(uint8_t led_id);
static uint32_t led_play(const struct ro_cfg_led_type *cfg_led_p);
static uint8_t app_led_start_sync(uint32_t led_event_id, uint32_t delay_ms);
static uint32_t sync_led_action_callback(uint32_t led_evt_id, uint32_t start_rtc_ms);

/*
 * ENUMERATIONS
 ****************************************************************************
 */
#define LED_MSG_ID_NORNAL_LIGHT_DONE_0   0   //don't change this macro
#define LED_MSG_ID_NORMAL_LIGHT_DONE_MAX 16
#define LED_MSG_ID_ACTION_DONE           LED_MSG_ID_NORMAL_LIGHT_DONE_MAX
#define LED_MSG_ID_SYNC_CALLBACK         (LED_MSG_ID_ACTION_DONE + 1)
#define LED_MSG_ID_SYNC_LED_PLAY_EVT     (LED_MSG_ID_ACTION_DONE + 2)
#define LED_MSG_ID_SYNC_LED_PLAY_STATE   (LED_MSG_ID_ACTION_DONE + 3)
#define LED_REMOTE_MSG_ID_SYNC_REQUST    (LED_MSG_ID_ACTION_DONE + 4)
#define LED_MSG_ID_SYNC_STATE_ANTI_SHAKE (LED_MSG_ID_ACTION_DONE + 5)

/*
 * TYPE DEFINITIONS
 ****************************************************************************
 */
typedef struct {
    /* id of event which trigger the led action. */
    uint32_t led_evt_id;
    /* local rtc in ms for perform the led action. */
    uint32_t start_rtc_ms;
} led_sync_msg_t;

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************
 */
static bool_t led_event_playing_flag[MAX_LED_NUM] = {false, false, false};
static const ro_cfg_led_list_t *g_cfg_event_led_list = NULL;
static const ro_cfg_led_list_t *g_cfg_state_led_list = NULL;
static uint32_t custom_state = 0;
static bool_t led_enabled = true;
static bool_t state_led_enabled = true;
static bool_t sync_state_pending = false;
static bool_t sync_running = false;
static uint32_t pending_sync_event = 0;

/*
 * LOCAL FUNCTIONS DEFINITIONS
 ****************************************************************************
 */

static inline bool_t is_sync_event(uint32_t event)
{
    for (uint32_t i = 0; i < sizeof(sync_event_list) / sizeof(sync_event_list[0]); i++) {
        if (sync_event_list[i] == event) {
            return true;
        }
    }

    return false;
}

static bool_t led_get_event_playing_flag(uint8_t led_id)
{
    bool_t ret = false;
    if (led_id < MAX_LED_NUM) {
        ret = led_event_playing_flag[led_id];
    }
    return ret;
}

static void led_set_event_playing_flag(uint8_t led_id, bool_t flag)
{
    if (led_id < MAX_LED_NUM) {
        led_event_playing_flag[led_id] = flag;
    }
}

static bool_t led_is_state_play_allowed(uint32_t state)
{
    bool_t ret = true;
    uint32_t i = 0;
    uint32_t led_num = 0;
    led_num = g_cfg_state_led_list->num;
    for (i = 0; i < led_num; i++) {
        if (g_cfg_state_led_list->leds[i].state_or_event == state) {
            if (led_get_event_playing_flag(g_cfg_state_led_list->leds[i].style.id) == true) {
                ret = false;
                break;
            }
        }
    }
    DBGLOG_LED_DBG("led_is_state_play_allowed state %d,ret %d\n", state, ret);
    return ret;
}

static void led_handle_action_end(uint8_t led_id)
{
    DBGLOG_LED_DBG("led action done %d\n", led_id);
    led_set_event_playing_flag(led_id, false);

    if (ro_feat_cfg()->sync_sys_state && app_wws_is_connected_slave()) {
        app_wws_send_remote_msg(MSG_TYPE_LED, LED_REMOTE_MSG_ID_SYNC_REQUST, 0, NULL);
    } else {
        app_led_indicate_state(app_bt_get_sys_state());
    }
}

static void led_action_end_callback(uint8_t led_id)
{
    /* send a message to main task */
    app_send_msg(MSG_TYPE_LED, LED_MSG_ID_ACTION_DONE, &led_id, sizeof(uint8_t));
}

static uint32_t led_play(const struct ro_cfg_led_type *cfg_led_p)
{
    led_param_t led_param;
    uint32_t ret = RET_OK;
    uint8_t led_id = cfg_led_p->id;

    if (led_id >= MAX_LED_NUM) {
        DBGLOG_LED_ERR("led_play invalid id:%d\n", cfg_led_p->id);
        return RET_FAIL;
    }

    if ((cfg_led_p->mode == RO_LED_MODE_OFF) || (cfg_led_p->mode > RO_LED_MODE_NORMAL_LIGHT)) {
        DBGLOG_LED_DBG("ledplay id:%d mode:%d \n", led_id, cfg_led_p->mode);
        led_off(led_id);
        if ((cfg_led_p->loop != 0) && (cfg_led_p->off != 0)) {
            app_send_msg_delay(MSG_TYPE_LED, LED_MSG_ID_NORNAL_LIGHT_DONE_0 + led_id, &led_id,
                               sizeof(uint8_t), cfg_led_p->loop * cfg_led_p->off);
        }
        return RET_OK;
    }

    switch (cfg_led_p->mode) {
        case RO_LED_MODE_BLINK:
            led_param.mode = LED_MODE_BLINK;
            break;
        case RO_LED_MODE_DIM:
            led_param.mode = LED_MODE_DIM;
            break;
        case RO_LED_MODE_NORMAL_LIGHT:
            led_param.mode = LED_MODE_NORMAL_LIGHT;
            break;
        case RO_LED_MODE_OFF:
            break;
        default:
            assert(0);
            break;
    }
    led_param.on_duty = cfg_led_p->on;
    led_param.off_duty = cfg_led_p->off;
    led_param.blink_cnt = cfg_led_p->flash;
    led_param.dim_duty = cfg_led_p->dim;
    led_param.interval = cfg_led_p->repeat_delay;
    led_param.loop = cfg_led_p->loop;

    DBGLOG_LED_DBG("ledplay id:%d mode:%d on:%d off:%d interval:%d\n", led_id, cfg_led_p->mode,
                   led_param.on_duty, led_param.off_duty, led_param.interval);

    ret = led_config((RESOURCE_GPIO_ID)led_id, &led_param, led_action_end_callback);
    if (RET_OK != ret) {
        DBGLOG_LED_ERR(" call led_config cfg ERROR %d\n", ret);
        return ret;
    }

    ret = led_start_action(led_id, cfg_led_p->start_from_on);
    if (RET_OK != ret) {
        DBGLOG_LED_ERR(" call led_start_action ERROR %d\n", ret);
    }

    if ((cfg_led_p->mode == RO_LED_MODE_NORMAL_LIGHT) && (cfg_led_p->loop != 0)
        && (cfg_led_p->on != 0)) {
        app_send_msg_delay(MSG_TYPE_LED, LED_MSG_ID_NORNAL_LIGHT_DONE_0 + led_id, &led_id,
                           sizeof(uint8_t), cfg_led_p->loop * cfg_led_p->on);
    }

    return ret;
}

static uint32_t do_play_event_led(uint32_t event_id)
{
    uint32_t ret = RET_OK;
    uint32_t i = 0;
    uint32_t led_event_num = 0;
    const struct ro_cfg_led_type *cfg_led_p;

    if (!led_enabled) {
        return RET_OK;
    }

    led_event_num = g_cfg_event_led_list->num;
    for (i = 0; i < led_event_num; i++) {
        if (g_cfg_event_led_list->leds[i].state_or_event == event_id) {
            DBGLOG_LED_DBG("led(%d) event: %d\n", i, event_id);
            cfg_led_p = &g_cfg_event_led_list->leds[i].style;
            led_off(cfg_led_p->id);
            app_cancel_msg(MSG_TYPE_LED, LED_MSG_ID_NORNAL_LIGHT_DONE_0 + cfg_led_p->id);
            led_set_event_playing_flag(cfg_led_p->id, true);
            ret = led_play(cfg_led_p);
        }
    }
    return ret;
}

static uint32_t do_play_state_led(uint32_t state)
{
    uint32_t i;
    uint32_t state_led_num;
    uint32_t ret = RET_OK;

    if (app_bt_is_in_audio_test_mode() || app_bt_is_in_dut_mode()) {
        return RET_OK;
    }

    if ((!led_enabled) || (!state_led_enabled)) {
        DBGLOG_LED_DBG("do_play_state_led led_enabled:%d state_led_enabled:%d\n", led_enabled,
                       state_led_enabled);
        return RET_OK;
    }

    if (custom_state != 0) {
        DBGLOG_LED_DBG("do_play_state_led state:0x%X => custom_state:0x%X", state, custom_state);
        state = custom_state;
    }

    DBGLOG_LED_DBG("do_play_state_led 0x%X\n", state);
    if (!led_is_state_play_allowed(state)) {
        return RET_OK;
    }

    for (i = 0; i < MAX_LED_NUM; i++) {
        if (!led_get_event_playing_flag(i)) {
            led_off(i);
        }
    }

    state_led_num = g_cfg_state_led_list->num;
    for (i = 0; i < state_led_num; i++) {
        if (g_cfg_state_led_list->leds[i].state_or_event == state) {
            DBGLOG_LED_DBG("led(%d) state: 0x%x\n", i, state);
            const struct ro_cfg_led_type *cfg_led_p = &g_cfg_state_led_list->leds[i].style;
            ret = led_play(cfg_led_p);
        }
    }

    return ret;
}

static void handle_sync_msg(const led_sync_msg_t *msg)
{
    uint32_t now_rtc_ms = iot_rtc_get_global_time_ms();
    uint32_t margin_ms = msg->start_rtc_ms - now_rtc_ms;
    uint32_t id;
    id = msg->led_evt_id;

    sync_running = false;

    if (id & SYNC_LED_STATE_FLAG) {
        id = id & (~SYNC_LED_STATE_FLAG);
        DBGLOG_LED_DBG("sync led msg: state:0x%X, target:%lu, now:%lu, margin:%lu\n", id,
                       msg->start_rtc_ms, now_rtc_ms, margin_ms);
        app_cancel_msg(MSG_TYPE_LED, LED_MSG_ID_SYNC_LED_PLAY_STATE);
        if (msg->start_rtc_ms > now_rtc_ms) {
            app_send_msg_delay(MSG_TYPE_LED, LED_MSG_ID_SYNC_LED_PLAY_STATE, &id, sizeof(id),
                               margin_ms);
        } else {
            DBGLOG_LED_ERR("sync state led msg error: overtime\n");
            do_play_state_led(id);
        }
    } else {
        DBGLOG_LED_DBG("sync led msg: evt:%d, target:%lu, now:%lu, margin:%lu\n", id,
                       msg->start_rtc_ms, now_rtc_ms, margin_ms);
        app_cancel_msg(MSG_TYPE_LED, LED_MSG_ID_SYNC_LED_PLAY_EVT);
        if (msg->start_rtc_ms > now_rtc_ms) {
            app_send_msg_delay(MSG_TYPE_LED, LED_MSG_ID_SYNC_LED_PLAY_EVT, &id, sizeof(id),
                               margin_ms);
        } else {
            DBGLOG_LED_ERR("sync event msg error: overtime\n");
            do_play_event_led(msg->led_evt_id);
        }
    }

    if (pending_sync_event != 0) {
        uint32_t event_id = pending_sync_event;
        pending_sync_event = 0;
        if (app_wws_is_master()) {
            app_led_indicate_event(event_id);
        }
    } else if (sync_state_pending) {
        sync_state_pending = false;
        if (app_wws_is_master()) {
            app_led_indicate_state(app_bt_get_sys_state());
        }
    }
}

static void app_led_handle_msg(uint16_t msg_id, void *param)
{
    if (msg_id == LED_MSG_ID_ACTION_DONE) {
        led_handle_action_end(*((uint8_t *)param));
    } else if (msg_id < LED_MSG_ID_NORNAL_LIGHT_DONE_0 + MAX_LED_NUM) {
        led_off(msg_id - LED_MSG_ID_NORNAL_LIGHT_DONE_0);
        led_handle_action_end(msg_id - LED_MSG_ID_NORNAL_LIGHT_DONE_0);
    } else if (msg_id == LED_MSG_ID_SYNC_CALLBACK) {
        handle_sync_msg((led_sync_msg_t *)param);
    } else if (msg_id == LED_MSG_ID_SYNC_LED_PLAY_EVT) {
        do_play_event_led(*((uint32_t *)param));
    } else if (msg_id == LED_MSG_ID_SYNC_LED_PLAY_STATE) {
        do_play_state_led(*((uint32_t *)param));
    } else if (msg_id == LED_REMOTE_MSG_ID_SYNC_REQUST) {
        app_led_indicate_state(app_bt_get_sys_state());
    } else if (msg_id == LED_MSG_ID_SYNC_STATE_ANTI_SHAKE) {
        if (app_wws_is_master()) {
            if (sync_running) {
                sync_state_pending = true;
                DBGLOG_LED_DBG("LED_MSG_ID_SYNC_STATE_ANTI_SHAKE other sync running\n");
            } else {
                app_led_start_sync(app_bt_get_sys_state() | SYNC_LED_STATE_FLAG, SYNC_LED_DELAY_MS);
            }
        } else {
            DBGLOG_LED_DBG("LED_MSG_ID_SYNC_STATE_ANTI_SHAKE ignored by slave\n");
        }
    } else {
        DBGLOG_LED_ERR("unhandled led msg %d\n", msg_id);
    }
}

/*
 * EXPORTED FUNCTIONS DEFINITIONS
 ****************************************************************************
 */
void app_led_init(void)
{
    uint32_t ret = RET_OK;
    sync_reg_start_led_action_cb(sync_led_action_callback);
    app_register_msg_handler(MSG_TYPE_LED, app_led_handle_msg);

    g_cfg_state_led_list = ro_cfg()->state_led;
    g_cfg_event_led_list = ro_cfg()->event_led;

    ret = led_init();
    if (ret != RET_OK) {
        return;
    }
}

void app_led_deinit(void)
{
    uint32_t i = 0;

    g_cfg_event_led_list = NULL;

    for (i = 0; i < MAX_LED_NUM; i++) {
        led_off(i);
    }

    led_deinit();
}

/**
 * @brief led app requst to sync led action.
 * @param led_event_id: id of event that trigger the led action.
 * @param delay_ms: time to delay for led action.
 *     0 - use default delay set by bt core.
 * @return RET_OK if it's OK to perform sync led action, other value if failed.
 */
static uint8_t app_led_start_sync(uint32_t led_event_id, uint32_t delay_ms)
{
    uint8_t result = RET_FAIL;
    bt_sync_led_action_cmd_t param;

    param.led_evt_id = led_event_id;
    param.delay_ms = delay_ms;
    result = app_bt_send_rpc_cmd(BT_CMD_SYNC_LED_ACTION, &param, sizeof(param));

    if (!result) {
        sync_running = true;
    }

    DBGLOG_LED_DBG("sync_led event_id:0x%X, delay:%d, result:%d\n", led_event_id, delay_ms, result);
    return result;
}

static uint32_t sync_led_action_callback(uint32_t led_evt_id, uint32_t start_rtc_ms)
{
    led_sync_msg_t led_action_msg;
    led_action_msg.led_evt_id = led_evt_id;
    led_action_msg.start_rtc_ms = start_rtc_ms;
    app_send_msg(MSG_TYPE_LED, LED_MSG_ID_SYNC_CALLBACK, &led_action_msg, sizeof(led_action_msg));
    return RET_OK;
}

void app_led_indicate_event(uint32_t event_id)
{
    if (app_econn_handle_event_led(event_id)) {
        DBGLOG_EVT_DBG("app_led_indicate_event event_id:%d handled by econn\n", event_id);
        return;
    }

    if (app_bt_is_in_audio_test_mode() || app_bt_is_in_dut_mode()) {
        if (event_id == EVTUSR_ENTER_DUT_MODE) {
            for (int i = 0; i < MAX_LED_NUM; i++) {
                led_off(i);
            }
        } else {
            return;
        }
    }

    if (is_sync_event(event_id)) {
        if (app_wws_is_connected()) {
            if (app_wws_is_master()) {
                if (sync_running) {
                    pending_sync_event = event_id;
                    DBGLOG_LED_DBG("app_led_indicate_event %d other sync running\n");
                } else {
                    app_led_start_sync(event_id, SYNC_LED_DELAY_MS);
                }
            } else {
                DBGLOG_LED_DBG("sync led ignored by slave");
            }
            return;
        }
    }
    do_play_event_led(event_id);
}

void app_led_indicate_state(uint32_t state)
{
    if (app_econn_handle_state_led(state)) {
        DBGLOG_EVT_DBG("app_led_indicate_state state:0x%X handled by econn\n", state);
        return;
    }

    if (app_bt_is_in_audio_test_mode() || app_bt_is_in_dut_mode()) {
        return;
    }

    if (ro_feat_cfg()->sync_sys_state) {
        if (app_wws_is_connected()) {
            if (app_wws_is_master()) {
                app_cancel_msg(MSG_TYPE_LED, LED_MSG_ID_SYNC_STATE_ANTI_SHAKE);
                app_send_msg_delay(MSG_TYPE_LED, LED_MSG_ID_SYNC_STATE_ANTI_SHAKE, NULL, 0,
                                   SYNC_STATE_LED_ANTI_SHAKE_MS);
            } else {
                app_cancel_msg(MSG_TYPE_LED, LED_MSG_ID_SYNC_STATE_ANTI_SHAKE);
                DBGLOG_LED_DBG("sync led ignored by slave");
            }
            return;
        }
    }

    do_play_state_led(state);
}

bool_t app_led_is_event_playing(void)
{
    for (int i = 0; i < MAX_LED_NUM; i++) {
        if (led_get_event_playing_flag(i)) {
            return true;
        }
    }

    return false;
}

void app_led_set_custom_state(uint32_t state)
{
    custom_state = state;
    if (ro_feat_cfg()->sync_sys_state && app_wws_is_connected_slave()) {
        app_wws_send_remote_msg(MSG_TYPE_LED, LED_REMOTE_MSG_ID_SYNC_REQUST, 0, NULL);
    } else {
        app_led_indicate_state(app_bt_get_sys_state());
    }
}

void app_led_set_enabled(bool_t enable)
{
    DBGLOG_LED_DBG("app_led_set_enabled:%d\n", enable);

    led_enabled = enable;

    if (enable) {
        if (ro_feat_cfg()->sync_sys_state && app_wws_is_connected_slave()) {
            app_wws_send_remote_msg(MSG_TYPE_LED, LED_REMOTE_MSG_ID_SYNC_REQUST, 0, NULL);
        } else {
            app_led_indicate_state(app_bt_get_sys_state());
        }
    } else {
        for (int i = 0; i < MAX_LED_NUM; i++) {
            led_off(i);
            app_cancel_msg(MSG_TYPE_LED, LED_MSG_ID_NORNAL_LIGHT_DONE_0 + i);
            led_event_playing_flag[i] = false;
        }
    }
}

void app_led_set_state_led_enabled(bool_t enable)
{
    DBGLOG_LED_DBG("app_led_set_state_led_enabled:%d\n", enable);

    state_led_enabled = enable;
    if (enable) {
        if (ro_feat_cfg()->sync_sys_state && app_wws_is_connected_slave()) {
            app_wws_send_remote_msg(MSG_TYPE_LED, LED_REMOTE_MSG_ID_SYNC_REQUST, 0, NULL);
        } else {
            app_led_indicate_state(app_bt_get_sys_state());
        }
    } else {
        for (int i = 0; i < MAX_LED_NUM; i++) {
            if (!led_get_event_playing_flag(i)) {
                led_off(i);
                app_cancel_msg(MSG_TYPE_LED, LED_MSG_ID_NORNAL_LIGHT_DONE_0 + i);
            }
        }
    }
}

void app_led_handle_wws_state_changed(bool_t connected)
{
    UNUSED(connected);

    if (ro_feat_cfg()->sync_sys_state) {
        app_led_indicate_state(app_bt_get_sys_state());
    }
}

void app_led_handle_wws_role_changed(bool_t is_master)
{
    if (ro_feat_cfg()->sync_sys_state) {
        if (is_master) {
            app_led_indicate_state(app_bt_get_sys_state());
        } else {
            app_cancel_msg(MSG_TYPE_LED, LED_MSG_ID_SYNC_STATE_ANTI_SHAKE);
        }
    }
}
