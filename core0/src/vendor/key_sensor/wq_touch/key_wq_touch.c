#include "key_sensor.h"

#if (KEY_DRIVER_SELECTION == KEY_DRIVER_WQ_TOUCH)

#include "string.h"
#include "os_timer.h"
#include "os_utils.h"
#include "key_base.h"
#include "iot_touch_key.h"
#include "iot_resource.h"
#include "key_wq_touch.h"
#include "vendor_msg.h"
#include "boot_reason.h"
#include "cli.h"
#include "modules.h"

#define APP_CLI_MSG_ID_GET_TOUCH_KEY_INFO 16

#define KEY_WQ_TOUCH_TIME_OFFSET 500 /* ms */
#define KEY_WQ_TOUCH_TIMER       100 /* ms */
#ifndef KEY_WQ_TOUCH_MAX_HOLD_TIME
#define KEY_WQ_TOUCH_MAX_HOLD_TIME 12000 /* ms */
#endif
#define KEY_WQ_TOUCH_GET_CDC_TIMER 25 /* ms */

#define MAX_PAD_NUM (2)

#define KEY_WQ_TOUCH_MAX_THRES           2047
#define KEY_WQ_TOUCH_MAX_TRIG_TIMES      14
#define KEY_WQ_TOUCH_DEFAULT_CLIMB_THRES 960
#define KEY_WQ_TOUCH_DEFAULT_FALL_THRES  512

typedef enum {
    TOUCH_MSG_ID_INIT_PRESSED = 0,
    TOUCH_MSG_ID_PRESSED,
    TOUCH_MSG_ID_RELEASED,
    TOUCH_MSG_ID_CHECK_EVENT,
    TOUCH_MSG_ID_ENABLE_INTR,
    TOUCH_MSG_ID_SET_ENABLE,
    TOUCH_MSG_ID_ADJUST_THRS,
    TOUCH_MSG_ID_ADJUST_TRIG_TIMES,
} touch_msg_id_t;

typedef struct {
    timer_id_t timer;             /** key recognition timer */
    timer_id_t enable_intr_timer; /** enable intr timer */
    timer_id_t print_cdc_timer;   /** print cdc timer */
    bool_t is_shutdown;           /** if app is shuting down. */
    uint16_t release_check_ticks; /** count the self release check time. */
    uint16_t climb_thres;         /** configured climb threshold value. */
    uint16_t fall_thres;          /** configured fall threshold value. */
    uint8_t climb_trig_times;     /** configured climb trig times value. */
    uint8_t fall_trig_times;      /** configured fall trig times value. */
    uint8_t num;                  /** configured touch num */
    uint16_t pad[MAX_PAD_NUM];    /** configured touch list */
    uint8_t key_id[MAX_PAD_NUM];
} key_touch_context_t;

typedef struct {
    bool_t enabled;            /** if touch key enabled. */
    uint16_t climb_intr_times; /** count the self release check time. */
    uint16_t fall_intr_times;  /** configured climb threshold value. */
    uint32_t cdc;              /** configured touch num */
} __attribute__((packed)) key_touch_dump_t;

static key_touch_context_t _touch_context = {0};
static key_touch_context_t *touch_context = &_touch_context;
static bool_t first_intr = true;
static bool_t touch_enabled = true; /** if touch is enabled. */
static volatile uint16_t new_climb_thres = 0;
static volatile uint16_t new_fall_thres = 0;
static volatile uint16_t new_climb_trig_times = 0;
static volatile uint16_t new_fall_trig_times = 0;
static volatile uint16_t climb_intr_times = 0;
static volatile uint16_t fall_intr_times = 0;

static void isr_state_changed(IOT_TK_PAD_ID pad_id, IOT_TOUCH_KEY_INT int_type)
    IRAM_TEXT(isr_state_changed);
