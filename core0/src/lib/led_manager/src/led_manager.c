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
#include "os_timer.h"
#include "lib_dbglog.h"

#include "iot_gpio.h"
#include "iot_ledc.h"
#include "iot_resource.h"
#include "led_manager.h"

#if defined(LOW_POWER_ENABLE) && defined(BUILD_CORE_CORE0)
#include "power_mgnt.h"
#endif

enum {
    LED_STATE_ON,
    LED_STATE_DIM_OFF,
    LED_STATE_OFF,
    LED_STATE_UNKNOWN = 0xff,
};

enum {   //
    LED_GPIO_RELEASED,
    LED_GPIO_CLAIMED
};

typedef struct {
    uint8_t gpio_num;
    uint8_t is_claimed;
    uint8_t pin;
    IOT_GPIO_PULL_MODE pull_mode;
} led_gpio_t;

typedef struct {
    bool_t high_on;
    bool_t valid;
    uint8_t id;
    uint8_t target_cnt;
    uint8_t ledc_id;
    uint8_t blink_mode;
    uint8_t state;
    uint8_t curr_cnt;
    IOT_LED_LEDC_MODULE module;
    led_gpio_t gpio;
    uint32_t dim_peroid;
    uint32_t on_duty;
    uint32_t off_duty;
    uint32_t target_loop_cnt;
    uint32_t curr_loop_cnt;
    uint32_t loop_interval;
    uint32_t timer;
    led_end_callback callback;
} led_ctxt;

static led_ctxt g_led_ctxt[LED_MAX_NUM];

#if defined(LOW_POWER_ENABLE) && defined(BUILD_CORE_CORE0)
uint8_t led_sleep_vote = 0xff;
static void led_set_sleep_vote(uint8_t led_id)
{
    if (led_id >= LED_MAX_NUM) {
        assert(0);
        return; //lint !e527: codestyle
    }

    led_sleep_vote |= (uint8_t)BIT(led_id);

    if (led_sleep_vote == 0xff) {
        power_mgnt_set_sleep_vote(POWER_SLEEP_LED);
    }
}

static void led_clear_sleep_vote(uint8_t led_id)
{
    led_sleep_vote &= ~BIT(led_id);
    power_mgnt_clear_sleep_vote(POWER_SLEEP_LED);
}
#endif

