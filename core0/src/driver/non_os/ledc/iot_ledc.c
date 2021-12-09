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

#include "iot_gpio.h"
#include "pin.h"
#include "gpio_mtx.h"

#include "ledc.h"
#include "iot_ledc.h"
#include "driver_dbglog.h"
#include "iot_timer.h"

#define LEDC_TMR_REF_CLK_FREQ   32768
#define LED_MIN_PWM_CYCLE_MS    18   //18ms 55Hz
#define LEDC_BREATH_LIGHT_SCALE 1

#define LEDC_DEFUALT_TMR_CLK_DIV 0
#define LEDC_NORMAL_SHRESHOLD    65535

enum {
    LEDC_NOT_OPENED = 0,
    LEDC_OPENED,
};

typedef struct {
    uint8_t pmm_ledc_status[PMM_LEDC_MAX_NUM];
    uint8_t dtop_ledc_status[DTOP_LEDC_MAX_NUM];
} iot_ledc_drivr_status_t;

static iot_ledc_drivr_status_t g_ledc_status = {0};

int32_t iot_ledc_pin_sel(IOT_LED_LEDC_MODULE module, uint8_t ledc_id, uint16_t pin)
{
    //pmm ledc just ledc0 and ledc1 can bind to gpio
    if (module == IOT_LED_LEDC_MODULE_PMM) {
        if (ledc_id == 0) {
            pin_set_func(pin, 4);
        } else if (ledc_id == 1) {
            pin_set_func(pin, 5);
        } else {
            DBGLOG_DRIVER_ERROR("[LED]ledc assgin fail\n");
            return -1;
        }
    } else {
        pin_set_func(pin, 0);
        gpio_mtx_set_out_signal(pin, (GPIO_MTX_SIGNAL_OUT)(GPIO_MTX_LED0_OUT + ledc_id));
    }

    return RET_OK;
}

void iot_ledc_pin_release(IOT_LED_LEDC_MODULE module, uint16_t pin)
{
    if (module == IOT_LED_LEDC_MODULE_PMM) {
        //set pin func as none and pad input
        pin_set_func(pin, 0x2);
    } else {
        gpio_mtx_set_out_signal(pin, GPIO_MTX_OUT_DEFAULT);
    }
}

void iot_ledc_init(IOT_LED_LEDC_MODULE module)
{
    ledc_init((LEDC_CTRL)module);
}

int8_t iot_ledc_assign(IOT_LED_LEDC_MODULE module)
{
    int8_t i = 0;
    if (module == IOT_LED_LEDC_MODULE_PMM) {
        for (i = 0; i < PMM_LEDC_MAX_NUM; i++) {
            if (g_ledc_status.pmm_ledc_status[i] == LEDC_NOT_OPENED) {
                iot_ledc_open(IOT_LED_LEDC_MODULE_PMM, (uint8_t)i);
                return i;
            }
        }
    } else {
        for (i = 0; i < DTOP_LEDC_MAX_NUM; i++) {
            if (g_ledc_status.dtop_ledc_status[i] == LEDC_NOT_OPENED) {
                iot_ledc_open(IOT_LED_LEDC_MODULE_DTOP, (uint8_t)i);
                return i;
            }
        }
    }

    return -1;
}

void iot_ledc_deinit(void)
{
    uint8_t i;

    for (i = 0; i < PMM_LEDC_MAX_NUM; i++) {
        iot_ledc_off(IOT_LED_LEDC_MODULE_PMM, i);
    }

    for (i = 0; i < DTOP_LEDC_MAX_NUM; i++) {
        iot_ledc_off(IOT_LED_LEDC_MODULE_DTOP, i);
    }

    ledc_deinit((LEDC_CTRL)IOT_LED_LEDC_MODULE_PMM);
    ledc_deinit((LEDC_CTRL)IOT_LED_LEDC_MODULE_DTOP);
}

uint8_t iot_ledc_open(IOT_LED_LEDC_MODULE module, uint8_t id)
{
    //ledc id = id
    uint8_t tmr_id = id;
    if (module == IOT_LED_LEDC_MODULE_PMM) {
        ledc_tmr_sel((LEDC_CTRL)IOT_LED_LEDC_MODULE_PMM, id, tmr_id);
        ledc_tmr_cfg((LEDC_CTRL)IOT_LED_LEDC_MODULE_PMM, tmr_id, LEDC_DEFUALT_TMR_CLK_DIV);
        g_ledc_status.pmm_ledc_status[id] = LEDC_OPENED;
    } else {
        ledc_tmr_sel((LEDC_CTRL)IOT_LED_LEDC_MODULE_DTOP, id, tmr_id);
        ledc_tmr_cfg((LEDC_CTRL)IOT_LED_LEDC_MODULE_DTOP, tmr_id, LEDC_DEFUALT_TMR_CLK_DIV);
        g_ledc_status.dtop_ledc_status[id] = LEDC_OPENED;
    }

    return RET_OK;
}