static void isr_state_changed(IOT_TK_PAD_ID pad_id, IOT_TOUCH_KEY_INT int_type)
{
    if (int_type == IOT_TOUCH_KEY_INT_PRESS_MID) {
        climb_intr_times++;
        vendor_send_msg_from_isr(VENDOR_MSG_TYPE_KEY_WQ_TOUCH, TOUCH_MSG_ID_PRESSED, pad_id);
    } else if (int_type == IOT_TOUCH_KEY_INT_PRESS_RELEASE) {
        fall_intr_times++;
        vendor_send_msg_from_isr(VENDOR_MSG_TYPE_KEY_WQ_TOUCH, TOUCH_MSG_ID_RELEASED, pad_id);
    }

    return;
}

static uint32_t key_wq_touch_enable(uint8_t key_id, uint8_t key_src, uint16_t *io_id)
{
    uint8_t touch_id = INVALID_UCHAR;
    iot_touch_key_point_num_t point_num = {0};

    touch_id = iot_resource_lookup_touch_id((RESOURCE_GPIO_ID)key_id + GPIO_TOUCH_KEY_0);

    if (touch_id == INVALID_UCHAR) {
        DBGLOG_KEY_SENSOR_ERROR("get invalid touch_id via key_id %d\n", key_id);
        return RET_NOT_EXIST;
    }

    /* register io to key base module before open touch */
    key_base_register(key_id, key_src, touch_id);

    /* recommened values from BSP. */
    point_num.monitor_point_num = IOT_TOUCH_KEY_PHASE_POINT_NUM_8;
    point_num.work_point_num = IOT_TOUCH_KEY_PHASE_POINT_NUM_8;

    DBGLOG_KEY_SENSOR_INFO(
        "key_touch, set touch id %d, key_id %d, mon_point_num %d, work_point_num %d\n", touch_id,
        key_id, point_num.monitor_point_num, point_num.work_point_num);

    iot_touch_key_set_pad_info((IOT_TK_PAD_ID)touch_id, IOT_TK_RELATIVE_MODE, &point_num);
    *io_id = touch_id;

    return RET_OK;
}

static void key_wq_touch_set_pad_info(const key_id_cfg_t *id_cfg)
{
    uint32_t i;
    uint16_t io_id = INVALID_WORD16;

    if (id_cfg == NULL) {
        DBGLOG_KEY_SENSOR_ERROR("key_wq_touch_set_pad_info, id_cfg illegal");
        return;
    }

    for (i = 0; i < id_cfg->num; i++) {

        if (id_cfg->src[i] != KEY_SRC_TOUCH) {
            continue;
        }

        if (touch_context->num < MAX_PAD_NUM) {
            if (!key_wq_touch_enable(id_cfg->id[i], id_cfg->src[i], &io_id)) {
                touch_context->pad[touch_context->num] = io_id;
                touch_context->key_id[touch_context->num] = id_cfg->id[i];
                touch_context->num++;
            }
        } else {
            DBGLOG_KEY_SENSOR_ERROR("key_wq_touch_set_pad_info, exceed maximum touch num\n");
        }
    }
}

static void key_wq_touch_timer_cb(timer_id_t timer_id, void *arg)
{
    UNUSED(timer_id);
    UNUSED(arg);

    vendor_send_msg(VENDOR_MSG_TYPE_KEY_WQ_TOUCH, TOUCH_MSG_ID_CHECK_EVENT, 0);
}

static void key_wq_touch_enable_intr_timer_cb(timer_id_t timer_id, void *arg)
{
    UNUSED(timer_id);
    UNUSED(arg);

    vendor_send_msg(VENDOR_MSG_TYPE_KEY_WQ_TOUCH, TOUCH_MSG_ID_ENABLE_INTR, 0);
}

static void key_wq_touch_timer_dbg_cb(timer_id_t timer_id, void *arg)
{
    UNUSED(timer_id);
    UNUSED(arg);

    uint32_t i, cdc_value = 0;

    for (i = 0; i < touch_context->num; i++) {
        iot_touch_key_read_pad_cdc(touch_context->pad[i], &cdc_value);
        DBGLOG_KEY_SENSOR_INFO("+++++++++++++++++cdc %d.\n", cdc_value);
    }
}

