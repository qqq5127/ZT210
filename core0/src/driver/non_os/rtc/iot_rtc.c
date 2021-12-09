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

#include "iot_irq.h"
#include "iot_rtc.h"

#include "rtc.h"

#ifdef LOW_POWER_ENABLE
#include "dev_pm.h"
#endif

#define IOT_RTC_MAX_TIMER_NUM   16U
#define IOT_RTC_PROCESSING_TICK 2U

#define IOT_RTC_HANDLER_TIMER IOT_RTC_TIMER_0
#define IOT_RTC_FREERUN_TIMER IOT_RTC_TIMER_1
#define IOT_RTC_GLOBAL_TIMER  RTC_TIMER_7   // pmm timer1

#define TIME_MS_TO_RTC_TICK(x) (((x) << 12) / 125)
#define TIME_S_TO_RTC_TICK(x)  ((x) << 15)

typedef struct iot_rtc_timer_handler {
    uint32_t start;
    uint32_t stop;
    iot_rtc_timer_callback cb;
    void *param;
    bool_t in_use;
} iot_rtc_timer_handler_t;

typedef struct iot_rtc_timer_state {
    iot_rtc_timer_handler_t timer[IOT_RTC_MAX_TIMER_NUM];
    iot_irq_t irq;
    uint8_t next;
} iot_rtc_timer_state_t;

typedef struct iot_rtc_save_state {
    uint32_t save_time;
    uint32_t save_start;
} iot_rtc_save_state_t;

static uint32_t iot_rtc_isr_handler(uint32_t vector, uint32_t data);

static iot_rtc_timer_state_t iot_rtc_info;
static bool_t iot_rtc_initialized = false;

#ifdef LOW_POWER_ENABLE
static iot_rtc_save_state_t iot_rtc_save_st;
static struct pm_operation iot_rtc_pm = {
    .node = LIST_HEAD_INIT(iot_rtc_pm.node),
    .save = iot_rtc_save,
    .restore = iot_rtc_restore,
    .data = (uint32_t)&iot_rtc_save_st,
};
#endif

static inline RTC_TIMER iot_rtc_get_timer_id(IOT_RTC_TIMER timer) IRAM_TEXT(iot_rtc_get_timer_id);
static inline RTC_TIMER iot_rtc_get_timer_id(IOT_RTC_TIMER timer)
{
    uint32_t cpu = cpu_get_mhartid();
    IOT_RTC_TIMER ret = IOT_RTC_TIMER_0;
    if (cpu == 0) {
        ret = timer;
    } else if (cpu == 1 || cpu == 2) {
        ret = (timer + IOT_RTC_TIMER_MAX);
    } else if (cpu == 3) {
        ret = (IOT_RTC_TIMER)(timer + 2 * IOT_RTC_TIMER_MAX);
    } else {
        ret = IOT_RTC_TIMER_NONE;
    }

    return (RTC_TIMER)ret;
}

static uint8_t iot_rtc_get_idle_handler(void) IRAM_TEXT(iot_rtc_get_idle_handler);
static uint8_t iot_rtc_get_idle_handler(void)
{
    for (uint8_t i = 0; i < IOT_RTC_MAX_TIMER_NUM; i++) {
        if (!iot_rtc_info.timer[i].in_use) {
            return i;
        }
    }

    return IOT_RTC_MAX_TIMER_NUM;
}

static void iot_rtc_timer_interrupt_init(void)
{
    uint32_t vector = rtc_timer_get_vector(iot_rtc_get_timer_id(IOT_RTC_HANDLER_TIMER));

    iot_rtc_info.irq = iot_irq_create(vector, 0, iot_rtc_isr_handler);
    iot_irq_unmask(iot_rtc_info.irq);
}

static inline uint32_t iot_rtc_get_time_diff(uint32_t start, uint32_t end)
    IRAM_TEXT(iot_rtc_get_time_diff);
static inline uint32_t iot_rtc_get_time_diff(uint32_t start, uint32_t end)
{
    return end - start;
}

