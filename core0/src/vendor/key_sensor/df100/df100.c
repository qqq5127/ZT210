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
#include "key_sensor.h"

#if KEY_DRIVER_SELECTION == KEY_DRIVER_DF100
#include "types.h"
#include "string.h"
#include "os_timer.h"
#include "os_utils.h"
#include "iot_i2c.h"
#include "iot_gpio.h"
#include "iot_debounce.h"
#include "vendor_msg.h"
#include "df100.h"

#define GPIO_VALUE_UNKNOWN 0
#define GPIO_VALUE_HIGH    1
#define GPIO_VALUE_LOW     2

#define PRESSURE_THRESHOLD 0x250

/*pressure SCL/SDA pin*/
#define PRESSURE_SCL_GPIO IOT_GPIO_64
#define PRESSURE_SDA_GPIO IOT_GPIO_63
#define PRESSURE_INT_GPIO IOT_GPIO_70
#define PRESSURE_VCC_GPIO IOT_GPIO_73

/*DF100 i2c ADDR */
#define PRESSURE_ADDR (0x4A)

#define ANTI_SHAKE_TIME         80     //ms
#define LONG_PRESS_TIME         1500   //ms
#define LONG_PRESS_RELEASE_TIME 100    //ms
#define CLICK_TIMEOUT           800    //ms

#define MSG_ID_INT_HAPPENED 1

#define FIFO_LEN 32

typedef struct {
    uint32_t time;
    bool_t pressed;
} int_state_t;

static timer_id_t check_timer = 0;
static bool_t inited = false;

static key_callback_t key_callback = NULL;

static uint32_t cur_pos = 0;
static int_state_t int_states[FIFO_LEN];

static bool_t fifo_handling = false;
static bool_t long_press_handling = false;

static void fifo_in(bool_t pressed)
{
    if (fifo_handling) {
        return;
    }

    int_states[cur_pos].time = os_boot_time32();
    int_states[cur_pos].pressed = pressed;
    cur_pos = (cur_pos + 1) % FIFO_LEN;
}

static void fifi_reset(void)
{
    memset(int_states, 0, sizeof(int_states));
}

static void send_key_event(uint8_t key_type)
{
    key_pressed_info_t key_press_info;
    if (!key_callback) {
        DBGLOG_KEY_SENSOR_ERROR("df100 send key error, key_callback==NULL\n");
        return;
    }

    key_press_info.num = 1;
    key_press_info.id[0] = 1;
    key_press_info.src[0] = 0;
    key_press_info.type[0] = key_type;

    key_callback(&key_press_info);
}

static void check_timer_handler(timer_id_t timer_id, void *arg)
{
    uint32_t pos;
    uint32_t prev_pos;
    uint32_t tm;
    bool_t pressed;
    uint32_t click_count = 0;
    uint32_t last_tm = 0;
    bool_t last_pressed = false;

    UNUSED(timer_id);
    UNUSED(arg);

    DBGLOG_KEY_SENSOR_INFO("df100 check_timer_handler\n");

    fifo_handling = true;

    prev_pos = (cur_pos + FIFO_LEN - 1) % FIFO_LEN;
    if (int_states[prev_pos].pressed) {   //long press timer occured
        if (!iot_gpio_read(PRESSURE_INT_GPIO)) {
            send_key_event(BTN_TYPE_LONG);
            long_press_handling = true;
            DBGLOG_KEY_SENSOR_INFO("long press\n");
        }
    } else if (!long_press_handling) {   //click timeout occured
        pos = cur_pos;
        do {
            tm = int_states[pos].time;
            pressed = int_states[pos].pressed;

            if (tm == 0) {
                pos = (pos + 1) % FIFO_LEN;
                continue;
            }

            if ((last_tm != 0) && (tm - last_tm >= ANTI_SHAKE_TIME)) {
                if (last_pressed && (!pressed))   //button up, click count +1
                {
                    click_count += 1;
                }
            } else if (last_tm != 0) {
                DBGLOG_KEY_SENSOR_INFO("%d too short %d->%d diff:%d\n", pos, last_pressed, pressed,
                                       tm - last_tm);
            }

            last_tm = tm;
            last_pressed = pressed;
            pos = (pos + 1) % FIFO_LEN;
        } while (pos != cur_pos);
    } else {
        long_press_handling = false;
    }

    memset(int_states, 0, sizeof(int_states));
    cur_pos = 0;
    fifo_handling = false;

    if (click_count >= 1) {
        DBGLOG_KEY_SENSOR_INFO("click count:%d\n", click_count);
        switch (click_count) {
            case 1:
                send_key_event(BTN_TYPE_SINGLE);
                break;
            case 2:
                send_key_event(BTN_TYPE_DOUBLE);
                break;
            case 3:
                send_key_event(BTN_TYPE_TRIPLE);
                break;
            case 4:
                send_key_event(BTN_TYPE_QUADRUPLE);
                break;
            default:
                break;
        }
    }
}