static void key_wq_touch_init_pressed(uint16_t io_id)
{
    key_base_pressed(io_id, KEY_WQ_TOUCH_TIME_OFFSET);

    /* start timer. if other keys in BTN_STATE_PRESS or BTN_STATE_RELEASE
       state, timer is still active. */
    if (os_is_timer_active(touch_context->timer) == 0) {
        os_start_timer(touch_context->timer, KEY_WQ_TOUCH_TIMER);
        DBGLOG_KEY_SENSOR_INFO("start wq touch timer.\n");
    }
}

static void key_wq_touch_pressed(uint16_t io_id)
{
    uint32_t time_offset = 0;

    if ((touch_context->is_shutdown) || (!touch_enabled)) {
        return;
    }

    if (first_intr == true) {
        if (BOOT_REASON_SLEEP == boot_reason_get_reason()) {
            BOOT_REASON_WAKEUP_SOURCE wakeup_source = boot_reason_get_wakeup_source();
            if (BOOT_REASON_WAKEUP_SRC_TK == wakeup_source) {
                DBGLOG_KEY_SENSOR_INFO("first press interrupt for touch source wake up.\n");
                time_offset = KEY_WQ_TOUCH_TIME_OFFSET;
            }
        }
        first_intr = false;
    }

    key_base_pressed(io_id, time_offset);

    /* start timer. if other keys in BTN_STATE_PRESS or BTN_STATE_RELEASE
       state, timer is still active. */
    if (os_is_timer_active(touch_context->timer) == 0) {
        os_start_timer(touch_context->timer, KEY_WQ_TOUCH_TIMER);
        DBGLOG_KEY_SENSOR_INFO("start wq touch timer.\n");
    }
}

static void key_wq_touch_released(uint16_t io_id)
{
    touch_context->release_check_ticks = 0;

    key_base_released(io_id);
}

static void key_wq_touch_do_release_key(void)
{
    uint8_t ret;
    uint32_t i;

    for (i = 0; i < touch_context->num; i++) {

        if (false == key_base_is_key_pressed(touch_context->key_id[i], KEY_SRC_TOUCH)) {
            continue;
        }

        uint16_t pad_id = touch_context->pad[i];

        DBGLOG_KEY_SENSOR_INFO("key_wq_touch_do_release_key, self release pad %d.\n", pad_id);
        key_wq_touch_released(pad_id);
        if (RET_OK != (ret = iot_touch_key_reset_pad((IOT_TK_PAD_ID)pad_id))) {
            DBGLOG_KEY_SENSOR_ERROR("reset pad failed, pad %d, ret %d.\n", pad_id, ret);
        }
    }
}

static void key_wq_touch_do_adj_thres(void)
{
    uint8_t ret;
    uint32_t i;

    for (i = 0; i < touch_context->num; i++) {

        IOT_TK_PAD_ID pad_id = (IOT_TK_PAD_ID)touch_context->pad[i];
        DBGLOG_KEY_SENSOR_INFO(
            "key_wq_touch_do_adj_thres, pad %d, new_climb_thres %d, new_fall_thres %d\n", pad_id,
            new_climb_thres, new_fall_thres);

        if (key_base_is_key_pressed(touch_context->key_id[i], KEY_SRC_TOUCH)) {
            DBGLOG_KEY_SENSOR_INFO("key_wq_touch_do_adj_thres, self release pad %d.\n", pad_id);
            key_wq_touch_released(pad_id);
        }

        if (RET_OK != (ret = iot_touch_key_reset_pad((IOT_TK_PAD_ID)pad_id))) {
            DBGLOG_KEY_SENSOR_ERROR("reset pad failed, pad %d, ret %d.\n", pad_id, ret);
        }

        if (RET_OK
            != (ret = iot_touch_key_change_pad_thrs(pad_id, new_fall_thres, new_climb_thres))) {
            DBGLOG_KEY_SENSOR_ERROR("change pad thres failed, pad %d, ret %d.\n", pad_id, ret);
        }
    }
}

