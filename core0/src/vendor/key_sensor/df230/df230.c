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
#include "key_base.h"

#if KEY_DRIVER_SELECTION == KEY_DRIVER_DF230
#include "types.h"
#include "modules.h"
#include "string.h"
#include "os_timer.h"
#include "os_utils.h"
#include "iot_i2c.h"
#include "iot_gpio.h"
#include "iot_debounce.h"
#include "vendor_msg.h"
#include "df230.h"
#include "iot_resource.h"

#ifndef DF230_SCL_GPIO
#define DF230_SCL_GPIO GPIO_CUSTOMIZE_5
#endif

#ifndef DF230_SDA_GPIO
#define DF230_SDA_GPIO GPIO_CUSTOMIZE_6
#endif

#ifndef DF230_INT_GPIO
#define DF230_INT_GPIO GPIO_CUSTOMIZE_7
#endif

#ifndef DF230_VCC_GPIO
#define DF230_VCC_GPIO GPIO_CUSTOMIZE_8
#endif

//#define DF230_BEBUG
#ifdef DF230_BEBUG
#define BEBUG_GPIO IOT_GPIO_03
#endif

#define MSG_ID_INT_HAPPENED 1
#define MSG_ID_READ_DATA    2

#define GPIO_VALUE_UNKNOWN 0
#define GPIO_VALUE_HIGH    1
#define GPIO_VALUE_LOW     2

#define ANTI_SHAKE_TIME         50     //ms
#define LONG_PRESS_TIME         1500   //ms
#define LONG_PRESS_RELEASE_TIME 100    //ms
#define CLICK_TIMEOUT           1000   //ms

#define GPIO_POLLING_TIMER_INTERVAL ANTI_SHAKE_TIME   //ms
#define FORCE_SENSITIVITY           10
#define FORCE_REALEASE_DIFF         (FORCE_SENSITIVITY / 2)
#define INTERRUPT_SENSITIVITY       0x02

#define INT_GPIO_DEBOUNCE

/*pressure SCL/SDA pin*/
#define SCL_GPIO DF230_SCL_GPIO
#define SDA_GPIO DF230_SDA_GPIO
#define INT_GPIO DF230_INT_GPIO
#define VCC_GPIO DF230_VCC_GPIO

/*DF_230 i2c ADDR */
#define PRESSURE_ADDR      (0x27)   //SA0 = GND : 0x26;SA0 = VDD || float : 0x27
#define DF230_CHIPID_ADDR  (0x01)
#define DF230_CHIPID_VALUE (0x13)

static key_callback_t key_callback = NULL;

static timer_id_t check_timer_idle = 0;
static timer_id_t check_timer = 0;
static timer_id_t gpio_timer = 0;
static bool_t df230_inited = false;

static bool_t last_pressed = 0;
static bool_t check_inprocess = false;
static bool_t read_base_enable = true;

static int16_t force_base_data = 0x7FFF;

typedef struct {
    uint32_t time;
    bool_t pressed;
} int_state_t;

#define FIFO_LEN 32
static uint32_t cur_pos = 0;
static int_state_t int_states[FIFO_LEN];

static bool_t fifo_handling = false;
static bool_t long_press_handling = false;

static uint8_t gpio_vcc = 0xff;
static uint8_t gpio_int = 0xff;
static uint8_t gpio_scl = 0xff;
static uint8_t gpio_sda = 0xff;

static void fifo_in(bool_t pressed)
{
    if (fifo_handling) {
        return;
    }

    int_states[cur_pos].time = os_boot_time32();
    int_states[cur_pos].pressed = pressed;
    cur_pos = (cur_pos + 1) % FIFO_LEN;
}

static void fifo_reset(void)
{
    memset(int_states, 0, sizeof(int_states));
}