uint8_t iot_ledc_on(IOT_LED_LEDC_MODULE module, uint8_t id)
{
    if (module == IOT_LED_LEDC_MODULE_PMM) {
        if (g_ledc_status.pmm_ledc_status[id] == LEDC_NOT_OPENED) {
            DBGLOG_DRIVER_ERROR("[LED]ledc id:%d is not open\n", id);
            return RET_FAIL;
        }
    } else {
        if (g_ledc_status.dtop_ledc_status[id] == LEDC_NOT_OPENED) {
            DBGLOG_DRIVER_ERROR("[LED]ledc id:%d is not open\n", id);
            return RET_FAIL;
        }
    }

    ledc_start((LEDC_CTRL)module, id);
    return RET_OK;
}

/*
* 32bit data sqrt,result 16bit
* data = (result)**2
* data = bit31 * 2**30 + bit30 * 2**30 + ...+bit0
* result = bit15 * 2**14 + bit14 * 2**13 + ...+bit0
* get the msb of data is bitN,and msb bitx of result is depend on the N.
* x = N/2 or (N+1)/2,all the bit of result can be calculated like this.
*/
static uint16_t sqrt_16(uint32_t data)
{
    uint16_t result, i;
    uint32_t tmp, ttp;
    if (data == 0)
        return 0;

    result = 0;

    tmp = (data >> 30);   //get the msb of result
    data <<= 2;
    if (tmp > 1) {
        result++;        //msb of result is 1
        tmp -= result;   //get the (data - 2**msb)
    }

    for (i = 15; i > 0; i--) {
        result <<= 1;   //next bit

        tmp <<= 2;
        tmp += (data >> 30);

        ttp = result;
        ttp = (ttp << 1) + 1;

        data <<= 2;
        if (tmp >= ttp) {
            tmp -= ttp;
            result++;   //bit = 1
        }
    }

    return result;
}

/*
* tmr = 32kHz
* pwm cycle=(threshold/scale)/32k
* dim = (threshold/scale)*pwm cycle
*/
uint32_t iot_ledc_breath_config(IOT_LED_LEDC_MODULE module, uint8_t ledc_id, uint32_t dim,
                                bool_t high_on)
{
    uint16_t tmp;
    uint16_t threshold;
    uint8_t scale;
    uint32_t pwm_cycle;

    LEDC_CTRL led_module = (LEDC_CTRL)module;

    ledc_tmr_close(led_module, ledc_id);
    ledc_stop(led_module, ledc_id);
    iot_ledc_open(module, ledc_id);
    //clk div 1,means 32k tmr_id = ledc_id
    ledc_tmr_cfg(led_module, ledc_id, 0);

    tmp = sqrt_16((dim * LEDC_TMR_REF_CLK_FREQ) / 1000);

    pwm_cycle = (tmp * 1000) / LEDC_TMR_REF_CLK_FREQ;
    if (pwm_cycle >= LED_MIN_PWM_CYCLE_MS) {
        DBGLOG_DRIVER_ERROR("[LED]ledc breath time is out of range\n");
        return RET_FAIL;
    }
    scale = LEDC_BREATH_LIGHT_SCALE;
    threshold = (uint16_t)(scale * tmp);

    ledc_set_duty_mode(led_module, ledc_id, LEDC_DUTY_MODE_CHANGE_LOOP2);
    ledc_set_duty_scale(led_module, ledc_id, scale);
    ledc_set_duty_thrs(led_module, ledc_id, threshold);
    ledc_set_duty_num(led_module, ledc_id, threshold / LEDC_BREATH_LIGHT_SCALE);

    if (high_on) {
        ledc_set_outinv(led_module, ledc_id, 1);
        ledc_set_idle_output(led_module, ledc_id, LEDC_OUTPUT_LVL_L);
    } else {
        ledc_set_outinv(led_module, ledc_id, 0);
        ledc_set_idle_output(led_module, ledc_id, LEDC_OUTPUT_LVL_H);
    }

    return RET_OK;
}

