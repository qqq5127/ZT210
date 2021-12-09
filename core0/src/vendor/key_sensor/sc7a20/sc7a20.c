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

#if KEY_DRIVER_SELECTION == KEY_DRIVER_SC7A20
#include "types.h"
#include "assert.h"
#include "iot_i2c.h"
#include "os_timer.h"
#include "iot_gpio.h"
#include "iot_resource.h"
#include "os_queue.h"
#include "sc7a20_priv.h"
#include "sc7a20.h"
#include "vendor_msg.h"

#define MY_MID (BASIC_MID_BASE + 38)

#define MSGID_GPIO_INT 1

#define CLICK_CHECK_INTERVAL (unsigned short)50   //interval is 50ms
#define CLICK_PP_NUM         (unsigned short)10   //10-0.5s
#define CLICK_MAX_NUM        (unsigned short)60   //3s

static uint8_t sl_click_timer_en = 0;
static uint8_t sl_click_status = 0;
static uint16_t click_timer_cnt = 0;
static uint16_t click_timer_total_cnt = 0;
static uint8_t click_click_final_cnt = 0;

static uint8_t sl_pp_num = 0;

#if SL_SENSOR_ALOG_RELEASE_ENABLE == 0
int16_t SL_DEBUG_DATA[10][128];
uint8_t SL_DEBUG_DATA_LEN;
#endif

static uint8_t int_gpio = 0;
static uint8_t gpio_intterupt_set = 0;
static timer_id_t g_sensor_timer_id = 0;

static key_callback_t key_callback;
static uint8_t key_id = 0;

/**
 * @brief This function is to get sl_pp_num.
 *
 * @param fun_flag is the flag clean sl_pp_num or not.
 * @return uint8_t sl_pp_num value.
 */
static uint8_t get_click_pp_cnt(uint8_t fun_flag)
{
    if (fun_flag == 0) {
        sl_pp_num = 0;
    }

    return sl_pp_num;
}

/**
 * @brief This function is to square root of a number.
 *
 * @param sqrt_data need to square data.
 * @return uint8_t value of result.
 */
static uint32_t click_sqrt(uint32_t sqrt_data)
{
    uint32_t sl_sort_low, sl_sort_up, sl_sort_mid;
    uint8_t sl_sqrt_num = 0;

    if (sqrt_data == 0) {
        sqrt_data = 2;
    }

    sl_sort_low = 0;
    sl_sort_up = sqrt_data;
    sl_sort_mid = (sl_sort_up + sl_sort_low) / 2;

    while (sl_sqrt_num < 200) {
        if ((sl_sort_mid * sl_sort_mid) > sqrt_data) {
            sl_sort_up = sl_sort_mid;
        } else {
            sl_sort_low = sl_sort_mid;
        }

        if ((sl_sort_up - sl_sort_low) == 1) {
            if ((sl_sort_up * sl_sort_up) - sqrt_data > sqrt_data - (sl_sort_low * sl_sort_low)) {
                return sl_sort_low;
            } else {
                return sl_sort_up;
            }
        }

        sl_sort_mid = (sl_sort_up + sl_sort_low) / 2;
        sl_sqrt_num++;
    }
    return 0;
}

/**
 * @brief This function is to write data to i2c device.
 *
 * @param add is the i2c device address.
 * @param reg is device reg address
 * @param data is write to reg data
 * @return uint8_t 0 for success else error.
 */
static uint8_t sc7a20_i2c_write(uint8_t add, uint8_t reg, uint8_t data)
{
    uint8_t ret = 0;

    ret = iot_i2c_master_transmit_to_memory_poll(IOT_I2C_PORT_0, add, reg, IOT_I2C_MEMORY_ADDR_8BIT,
                                                 &data, 1, 1000);
    if (ret != 0) {
        DBGLOG_KEY_SENSOR_ERROR("write data error!\n");
        return 1;
    }

    return ret;
}