static uint8_t iot_rtc_get_next_handler(void) IRAM_TEXT(iot_rtc_get_next_handler);
static uint8_t iot_rtc_get_next_handler(void)
{
    uint8_t cloest = 0;
    bool_t has_next = false;
    uint32_t curr_time = iot_rtc_get_time();

    for (uint8_t i = 0; i < IOT_RTC_MAX_TIMER_NUM; i++) {
        if (iot_rtc_info.timer[i].in_use) {
            if (!has_next) {
                cloest = i;
                has_next = true;
                continue;
            } else if (iot_rtc_get_time_diff(curr_time, iot_rtc_info.timer[i].stop)
                       < iot_rtc_get_time_diff(curr_time, iot_rtc_info.timer[cloest].stop)) {
                cloest = i;
            }
        }
    }

    return cloest;
}

uint8_t iot_rtc_init(void)
{
    if (iot_rtc_initialized) {
        return RET_OK;
    }

    // Init all timer of own sub system
    for (IOT_RTC_TIMER i = IOT_RTC_TIMER_0; i < IOT_RTC_TIMER_MAX; i++) {
        rtc_init(iot_rtc_get_timer_id(i));
    }

    // Init all rtc status
    for (IOT_RTC_TIMER i = IOT_RTC_TIMER_0; i < IOT_RTC_MAX_TIMER_NUM; i++) {
        iot_rtc_info.timer[i].in_use = false;
    }

    // Init handler timer interrupt
    iot_rtc_timer_interrupt_init();

    // Enable count mode
    rtc_timer_count_mode_enable(iot_rtc_get_timer_id(IOT_RTC_FREERUN_TIMER), true);

    // Init freerun timer
    rtc_timer_set_value(iot_rtc_get_timer_id(IOT_RTC_FREERUN_TIMER), MAX_UINT32);
    rtc_timer_enable(iot_rtc_get_timer_id(IOT_RTC_FREERUN_TIMER));

    iot_rtc_info.next = IOT_RTC_MAX_TIMER_NUM;
    iot_rtc_initialized = true;

#ifdef LOW_POWER_ENABLE
    iot_dev_pm_register(&iot_rtc_pm);
#endif

    return RET_OK;
}

uint8_t iot_rtc_deinit(void)
{
    if (iot_rtc_initialized) {
        iot_rtc_info.next = IOT_RTC_MAX_TIMER_NUM;
        iot_rtc_initialized = false;

        // Init all timer of own sub system
        for (uint32_t i = IOT_RTC_TIMER_0; i < IOT_RTC_TIMER_MAX; i++) {
            rtc_deinit(iot_rtc_get_timer_id((IOT_RTC_TIMER)i));
        }

        // Init all rtc status
        for (uint32_t i = 0; i < IOT_RTC_MAX_TIMER_NUM; i++) {
            iot_rtc_info.timer[i].in_use = false;
        }
    }

#ifdef LOW_POWER_ENABLE
    iot_dev_pm_unregister(&iot_rtc_pm);
#endif

    return RET_OK;
}

uint32_t iot_rtc_save(uint32_t data) IRAM_TEXT(iot_rtc_save);
uint32_t iot_rtc_save(uint32_t data)
{
    UNUSED(data);
#ifdef LOW_POWER_ENABLE
    /**
     * 1. save current free run rtc timer tick
     * 2. save globol rtc time as save start
     */
    iot_rtc_save_st.save_time = iot_rtc_get_time();
    iot_rtc_save_st.save_start = iot_rtc_get_global_time();
#endif
    return RET_OK;
}

uint32_t iot_rtc_restore(uint32_t data) IRAM_TEXT(iot_rtc_restore);
uint32_t iot_rtc_restore(uint32_t data)
{
    UNUSED(data);
#ifdef LOW_POWER_ENABLE
    uint32_t end = iot_rtc_get_global_time();
    uint32_t dur = iot_rtc_get_time_diff(iot_rtc_save_st.save_start, end);
    // Enable count mode when restore the rtc timer
    rtc_timer_count_mode_enable(iot_rtc_get_timer_id(IOT_RTC_FREERUN_TIMER), true);
    rtc_timer_set_current_value(iot_rtc_get_timer_id(IOT_RTC_FREERUN_TIMER),
                                iot_rtc_save_st.save_time + dur);
    rtc_timer_enable(iot_rtc_get_timer_id(IOT_RTC_FREERUN_TIMER));
#endif
    return RET_OK;
}

