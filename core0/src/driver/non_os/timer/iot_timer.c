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
#include "riscv_cpu.h"
#include "chip_irq_vector.h"
#include "iot_irq.h"
#include "iot_timer.h"

#include "timer.h"

#ifdef LOW_POWER_ENABLE
#include "rtc.h"
#include "iot_rtc.h"
#include "dev_pm.h"
#endif

/*lint -esym(754, iot_timer_t::id) */
/*lint -esym(754, iot_timer_t::next) */
/*lint -esym(754, iot_timer_t::mode) */
/*lint -esym(754, iot_timer_t::handle) */
/*lint -esym(754, iot_timer_t::vector) */
typedef struct iot_timer_t {
    uint8_t id;
    uint8_t next;
    uint8_t mode;
    uint8_t active;
    int32_t handle;
    uint32_t vector;
    timer_isr_handler_t *isr_handle;
    bool_t valid;
} iot_timer_t;

#ifdef LOW_POWER_ENABLE
typedef struct iot_timer_save_state {
    uint32_t save_time;
    uint32_t save_start;
} iot_timer_save_state_t;

static iot_timer_save_state_t iot_timer_save_st;
static struct pm_operation timer_pm;
#endif

//timer[0] is not used as interrput timer
static iot_timer_t timer[IOT_USER_TIMER_MAX_VALID_NUM + 1];

static bool_t iot_timer_st;

static uint8_t iot_assgin_timer(void)
{
    for (uint8_t i = IOT_DEFAULT_USER_TIMER_ID; i <= IOT_USER_TIMER_MAX_VALID_NUM; i++) {
        if (timer[i].valid == false) {
            timer[i].valid = true;
            return i;
        }
    }

    return IOT_USER_TIMER_INVALID;
}

static uint32_t iot_timer_isr_handler(uint32_t vector, iot_addrword_t data)
    IRAM_TEXT(iot_timer_isr_handler);
static uint32_t iot_timer_isr_handler(uint32_t vector, iot_addrword_t data)
{
    uint32_t status = 0;
    uint8_t i;
    (void)vector;

    for (i = IOT_DEFAULT_USER_TIMER_ID; i <= IOT_USER_TIMER_MAX_VALID_NUM; i++) {
        uint32_t status_i = timer_int_get(i);
        status |= (status_i << i);
        if(status_i) {
            timer_int_clear(i);
        }
    }
    for (i = IOT_DEFAULT_USER_TIMER_ID; i <= IOT_USER_TIMER_MAX_VALID_NUM; i++) {
        if(GET_BIT_VALUE(status,i)) {
            timer_stop(i);
            if (timer[i].active && timer[i].isr_handle) {
                timer[i].isr_handle(data);
                timer[i].active = 0;
            }
        }
    }
    return 0;
}

static void iot_timer_interrupt_init(iot_addrword_t data)
{
    //only open GTM0 gptmr1-3 for using
    uint32_t vector = 0;

    switch (cpu_get_mhartid()) {
        case 0:
            vector = GTMR0_INT;
            break;
        case 1:
            vector = GTMR1_INT;
            break;
        case 2:
            vector = GTMR2_INT;
            break;
        default:
            assert(0);
    }
    uint32_t handle = iot_irq_create(vector, data, iot_timer_isr_handler);
    iot_irq_unmask(handle);
}

uint8_t iot_timer_create(timer_isr_handler_t *isr_handle, iot_addrword_t data)
{
    uint8_t id = iot_assgin_timer();
    (void)data;

    if (id == IOT_USER_TIMER_INVALID) {
        return RET_FAIL;
    }

    timer_init(id);
    timer_int_enable(id, false);
    iot_timer_stop(id);
    timer_int_clear(id);

    timer[id].isr_handle = isr_handle;
    timer_int_enable(id, true);

    return id;
}

void iot_timer_set(uint8_t id, uint32_t time, bool_t repeat)
{
    timer_set(id, time, repeat);
}

void iot_timer_start(uint8_t id)
{
    timer_start(id);
    timer[id].active = 1;
}

void iot_timer_stop(uint8_t id) IRAM_TEXT(iot_timer_stop);
void iot_timer_stop(uint8_t id)
{
    timer_stop(id);
    timer[id].active = 0;
}