/**
 * @brief This function is to read data from i2c device.
 *
 * @param add is the i2c device address.
 * @param reg is device reg address.
 * @param buf is store the data of read.
 * @param len want to read data len.
 * @return uint8_t real read data.
 */
static uint8_t sc7a20_i2c_read(uint8_t add, uint8_t reg, uint8_t len, uint8_t *buf)
{
    uint32_t read_ret = 0;

    read_ret = iot_i2c_master_receive_from_memory_poll(IOT_I2C_PORT_0, add, reg,
                                                       IOT_I2C_MEMORY_ADDR_8BIT, buf, len, 1000);
    if (read_ret != 0) {
        DBGLOG_KEY_SENSOR_ERROR("[err]i2c read failed: 0n%d.\n", read_ret);
    }
    return len;
}

/**
 * @brief This function is regularly check click status.
 *
 * @return number of click.
 */
static int8_t sc7a20_click_status(void)
{
    uint8_t click_e_cnt = 0;

    if (sl_click_timer_en == 1) {
        click_timer_cnt++;
        if ((click_timer_cnt < CLICK_PP_NUM) && (sl_click_status == 1)) {

#if SL_SENSOR_ALOG_RELEASE_ENABLE == 0
            DBGLOG_KEY_SENSOR_INFO("sl_click_status: %d\n", sl_click_status);
            DBGLOG_KEY_SENSOR_INFO("click_click_final_cnt: %d\n", click_click_final_cnt);
#endif
            sl_click_status = 0;
            click_timer_total_cnt = click_timer_total_cnt + click_timer_cnt;
            click_timer_cnt = 0;
            click_click_final_cnt++;
        }

        click_e_cnt = get_click_pp_cnt(1);

        if ((((click_timer_cnt >= CLICK_PP_NUM) || (click_timer_total_cnt >= CLICK_MAX_NUM))
             && (click_e_cnt < 1))
            || ((click_timer_cnt >= CLICK_PP_NUM) && (click_e_cnt > 0)))
        //      if((click_timer_cnt>=CLICK_PP_NUM)||(click_timer_total_cnt>=CLICK_MAX_NUM))
        {
            ///clear click timer flag
            sl_click_timer_en = 0;
            ///clear click timer cnt value
            click_timer_cnt = 0;
            click_timer_total_cnt = 0;

#if SL_SENSOR_ALOG_RELEASE_ENABLE == 0
            DBGLOG_KEY_SENSOR_INFO("final_cnt:%d click_e_cnt:%d ", click_click_final_cnt,
                                   click_e_cnt);
#endif
            if (click_e_cnt > 0) {
                click_e_cnt = get_click_pp_cnt(0);
                return 0;
            } else {
                return click_click_final_cnt;
            }
        }
    }

    return 0;
}

/**
 * @brief This function is read data from sensor.
 *
 * @param th1 is click amplitude threshold
 * @param th2 is pre-knock noise and thresholds
 * @return int8_t 0 click is invalid 1 click is valid.
 */