static inline int i2c_write(uint8_t addr, uint8_t reg, uint8_t data)
{
    uint8_t ret = 0;

    ret = iot_i2c_master_transmit_to_memory_poll(IOT_I2C_PORT_0, addr, reg,
                                                 IOT_I2C_MEMORY_ADDR_8BIT, &data, 1, 1000);
    if (ret != 0) {
        DBGLOG_KEY_SENSOR_ERROR("write i2c error %d!\n", ret);
    }

    return ret;
}

static inline int i2c_read(uint8_t addr, uint8_t reg, uint8_t *buf, uint16_t len)
{
    uint8_t ret = 0;

    ret = iot_i2c_master_receive_from_memory_poll(IOT_I2C_PORT_0, addr, reg,
                                                  IOT_I2C_MEMORY_ADDR_8BIT, buf, len, 1000);
    if (ret != 0) {
        DBGLOG_KEY_SENSOR_ERROR("i2c read error %d!\n", ret);
    }
    return ret;
}

static void gpio_int_hander(uint8_t value)
{
    uint8_t pressed;

    if (value == GPIO_VALUE_HIGH) {
        pressed = 0;
    } else if (value == GPIO_VALUE_LOW) {
        pressed = 1;
    } else {
        pressed = !iot_gpio_read(PRESSURE_INT_GPIO);
    }

    fifo_in(pressed);

    DBGLOG_KEY_SENSOR_INFO("df100 gpio_int_handler pressed:%d value:%d\n", pressed, value);

    os_stop_timer(check_timer);
    if (pressed) {
        send_key_event(BTN_TYPE_PRESS);
        if (!long_press_handling) {
            os_start_timer(check_timer, LONG_PRESS_TIME);
        }
    } else {
        if (long_press_handling) {
            os_start_timer(check_timer, LONG_PRESS_RELEASE_TIME);
        } else {
            os_start_timer(check_timer, CLICK_TIMEOUT);
        }
    }
}

static void gpio_int_isr(void) IRAM_TEXT(gpio_int_isr);
static void gpio_int_isr(void)
{
    vendor_send_msg_from_isr(VENDOR_MSG_TYPE_FORCE_TOUCH, MSG_ID_INT_HAPPENED, GPIO_VALUE_UNKNOWN);
}

static uint8_t df100_config(void)
{
    uint8_t check_flag = 0;

    i2c_read(PRESSURE_ADDR, 0x80, &check_flag, sizeof(check_flag));
    DBGLOG_KEY_SENSOR_INFO("check_flag = 0x%02x\n", check_flag);
    if ((check_flag >> 3) == 0x04) {
        DBGLOG_KEY_SENSOR_INFO("pressure check ok\n");
        i2c_write(PRESSURE_ADDR, 0X00, 0xB5);
        i2c_write(PRESSURE_ADDR, 0X01, 0x40);
        i2c_write(PRESSURE_ADDR, 0X06, 0x85);
        i2c_write(PRESSURE_ADDR, 0X07, 0X02);
        i2c_write(PRESSURE_ADDR, 0X08, 0X00);
        i2c_write(PRESSURE_ADDR, 0X09, 0XB3);
        i2c_write(PRESSURE_ADDR, 0X0C, (PRESSURE_THRESHOLD >> 4) & 0xFF);
        i2c_write(PRESSURE_ADDR, 0X0D, (PRESSURE_THRESHOLD << 4) & 0xF0);
        i2c_write(PRESSURE_ADDR, 0X0E, 0x13);
        i2c_write(PRESSURE_ADDR, 0X0F, 0X01);
        return 1;
    } else {
        DBGLOG_KEY_SENSOR_INFO("\n**********pressure init fail\n");
        return 0;
    }
}

static int df100_i2c_init(void)
{
    unsigned char i2c_flag = 0;

    IOT_I2C_PORT port = IOT_I2C_PORT_0;

    iot_i2c_config_t cfg;
    cfg.i2c_busrt_mode = 1;
    cfg.baudrate = 200000;
    cfg.wait_nack_max_time = 100;

    if (iot_i2c_init(port, &cfg)) {
        DBGLOG_KEY_SENSOR_ERROR("iot_i2c_init failed\n");
        return RET_FAIL;
    }

    iot_i2c_gpio_cfg_t gpio_cfg;
    gpio_cfg.scl = PRESSURE_SCL_GPIO;
    gpio_cfg.sda = PRESSURE_SDA_GPIO;

    if (iot_i2c_open(port, &gpio_cfg)) {
        DBGLOG_KEY_SENSOR_ERROR("iot_i2c_open failed\n");
        return RET_FAIL;
    }

    i2c_flag = df100_config();
    if (i2c_flag == 1) {
        DBGLOG_KEY_SENSOR_INFO("df100 init succeed\n");
        return 0;
    } else {
        return RET_FAIL;
    }
}