static void key_wq_touch_do_adj_trig_times(void)
{
    uint8_t ret;
    uint32_t i;

    for (i = 0; i < touch_context->num; i++) {

        IOT_TK_PAD_ID pad_id = (IOT_TK_PAD_ID)touch_context->pad[i];

        if (key_base_is_key_pressed(touch_context->key_id[i], KEY_SRC_TOUCH)) {
            DBGLOG_KEY_SENSOR_INFO("key_wq_touch_do_adj_trig_times, self release pad %d.\n",
                                   pad_id);
            key_wq_touch_released(pad_id);
        }

        if (RET_OK
            != (ret = iot_touch_key_change_pad_trig_times(pad_id, new_climb_trig_times,
                                                          new_fall_trig_times))) {
            DBGLOG_KEY_SENSOR_ERROR("change pad trig_times failed, pad %d, ret %d.\n", pad_id, ret);
        }
    }
}

static void key_wq_touch_self_release_check(void)
{
    if (touch_context->release_check_ticks < (KEY_WQ_TOUCH_MAX_HOLD_TIME / KEY_WQ_TOUCH_TIMER)) {
        touch_context->release_check_ticks++;
        return;
    }

    touch_context->release_check_ticks = 0;
    key_wq_touch_do_release_key();
}

static void key_wq_touch_check_type(void)
{
    /* sometimes touch driver won't give RELEASE interrupt if released slowly, so we
        add this self-release strategy to maintain the state of this io. */
    key_wq_touch_self_release_check();

    if (touch_context->is_shutdown) {
        if ((key_base_all_key_released()) && (os_is_timer_active(touch_context->timer) == 1)) {
            os_stop_timer(touch_context->timer);
            DBGLOG_KEY_SENSOR_INFO("stop wq touch timer before shutdown.\n");
        }
        return;
    }

    if (CHECK_DONE == key_base_check_type()) {

        if (os_is_timer_active(touch_context->timer) == 1) {
            os_stop_timer(touch_context->timer);
            DBGLOG_KEY_SENSOR_INFO("stop wq touch timer.\n");
        }
    }
}

static void key_wq_touch_enable_intr(void)
{
    uint8_t ret;
    uint32_t i;

    for (i = 0; i < touch_context->num; i++) {
        ret = iot_touch_key_enable_intr(touch_context->pad[i]);
        if (RET_OK != ret) {
            DBGLOG_KEY_SENSOR_ERROR("key_touch, enable touch %d intr failed, ret %d.\n",
                                    touch_context->pad[i], ret);
        }
    }

    if (touch_context->enable_intr_timer) {
        os_delete_timer(touch_context->enable_intr_timer);
        DBGLOG_KEY_SENSOR_INFO("delete enable intr timer.\n");
    }
    return;
}
static bool_t is_touch_thres_configured(void)
{
    return ((touch_context->climb_thres && touch_context->fall_thres) ? true : false);
}

static void key_wq_touch_do_set_enable(bool_t enable)
{
    if (enable) {
        /** enable */
        touch_enabled = true;
    } else {
        /** disable */
        touch_enabled = false;
        key_wq_touch_do_release_key();
    }
}

static void key_wq_touch_dump_all_info(uint32_t cdc)
{
    DBGLOG_KEY_SENSOR_INFO("touch enabled: %d\n", touch_enabled);
    DBGLOG_KEY_SENSOR_INFO("touch num: %d\n", touch_context->num);

    for (uint32_t i = 0; i < touch_context->num; i++) {
        DBGLOG_KEY_SENSOR_INFO("touch pad: %d, key id: %d\n", touch_context->pad[i],
                               touch_context->key_id[i]);
    }

    DBGLOG_KEY_SENSOR_INFO("climb thres: %d, fall thres: %d\n", touch_context->climb_thres,
                           touch_context->fall_thres);

    DBGLOG_KEY_SENSOR_INFO("climb_intr_times: %d, fall_intr_times: %d\n", climb_intr_times,
                           fall_intr_times);
    climb_intr_times = 0;
    fall_intr_times = 0;

    DBGLOG_KEY_SENSOR_INFO("cdc: %d\n", cdc);
}