static int8_t sc7a20_click_read(uint32_t th1, uint32_t th2)
{
    uint8_t i = 0, j = 0, k = 0;
    uint8_t click_num = 0;
    uint8_t fifo_len;
    uint8_t click_result = 0;

    uint32_t sc7a20_data = 0;
    uint32_t fifo_data_xyz[32] = {0};
    uint32_t click_sum = 0;

    uint8_t data1[5];
    int8_t data[5];

    sc7a20_i2c_read(SC7A20_ADDR, SC7A20_SRC_REG, 1, &fifo_len);
    if ((fifo_len & 0x40) == 0x40) {
        fifo_len = 32;
    } else {
        fifo_len = fifo_len & 0x1f;
    }

    for (i = 0; i < fifo_len; i++) {
        sc7a20_i2c_read(SC7A20_ADDR, 0xA8, 5, &data1[0]);
        data[0] = (int8_t)data1[0];
        data[2] = (int8_t)data1[2];
        data[4] = (int8_t)data1[4];
        sc7a20_data = (data[0]) * (data[0]) + (data[2]) * (data[2]) + (data[4]) * (data[4]);
        sc7a20_data = click_sqrt(sc7a20_data);
        fifo_data_xyz[i] = sc7a20_data;
    }

#if SL_SENSOR_ALOG_RELEASE_ENABLE == 0
    DBGLOG_KEY_SENSOR_INFO("fifo len %d, data: ", fifo_len);
    for (i = 0; i < fifo_len; i++) {
        DBGLOG_KEY_SENSOR_INFO("i = %d, %d ", i, fifo_data_xyz[i]);
    }
#endif

    k = 0;
    for (i = 1; i < fifo_len - 1; i++) {
        if ((fifo_data_xyz[i + 1] > th1) && (fifo_data_xyz[i - 1] < 30)) {
#if SL_SENSOR_ALOG_RELEASE_ENABLE == 0
            DBGLOG_KEY_SENSOR_INFO("in_th\n");
#endif
            if (click_num == 0) {
                click_sum = 0;   //first peak
                for (j = 0; j < i - 1; j++) {
                    if (fifo_data_xyz[j] > fifo_data_xyz[j + 1]) {
                        click_sum += fifo_data_xyz[j] - fifo_data_xyz[j + 1];
                    } else {
                        click_sum += fifo_data_xyz[j + 1] - fifo_data_xyz[j];
                    }
                }
#if SL_SENSOR_ALOG_RELEASE_ENABLE == 0
                DBGLOG_KEY_SENSOR_INFO("click_sum:%d!\n", click_sum);
#endif
                if (click_sum > th2) {
                    sl_pp_num++;
                    break;
                }
                k = i;
            } else {
                k = i;   //sencond peak
            }
        }

        if (k != 0) {
            if (fifo_data_xyz[i - 1] > fifo_data_xyz[i + 1]) {
                if (fifo_data_xyz[i - 1] - fifo_data_xyz[i + 1] > th1 - 10) {
                    if (i - k < 5) {
                        click_num = 1;
                        break;
                    }
                }
            }
        }
    }

    if (click_num == 1) {
        click_result = 1;
    } else {
        click_result = 0;
    }

    DBGLOG_KEY_SENSOR_INFO("click_result:%d!\n", click_result);

    return click_result;
}

/**
 * @brief This function is get click status.
 *
 * @return int8_t 0 click is invalid 1 click is valid.
 */
static int8_t sc7a20_click_alog(void)
{
    uint8_t click_status = 0;
    uint8_t chip_click_status = 0;

    if (sl_click_timer_en == 0) {
        sl_pp_num = 0;
    }
    /*
#if (GSENSOR_SC7A20_INT_PIN == 1)
    sc7a20_i2c_write(SC7A20_ADDR, SC7A20_CTRL_REG3, 0x00);//disable int1
#else
    sc7a20_i2c_write(SC7A20_ADDR, SC7A20_CTRL_REG6, 0x00);//disable int2
#endif
*/
    click_status = sc7a20_click_read(30, 40);

    if (click_status == 1) {
        if (sl_click_timer_en == 0) {
            //set click timer flag
            sl_click_timer_en = 1;
            //clear click timer cnt value
            click_timer_cnt = 0;
            click_timer_total_cnt = 0;
            click_click_final_cnt = 0;
        }
        sl_click_status = 1;
    }
#if (GSENSOR_SC7A20_INT_PIN == 1)
    sc7a20_i2c_write(SC7A20_ADDR, SC7A20_CTRL_REG3, 0x80);   // click TO int1
#else
    sc7a20_i2c_write(SC7A20_ADDR, SC7A20_CTRL_REG6, 0x80);   // click TO int2
#endif
    sc7a20_i2c_read(SC7A20_ADDR, SC7A20_CLICK_SRC, 1, &chip_click_status);

    return click_status;
}