static uint32_t led_action_with_config(uint8_t id, bool_t start_on)
{
    uint32_t duty = 0;

    if (g_led_ctxt[id].valid == false) {
        DBGLOG_LIB_ERROR("[LED]:%d action fail\n", id);
        return RET_FAIL;
    }

    if (g_led_ctxt[id].blink_mode == LED_MODE_BLINK) {
        if (start_on) {
#if defined(LOW_POWER_ENABLE) && defined(BUILD_CORE_CORE0)
            if (g_led_ctxt[id].module != IOT_LED_LEDC_MODULE_PMM) {
                led_clear_sleep_vote(id);
            }
            iot_ledc_init(g_led_ctxt[id].module);
            iot_ledc_pin_sel(g_led_ctxt[id].module, g_led_ctxt[id].ledc_id,
                             g_led_ctxt[id].gpio.pin);
            iot_gpio_set_pull_mode(g_led_ctxt[id].gpio.gpio_num, g_led_ctxt[id].gpio.pull_mode);
            iot_ledc_normal_light_config(g_led_ctxt[id].module, g_led_ctxt[id].ledc_id,
                                         g_led_ctxt[id].high_on);
#endif
            iot_ledc_on(g_led_ctxt[id].module, g_led_ctxt[id].ledc_id);
            duty = g_led_ctxt[id].on_duty;
            g_led_ctxt[id].state = LED_STATE_ON;
            os_start_timer(g_led_ctxt[id].timer, duty);
        } else {
            iot_ledc_off(g_led_ctxt[id].module, g_led_ctxt[id].ledc_id);
            duty = g_led_ctxt[id].off_duty;
            g_led_ctxt[id].state = LED_STATE_OFF;
            os_start_timer(g_led_ctxt[id].timer, duty);
#if defined(LOW_POWER_ENABLE) && defined(BUILD_CORE_CORE0)
            led_set_sleep_vote(id);
#endif
        }
    } else if (g_led_ctxt[id].blink_mode == LED_MODE_DIM) {
#if defined(LOW_POWER_ENABLE) && defined(BUILD_CORE_CORE0)
        if (g_led_ctxt[id].module != IOT_LED_LEDC_MODULE_PMM) {
            led_clear_sleep_vote(id);
        }
#endif
        iot_ledc_init(g_led_ctxt[id].module);
        iot_gpio_set_pull_mode(g_led_ctxt[id].gpio.gpio_num, g_led_ctxt[id].gpio.pull_mode);
        iot_ledc_pin_sel(g_led_ctxt[id].module, g_led_ctxt[id].ledc_id, g_led_ctxt[id].gpio.pin);
        iot_ledc_breath_config(g_led_ctxt[id].module, g_led_ctxt[id].ledc_id,
                               g_led_ctxt[id].dim_peroid, g_led_ctxt[id].high_on);
        iot_ledc_on(g_led_ctxt[id].module, g_led_ctxt[id].ledc_id);
        g_led_ctxt[id].state = LED_STATE_ON;
        duty = g_led_ctxt[id].dim_peroid + g_led_ctxt[id].on_duty;
        os_start_timer(g_led_ctxt[id].timer, duty);
    } else if (g_led_ctxt[id].blink_mode == LED_MODE_NORMAL_LIGHT) {
#if defined(LOW_POWER_ENABLE) && defined(BUILD_CORE_CORE0)
        if (g_led_ctxt[id].module != IOT_LED_LEDC_MODULE_PMM) {
            led_clear_sleep_vote(id);
        }
        iot_ledc_init(g_led_ctxt[id].module);
        iot_gpio_set_pull_mode(g_led_ctxt[id].gpio.gpio_num, g_led_ctxt[id].gpio.pull_mode);
        iot_ledc_pin_sel(g_led_ctxt[id].module, g_led_ctxt[id].ledc_id, g_led_ctxt[id].gpio.pin);
#endif
        iot_ledc_normal_light_config(g_led_ctxt[id].module, g_led_ctxt[id].ledc_id,
                                     g_led_ctxt[id].high_on);
        iot_ledc_on(g_led_ctxt[id].module, g_led_ctxt[id].ledc_id);
    } else {

        DBGLOG_LIB_ERROR("[LED] blink mode error\n");
        assert(0);
        return RET_FAIL; //lint !e527: codestyle
    }

    return RET_OK;
}