static void key_wq_touch_msg_handler(uint8_t msg_id, uint16_t msg_val)
{
    switch (msg_id) {
        case TOUCH_MSG_ID_INIT_PRESSED:
            key_wq_touch_init_pressed(msg_val);
            break;
        case TOUCH_MSG_ID_PRESSED:
            key_wq_touch_pressed(msg_val);
            break;
        case TOUCH_MSG_ID_RELEASED:
            key_wq_touch_released(msg_val);
            break;
        case TOUCH_MSG_ID_CHECK_EVENT:
            key_wq_touch_check_type();
            break;
        case TOUCH_MSG_ID_ENABLE_INTR:
            key_wq_touch_enable_intr();
            break;
        case TOUCH_MSG_ID_SET_ENABLE:
            key_wq_touch_do_set_enable((bool_t)msg_val);
            break;
        case TOUCH_MSG_ID_ADJUST_THRS:
            key_wq_touch_do_adj_thres();
            break;
        case TOUCH_MSG_ID_ADJUST_TRIG_TIMES:
            key_wq_touch_do_adj_trig_times();
            break;
        default:
            DBGLOG_KEY_SENSOR_ERROR("key_base_msg_handler unknown msg_id:%d\n", msg_id);
            break;
    }
}

void key_wq_touch_init(const key_id_cfg_t *key_id)
{
    DBGLOG_KEY_SENSOR_INFO("key_wq_touch_init\n");

    iot_touch_key_adjust_freq(IOT_TOUCH_KEY_DIV_32K_FREQ);

    vendor_register_msg_handler(VENDOR_MSG_TYPE_KEY_WQ_TOUCH, key_wq_touch_msg_handler);

    memset(touch_context, 0x00, sizeof(key_touch_context_t));
    touch_context->timer = os_create_timer(LIB_KEYMGMT_MID, true, key_wq_touch_timer_cb, NULL);
    assert(touch_context->timer);
    touch_context->enable_intr_timer =
        os_create_timer(LIB_KEYMGMT_MID, false, key_wq_touch_enable_intr_timer_cb, NULL);
    assert(touch_context->enable_intr_timer);
    touch_context->print_cdc_timer =
        os_create_timer(LIB_KEYMGMT_MID, true, key_wq_touch_timer_dbg_cb, NULL);
    assert(touch_context->print_cdc_timer);

    key_wq_touch_set_pad_info(key_id);
}

void key_wq_touch_deinit(bool_t wakeup_enable)
{
    DBGLOG_KEY_SENSOR_INFO("key_wq_touch_deinit\n");

    /* change touch freq to 8k for power saving. */
    iot_touch_key_adjust_freq(IOT_TOUCH_KEY_DIV_8K_FREQ);

    touch_context->is_shutdown = true;

    if (!wakeup_enable) {
        for (uint8_t i = 0; i < touch_context->num; i++) {
            uint8_t ret = iot_touch_key_close((IOT_TK_PAD_ID)touch_context->pad[i]);
            if (RET_OK != ret) {
                DBGLOG_KEY_SENSOR_ERROR("key_touch, close touch %d failed, ret %d\n",
                                        touch_context->pad[i], ret);
            } else {
                DBGLOG_KEY_SENSOR_INFO("key_touch, close touch %d success\n",
                                       touch_context->pad[i]);
            }
        }
    }
}