static void send_key_callback(key_pressed_type_t type)
{
    key_pressed_info_t info;

    if (!key_callback) {
        return;
    }

    info.num = 1;
    info.id[0] = key_id;
    info.type[0] = type;

    key_callback(&info);
}

/**
 * @brief This function is the timer process function.
 *
 * @param timer_id unusing
 * @param arg unusing
 * @return null.
 */
static void g_sensor_timer_handle(timer_id_t timer_id, void *arg)
{
    UNUSED(timer_id);
    UNUSED(arg);

    if (sc7a20_click_status()) {
        //DBGLOG_KEY_SENSOR_INFO("click_done, cnt=%d, click_enabled=%d, inbox=%d\n",
        //                 click_click_final_cnt, wq_cus_is_click_enabled(), cmc_is_inbox());
#if GSENSOR_ENABLE_TRIPLE_CLICK
        if (click_click_final_cnt >= 3) {
            send_key_callback(BTN_TYPE_TRIPLE);
        } else if (click_click_final_cnt == 2) {
            send_key_callback(BTN_TYPE_DOUBLE);
        }
#else
        //double click or more, we regonize as double click

        if (click_click_final_cnt >= 2) {
            send_key_callback(BTN_TYPE_DOUBLE);
            DBGLOG_KEY_SENSOR_INFO("double click!\n");
        }
#endif
    }

    if (!sl_click_timer_en) {
        //iot_low_power_request(low_power_g_sensor_id);
        DBGLOG_KEY_SENSOR_INFO("close timer!");
        os_stop_timer(g_sensor_timer_id);
    }
}

/**
 * @brief This function is the share task of callback.
 *
 * @param msg_id unusing
 * @param msg_value unusing
 */
static void sensor_click_msg_handler(uint8_t msg_id, uint16_t msg_value)
{
    UNUSED(msg_id);
    UNUSED(msg_value);

    DBGLOG_KEY_SENSOR_INFO("sensor_click_msg_handler!");

#if (GSENSOR_SC7A20_INT_PIN == 1)
    sc7a20_i2c_write(SC7A20_ADDR, SC7A20_CTRL_REG3, 0x00);   //disable int1
#else
    sc7a20_i2c_write(SC7A20_ADDR, SC7A20_CTRL_REG6, 0x00);       //disable int2
#endif

    if (sc7a20_click_alog()) {
        if (sl_click_timer_en && (!os_is_timer_active(g_sensor_timer_id))) {
            os_start_timer(g_sensor_timer_id, CLICK_CHECK_INTERVAL);
        }
    } else {
        if (!sl_click_timer_en) {
            //iot_low_power_request(low_power_g_sensor_id);
        }
        DBGLOG_KEY_SENSOR_INFO("Gsensor invalid click!");
    }
}

/**
 * @brief This function is post event to share task.
 */
static void sc7a20_handle_gpio_interrupt(void) IRAM_TEXT(sc7a20_handle_gpio_interrupt);
static void sc7a20_handle_gpio_interrupt(void)
{
    vendor_send_msg_from_isr(VENDOR_MSG_TYPE_GSENSOR, MSGID_GPIO_INT, 0);
}

/**
 * @brief This function is to wakeup from deep sleep.
 */
static void wakeup_isr(void)
{
    //iot_clear_low_power_request(low_power_g_sensor_id);
    iot_gpio_int_disable(int_gpio);
    iot_gpio_open_as_interrupt(int_gpio, IOT_GPIO_INT_EDGE_FALLING, sc7a20_handle_gpio_interrupt);
    iot_gpio_int_enable(int_gpio);
}

/**
 * @brief This function is to init gpio interrupt.
 *
 * @return int8_t 0 for success else error.
 */