static void send_key_event(uint8_t key_type)
{
    key_pressed_info_t key_press_info;

    if (!key_callback) {
        DBGLOG_KEY_SENSOR_ERROR("df230 send key error, key_callback==NULL\n");
        return;
    }

    key_press_info.num = 1;
    key_press_info.id[0] = 0;
    key_press_info.src[0] = KEY_SRC_EXTERNAL;
    key_press_info.type[0] = key_type;

    key_callback(&key_press_info);
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

static inline int i2c_read(uint8_t addr, uint8_t reg, uint8_t *buf, uint8_t len)
{
    uint8_t ret = 0;

    ret = iot_i2c_master_receive_from_memory_poll(IOT_I2C_PORT_0, addr, reg,
                                                  IOT_I2C_MEMORY_ADDR_8BIT, buf, len, 1000);
    if (ret != 0) {
        DBGLOG_KEY_SENSOR_ERROR("i2c read error %d!\n", ret);
    }
    return ret;
}

static int df230_i2c_init(void)
{
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

    gpio_cfg.scl = gpio_scl;
    gpio_cfg.sda = gpio_sda;

    if (iot_i2c_open(port, &gpio_cfg)) {
        DBGLOG_KEY_SENSOR_ERROR("iot_i2c_open failed\n");
        return RET_FAIL;
    }

    return 0;
}

static int16_t froce_data_read(void)
{
    uint8_t temp[2] = {0};
    uint16_t ret;

    if (i2c_read(PRESSURE_ADDR, 0x06, temp, sizeof(temp)) != 0) {
        DBGLOG_KEY_SENSOR_ERROR("read_data_failed!\n");
        return 0x7FFF;
    }

    ret = ((uint16_t)(temp[1]) << 2 | temp[0] >> 6);

    if (ret == 0) {
        if (i2c_read(PRESSURE_ADDR, 0x06, temp, sizeof(temp)) != 0) {
            DBGLOG_KEY_SENSOR_ERROR("read_data_failed!\n");
            return 0x7FFF;
        }
        ret = ((uint16_t)(temp[1]) << 2 | temp[0] >> 6);
    }

    return ret ? (int16_t)ret : 0x7FFF;
}

static void df230_open_active_interrupt(void)
{
    i2c_write(PRESSURE_ADDR, 0x19, 0x04);
    i2c_write(PRESSURE_ADDR, 0x16, 0x84);
    i2c_write(PRESSURE_ADDR, 0x10, 0x05);
    DBGLOG_KEY_SENSOR_INFO("df230_open_active_interrupt\n");
}

static void df230_close_active_interrupt(void)
{
    i2c_write(PRESSURE_ADDR, 0x19, 0x00);
    i2c_write(PRESSURE_ADDR, 0x16, 0x00);
    i2c_write(PRESSURE_ADDR, 0x10, 0x07);
    DBGLOG_KEY_SENSOR_INFO("df230_close_active_interrupt\n");
}

static void check_timer_handler(timer_id_t timer_id, void *arg)
{
    UNUSED(timer_id);
    UNUSED(arg);

    uint32_t pos;
    uint32_t prev_pos;
    uint32_t tm;
    bool_t pressed;
    uint32_t click_count = 0;
    uint32_t last_tm = 0;
    bool_t last_pressed = false;

    DBGLOG_KEY_SENSOR_INFO("check_timer_handler %d\n", os_boot_time32());

    fifo_handling = true;

    prev_pos = (cur_pos + FIFO_LEN - 1) % FIFO_LEN;
    if (int_states[prev_pos].pressed) {   //long press timer occured
        int16_t force_data;
        force_data = froce_data_read();
        if (force_data - force_base_data >= FORCE_SENSITIVITY) {
            send_key_event(BTN_TYPE_LONG);
            DBGLOG_KEY_SENSOR_INFO("long press\n");
            long_press_handling = true;
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
                if (last_pressed && (!pressed)) {   //button up, click count +1
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
    read_base_enable = true;

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

static void gpio_timer_handler(timer_id_t timer_id, void *arg)
{
    UNUSED(timer_id);
    UNUSED(arg);

    vendor_send_msg(VENDOR_MSG_TYPE_FORCE_TOUCH, MSG_ID_READ_DATA, GPIO_VALUE_UNKNOWN);
}

static void gpio_int_hander(uint8_t value)
{
    if (value != GPIO_VALUE_LOW) {
        return;
    }

    check_inprocess = false;
    last_pressed = 0;
    read_base_enable = false;

    os_stop_timer(gpio_timer);
    df230_close_active_interrupt();
    /*force_base_data = froce_data_read();*/
    DBGLOG_KEY_SENSOR_INFO("force_base_data = 0x%04x\n", force_base_data);

    os_start_timer(gpio_timer, 10);
}

static void read_data_process(void)
{
    int16_t data_tmp = froce_data_read();

    bool_t pressed = data_tmp - force_base_data
            >= (check_inprocess ? (FORCE_SENSITIVITY - FORCE_REALEASE_DIFF) : FORCE_SENSITIVITY)
        ? 1
        : 0;

#ifdef DF230_BEBUG
    iot_gpio_write(BEBUG_GPIO, pressed ? 0 : 1);
#endif

    os_start_timer(gpio_timer, GPIO_POLLING_TIMER_INTERVAL);

    if (!check_inprocess) {
        last_pressed = 0;
    }

    if (!check_inprocess && !pressed) {
        os_stop_timer(gpio_timer);
        df230_open_active_interrupt();
        read_base_enable = true;
        return;
    }

    if (last_pressed != pressed) {
        fifo_in(pressed);
        DBGLOG_KEY_SENSOR_INFO("read_data_process = 0x%04x\n", data_tmp);
    }

    if (pressed) {
        if (last_pressed != pressed) {
            DBGLOG_KEY_SENSOR_INFO("Falling edge..........\n");
            send_key_event(BTN_TYPE_PRESS);
            os_stop_timer(check_timer);
            check_inprocess = true;
            if (!long_press_handling) {
                os_start_timer(check_timer, LONG_PRESS_TIME);
            }
        }
    } else {
        if (last_pressed != pressed) {
            DBGLOG_KEY_SENSOR_INFO("rising edge..........\n");
            check_inprocess = false;
            os_stop_timer(check_timer);
            if (long_press_handling) {
                os_start_timer(check_timer, LONG_PRESS_RELEASE_TIME);
            } else {
                os_start_timer(check_timer, CLICK_TIMEOUT);
            }
        }
    }

    last_pressed = pressed;
}

static void msg_handler(uint8_t msg_id, uint16_t msg_value)
{
    if (!df230_inited) {
        return;
    }

    switch (msg_id) {
        case MSG_ID_INT_HAPPENED:
            gpio_int_hander(msg_value);
            break;
        case MSG_ID_READ_DATA:
            read_data_process();
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

static void gpio_int_isr(void) IRAM_TEXT(gpio_int_isr);
static void gpio_int_isr(void)
{
    vendor_send_msg_from_isr(VENDOR_MSG_TYPE_FORCE_TOUCH, MSG_ID_INT_HAPPENED, GPIO_VALUE_UNKNOWN);
}

static uint8_t df230_config(void)
{
    uint8_t check_flag = 0;
    uint8_t temp;

    i2c_read(PRESSURE_ADDR, DF230_CHIPID_ADDR, &check_flag, sizeof(check_flag));
    DBGLOG_KEY_SENSOR_INFO("check_flag = 0x%02x\n", check_flag);

    if (DF230_CHIPID_VALUE == check_flag) {
        DBGLOG_KEY_SENSOR_INFO("pressure check ok\n");
        i2c_write(PRESSURE_ADDR, 0x00, 0x24);   //soft reset
        os_delay(10);
        i2c_write(PRESSURE_ADDR, 0x11, 0x04);   //normal mode
        i2c_write(PRESSURE_ADDR, 0x0F, 0x01);   //disable WDT
        i2c_write(PRESSURE_ADDR, 0x20, 0x01);   //INT1 active low
        i2c_write(PRESSURE_ADDR, 0x21, 0x0C);   //latch 25ms
        i2c_write(PRESSURE_ADDR, 0x27, 0x00);
        i2c_write(PRESSURE_ADDR, 0x28, INTERRUPT_SENSITIVITY);   //interupt SENSITIVITY

        i2c_write(PRESSURE_ADDR, 0x7F, 0x83);
        i2c_write(PRESSURE_ADDR, 0x7F, 0x69);
        i2c_write(PRESSURE_ADDR, 0x7F, 0xBD);

        i2c_read(PRESSURE_ADDR, 0x8F, &temp, sizeof(temp));
        temp &= ~0x02;
        i2c_write(PRESSURE_ADDR, 0x8F, temp);

        os_delay(10);
        df230_open_active_interrupt();
        os_delay(10);

        return 1;
    } else {
        DBGLOG_KEY_SENSOR_ERROR("\n**********pressure init fail\n");
        return 0;
    }
}

static void check_timer_handler_idle(timer_id_t timer_id, void *arg)
{
    UNUSED(timer_id);
    UNUSED(arg);

    if (read_base_enable) {
        force_base_data = froce_data_read();
        DBGLOG_KEY_SENSOR_INFO("**********check_timer_handler_idle force_base_data = 0x%04x\n",
                               force_base_data);
    }
}

void df230_init(key_callback_t callback)
{
    DBGLOG_KEY_SENSOR_INFO("df230 init start!\n");

    uint8_t i2c_flag = 0;

    if (df230_inited) {
        DBGLOG_KEY_SENSOR_INFO("df230 already init!\n");
        return;
    }

    key_callback = callback;
    vendor_register_msg_handler(VENDOR_MSG_TYPE_FORCE_TOUCH, msg_handler);

    uint8_t pull_mode;

    gpio_vcc = iot_resource_lookup_gpio(VCC_GPIO);
    DBGLOG_KEY_SENSOR_INFO("df230 gpio_vcc = %d\n", gpio_vcc);
    if (gpio_vcc != 0xFF) {
        iot_gpio_open(gpio_vcc, IOT_GPIO_DIRECTION_OUTPUT);
        pull_mode = iot_resource_lookup_pull_mode(gpio_vcc);
        DBGLOG_KEY_SENSOR_INFO("df230 gpio_vcc pull_mode = %d\n", pull_mode);
        iot_gpio_set_pull_mode(gpio_vcc, pull_mode);
        iot_gpio_write(gpio_vcc, 1);
    }

    gpio_scl = iot_resource_lookup_gpio(SCL_GPIO);
    DBGLOG_KEY_SENSOR_INFO("df230 gpio_scl = %d\n", gpio_scl);

    if (gpio_scl != 0xFF) {
        pull_mode = iot_resource_lookup_pull_mode(gpio_scl);
        DBGLOG_KEY_SENSOR_INFO("df230 gpio_scl pull_mode = %d\n", pull_mode);
        if (gpio_scl == IOT_AONGPIO_00) {
            gpio_scl = IOT_GPIO_63;
        } else if (gpio_scl == IOT_AONGPIO_01) {
            gpio_scl = IOT_GPIO_64;
        }
        iot_gpio_open(gpio_scl, IOT_GPIO_DIRECTION_OUTPUT);
        iot_gpio_set_pull_mode(gpio_scl, pull_mode);
        iot_gpio_write(gpio_scl, 0);
    } else {
        DBGLOG_KEY_SENSOR_ERROR("i2c scl io get fail!\n");
        return;
    }

    gpio_sda = iot_resource_lookup_gpio(SDA_GPIO);
    DBGLOG_KEY_SENSOR_INFO("df230 gpio_sda = %d\n", gpio_sda);

    if (gpio_sda != 0xFF) {
        pull_mode = iot_resource_lookup_pull_mode(gpio_sda);
        DBGLOG_KEY_SENSOR_INFO("df230 gpio_sda pull_mode = %d\n", pull_mode);
        if (gpio_sda == IOT_AONGPIO_00) {
            gpio_sda = IOT_GPIO_63;
        } else if (gpio_sda == IOT_AONGPIO_01) {
            gpio_sda = IOT_GPIO_64;
        }
        iot_gpio_open(gpio_sda, IOT_GPIO_DIRECTION_OUTPUT);
        iot_gpio_set_pull_mode(gpio_sda, pull_mode);
        iot_gpio_write(gpio_sda, 0);
    } else {
        DBGLOG_KEY_SENSOR_ERROR("i2c sda io get fail!\n");
        return;
    }

    os_delay(20);
    if (gpio_vcc != 0xff) {
        iot_gpio_write(gpio_vcc, 0);
    }

    iot_gpio_close(gpio_scl);
    iot_gpio_close(gpio_sda);
    os_delay(20);

    fifo_reset();

    if (df230_i2c_init() != 0) {
        DBGLOG_KEY_SENSOR_ERROR("pressure_driver_init fail\n");
        return;
    }

    i2c_flag = df230_config();
    if (i2c_flag != 1) {
        DBGLOG_KEY_SENSOR_ERROR("df230 init fail!\n");
        return;
    }
    DBGLOG_KEY_SENSOR_INFO("df230 init succeed!\n");

#ifdef INT_GPIO_DEBOUNCE
    UNUSED(gpio_int_isr);
    gpio_int = iot_resource_lookup_gpio(INT_GPIO);

    if (gpio_int != 0xff) {
        if (iot_gpio_open(gpio_int, IOT_GPIO_DIRECTION_INPUT)) {
            DBGLOG_KEY_SENSOR_ERROR("df230 open gpio %d error\n", gpio_int);
            return;
        }
        pull_mode = iot_resource_lookup_pull_mode(gpio_int);
        DBGLOG_KEY_SENSOR_INFO("df230 gpio_int pull_mode = %d\n", pull_mode);
        iot_gpio_set_pull_mode(gpio_int, pull_mode);
    } else {
        DBGLOG_KEY_SENSOR_ERROR("int io get fail!\n");
        return;
    }

    iot_debounce_int_cfg_t int_cfg;
    int_cfg.cb = isr_state_changed;
    int_cfg.int_io_en = false;
    int_cfg.int_press_en = true;
    int_cfg.int_press_mid_en = true;

    iot_debounce_gpio(gpio_int, 50, IOT_DEBOUNCE_EDGE_FALLING, &int_cfg);
#else
    UNUSED(isr_state_changed);
    int ret = 0;
    uint8_t gpio_int = iot_resource_lookup_gpio(INT_GPIO);

    iot_gpio_close(gpio_int);
    ret = iot_gpio_open_as_interrupt(gpio_int, IOT_GPIO_INT_EDGE_FALLING, gpio_int_isr);
    assert(ret == RET_OK);
    pull_mode = iot_resource_lookup_pull_mode(gpio_int);
    iot_gpio_set_pull_mode(gpio_int, pull_mode);
    iot_gpio_int_enable(gpio_int);

    if (ret != 0) {
        DBGLOG_KEY_SENSOR_ERROR("open interrupt fail\n");
        return;
    }
#endif

#ifdef DF230_BEBUG
    iot_gpio_open(BEBUG_GPIO, IOT_GPIO_DIRECTION_OUTPUT);
    iot_gpio_set_pull_mode(BEBUG_GPIO, IOT_GPIO_PULL_UP);
    iot_gpio_write(BEBUG_GPIO, 1);
#endif
    force_base_data = froce_data_read();

    check_timer_idle = os_create_timer(LIB_KEYMGMT_MID, 1, check_timer_handler_idle, NULL);
    os_start_timer(check_timer_idle, 60000);

    check_timer = os_create_timer(LIB_KEYMGMT_MID, 0, check_timer_handler, NULL);
    gpio_timer = os_create_timer(LIB_KEYMGMT_MID, 0, gpio_timer_handler, NULL);

    df230_inited = true;
}

void df230_deinit(bool_t wakeup_enable)
{
    if (!df230_inited) {
        return;
    }

    i2c_write(PRESSURE_ADDR, 0x11, 0x85);   //power off DF230
    //i2c_write(PRESSURE_ADDR, 0x10, 0x26);

    if (!wakeup_enable) {
        iot_gpio_int_disable(gpio_int);
        iot_gpio_close(gpio_int);
    }

    os_stop_timer(check_timer);
    os_stop_timer(gpio_timer);
    os_stop_timer(check_timer_idle);

    df230_inited = false;
}

#endif /* KEY_DRIVER_SELECTION == KEY_DRIVER_DF230 */