void key_wq_touch_open(const key_thres_cfg_t *thres_cfg, bool_t can_power_on)
{
    uint8_t ret;
    uint32_t i, vref_ctrl;
    iot_touch_key_config_t tk_cfg = {0};

    UNUSED(can_power_on);

    if (touch_context->num == 0) {
        DBGLOG_KEY_SENSOR_ERROR("key_wq_touch_open, no touck key configured");
        return;
    }

    if (thres_cfg == NULL) {
        DBGLOG_KEY_SENSOR_ERROR("key_wq_touch_open, thres_cfg illegal");
        return;
    }

    tk_cfg.climb_thrs = thres_cfg->climb_thres % 10000;
    if (tk_cfg.climb_thrs == 0) {
        tk_cfg.climb_thrs = KEY_WQ_TOUCH_DEFAULT_CLIMB_THRES;
    }

    tk_cfg.fall_thrs = thres_cfg->fall_thres % 10000;
    if (tk_cfg.fall_thrs == 0) {
        tk_cfg.fall_thrs = KEY_WQ_TOUCH_DEFAULT_FALL_THRES;
    }

    tk_cfg.climb_trig_times = IOT_TOUCH_KEY_ANTI_SHAKE_CLIMB_TRIG_TIMES;
    tk_cfg.fall_trig_times = IOT_TOUCH_KEY_ANTI_SHAKE_FALL_TRIG_TIMES;

    touch_context->climb_trig_times = tk_cfg.climb_trig_times;
    touch_context->fall_trig_times = tk_cfg.fall_trig_times;

    vref_ctrl = thres_cfg->climb_thres / 10000;
    DBGLOG_KEY_SENSOR_INFO("key_touch, climb_thrs: %d, fall_thrs: %d, vref = %d\n",
                           tk_cfg.climb_thrs, tk_cfg.fall_thrs, vref_ctrl);

    touch_context->climb_thres = tk_cfg.climb_thrs;
    touch_context->fall_thres = tk_cfg.fall_thrs;

    for (i = 0; i < touch_context->num; i++) {
        ret = iot_touch_key_open((IOT_TK_PAD_ID)touch_context->pad[i], IOT_TK_RELATIVE_MODE,
                                 &tk_cfg, isr_state_changed);
        if (RET_OK != ret) {
            DBGLOG_KEY_SENSOR_ERROR("key_touch, open touch %d failed, ret %d.\n",
                                    touch_context->pad[i], ret);
        } else {
            DBGLOG_KEY_SENSOR_INFO("key_touch, open touch %d sucess.\n", touch_context->pad[i]);
        }
    }

    if (vref_ctrl != 0) {
        iot_touch_key_vref_ctrl_cfg(vref_ctrl);
    }

    os_start_timer(touch_context->enable_intr_timer, 20);
    return;
}

uint8_t key_wq_touch_set_enable(bool_t enable)
{
    if (!is_touch_thres_configured()) {
        DBGLOG_KEY_SENSOR_ERROR("key_wq_touch_adj_thres, touch thres not configured yet.\n");
        return RET_NOT_READY;
    }

    DBGLOG_KEY_SENSOR_INFO("key_wq_touch_set_enable enable: %d\n", enable);
    vendor_send_msg(VENDOR_MSG_TYPE_KEY_WQ_TOUCH, TOUCH_MSG_ID_SET_ENABLE, enable);

    return RET_OK;
}

uint8_t key_wq_touch_adj_thres(uint16_t climb_inc, uint16_t fall_inc)
{
    if (!is_touch_thres_configured()) {
        DBGLOG_KEY_SENSOR_ERROR("key_wq_touch_adj_thres, touch thres not configured yet.\n");
        return RET_NOT_READY;
    }

    DBGLOG_KEY_SENSOR_INFO("key_wq_touch_adj_thres, climb_inc %d, fall_inc %d\n", climb_inc,
                           fall_inc);

    new_climb_thres = touch_context->climb_thres + climb_inc;
    new_fall_thres = touch_context->fall_thres + fall_inc;

    assert(new_climb_thres <= KEY_WQ_TOUCH_MAX_THRES);
    assert(new_fall_thres <= KEY_WQ_TOUCH_MAX_THRES);

    vendor_send_msg(VENDOR_MSG_TYPE_KEY_WQ_TOUCH, TOUCH_MSG_ID_ADJUST_THRS, 0);

    return RET_OK;
}

