#include "key_sensor.h"

#if (KEY_DRIVER_SELECTION == KEY_DRIVER_EXT_TOUCH)
#include "string.h"
#include "os_timer.h"
#include "key_base.h"
#include "iot_gpio.h"
#include "iot_debounce.h"
#include "iot_resource.h"
#include "key_ext_touch.h"
#include "vendor_msg.h"
#include "boot_reason.h"

#ifndef RESOURCE_GPIO_TOUCH_VCC
#define RESOURCE_GPIO_TOUCH_VCC GPIO_CUSTOMIZE_5
#endif

#ifndef RESOURCE_GPIO_TOUCH_OUT
#define RESOURCE_GPIO_TOUCH_OUT GPIO_CUSTOMIZE_6
#endif

#ifndef EXT_TOUCH_SRC
#define EXT_TOUCH_SRC KEY_SRC_TOUCH
#endif

/* time compensation from waking-up pressing to the IO interrupt reporting. */
#define KEY_EXT_TOUCH_TIME_OFFSET 500 /* ms */

typedef enum {
    MSG_ID_INIT_PRESSED = 0,
    MSG_ID_PRESSED,
    MSG_ID_RELEASED,
    MSG_ID_CHECK_EVENT,
} touch_msg_id_t;

static timer_id_t timer = 0;
static bool_t is_shutdown = false;
static bool_t first_intr = true;
static bool_t touch_enabled = true;
static uint8_t vcc_gpio = 0xFF;
static uint8_t touch_gpio = 0xFF;
static IOT_GPIO_PULL_MODE vcc_pull_mode = IOT_GPIO_PULL_DOWN;
static IOT_GPIO_PULL_MODE touch_pull_mode = IOT_GPIO_PULL_DOWN;

static key_id_cfg_t key_id_cfg = {0};

static void isr_state_changed(uint16_t gpio, IOT_DEBOUNCE_INT int_type)
    IRAM_TEXT(isr_state_changed);
static void isr_state_changed(uint16_t gpio, IOT_DEBOUNCE_INT int_type)
{
    if (!touch_enabled) {
        return;
    }

    if (int_type == IOT_DEBOUNCE_INT_PRESS_MID) {
        vendor_send_msg_from_isr(VENDOR_MSG_TYPE_KEY_EXT_TOUCH, MSG_ID_PRESSED, gpio);
    } else if (int_type == IOT_DEBOUNCE_INT_PRESS) {
        vendor_send_msg_from_isr(VENDOR_MSG_TYPE_KEY_EXT_TOUCH, MSG_ID_RELEASED, gpio);
    }
}

static void ms_key_add(uint16_t gpio, IOT_GPIO_PULL_MODE pull_mode, uint8_t timer)
{
    uint8_t ret = RET_OK;
    iot_debounce_int_cfg_t int_cfg;

    ret = iot_gpio_open(gpio, IOT_GPIO_DIRECTION_INPUT);
    if (ret != RET_OK) {

        DBGLOG_KEY_SENSOR_ERROR("open gpio %d failed, ret = %d\n", gpio, ret);
        assert(ret == RET_OK);
        return;
    }

    iot_gpio_set_pull_mode(gpio, pull_mode);

    int_cfg.cb = isr_state_changed;
    int_cfg.int_io_en = false;
    int_cfg.int_press_en = true;
    int_cfg.int_press_mid_en = true;

    if (pull_mode == IOT_GPIO_PULL_UP) {
        ret = iot_debounce_gpio(gpio, timer, IOT_DEBOUNCE_EDGE_FALLING, &int_cfg);
    } else {
        ret = iot_debounce_gpio(gpio, timer, IOT_DEBOUNCE_EDGE_RAISING, &int_cfg);
    }
    return;
}

static uint32_t key_ext_touch_enable(uint8_t key_id, uint8_t key_src, uint8_t debounce_time)
{
    vcc_gpio = iot_resource_lookup_gpio(RESOURCE_GPIO_TOUCH_VCC);

    if (vcc_gpio == 0xFF) {
        DBGLOG_KEY_SENSOR_INFO("key_ext_touch_enable vcc gpio not exists\n");
    }

    touch_gpio = iot_resource_lookup_gpio(RESOURCE_GPIO_TOUCH_OUT);

    if (touch_gpio == 0xFF) {
        DBGLOG_KEY_SENSOR_INFO("get invalid gpio_id via key_id %d\n", key_id);
        return RET_NOT_EXIST;
    }

    if (vcc_gpio != 0xFF) {
        vcc_pull_mode = iot_resource_lookup_pull_mode(vcc_gpio);
        uint8_t ret = iot_gpio_open(vcc_gpio, IOT_GPIO_DIRECTION_OUTPUT);
        if (ret != RET_OK) {
            DBGLOG_KEY_SENSOR_ERROR("open gpio %d failed, ret = %d\n", vcc_gpio, ret);
            return ret;
        }

        iot_gpio_set_pull_mode(vcc_gpio, vcc_pull_mode);

        if (vcc_pull_mode == IOT_GPIO_PULL_UP) {
            iot_gpio_write(vcc_gpio, 0);
            DBGLOG_KEY_SENSOR_INFO("key_ext_touch_enable vcc:%d low\n", vcc_gpio);
        } else {
            iot_gpio_write(vcc_gpio, 1);
            DBGLOG_KEY_SENSOR_INFO("key_ext_touch_enable vcc:%d high\n", vcc_gpio);
        }
    }

    touch_pull_mode = iot_resource_lookup_pull_mode(touch_gpio);
    DBGLOG_KEY_SENSOR_INFO("ext_touch: key_id: %d, io_id: %d, pull mode: %d\n", key_id, touch_gpio,
                           touch_pull_mode);

    key_base_register(key_id, key_src, touch_gpio);
    ms_key_add(touch_gpio, touch_pull_mode, debounce_time);

    return RET_OK;
}