static void msg_handler(uint8_t msg_id, uint16_t msg_value)
{
    UNUSED(msg_value);

    switch (msg_id) {
        case MSG_ID_INT_HAPPENED:
            gpio_int_hander(msg_value);
            break;
        default:
            break;
    }
}

static void isr_state_changed(uint16_t gpio, IOT_DEBOUNCE_INT int_type)
{
    UNUSED(gpio);

    if (int_type == IOT_DEBOUNCE_INT_PRESS_MID) {
        vendor_send_msg_from_isr(VENDOR_MSG_TYPE_FORCE_TOUCH, MSG_ID_INT_HAPPENED, GPIO_VALUE_LOW);
    } else if (int_type == IOT_DEBOUNCE_INT_PRESS) {
        vendor_send_msg_from_isr(VENDOR_MSG_TYPE_FORCE_TOUCH, MSG_ID_INT_HAPPENED, GPIO_VALUE_HIGH);
    }

    return;
}

void df100_init(key_callback_t callback)
{
    key_callback = callback;

    if (inited) {
        return;
    }

    vendor_register_msg_handler(VENDOR_MSG_TYPE_FORCE_TOUCH, msg_handler);

    iot_gpio_open(PRESSURE_VCC_GPIO, IOT_GPIO_DIRECTION_OUTPUT);
    iot_gpio_set_pull_mode(PRESSURE_VCC_GPIO, IOT_GPIO_PULL_UP);
    iot_gpio_write(PRESSURE_VCC_GPIO, 1);

    iot_gpio_open(PRESSURE_SCL_GPIO, IOT_GPIO_DIRECTION_OUTPUT);
    iot_gpio_set_pull_mode(PRESSURE_SCL_GPIO, IOT_GPIO_PULL_UP);
    iot_gpio_write(PRESSURE_SCL_GPIO, 0);

    iot_gpio_open(PRESSURE_SDA_GPIO, IOT_GPIO_DIRECTION_OUTPUT);
    iot_gpio_set_pull_mode(PRESSURE_SDA_GPIO, IOT_GPIO_PULL_UP);
    iot_gpio_write(PRESSURE_SDA_GPIO, 0);

    iot_gpio_open(PRESSURE_INT_GPIO, IOT_GPIO_DIRECTION_OUTPUT);
    iot_gpio_write(PRESSURE_INT_GPIO, 0);

    os_delay(10);
    iot_gpio_write(PRESSURE_VCC_GPIO, 0);
    iot_gpio_close(PRESSURE_SCL_GPIO);
    iot_gpio_close(PRESSURE_SDA_GPIO);
    iot_gpio_close(PRESSURE_INT_GPIO);
    os_delay(50);

    if (df100_i2c_init()) {
        DBGLOG_KEY_SENSOR_INFO("pressure_driver_init fail\n");
        return;
    }

    fifi_reset();

    check_timer = os_create_timer(LIB_KEYMGMT_MID, 0, check_timer_handler, NULL);

#if 0
    if (iot_gpio_open_as_interrupt(PRESSURE_INT_GPIO, IOT_GPIO_INT_EDGE_BOTH, gpio_int_isr)) {
        DBGLOG_KEY_SENSOR_ERROR("df100 open interrupt %d error\n", PRESSURE_INT_GPIO);
        return;
    }
    iot_gpio_set_pull_mode(PRESSURE_INT_GPIO, IOT_GPIO_PULL_UP);
#else
    UNUSED(gpio_int_isr);
    if (iot_gpio_open(PRESSURE_INT_GPIO, IOT_GPIO_DIRECTION_INPUT)) {
        DBGLOG_KEY_SENSOR_ERROR("df100 open gpio %d error\n", PRESSURE_INT_GPIO);
        return;
    }
    iot_gpio_set_pull_mode(PRESSURE_INT_GPIO, IOT_GPIO_PULL_UP);

    iot_debounce_int_cfg_t int_cfg;
    int_cfg.cb = isr_state_changed;
    int_cfg.int_io_en = false;
    int_cfg.int_press_en = true;
    int_cfg.int_press_mid_en = true;

    iot_debounce_gpio(PRESSURE_INT_GPIO, 50, IOT_DEBOUNCE_EDGE_FALLING, &int_cfg);
#endif

    inited = true;
}

void df100_deinit(bool_t wakeup_enable)
{
    if (!inited) {
        return;
    }

    i2c_write(PRESSURE_ADDR, 0X00, 0xB4);

    if (!wakeup_enable) {
        iot_gpio_int_disable(PRESSURE_INT_GPIO);
        iot_gpio_close(PRESSURE_INT_GPIO);
        iot_gpio_write(PRESSURE_VCC_GPIO, 1);
    }

    os_stop_timer(check_timer);
    os_delete_timer(check_timer);
    inited = false;
}

#endif /* KEY_DRIVER_SELECTION == KEY_DRIVER_DF100 */