uint8_t iot_rtc_set_time(IOT_RTC_TIMER timer, uint32_t time)
{
    RTC_TIMER id = iot_rtc_get_timer_id(timer);
    rtc_timer_set_value(id, time);
    return RET_OK;
}

uint32_t iot_rtc_get_time(void) IRAM_TEXT(iot_rtc_get_time);
uint32_t iot_rtc_get_time(void)
{
    return rtc_timer_get_value(iot_rtc_get_timer_id(IOT_RTC_FREERUN_TIMER));
}

uint32_t iot_rtc_get_time_us(void) IRAM_TEXT(iot_rtc_get_time_us);
uint32_t iot_rtc_get_time_us(void)
{
    uint32_t tick = iot_rtc_get_time();
#if RTC_CLK_HZ == RCO_32K_CLK_HZ
    /*  us = tick / 32768 * 1000000
           = tick * 15625 / 512
           = tick * (16384 - 512 - 256 + 8 + 1) / 512 */
    return ((tick << 5) - tick - (tick >> 1) + (tick >> 6) + (tick >> 9));
#elif RTC_CLK_HZ == XTAL_32K_CLK_HZ
    // tick * 32
    return (tick << 5);
#else
    #error "Unknown RTC clk freq."
#endif
}

uint32_t iot_rtc_get_time_ms(void) IRAM_TEXT(iot_rtc_get_time_ms);
uint32_t iot_rtc_get_time_ms(void)
{
    /**  tick / 32768 * 1000
     *  = tick * 125 / 4096
     *  = tick * (128 - 2 - 1) / 4096
     *  = (tick << 7 - tick << 1 - tick) >> 12
     */
    uint32_t tick = iot_rtc_get_time();
#if RTC_CLK_HZ == RCO_32K_CLK_HZ
    return ((tick << 7) - (tick << 1) - tick) >> 12;
#elif RTC_CLK_HZ == XTAL_32K_CLK_HZ
    // tick / 31.25
    return (((tick >> 5) + (tick >> 10) - (tick >> 12)) + (tick >> 16) + (tick >> 19) + (tick >> 21)
            - (tick >> 24));
#else
    #error "Unknown RTC clk freq."
#endif
}

uint32_t iot_rtc_get_time_s(void) IRAM_TEXT(iot_rtc_get_time_s);
uint32_t iot_rtc_get_time_s(void)
{
    /* tick / 32768 */
    return iot_rtc_get_time() >> 12;
}

uint32_t iot_rtc_add_timer(uint32_t time, iot_rtc_timer_callback cb, void *param)
    IRAM_TEXT(iot_rtc_add_timer);
