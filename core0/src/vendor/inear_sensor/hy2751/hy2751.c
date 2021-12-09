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
#include "inear_sensor.h"

#if INEAR_DRIVER_SELECTION == INEAR_DRIVER_LIGHT_HY2751
#include "os_timer.h"
#include "lib_dbglog.h"
#include "hy2751.h"
#include "iot_adc.h"
#include "iot_gpio.h"
#include "os_utils.h"
#include "string.h"
#include "iot_adc.h"
#include "iot_uart.h"
#include "vendor_msg.h"
#include "iot_resource.h"

#ifndef HY2751_LIGHT_GPIO
#define HY2751_LIGHT_GPIO GPIO_CUSTOMIZE_1
#endif

#ifndef HY2751_ADC_GPIO
#define HY2751_ADC_GPIO GPIO_CUSTOMIZE_2
#endif

#ifndef HY2751_INEAR_DETECTION_INTERVAL
#define HY2751_INEAR_DETECTION_INTERVAL (500)
#endif

#ifndef HY2751_ADC_SAMPLE_COUNT
#define HY2751_ADC_SAMPLE_COUNT 1   //1 OR 4
#endif

//15K/(15K+33K)  Resistance
#ifndef HY2751_IN_EAR_LEVEL
#define HY2751_IN_EAR_LEVEL 850
#endif

#ifndef HY2751_IN_EAR_ADC_DIFF_LEVEL
#define HY2751_IN_EAR_ADC_DIFF_LEVEL 15
#endif

#ifndef HY2751_ADC_SUN_CHECK_LEVEL
#define HY2751_ADC_SUN_CHECK_LEVEL 850
#endif

static timer_id_t inear_hy2751_timer_id = 0;
static inear_callback_t inear_callback = NULL;
static uint8_t cur_inear_status;

typedef struct light_adc_data {
    uint8_t pos;
    int32_t lighton_adc_val[4];
    int32_t lightoff_adc_val[4];
    float lighton_voltage;
    float lightoff_voltage;
} light_adc_data_t;

static light_adc_data_t light_adc_data = {0};
static uint8_t gpio_light;
static uint8_t gpio_adc;

static void light_on(void)
{
    iot_gpio_write(gpio_light, 0);
}

static void light_off(void)
{
    iot_gpio_write(gpio_light, 1);
}

static int32_t voltage_filter(int32_t *p)
{
    int32_t voltage_max[3], voltage_min[3];

    voltage_max[0] = p[0] > p[1] ? p[0] : p[1];
    voltage_max[1] = p[2] > p[3] ? p[2] : p[3];
    voltage_max[2] = voltage_max[0] > voltage_max[1] ? voltage_max[0] : voltage_max[1];

    voltage_min[0] = p[0] < p[1] ? p[0] : p[1];
    voltage_min[1] = p[2] < p[3] ? p[2] : p[3];
    voltage_min[2] = voltage_min[0] < voltage_min[1] ? voltage_min[0] : voltage_min[1];

    voltage_min[0] = *p + *(p + 1) + *(p + 2) + *(p + 3) - voltage_max[2] - voltage_min[2];
    voltage_min[0] = voltage_min[0] >> 1;
    return voltage_min[0];
}

static void inear_detection_timer_handle(timer_id_t timer_id, void *arg)
{
    UNUSED(timer_id);
    UNUSED(arg);

    vendor_send_msg(VENDOR_MSG_TYPE_INEAR, 0, 0);
}