static void led_timer_func(uint32_t timer_id, void *arg)
{
    (void)timer_id;
    led_ctxt *ctxt = (led_ctxt *)arg;
    os_stop_timer(ctxt->timer);
    uint32_t duty = 10;

    if (ctxt->valid == false) {
        return;
    }

    if (ctxt->state == LED_STATE_ON) {
        ctxt->curr_cnt++;
        if (ctxt->target_cnt == ctxt->curr_cnt) {
            ctxt->curr_loop_cnt++;
        }

        if (ctxt->blink_mode == LED_MODE_BLINK) {
            iot_ledc_off(ctxt->module, ctxt->ledc_id);
            ctxt->state = LED_STATE_OFF;
            if (ctxt->curr_cnt == ctxt->target_cnt) {
                ctxt->curr_cnt = 0;
                duty = ctxt->off_duty + ctxt->loop_interval;
            } else {
                duty = ctxt->off_duty;
            }
            os_start_timer(ctxt->timer, duty);
#if defined(LOW_POWER_ENABLE) && defined(BUILD_CORE_CORE0)
            led_set_sleep_vote(ctxt->id);
#endif
        } else if (ctxt->blink_mode == LED_MODE_DIM) {
#if defined(LOW_POWER_ENABLE) && defined(BUILD_CORE_CORE0)
            if (ctxt->module != IOT_LED_LEDC_MODULE_PMM) {
                led_clear_sleep_vote(ctxt->id);
            }
#endif
            iot_ledc_breath_on2off(ctxt->module, ctxt->ledc_id, ctxt->high_on);

            ctxt->state = LED_STATE_DIM_OFF;
            os_start_timer(ctxt->timer, ctxt->dim_peroid);
        }

    } else if (ctxt->state == LED_STATE_DIM_OFF) {
        ctxt->state = LED_STATE_OFF;
        if (ctxt->curr_cnt == ctxt->target_cnt) {
            ctxt->curr_cnt = 0;
            duty = ctxt->off_duty + ctxt->loop_interval;
        } else {
            duty = ctxt->off_duty;
        }
        os_start_timer(ctxt->timer, duty);
#if defined(LOW_POWER_ENABLE) && defined(BUILD_CORE_CORE0)
        led_set_sleep_vote(ctxt->id);
#endif
    } else if (ctxt->state == LED_STATE_OFF) {
        if ((ctxt->curr_loop_cnt == ctxt->target_loop_cnt) && (ctxt->target_loop_cnt)) {
            led_off(ctxt->id);
            ctxt->curr_loop_cnt = 0;

            if (ctxt->callback) {
                ctxt->callback(ctxt->id);
            }
            return;
        } else {
#if defined(LOW_POWER_ENABLE) && defined(BUILD_CORE_CORE0)
            if (ctxt->module != IOT_LED_LEDC_MODULE_PMM) {
                led_clear_sleep_vote(ctxt->id);
            }
#endif
            iot_ledc_init(ctxt->module);
            iot_gpio_set_pull_mode(ctxt->gpio.gpio_num, ctxt->gpio.pull_mode);
            iot_ledc_pin_sel(ctxt->module, ctxt->ledc_id, ctxt->gpio.pin);

            ctxt->state = LED_STATE_ON;
            if (ctxt->blink_mode == LED_MODE_DIM) {
                iot_ledc_breath_config(ctxt->module, ctxt->ledc_id, ctxt->dim_peroid,
                                       ctxt->high_on);
                duty = ctxt->dim_peroid + ctxt->on_duty;
            } else if (ctxt->blink_mode == LED_MODE_BLINK) {
                iot_ledc_normal_light_config(ctxt->module, ctxt->ledc_id, ctxt->high_on);
                duty = ctxt->on_duty;
            }
            iot_ledc_on(ctxt->module, ctxt->ledc_id);
            os_start_timer(ctxt->timer, duty);
        }

    } else {
    }
}