void key_wq_touch_adj_freq(WQ_TOUCH_KEY_DIV_FREQ freq)
{
    iot_touch_key_adjust_freq((IOT_TOUCH_KEY_DIV_FREQ)freq);
}

uint8_t key_wq_touch_adj_trig_times(uint8_t climb_trig_times, uint8_t fall_trig_times)
{
    if (!is_touch_thres_configured()) {
        DBGLOG_KEY_SENSOR_ERROR("key_wq_touch_adj_trig_times, touch not configured yet.\n");
        return RET_NOT_READY;
    }

    new_climb_trig_times = touch_context->climb_trig_times;
    if (climb_trig_times != 0) {
        new_climb_trig_times = climb_trig_times;
    }

    new_fall_trig_times = touch_context->fall_trig_times;
    if (fall_trig_times != 0) {
        new_fall_trig_times = fall_trig_times;
    }

    assert(new_climb_trig_times <= KEY_WQ_TOUCH_MAX_TRIG_TIMES);
    assert(new_fall_trig_times <= KEY_WQ_TOUCH_MAX_TRIG_TIMES);

    DBGLOG_KEY_SENSOR_INFO("key_wq_touch_adj_trig_times, climb_trig_times %d, fall_trig_times %d\n",
                           new_climb_trig_times, new_fall_trig_times);

    vendor_send_msg(VENDOR_MSG_TYPE_KEY_WQ_TOUCH, TOUCH_MSG_ID_ADJUST_TRIG_TIMES, 0);

    return RET_OK;
}

static void cli_get_key_info_handler(uint8_t *buffer, uint32_t length)
{
    uint32_t cdc;
    key_touch_dump_t info;

    UNUSED(buffer);
    memset(&info, 0x00, sizeof(info));

    if (length != 0) {
        DBGLOG_KEY_SENSOR_ERROR("cli_get_key_info_handler invalid length:%d\n", length);
        cli_interface_msg_response(CLI_MODULEID_APPLICATION, APP_CLI_MSG_ID_GET_TOUCH_KEY_INFO,
                                   NULL, 0, 0, RET_FAIL);
        return;
    }

    if (touch_context->num == 0) {
        DBGLOG_KEY_SENSOR_ERROR("cli_get_key_info_handler touch_context->num==0\n");
        cli_interface_msg_response(CLI_MODULEID_APPLICATION, APP_CLI_MSG_ID_GET_TOUCH_KEY_INFO,
                                   NULL, 0, 0, RET_FAIL);
        return;
    }

    if (RET_OK != iot_touch_key_read_pad_cdc((IOT_TK_PAD_ID)touch_context->pad[0], &cdc)) {
        DBGLOG_KEY_SENSOR_ERROR("cli_get_key_info_handler get %d cdc failed.\n",
                                touch_context->pad[0]);
        cli_interface_msg_response(CLI_MODULEID_APPLICATION, APP_CLI_MSG_ID_GET_TOUCH_KEY_INFO,
                                   NULL, 0, 0, RET_FAIL);
        return;
    }

    info.enabled = touch_enabled;
    info.climb_intr_times = climb_intr_times;
    info.fall_intr_times = fall_intr_times;
    info.cdc = cdc;
    cli_interface_msg_response(CLI_MODULEID_APPLICATION, APP_CLI_MSG_ID_GET_TOUCH_KEY_INFO,
                               (uint8_t *)&info, sizeof(info), 0, RET_OK);
    key_wq_touch_dump_all_info(cdc);
    iot_touch_key_dump_all_config();

    if (os_is_timer_active(touch_context->print_cdc_timer)) {
        os_stop_timer(touch_context->print_cdc_timer);
    } else {
        os_start_timer(touch_context->print_cdc_timer, KEY_WQ_TOUCH_GET_CDC_TIMER);
    }
}

CLI_ADD_COMMAND(CLI_MODULEID_APPLICATION, APP_CLI_MSG_ID_GET_TOUCH_KEY_INFO,
                cli_get_key_info_handler);

#endif   //(KEY_DRIVER_SELECTION == KEY_DRIVER_WQ_TOUCH)