void iot_timer_release(uint8_t id)
{
    timer_int_enable(id, false);
    iot_timer_stop(id);
    timer_int_clear(id);
    timer[id].valid = false;
}

uint32_t iot_timer_save(uint32_t data)
{
    UNUSED(data);
#ifdef LOW_POWER_ENABLE
    /**
     * 1. save current free run timer tick
     * 2. save globol rtc time as save start
     */
    iot_timer_save_st.save_time = iot_timer_get_time();
    iot_timer_save_st.save_start = iot_rtc_get_global_time();
#endif
    return RET_OK;
}

uint32_t iot_timer_restore(uint32_t data)
{
    UNUSED(data);
#ifdef LOW_POWER_ENABLE
    uint32_t end = iot_rtc_get_global_time();
    uint64_t dur = end - iot_timer_save_st.save_start;

#if RTC_CLK_HZ == RCO_32K_CLK_HZ
    /* time = tick * 1000000 / 32768
            = tick * 15625 / 512
            = tick * (16384-512-256+8+1) / 512 */
    uint64_t dur_us = ((dur << 14) - (dur << 9) - (dur << 8) + (dur << 3) + dur) >> 9;
#elif RTC_CLK_HZ == XTAL_32K_CLK_HZ
    /* time = tick * 1000000 / 31250
            = time * 32 */
    uint64_t dur_us = dur << 5;
#else
    /* time = tick * 1000000 / 32000
            = time * 125 / 4
            = time * (128 - 2 - 1) / 4 */
    uint64_t dur_us = ((dur << 7) - (dur << 1) - dur) >> 2;
#endif

    timer_init(IOT_DEFAULT_US_TIMER_ID);
    timer_set_value(IOT_DEFAULT_US_TIMER_ID, (uint32_t)(iot_timer_save_st.save_time + dur_us));
    timer_set(IOT_DEFAULT_US_TIMER_ID, 0xFFFFFFFF, true);
    timer_start(IOT_DEFAULT_US_TIMER_ID);

    for (uint8_t i = IOT_DEFAULT_USER_TIMER_ID; i <= IOT_USER_TIMER_MAX_VALID_NUM; i++) {
        if (timer[i].valid) {
            timer_init(i);
            timer_int_enable(i, false);
            iot_timer_stop(i);
            timer_int_clear(i);
            timer_int_enable(i, true);
        }
    }
#endif
    return RET_OK;
}

void iot_timer_init(void) IRAM_TEXT(iot_timer_init);
void iot_timer_init(void)
{
    timer_init(IOT_DEFAULT_US_TIMER_ID);

    timer_set(IOT_DEFAULT_US_TIMER_ID, 0xFFFFFFFF, true);
    timer_start(IOT_DEFAULT_US_TIMER_ID);

    iot_timer_interrupt_init(0);
    iot_timer_st = true;

#ifdef LOW_POWER_ENABLE
    list_init(&timer_pm.node);
    timer_pm.data = (uint32_t)&iot_timer_save_st;
    timer_pm.save = iot_timer_save;
    timer_pm.restore = iot_timer_restore;
    iot_dev_pm_register(&timer_pm);
#endif
}

uint32_t iot_timer_get_time(void) IRAM_TEXT(iot_timer_get_time);
uint32_t iot_timer_get_time(void)
{
    return timer_get_time(IOT_DEFAULT_US_TIMER_ID);
}

void iot_timer_delay_us(uint32_t delay_us) IRAM_TEXT(iot_timer_delay_us);
void iot_timer_delay_us(uint32_t delay_us)
{
    uint32_t start;
    uint32_t now;
    uint32_t delta = 0;

    if (!delay_us) {
        return;
    }

    if (!iot_timer_st) {
        iot_timer_init();
    }

    start = iot_timer_get_time();
    while (delta < delay_us) {
        now = iot_timer_get_time();
        delta = now - start;
    }
}

void iot_timer_delay_ms(uint32_t ms) IRAM_TEXT(iot_timer_delay_ms);
void iot_timer_delay_ms(uint32_t ms)
{
    iot_timer_delay_us(ms * 1000);
}