static int8_t gpio_intterrupt_init(void)
{
    int8_t ret = 0;
    uint8_t pull_mode;

    uint8_t gpio = iot_resource_lookup_gpio(SC7A20_INT_GPIO);

    if (gpio) {
        int_gpio = gpio;

        pull_mode = iot_resource_lookup_pull_mode(gpio);
        DBGLOG_KEY_SENSOR_INFO("sc7a20 gpio from pib:%d pull:%d\n", gpio, pull_mode);
    } else {

        int_gpio = SC7A20_INT_PIN_DEFAULT;
        pull_mode = IOT_GPIO_PULL_NONE;
        DBGLOG_KEY_SENSOR_ERROR("sc7a20 gpio not configured, disabled\n");

        return -1;
    }

    iot_gpio_close(int_gpio);
    ret = iot_gpio_open_as_interrupt(int_gpio, IOT_GPIO_INT_EDGE_FALLING,
                                     sc7a20_handle_gpio_interrupt);
    assert(ret == RET_OK);
    iot_gpio_set_pull_mode(int_gpio, IOT_GPIO_PULL_DOWN);
    iot_gpio_int_enable(int_gpio);
    //ret |= iot_set_low_power_wakeup_source_with_handler(sys_power_state_light_sleep,
    //                                 wakeup_source_io,
    //                                 int_gpio,
    //                                 GPIO_INT_LEVEL_HIGH, (void *)wakeup_isr);
    UNUSED(wakeup_isr);
    if (ret != 0) {
        DBGLOG_KEY_SENSOR_ERROR("gpio_set_interrupt failed!\n");
    } else {
        gpio_intterupt_set = 1;
        DBGLOG_KEY_SENSOR_INFO("gpio_set_interrupt successfully!\n");
    }

    return ret;
}

/**
 * @brief This function is check sc7a20 id.
 *
 * @return int8_t 0 for success else error.
 */
static int8_t sc7a20_check(void)
{
    uint8_t reg_value = 0;
    sc7a20_i2c_read(SC7A20_ADDR, SC7A20_WHO_AM_I, 1, &reg_value);
    DBGLOG_KEY_SENSOR_INFO("sc7a20 check value = 0x%02x\n", reg_value);
    if (reg_value == 0x11)
        return 0x01;
    else
        return 0x00;
}

/**
 * @brief This function is to config sc7a20.
 *
 * @return int8_t 0 for success else error.
 */
static int8_t sc7a20_config(void)
{
    uint8_t check_flag = 0;

    check_flag = sc7a20_check();
    if (check_flag == 1) {
        sc7a20_i2c_write(SC7A20_ADDR, SC7A20_CTRL_REG1, 0x7F);   //ODR 0x7f  400Hz
        sc7a20_i2c_write(SC7A20_ADDR, SC7A20_CTRL_REG2, 0x0C);   //HP 0x0C
#if (GSENSOR_SC7A20_INT_PIN == 1)
        sc7a20_i2c_write(SC7A20_ADDR, SC7A20_CTRL_REG3, 0x80);   // click TO int1
#else
        sc7a20_i2c_write(SC7A20_ADDR, SC7A20_CTRL_REG6, 0x80);   // click TO int2
#endif
        sc7a20_i2c_write(SC7A20_ADDR, SC7A20_CTRL_REG4,
                         0xD0);   // high byte in lower addr DLPF Closed
        sc7a20_i2c_write(SC7A20_ADDR, SC7A20_CTRL_REG5, 0x40);       //0x40 fifo mode open
        sc7a20_i2c_write(SC7A20_ADDR, SC7A20_FIFO_CTRL_REG, 0x80);   //steram mode
        sc7a20_i2c_write(SC7A20_ADDR, SC7A20_CLICK_CFG, 0x15);       //xyz signal click
#if GSENSOR_ENABLE_TRIPLE_CLICK
        sc7a20_i2c_write(SC7A20_ADDR, SC7A20_CLICK_THS, 0x38);   //62.6mg(4g)*
#else
        sc7a20_i2c_write(SC7A20_ADDR, SC7A20_CLICK_THS, 0x28);   //62.6mg(4g)*
#endif
        sc7a20_i2c_write(SC7A20_ADDR, SC7A20_TIME_LIMIT, 0x05);     // peak num must <5
        sc7a20_i2c_write(SC7A20_ADDR, SC7A20_TIME_LATENCY, 0x10);   //click int time
        //sc7a20_i2c_write(SC7A20_ADDR, SC7A20_CTRL_REG6, 0x02);//default level is high
        return 1;
    } else {
        return 0;
    }
}