uint32_t led_config(RESOURCE_GPIO_ID id, const led_param_t *led_param, led_end_callback cb)
{
    uint32_t ret = RET_OK;
    IOT_GPIO_PULL_MODE pull_mode;
    uint8_t gpio_iomap;

    led_off(id);

    //get led io config
    gpio_iomap = iot_resource_lookup_gpio(GPIO_LED_0 + id);

    if (gpio_iomap >= IOT_GPIO_NUM) {
        DBGLOG_LIB_ERROR("[LED] IO config gpio:%d fail!\n", gpio_iomap);
        return RET_FAIL;
    }
    pull_mode = iot_resource_lookup_pull_mode(gpio_iomap);

    //gpio_num should use pmm_aon but pin use pin num
    if (gpio_iomap == IOT_GPIO_63 || gpio_iomap == IOT_GPIO_64) {
        g_led_ctxt[id].gpio.gpio_num = IOT_AONGPIO_00 + (gpio_iomap - IOT_GPIO_63);
        g_led_ctxt[id].gpio.pin = gpio_iomap;
    } else if (gpio_iomap == IOT_AONGPIO_00 || gpio_iomap == IOT_AONGPIO_01) {
        g_led_ctxt[id].gpio.gpio_num = gpio_iomap;
        g_led_ctxt[id].gpio.pin = IOT_GPIO_63 + (gpio_iomap - IOT_AONGPIO_00);
    } else {
        g_led_ctxt[id].gpio.gpio_num = gpio_iomap;
        g_led_ctxt[id].gpio.pin = gpio_iomap;
    }

    if ((iot_gpio_open(g_led_ctxt[id].gpio.gpio_num, IOT_GPIO_DIRECTION_INPUT) != RET_OK)) {
        DBGLOG_LIB_ERROR("[LED] claim gpio:%d fail!\n", g_led_ctxt[id].gpio.gpio_num);
        return RET_FAIL;
    }
    g_led_ctxt[id].gpio.is_claimed = LED_GPIO_CLAIMED;

    if (pull_mode == IOT_GPIO_PULL_DOWN) {
        g_led_ctxt[id].high_on = true;
    } else if (pull_mode == IOT_GPIO_PULL_UP) {
        g_led_ctxt[id].high_on = false;
    } else {
        DBGLOG_LIB_ERROR("[LED] pull mode cfg error\n");
        assert(0);
        return RET_FAIL; //lint !e527: codestyle
    }

    //set pull mode to close led
    iot_gpio_set_pull_mode(g_led_ctxt[id].gpio.gpio_num, (IOT_GPIO_PULL_MODE)pull_mode);
    g_led_ctxt[id].gpio.pull_mode = pull_mode;

    if (g_led_ctxt[id].gpio.gpio_num == IOT_AONGPIO_00
        || g_led_ctxt[id].gpio.gpio_num == IOT_AONGPIO_01) {
        iot_ledc_init(IOT_LED_LEDC_MODULE_PMM);

        int8_t tmp_id = iot_ledc_assign(IOT_LED_LEDC_MODULE_PMM);
        if (tmp_id >= PMM_LEDC_MAX_NUM || tmp_id < 0) {
            DBGLOG_LIB_ERROR("[LED] assaign pmm ledc fail!\n");
            return RET_FAIL;
        }

        g_led_ctxt[id].ledc_id = (uint8_t)tmp_id;
        iot_ledc_pin_sel(IOT_LED_LEDC_MODULE_PMM, g_led_ctxt[id].ledc_id, g_led_ctxt[id].gpio.pin);
        g_led_ctxt[id].module = IOT_LED_LEDC_MODULE_PMM;
    } else {
        iot_ledc_init(IOT_LED_LEDC_MODULE_DTOP);

        int8_t tmp_id = iot_ledc_assign(IOT_LED_LEDC_MODULE_DTOP);
        if (tmp_id >= DTOP_LEDC_MAX_NUM || tmp_id < 0) {
            DBGLOG_LIB_ERROR("[LED] assaign dtop ledc fail!\n");
            return RET_FAIL;
        }

        g_led_ctxt[id].ledc_id = (uint8_t)tmp_id;
        iot_ledc_pin_sel(IOT_LED_LEDC_MODULE_DTOP, g_led_ctxt[id].ledc_id, g_led_ctxt[id].gpio.pin);
        g_led_ctxt[id].module = IOT_LED_LEDC_MODULE_DTOP;
    }

    if (led_param->mode == LED_MODE_NORMAL_LIGHT) {
        g_led_ctxt[id].valid = true;
        g_led_ctxt[id].blink_mode = led_param->mode;
        return ret;

    } else if (led_param->mode == LED_MODE_BLINK) {
        g_led_ctxt[id].dim_peroid = 0;
    } else if (led_param->mode == LED_MODE_DIM) {
        if (led_param->dim_duty < LEDC_DIM_MIN_MS) {
            g_led_ctxt[id].dim_peroid = LEDC_DIM_MIN_MS;
        } else {
            g_led_ctxt[id].dim_peroid = led_param->dim_duty;
        }
    } else {
        ret = RET_FAIL;
    }

    //on off duty min 10ms
    if (led_param->on_duty < LEDC_ON_OFF_MIN_MS) {
        g_led_ctxt[id].on_duty = LEDC_ON_OFF_MIN_MS;
    } else {
        g_led_ctxt[id].on_duty = led_param->on_duty;
    }

    //on off duty min 10ms
    if (led_param->off_duty < LEDC_ON_OFF_MIN_MS) {
        g_led_ctxt[id].off_duty = LEDC_ON_OFF_MIN_MS;
    } else {
        g_led_ctxt[id].off_duty = led_param->off_duty;
    }

    g_led_ctxt[id].target_cnt = (uint8_t)led_param->blink_cnt;
    g_led_ctxt[id].loop_interval = led_param->interval;
    g_led_ctxt[id].target_loop_cnt = led_param->loop;
    g_led_ctxt[id].callback = cb;

    if (ret != RET_OK) {
        g_led_ctxt[id].valid = false;
        DBGLOG_LIB_ERROR("[LED] breath config fial\n");
        return ret;
    }

    g_led_ctxt[id].valid = true;
    g_led_ctxt[id].blink_mode = led_param->mode;
    DBGLOG_LIB_INFO(
        "[LED]%d,ledc_id:%d,gpio:%d,mode:%d,high_on:%d,on_duty:%d,off_duty:%d,blink_cnt:%d,dim_duty:%d,interval:%d,loop:%d\n",
        id, g_led_ctxt[id].ledc_id, g_led_ctxt[id].gpio.gpio_num, led_param->mode,
        (uint8_t)(g_led_ctxt[id].high_on ? 1 : 0), led_param->on_duty, led_param->off_duty,
        led_param->blink_cnt, led_param->dim_duty, led_param->interval, led_param->loop);
    return ret;
}