static void inear_msg_handler(uint8_t msg_id, uint16_t msg_value)
{
    UNUSED(msg_id);
    UNUSED(msg_value);

    uint32_t is_inear = 0;
    light_adc_data_t *p = &light_adc_data;

    light_off();

    p->lightoff_adc_val[p->pos] = iot_adc_poll_data(gpio_adc, 0, 1);

    light_on();
    os_delay(5);

    p->lighton_adc_val[p->pos] = iot_adc_poll_data(gpio_adc, 0, 1);

    light_off();

    if (p->pos < HY2751_ADC_SAMPLE_COUNT) {
        p->pos++;
    }

    if (HY2751_ADC_SAMPLE_COUNT == p->pos) {

        if (4 == p->pos) {
            p->lightoff_voltage = iot_adc_2_mv(0, voltage_filter(&p->lightoff_adc_val[0]));
            p->lighton_voltage = iot_adc_2_mv(0, voltage_filter(&p->lighton_adc_val[0]));
        } else if (1 == p->pos) {
            p->lightoff_voltage = iot_adc_2_mv(0, p->lightoff_adc_val[0]);
            p->lighton_voltage = iot_adc_2_mv(0, p->lighton_adc_val[0]);
        }

        if (p->lightoff_voltage > HY2751_ADC_SUN_CHECK_LEVEL) {

            if ((p->lighton_voltage - p->lightoff_voltage) > HY2751_IN_EAR_ADC_DIFF_LEVEL) {
                is_inear = 1;
            } else {
                is_inear = 0;
            }
        } else {
            if (p->lighton_voltage > HY2751_IN_EAR_LEVEL) {
                is_inear = 1;
            } else {
                is_inear = 0;
            }
        }

        if (is_inear == cur_inear_status) {
            memset(&light_adc_data, 0, sizeof(light_adc_data));
            return;
        }

        DBGLOG_INEAR_SENSOR_INFO("lightoff_vol = %d, lighton_vol = %d, is_inear:%d\n",
                                 (int16_t)p->lightoff_voltage, (int16_t)p->lighton_voltage,
                                 is_inear);

        inear_callback((bool_t)is_inear);
        memset(&light_adc_data, 0, sizeof(light_adc_data));
        cur_inear_status = is_inear;
    }
}

void inear_hy2751_init(inear_callback_t callback)
{
    inear_callback = callback;

    memset(&light_adc_data, 0, sizeof(light_adc_data));

    gpio_light = iot_resource_lookup_gpio(HY2751_LIGHT_GPIO);
    DBGLOG_INEAR_SENSOR_INFO("hy2751 gpio_light num = %d\n", gpio_light);
    if (gpio_light != 0xff) {
        iot_gpio_open(gpio_light, IOT_GPIO_DIRECTION_OUTPUT);
        uint8_t pull_mode = iot_resource_lookup_pull_mode(gpio_light);
        DBGLOG_INEAR_SENSOR_INFO("hy2751 pull_mode = %d\n", pull_mode);
        iot_gpio_set_pull_mode(gpio_light, pull_mode);
    } else {
        DBGLOG_INEAR_SENSOR_ERROR("hy2751 light io get fail\n");
        return;
    }

    gpio_adc = iot_resource_lookup_gpio(HY2751_ADC_GPIO);
    if ((gpio_adc >= IOT_GPIO_71) && (gpio_adc <= IOT_GPIO_74)) {
        gpio_adc = gpio_adc - IOT_GPIO_71 + (uint8_t)IOT_ADC_EXT_SIG_CH0;
        DBGLOG_INEAR_SENSOR_INFO("hy2751 gpio_adc channel number = %d\n", gpio_adc);
    } else {
        DBGLOG_INEAR_SENSOR_ERROR("hy2751 adc io get fail\n");
        return;
    }

    light_off();

    vendor_register_msg_handler(VENDOR_MSG_TYPE_INEAR, inear_msg_handler);

    inear_hy2751_timer_id = os_create_timer(LIB_KEYMGMT_MID, 1, inear_detection_timer_handle, NULL);
    if (!inear_hy2751_timer_id) {
        DBGLOG_INEAR_SENSOR_ERROR("inear_hy2751_timer_id create failed!\n");
    }

    os_start_timer(inear_hy2751_timer_id, HY2751_INEAR_DETECTION_INTERVAL);
}

void inear_hy2751_deinit(void)
{
    if (!inear_hy2751_timer_id) {
        return;
    }

    os_stop_timer(inear_hy2751_timer_id);

    light_off();

    os_delete_timer(inear_hy2751_timer_id);
    inear_hy2751_timer_id = 0;
}

#endif /*INEAR_DRIVER_SELECTION == INEAR_DRIVER_LIGHT_HY2751 */