uint32_t iot_ledc_blink_config(IOT_LED_LEDC_MODULE module, uint8_t ledc_id, uint32_t on_duty,
                               uint32_t off_duty, bool_t high_on, IOT_LED_ACTION_MODE mode)
{
    uint16_t threshold = 4192;
    LEDC_CTRL led_module = (LEDC_CTRL)module;
    uint16_t clk_div = (uint16_t)(((on_duty + off_duty) * (32768 / threshold)) / 1000);

    ledc_tmr_close(led_module, ledc_id);
    ledc_stop(led_module, ledc_id);
    iot_ledc_open(module, ledc_id);
    ledc_tmr_cfg(led_module, ledc_id, clk_div);
    ledc_set_duty_mode(led_module, ledc_id, LEDC_DUTY_MODE_NORMAL);
    ledc_set_duty_thrs(led_module, ledc_id, threshold);
    uint16_t l2hv = (uint16_t)((off_duty * threshold) / ((off_duty + on_duty) * 2));
    if (high_on) {
        ledc_set_hl_val(led_module, ledc_id, threshold - l2hv, l2hv);
        ledc_set_idle_output(led_module, ledc_id, LEDC_OUTPUT_LVL_L);
    } else {
        ledc_set_hl_val(led_module, ledc_id, l2hv, threshold - l2hv);
        ledc_set_idle_output(led_module, ledc_id, LEDC_OUTPUT_LVL_H);
    }

    if (mode == IOT_LED_ACTION_MODE_OFF2ON) {
        ledc_set_outinv(led_module, ledc_id, 0);
    } else {
        ledc_set_outinv(led_module, ledc_id, 1);
    }

    return RET_OK;
}

uint32_t iot_ledc_normal_light_config(IOT_LED_LEDC_MODULE module, uint8_t ledc_id, bool_t high_on)
{
    LEDC_CTRL led_module = (LEDC_CTRL)module;

    ledc_tmr_close(led_module, ledc_id);
    ledc_stop(led_module, ledc_id);
    iot_ledc_open(module, ledc_id);
    ledc_tmr_cfg(led_module, ledc_id, LEDC_DEFUALT_TMR_CLK_DIV);
    ledc_set_duty_mode(led_module, ledc_id, LEDC_DUTY_MODE_NORMAL);
    ledc_set_duty_thrs(led_module, ledc_id, LEDC_NORMAL_SHRESHOLD);

    ledc_set_hl_val(led_module, ledc_id, LEDC_NORMAL_SHRESHOLD, 0);

    if (high_on) {
        ledc_set_outinv(led_module, ledc_id, 0);
        ledc_set_idle_output(led_module, ledc_id, LEDC_OUTPUT_LVL_L);
    } else {
        ledc_set_outinv(led_module, ledc_id, 1);
        ledc_set_idle_output(led_module, ledc_id, LEDC_OUTPUT_LVL_H);
    }
    return RET_OK;
}

uint8_t iot_ledc_off(IOT_LED_LEDC_MODULE module, uint8_t id)
{
    ledc_stop((LEDC_CTRL)module, id);
    return RET_OK;
}

uint8_t iot_ledc_close(IOT_LED_LEDC_MODULE module, uint8_t id)
{
    if (module == IOT_LED_LEDC_MODULE_PMM) {
        g_ledc_status.pmm_ledc_status[id] = LEDC_NOT_OPENED;
    } else {
        g_ledc_status.dtop_ledc_status[id] = LEDC_NOT_OPENED;
    }

    ledc_tmr_close((LEDC_CTRL)module, id);
    ledc_set_idle_output((LEDC_CTRL)module, id, LEDC_OUTPUT_LVL_L);

    return RET_OK;
}

void iot_ledc_breath_on2off(IOT_LED_LEDC_MODULE module, uint8_t id, bool_t high_on)
{
    LEDC_CTRL led_module = (LEDC_CTRL)module;

    ledc_set_outinv(led_module, id, (high_on ? 0 : 1));
    ledc_set_duty_load(led_module, id, 0, true);
    iot_timer_delay_us(50);
    ledc_set_duty_load(led_module, id, 0, false);
}

void iot_ledc_breath_off2on(IOT_LED_LEDC_MODULE module, uint8_t id, bool_t high_on)
{
    LEDC_CTRL led_module = (LEDC_CTRL)module;

    ledc_set_duty_load(led_module, id, 0, true);
    iot_timer_delay_us(50);

    ledc_set_duty_load(led_module, id, 0, false);
    ledc_set_outinv(led_module, id, (high_on ? 1 : 0));
}