uint32_t led_start_action(uint8_t id, bool_t on)
{
    if (g_led_ctxt[id].valid == false) {
        return RET_FAIL;
    }
    g_led_ctxt[id].curr_cnt = 0;
    g_led_ctxt[id].curr_loop_cnt = 0;

    led_action_with_config(id, on);
    return RET_OK;
}

uint32_t led_off(uint8_t id)
{
    if (g_led_ctxt[id].valid == false) {
        return RET_OK;
    }

    DBGLOG_LIB_INFO("[LED]%d off\n", id);
    os_stop_timer(g_led_ctxt[id].timer);
    iot_ledc_off(g_led_ctxt[id].module, g_led_ctxt[id].ledc_id);
    iot_ledc_pin_release(g_led_ctxt[id].module, g_led_ctxt[id].gpio.pin);
    iot_ledc_close(g_led_ctxt[id].module, g_led_ctxt[id].ledc_id);

    if (g_led_ctxt[id].gpio.is_claimed == LED_GPIO_CLAIMED) {
        iot_gpio_close(g_led_ctxt[id].gpio.gpio_num);
        g_led_ctxt[id].gpio.is_claimed = LED_GPIO_RELEASED;
    }
    g_led_ctxt[id].valid = false;

#if defined(LOW_POWER_ENABLE) && defined(BUILD_CORE_CORE0)
    led_set_sleep_vote(id);
#endif
    return RET_OK;
}

uint32_t led_init(void)
{
    for (uint8_t i = 0; i < LED_MAX_NUM; i++) {
        g_led_ctxt[i].timer =
            os_create_timer(IOT_DRIVER_MID, false, led_timer_func, &g_led_ctxt[i]);

        if (!g_led_ctxt[i].timer) {
            DBGLOG_LIB_INFO("[LED]%d timer fail\n", i);
            assert(0);
            return RET_FAIL; //lint !e527: codestyle
        }

        g_led_ctxt[i].id = i;
        g_led_ctxt[i].state = LED_STATE_UNKNOWN;
        led_sleep_vote = 0xff;
    }

#if defined(LOW_POWER_ENABLE) && defined(BUILD_CORE_CORE0)
    power_mgnt_set_sleep_vote(POWER_SLEEP_LED);
#endif

    return RET_OK;
}

uint32_t led_deinit(void)
{
    for (uint8_t i = 0; i < LED_MAX_NUM; i++) {
        if (g_led_ctxt[i].timer) {
            os_delete_timer(g_led_ctxt[i].timer);
        }
        g_led_ctxt[i].valid = false;
    }
    iot_ledc_deinit();

    led_sleep_vote = 0xff;
#if defined(LOW_POWER_ENABLE) && defined(BUILD_CORE_CORE0)
    power_mgnt_set_sleep_vote(POWER_SLEEP_LED);
#endif
    return RET_OK;
}