static void key_debounce_timer_cb(timer_id_t timer_id, void *arg)
{
    UNUSED(timer_id);
    UNUSED(arg);

    vendor_send_msg(VENDOR_MSG_TYPE_KEY_EXT_TOUCH, MSG_ID_CHECK_EVENT, 0);
}

static void key_ext_touch_init_pressed(uint16_t io_id)
{
    key_base_pressed(io_id, KEY_EXT_TOUCH_TIME_OFFSET);

    /* start timer. if other keys in BTN_STATE_PRESS or BTN_STATE_RELEASE
       state, timer is still active. */
    if (os_is_timer_active(timer) == 0) {
        os_start_timer(timer, 100);
        DBGLOG_KEY_SENSOR_INFO("start debounce io timer.\n");
    }
}

static void key_ext_touch_pressed(uint16_t io_id)
{
    uint32_t time_offset = 0;

    if (is_shutdown || (!touch_enabled)) {
        return;
    }

    if (first_intr == true) {
        if (BOOT_REASON_SLEEP == boot_reason_get_reason()) {
            BOOT_REASON_WAKEUP_SOURCE wakeup_source = boot_reason_get_wakeup_source();
            if (BOOT_REASON_WAKEUP_SRC_DEB == wakeup_source) {
                DBGLOG_KEY_SENSOR_INFO("first press interrupt for debounce source wake up.\n");
                time_offset = KEY_EXT_TOUCH_TIME_OFFSET;
            }
        }
        first_intr = false;
    }

    key_base_pressed(io_id, time_offset);

    /* start timer. if other keys in BTN_STATE_PRESS or BTN_STATE_RELEASE
       state, timer is still active. */
    if (os_is_timer_active(timer) == 0) {
        os_start_timer(timer, 100);
        DBGLOG_KEY_SENSOR_INFO("start debounce io timer.\n");
    }
}

static void key_ext_touch_released(uint16_t io_id)
{
    key_base_released(io_id);
}

static void key_ext_touch_check_type(void)
{
    if (is_shutdown) {
        return;
    }

    if (CHECK_DONE == key_base_check_type()) {

        if (os_is_timer_active(timer) == 1) {
            os_stop_timer(timer);
            DBGLOG_KEY_SENSOR_INFO("stop debounce io timer.\n");
        }
    }
}

static void key_ext_touch_msg_handler(uint8_t msg_id, uint16_t io_id)
{
    switch (msg_id) {
        case MSG_ID_INIT_PRESSED:
            key_ext_touch_init_pressed(io_id);
            break;
        case MSG_ID_PRESSED:
            key_ext_touch_pressed(io_id);
            break;
        case MSG_ID_RELEASED:
            key_ext_touch_released(io_id);
            break;
        case MSG_ID_CHECK_EVENT:
            key_ext_touch_check_type();
            break;
        default:
            DBGLOG_KEY_SENSOR_ERROR("key_ext_touch_msg_handler unknown msg_id:%d\n", msg_id);
            break;
    }
}

void key_ext_touch_init(void)
{
    DBGLOG_KEY_SENSOR_INFO("key_ext_touch_init\n");

    vendor_register_msg_handler(VENDOR_MSG_TYPE_KEY_EXT_TOUCH, key_ext_touch_msg_handler);
    timer = os_create_timer(LIB_KEYMGMT_MID, true, key_debounce_timer_cb, NULL);
}

void key_ext_touch_deinit(bool_t wakeup_enable)
{
    DBGLOG_KEY_SENSOR_INFO("key_ext_touch_deinit\n");

    is_shutdown = true;

    if (timer && os_is_timer_active(timer)) {
        os_stop_timer(timer);
    }

    if (!wakeup_enable) {
        if (vcc_gpio != 0xFF) {
            if (vcc_pull_mode == IOT_GPIO_PULL_UP) {
                iot_gpio_write(vcc_gpio, 1);
            } else {
                iot_gpio_write(vcc_gpio, 0);
            }
            iot_gpio_close(vcc_gpio);
        }

        if (touch_gpio != 0xFF) {
            if (touch_pull_mode == IOT_GPIO_PULL_UP) {
                iot_gpio_write(touch_gpio, 1);
            } else {
                iot_gpio_write(touch_gpio, 0);
            }
            iot_debounce_gpio_disable(touch_gpio);
            iot_gpio_close(touch_gpio);
        }
    }
}

void key_ext_touch_open(const key_id_cfg_t *id_cfg, const key_time_cfg_t *time_cfg)
{
    uint32_t i;

    if ((id_cfg == NULL) || (time_cfg == NULL)) {
        DBGLOG_KEY_SENSOR_ERROR("key_ext_touch_open illegal, id_cfg = %d, time_cfg = %d\n", id_cfg,
                                time_cfg);
        return;
    }

    memcpy(&key_id_cfg, id_cfg, sizeof(key_id_cfg_t));

    for (i = 0; i < id_cfg->num; i++) {

        if ((EXT_TOUCH_SRC != 0xFF) && (id_cfg->src[i] != EXT_TOUCH_SRC)) {
            continue;
        }

        key_ext_touch_enable(id_cfg->id[i], id_cfg->src[i], time_cfg->debounce_time);
        break;
    }
}

void key_ext_touch_set_enabled(bool_t enable)
{
    if (enable) {
        /** enable */
        touch_enabled = true;
    } else {
        /** disable */
        touch_enabled = false;
        key_ext_touch_released(touch_gpio);
    }
}
#endif
