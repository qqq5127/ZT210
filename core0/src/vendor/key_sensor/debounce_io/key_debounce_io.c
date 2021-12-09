#include "key_sensor.h"

#if (KEY_DRIVER_SELECTION == KEY_DRIVER_DEBOUNCE_IO)
#include "string.h"
#include "os_timer.h"
#include "key_base.h"
#include "iot_gpio.h"
#include "iot_debounce.h"
#include "iot_resource.h"
#include "key_debounce_io.h"
#include "vendor_msg.h"
#include "boot_reason.h"

/* time compensation from waking-up pressing to the IO interrupt reporting. */
#define KEY_DEBOUNCE_IO_TIME_OFFSET 500 /* ms */

typedef enum {
    DEBOUNCE_MSG_ID_INIT_PRESSED = 0,
    DEBOUNCE_MSG_ID_PRESSED,
    DEBOUNCE_MSG_ID_RELEASED,
    DEBOUNCE_MSG_ID_CHECK_EVENT,
} debounce_msg_id_t;

typedef struct {
    /** timer */
    timer_id_t timer;
    /** if app is shuting down. */
    bool_t is_shutdown;

} key_debounce_context_t;

static key_debounce_context_t _debounce_context;
static key_debounce_context_t *debounce_context = &_debounce_context;
static bool_t first_intr = true;

static key_id_cfg_t key_id_cfg = {0};

static void isr_state_changed(uint16_t gpio, IOT_DEBOUNCE_INT int_type)
    IRAM_TEXT(isr_state_changed);