uint32_t iot_rtc_add_timer(uint32_t time, iot_rtc_timer_callback cb, void *param)
{
    uint32_t curr_time = iot_rtc_get_time();
    RTC_TIMER timer_id = iot_rtc_get_timer_id(IOT_RTC_HANDLER_TIMER);

    // Overflow is ok
    uint32_t stop_time;
    uint32_t vector = (uint32_t)rtc_timer_get_vector(timer_id);

    uint32_t mask = cpu_disable_irq();
    uint8_t handler_id = iot_rtc_get_idle_handler();
    assert(handler_id < IOT_RTC_MAX_TIMER_NUM);

    iot_rtc_timer_handler_t *handler = &iot_rtc_info.timer[handler_id];

    // Rtc timer need at least 2 tick to processing
    stop_time = (uint32_t)(curr_time + MAX(IOT_RTC_PROCESSING_TICK, time));

    handler->start = curr_time;
    handler->stop = stop_time;
    handler->cb = cb;
    handler->param = param;
    handler->in_use = true;
    if (iot_rtc_info.next < IOT_RTC_MAX_TIMER_NUM) {
        /* There is a running timer */
        iot_rtc_timer_handler_t *next_handler = &iot_rtc_info.timer[iot_rtc_info.next];
        uint32_t next_left = iot_rtc_get_time_diff(curr_time, next_handler->stop);
        uint32_t curr_left = iot_rtc_get_time_diff(curr_time, stop_time);
        if (next_left < curr_left) {
            /* Next timer is closer, do nothing here */
            cpu_restore_irq(mask);
            return handler_id;
        } else {
            /* Current timer is closer, reconfig the timer */
            rtc_timer_set_value(timer_id,
                                iot_rtc_get_time_diff(iot_rtc_get_time(), stop_time)
                                    - IOT_RTC_PROCESSING_TICK);
            iot_irq_set_data(vector, handler_id);

            iot_rtc_info.next = handler_id;
        }
    } else {
        /* All timer are idle, set current one as target */
        rtc_timer_clear(timer_id);
        iot_irq_set_data(vector, handler_id);
        rtc_timer_int_enable(timer_id);

        rtc_timer_set_value(timer_id,
                            iot_rtc_get_time_diff(iot_rtc_get_time(), stop_time)
                                - IOT_RTC_PROCESSING_TICK);
        rtc_timer_enable(timer_id);
        iot_rtc_info.next = handler_id;
    }

    cpu_restore_irq(mask);
    return handler_id;
}

uint32_t iot_rtc_add_timer_ms(uint32_t time_ms, iot_rtc_timer_callback cb, void *param)
{
    uint32_t time_tick = TIME_MS_TO_RTC_TICK(time_ms);

    return iot_rtc_add_timer(time_tick, cb, param);
}

uint32_t iot_rtc_add_timer_s(uint32_t time_s, iot_rtc_timer_callback cb, void *param)
{
    uint32_t time_tick = TIME_S_TO_RTC_TICK(time_s);

    return iot_rtc_add_timer(time_tick, cb, param);
}

void iot_rtc_delete_timer(uint32_t timer_id) IRAM_TEXT(iot_rtc_delete_timer);
void iot_rtc_delete_timer(uint32_t timer_id)
{
    assert(timer_id < IOT_RTC_MAX_TIMER_NUM);

    /* timer are already done. */
    if (!iot_rtc_info.timer[timer_id].in_use) {
        return;
    }

    uint32_t mask = cpu_disable_irq();

    /* mark the timer as nonvalid */
    iot_rtc_info.timer[timer_id].in_use = false;

    /* timer are next timer */
    if (iot_rtc_info.next == timer_id) {
        // Add next timer
        uint8_t next_id = iot_rtc_get_next_handler();
        if (next_id < IOT_RTC_MAX_TIMER_NUM) {
            iot_rtc_timer_handler_t *handler = &iot_rtc_info.timer[next_id];
            uint32_t next_time = iot_rtc_get_time_diff(iot_rtc_get_time(), handler->stop);
            iot_irq_set_data(rtc_timer_get_vector(iot_rtc_get_timer_id(IOT_RTC_HANDLER_TIMER)),
                             next_id);
            rtc_timer_set_value(iot_rtc_get_timer_id(IOT_RTC_HANDLER_TIMER),
                                next_time - IOT_RTC_PROCESSING_TICK);
            iot_rtc_info.next = next_id;
        } else {
            /* no next timer handler */
            rtc_timer_int_disable(iot_rtc_get_timer_id(IOT_RTC_HANDLER_TIMER));
            rtc_timer_disable(iot_rtc_get_timer_id(IOT_RTC_HANDLER_TIMER));
            iot_rtc_info.next = IOT_RTC_MAX_TIMER_NUM;
        }
    }

    cpu_restore_irq(mask);
}