/**
 * @brief This function is to power down sc7a20.
 *
 * @return null.
 */
static void sl_sc7a20_power_down(void)
{
#if (GSENSOR_SC7A20_INT_PIN == 1)
    sc7a20_i2c_write(SC7A20_ADDR, SC7A20_CTRL_REG3, 0x00);   //disable int1
#else
    sc7a20_i2c_write(SC7A20_ADDR, SC7A20_CTRL_REG6, 0x00);       //disable int2
#endif

    sc7a20_i2c_write(SC7A20_ADDR, SC7A20_CTRL_REG1, 0x00);   //power down
}

/**
 * @brief This function is to init i2c of sc7a20.
 *
 * @return null.
 */
static void sc7a20_i2c_init(void)
{
    unsigned char i2c_flag = 0;
    DBGLOG_KEY_SENSOR_INFO("sc7a20 init start!\n");

    IOT_I2C_PORT port = IOT_I2C_PORT_0;

    iot_i2c_config_t cfg;
    cfg.i2c_busrt_mode = 1;
    cfg.baudrate = 200000;
    cfg.wait_nack_max_time = 100;

    iot_i2c_init(port, &cfg);

    iot_i2c_gpio_cfg_t gpio_cfg;
    gpio_cfg.scl = GSENSOR_SC7A20_INT_SCL;
    gpio_cfg.sda = GSENSOR_SC7A20_INT_SDA;

    iot_i2c_open(port, &gpio_cfg);

    i2c_flag = sc7a20_config();

    if (i2c_flag == 1) {
        DBGLOG_KEY_SENSOR_INFO("sc7a20 init Sucesses\n");
    }
}

/**
 * @brief This function is to deinit sc7a20.
 *
 * @return null.
 */
void sc7a20_deinit(bool_t wakeup_enable)
{
    if (gpio_intterupt_set == 0) {
        return;
    }

    sl_sc7a20_power_down();

    if (!wakeup_enable) {
        iot_gpio_int_disable(int_gpio);
        iot_gpio_close(int_gpio);
        iot_i2c_close(IOT_I2C_PORT_0);
    }

    os_stop_timer(g_sensor_timer_id);
    os_delete_timer(g_sensor_timer_id);

    g_sensor_timer_id = 0;
    gpio_intterupt_set = 0;
}

/**
 * @brief This function is to init sc7a20.
 *
 * @return null
 */
void sc7a20_init(key_callback_t callback)
{

    key_callback = callback;

    if (gpio_intterupt_set == 0) {
        vendor_register_msg_handler(VENDOR_MSG_TYPE_GSENSOR, sensor_click_msg_handler);
        if (gpio_intterrupt_init()) {
            DBGLOG_KEY_SENSOR_INFO("gpio interrupt init fail!\n");
            return;
        }

        g_sensor_timer_id = os_create_timer(MY_MID, 1, g_sensor_timer_handle, NULL);
        if (g_sensor_timer_id) {
            DBGLOG_KEY_SENSOR_INFO("gsensor create timer succeeful!\n");
        }

        sc7a20_i2c_init();
    }
}

void sc7a20_set_param(const key_id_cfg_t *id_cfg, const key_time_cfg_t *time_cfg)
{
    UNUSED(time_cfg);

    for (int i = 0; i < id_cfg->num; i++) {
        key_id = id_cfg->key_id[i];
        break;
    }
}
#endif   //KEY_DRIVER_SELECTION == KEY_DRIVER_SC7A20