static void isr_state_changed(uint16_t gpio, IOT_DEBOUNCE_INT int_type)
{
    if (int_type == IOT_DEBOUNCE_INT_PRESS_MID) {
        vendor_send_msg_from_isr(VENDOR_MSG_TYPE_KEY_DEBOUNCE_IO, DEBOUNCE_MSG_ID_PRESSED, gpio);
    } else if (int_type == IOT_DEBOUNCE_INT_PRESS) {
        vendor_send_msg_from_isr(VENDOR_MSG_TYPE_KEY_DEBOUNCE_IO, DEBOUNCE_MSG_ID_RELEASED, gpio);
    }

    return;
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

static uint32_t key_debounce_io_enable(uint8_t key_id, uint8_t key_src, uint8_t debounce_time)
{
    uint8_t gpio_id, pull_mode;

    gpio_id = iot_resource_lookup_gpio((RESOURCE_GPIO_ID)key_id + GPIO_KEY_0);

    if (gpio_id == INVALID_UCHAR) {
        DBGLOG_KEY_SENSOR_INFO("get invalid gpio_id via key_id %d\n", key_id);
        return RET_NOT_EXIST;
    }

    pull_mode = iot_resource_lookup_pull_mode(gpio_id);
    DBGLOG_KEY_SENSOR_INFO("debounce: key_id: %d, io_id: %d, pull mode: %d\n", key_id, gpio_id,
                           pull_mode);

    key_base_register(key_id, key_src, gpio_id);
    ms_key_add(gpio_id, (IOT_GPIO_PULL_MODE)pull_mode, debounce_time);

    return RET_OK;
}

static void key_debounce_timer_cb(timer_id_t timer_id, void *arg)
{
    UNUSED(timer_id);
    UNUSED(arg);

    vendor_send_msg(VENDOR_MSG_TYPE_KEY_DEBOUNCE_IO, DEBOUNCE_MSG_ID_CHECK_EVENT, 0);
}

static void key_debounce_io_init_pressed(uint16_t io_id)
{
    key_base_pressed(io_id, KEY_DEBOUNCE_IO_TIME_OFFSET);

    /* start timer. if other keys in BTN_STATE_PRESS or BTN_STATE_RELEASE
       state, timer is still active. */
    if (os_is_timer_active(debounce_context->timer) == 0) {
        os_start_timer(debounce_context->timer, 100);
        DBGLOG_KEY_SENSOR_INFO("start debounce io timer.\n");
    }
}

static void key_debounce_io_pressed(uint16_t io_id)
{
    uint32_t time_offset = 0;

    if (debounce_context->is_shutdown) {
        return;
    }

    if (first_intr == true) {
        if (BOOT_REASON_SLEEP == boot_reason_get_reason()) {
            BOOT_REASON_WAKEUP_SOURCE wakeup_source = boot_reason_get_wakeup_source();
            if (BOOT_REASON_WAKEUP_SRC_DEB == wakeup_source) {
                DBGLOG_KEY_SENSOR_INFO("first press interrupt for debounce source wake up.\n");
                time_offset = KEY_DEBOUNCE_IO_TIME_OFFSET;
            }
        }
        first_intr = false;
    }

    key_base_pressed(io_id, time_offset);

    /* start timer. if other keys in BTN_STATE_PRESS or BTN_STATE_RELEASE
       state, timer is still active. */
    if (os_is_timer_active(debounce_context->timer) == 0) {
        os_start_timer(debounce_context->timer, 100);
        DBGLOG_KEY_SENSOR_INFO("start debounce io timer.\n");
    }
}

static void key_debounce_io_released(uint16_t io_id)
{
    key_base_released(io_id);
}

static void key_debounce_io_check_type(void)
{
    if (debounce_context->is_shutdown) {
        return;
    }

    if (CHECK_DONE == key_base_check_type()) {

        if (os_is_timer_active(debounce_context->timer) == 1) {
            os_stop_timer(debounce_context->timer);
            DBGLOG_KEY_SENSOR_INFO("stop debounce io timer.\n");
        }
    }
}

static void key_debounce_io_msg_handler(uint8_t msg_id, uint16_t io_id)
{
    switch (msg_id) {
        case DEBOUNCE_MSG_ID_INIT_PRESSED:
            key_debounce_io_init_pressed(io_id);
            break;
        case DEBOUNCE_MSG_ID_PRESSED:
            key_debounce_io_pressed(io_id);
            break;
        case DEBOUNCE_MSG_ID_RELEASED:
            key_debounce_io_released(io_id);
            break;
        case DEBOUNCE_MSG_ID_CHECK_EVENT:
            key_debounce_io_check_type();
            break;
        default:
            DBGLOG_KEY_SENSOR_ERROR("key_debounce_io_msg_handler unknown msg_id:%d\n", msg_id);
            break;
    }
}

void key_debounce_io_init(void)
{
    DBGLOG_KEY_SENSOR_INFO("key_debounce_io_init\n");

    vendor_register_msg_handler(VENDOR_MSG_TYPE_KEY_DEBOUNCE_IO, key_debounce_io_msg_handler);
    memset(debounce_context, 0x00, sizeof(key_debounce_context_t));
    debounce_context->timer = os_create_timer(LIB_KEYMGMT_MID, true, key_debounce_timer_cb, NULL);
}

void key_debounce_io_deinit(bool_t wakeup_enable)
{
    DBGLOG_KEY_SENSOR_INFO("key_debounce_io_deinit\n");

    debounce_context->is_shutdown = true;

    if (debounce_context->timer && os_is_timer_active(debounce_context->timer)) {
        os_stop_timer(debounce_context->timer);
    }

    if (!wakeup_enable) {
        for (uint8_t i = 0; i < key_id_cfg.num; i++) {

            if (key_id_cfg.src[i] != KEY_SRC_IO) {
                continue;
            }

            uint8_t gpio_id =
                iot_resource_lookup_gpio((RESOURCE_GPIO_ID)key_id_cfg.src[i] + GPIO_KEY_0);
            if (gpio_id == INVALID_UCHAR) {
                DBGLOG_KEY_SENSOR_INFO("get invalid gpio_id via key_id %d\n", gpio_id);
                continue;
            }
            iot_debounce_gpio_disable(gpio_id);
            iot_gpio_close(gpio_id);
        }
    }
}

void key_debounce_io_open(const key_id_cfg_t *id_cfg, const key_time_cfg_t *time_cfg)
{
    uint32_t i;

    if ((id_cfg == NULL) || (time_cfg == NULL)) {
        DBGLOG_KEY_SENSOR_ERROR("key_debounce_io_open illegal, id_cfg = %d, time_cfg = %d\n",
                                id_cfg, time_cfg);
        return;
    }

    memcpy(&key_id_cfg, id_cfg, sizeof(key_id_cfg_t));

    for (i = 0; i < id_cfg->num; i++) {

        if (id_cfg->src[i] != KEY_SRC_IO) {
            continue;
        }

        if (!key_debounce_io_enable(id_cfg->id[i], id_cfg->src[i], time_cfg->debounce_time)) {
        }
    }
}

#endif