static uint32_t iot_rtc_isr_handler(uint32_t vector, uint32_t data) IRAM_TEXT(iot_rtc_isr_handler);
static uint32_t iot_rtc_isr_handler(uint32_t vector, uint32_t data)
{
    uint8_t handler_id = (uint8_t)data;
    iot_rtc_timer_handler_t *handler = &iot_rtc_info.timer[handler_id];
    uint32_t next_time;

    UNUSED(vector);
    UNUSED(data);

    if (handler->cb != NULL) {
        handler->cb(handler->param);
    }

    // Mark the timer as idle
    handler->in_use = false;
    handler = NULL;

    // Add next timer
    uint8_t next_id = iot_rtc_get_next_handler();
    if (next_id < IOT_RTC_MAX_TIMER_NUM) {
        handler = &iot_rtc_info.timer[next_id];
        next_time = iot_rtc_get_time_diff(iot_rtc_get_time(), handler->stop);
        iot_irq_set_data(rtc_timer_get_vector(iot_rtc_get_timer_id(IOT_RTC_HANDLER_TIMER)),
                         next_id);
        rtc_timer_set_value(iot_rtc_get_timer_id(IOT_RTC_HANDLER_TIMER),
                            next_time - IOT_RTC_PROCESSING_TICK);
        iot_rtc_info.next = next_id;
    } else {
        /* no next timer handler */
        rtc_timer_int_disable(iot_rtc_get_timer_id(IOT_RTC_HANDLER_TIMER));
        rtc_timer_disable(iot_rtc_get_timer_id(IOT_RTC_HANDLER_TIMER));
        iot_rtc_info.next = IOT_RTC_MAX_TIMER_NUM;
    }

    rtc_timer_int_clear(iot_rtc_get_timer_id(IOT_RTC_HANDLER_TIMER));

    return 0;
}

uint8_t iot_rtc_global_init(void)
{
    rtc_init(IOT_RTC_GLOBAL_TIMER);
    rtc_timer_clear(IOT_RTC_GLOBAL_TIMER);
    rtc_timer_count_mode_enable(IOT_RTC_GLOBAL_TIMER, true);
    rtc_timer_enable(IOT_RTC_GLOBAL_TIMER);
    return RET_OK;
}

uint32_t iot_rtc_get_global_time(void) IRAM_TEXT(iot_rtc_get_global_time);
uint32_t iot_rtc_get_global_time(void)
{
    return rtc_timer_get_value(IOT_RTC_GLOBAL_TIMER);
}

uint32_t iot_rtc_get_global_time_us(void) IRAM_TEXT(iot_rtc_get_global_time_us);
uint32_t iot_rtc_get_global_time_us(void)
{
    uint32_t tick = iot_rtc_get_global_time();
#if RTC_CLK_HZ == RCO_32K_CLK_HZ
    /*  us = tick / 32768 * 1000000
           = tick * 15625 / 512
           = tick * (16384 - 512 - 256 + 8 + 1) / 512 */
    return ((tick << 5) - tick - (tick >> 1) + (tick >> 6) + (tick >> 9));
#elif RTC_CLK_HZ == XTAL_32K_CLK_HZ
    // tick * 32
    return (tick << 5);
#else
    #error "Unknown RTC clk freq."
#endif
}


uint32_t iot_rtc_get_global_time_ms(void) IRAM_TEXT(iot_rtc_get_global_time_ms);
uint32_t iot_rtc_get_global_time_ms(void)
{
    /**  tick / 32768 * 1000
     *  = tick * 125 / 4096
     *  = tick * (128 - 2 - 1) / 4096
     *  = (tick << 7 - tick << 1 - tick) >> 12
     */
    uint32_t tick = iot_rtc_get_global_time();
#if RTC_CLK_HZ == RCO_32K_CLK_HZ
    return ((tick << 7) - (tick << 1) - tick) >> 12;
#elif RTC_CLK_HZ == XTAL_32K_CLK_HZ
    // tick / 31.25
    return (((tick >> 5) + (tick >> 10) - (tick >> 12)) + (tick >> 16) + (tick >> 19) + (tick >> 21)
            - (tick >> 24));
#else
    // tick / 32;
    return tick >> 5;
#endif
}

uint32_t iot_rtc_get_global_time_s(void)
{
    /* tick / 32768 */
    return iot_rtc_get_global_time() >> 12;
}
