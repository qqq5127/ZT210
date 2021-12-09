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
#include "wic.h"
#include "iot_wic.h"
#include "iot_irq.h"
#include "os_utils.h"
#include "riscv_cpu.h"
#include "critical_sec.h"
#include "iot_memory_config.h"
#include "intc.h"
#include "pmm.h"
#include "iot_ipc.h"
#include "iot_timer.h"
#ifdef LOW_POWER_ENABLE
#include "dev_pm.h"
#endif

static uint32_t cpu_query_counter[NUM_WIC_PCORE] = {0};
static volatile bool_t cpu_query_stat[NUM_WIC_PCORE] = {false, false};  /* remote target query cpu status */

static os_event_h wic_event;
static int32_t wic_event_group[EVENT_WIC_MAX] = {0};

#ifdef LOW_POWER_ENABLE
struct pm_operation wic_pm = {
    .node = LIST_HEAD_INIT(wic_pm.node),
    .save = NULL,
    .restore = iot_wic_restore,
};
#endif

#define WIC_SET_SCRATCH_GAP_TIME  4 /* us */

#define DEPENDENCE_SCRATCH_BASE  WIC_DEPENDENCE_SCRATCH_START

static void iot_wic_hold_target(IOT_WIC_CORE core) IRAM_TEXT(iot_wic_hold_target);
static void iot_wic_hold_target(IOT_WIC_CORE core)
{
    volatile uint32_t *scratch =
        ((volatile uint32_t *)DEPENDENCE_SCRATCH_BASE) + IOT_WIC_CORE_MAX * core;

    cpu_critical_enter();
    *(scratch + IOT_WIC_SELF) = 1;
    cpu_critical_exit();
}

static void iot_wic_release_target(IOT_WIC_CORE core) IRAM_TEXT(iot_wic_release_target);
static void iot_wic_release_target(IOT_WIC_CORE core)
{
    volatile uint32_t *scratch =
        ((volatile uint32_t *)DEPENDENCE_SCRATCH_BASE) + IOT_WIC_CORE_MAX * core;

    cpu_critical_enter();
    *(scratch + IOT_WIC_SELF) = 0;
    cpu_critical_exit();
}

static void iot_wic_release_all_target(void)
{
    volatile uint32_t *scratch = (volatile uint32_t *)DEPENDENCE_SCRATCH_BASE;
    int i;
    for (i = 0; i < IOT_WIC_CORE_MAX; i++) {
        *(scratch + i * IOT_WIC_CORE_MAX  + IOT_WIC_SELF) = 0;
    }
}

bool_t iot_wic_if_be_hold(void) IRAM_TEXT(iot_wic_if_be_hold);
bool_t iot_wic_if_be_hold(void)
{
    volatile uint32_t *scratch =
        ((volatile uint32_t *)DEPENDENCE_SCRATCH_BASE) + IOT_WIC_CORE_MAX * IOT_WIC_SELF;  //lint !e835:codestyle !e845:only core0 IOT_WIC_SELF is 0.

    int i;
    for (i = 0; i < IOT_WIC_CORE_MAX; i++) {
        if (*(scratch + i)) {
            return true;
        }
    }

    return false;
}

uint32_t iot_wic_query(IOT_WIC_CORE core, bool_t hold) IRAM_TEXT(iot_wic_query);
uint32_t iot_wic_query(IOT_WIC_CORE core, bool_t hold)
{
    if (core == IOT_WIC_SELF) {
        return RET_OK;
    }

    WIC_REG r = wic_core2reg((WIC_CORE)core);
    cpu_critical_enter();

    /* if no need to hold target, just send a wic */
    if (!hold) {
        //iot_wic_hold_target(core); // using softirq as scractch for ipc
        /*
         * !!! workaround
         * timing gap between scratch setting & wic
         * To cover remote timing pap between scratch picking & wfi
         */
        iot_timer_delay_us(WIC_SET_SCRATCH_GAP_TIME);

        wic_set_query_status(r);
        cpu_critical_exit();
        return RET_OK;
    }

    if (!cpu_query_counter[r]) {
        iot_wic_hold_target(core);
        /*
         * !!! workaround
         * timing gap between scratch setting & wic
         * To cover remote timing pap between scratch picking & wfi
         */
        iot_timer_delay_us(WIC_SET_SCRATCH_GAP_TIME);

        wic_set_query_status(r);
    }
    cpu_query_counter[r]++;
    cpu_critical_exit();

    if (!cpu_query_stat[r]) {
        return RET_NOT_READY;
    } else {
        return RET_OK;
    }
}

/* only called from ISR */
void iot_wic_finish(IOT_WIC_CORE core) IRAM_TEXT(iot_wic_finish);
void iot_wic_finish(IOT_WIC_CORE core)
{
    if (core == IOT_WIC_SELF) {
        return;
    }

    WIC_REG r = wic_core2reg((WIC_CORE)core);
    cpu_critical_enter();
    assert(cpu_query_counter[r] > 0);
    cpu_query_counter[r]--;
    if ((!cpu_query_counter[r]) && (cpu_query_stat[r])) {
        /* make sure dependence scratch is clear after query clear */
        iot_wic_release_target(core);
        cpu_query_stat[r] = false;
    }
    cpu_critical_exit();
}

void iot_wic_poll(IOT_WIC_CORE core) IRAM_TEXT(iot_wic_poll);
void iot_wic_poll(IOT_WIC_CORE core)
{
    if (core == IOT_WIC_SELF) {
        return;
    }

    uint32_t cnt = 0;
    while (!cpu_query_stat[(bool_t)wic_core2reg((WIC_CORE)core)]) {
        assert(cnt++ < 100);
        os_delay(1);
    }
}

uint32_t iot_wic_dma_query(IOT_WIC_CORE core)
{
    if (core == IOT_WIC_SELF) {
        return RET_OK;
    }

    wic_set_dma_query_status((WIC_REG)wic_core2reg((WIC_CORE)core));
    return RET_OK;
}

void iot_wic_dma_finish(IOT_WIC_CORE core)
{
    if (core == IOT_WIC_SELF) {
        return;
    }

    wic_clear_dma_query_status((WIC_REG)wic_core2reg((WIC_CORE)core));
}

uint32_t iot_wic_event_register(WIC_EVENT event, wic_event_callback cb)
{
    uint32_t id;

    for (id = 0; id < EVENT_WIC_MAX; id++) {
        if (event & BIT(id)) {
            if (!wic_event_group[id]) {
                wic_event_group[id] = iot_share_task_msg_register((iot_share_event_func)cb);
            }
        }
    }

    return RET_OK;
}

static uint32_t iot_wic_wakeup_isr_handler(uint32_t vector, uint32_t r) IRAM_TEXT(iot_wic_wakeup_isr_handler);
static uint32_t iot_wic_wakeup_isr_handler(uint32_t vector, uint32_t r)
{
    UNUSED(vector);

    if (wic_get_wakeup_int_status((WIC_REG)r)) {
        wic_clear_wakeup_int_status((WIC_REG)r);

        if (r == 0) {
            os_set_event_isr(wic_event, 1 << EVENT_WAKEUP_R1);
            if (wic_event_group[EVENT_WAKEUP_R1]) {
                iot_share_task_post_msg_from_isr(
                    IOT_SHARE_TASK_QUEUE_HP, wic_event_group[EVENT_WAKEUP_R1],
                    (void *)(1 << EVENT_WAKEUP_R1));
            }

#if defined(BUILD_CORE_CORE0)
            iot_ipc_recv_message(BT_CORE);
#else
            iot_ipc_recv_message(DTOP_CORE);
#endif
        } else if (r == 1) {

            os_set_event_isr(wic_event, 1 << EVENT_WAKEUP_R2);
            if (wic_event_group[EVENT_WAKEUP_R2]) {
                iot_share_task_post_msg_from_isr(
                    IOT_SHARE_TASK_QUEUE_HP, wic_event_group[EVENT_WAKEUP_R2],
                    (void *)(1 << EVENT_WAKEUP_R2));
            }

#if defined(BUILD_CORE_DSP)
            iot_ipc_recv_message(BT_CORE);
#else
            iot_ipc_recv_message(AUD_CORE);
#endif
        }
    }

    // dummy
    if (wic_get_dma_done_int_status((WIC_REG)r)) {
        wic_clear_dma_done_int_status((WIC_REG)r);
    }

    return RET_OK;
}

static uint32_t iot_wic_query_isr_handler(uint32_t vector, uint32_t r) IRAM_TEXT(iot_wic_query_isr_handler);
static uint32_t iot_wic_query_isr_handler(uint32_t vector, uint32_t r)
{
    UNUSED(vector);

    /* when wic query irq generated, query raw status must be set by hardware */
    wic_clear_query_status((WIC_REG)r);

    if (wic_get_query_int_status((WIC_REG)r)) {

        wic_clear_query_int_status((WIC_REG)r);

        if (r == 0) {
            os_set_event_isr(wic_event, 1 << EVENT_QUERY_R1);
            if (wic_event_group[EVENT_QUERY_R1]) {
                iot_share_task_post_msg_from_isr(
                    IOT_SHARE_TASK_QUEUE_HP, wic_event_group[EVENT_QUERY_R1],
                    (void *)(1 << EVENT_QUERY_R1));
            }

        } else if (r == 1) {
            os_set_event_isr(wic_event, 1 << EVENT_QUERY_R2);
            if (wic_event_group[EVENT_QUERY_R2]) {
                iot_share_task_post_msg_from_isr(
                    IOT_SHARE_TASK_QUEUE_HP, wic_event_group[EVENT_QUERY_R2],
                    (void *)(1 << EVENT_QUERY_R2));
            }
        }
    }

    if (!cpu_query_counter[r]) {
        /* make sure dependence scratch is clear after query clear */
        cpu_query_stat[r] = false;
        iot_wic_release_target((IOT_WIC_CORE)wic_reg2core((WIC_REG)r));
    } else {
        /* mark wic query status, will clear dependence scratch in iot_wic_finish() */
        cpu_query_stat[r] = true;
    }

    return RET_OK;
}

uint32_t iot_wic_wait_event(WIC_EVENT event, WIC_EVENT *ret_event)
{
    bool_t ret;
    uint32_t recv;

    if (!wic_event) {
        return RET_FAIL;
    }

    ret = os_wait_event_with_v(
        wic_event, event, EVENT_FLAG_OR | EVENT_FLAG_CLEAR, MAX_TIME, &recv);
    if (!ret) {
        return RET_FAIL;
    }

    if (ret_event) {
        *ret_event = (WIC_EVENT)recv;
    }

    return RET_OK;
}

uint32_t iot_wic_restore(uint32_t data)
{
    UNUSED(data);

    int i;
    for (i = 0; i < NUM_WIC_PCORE; i++) {
        wic_cpu_query_int_enable((WIC_REG)i);
        wic_cpu_wakeup_int_enable((WIC_REG)i);
    }
    iot_wic_release_all_target();

    return RET_OK;
}

uint32_t iot_wic_init(void)
{
    uint32_t i;
    iot_irq_t iot_wic_query_int_irq1;
    iot_irq_t iot_wic_query_int_irq2;
    iot_irq_t iot_wic_wakeup_int_irq1;
    iot_irq_t iot_wic_wakeup_int_irq2;

    wic_init();

#if defined(BUILD_CORE_CORE0)
    volatile uint32_t *scratch = (volatile uint32_t *)DEPENDENCE_SCRATCH_BASE;
    for (i = 0; i < IOT_WIC_CORE_MAX * IOT_WIC_CORE_MAX; i++) {      //lint !e648:can't overflow.
        *(scratch + i) = 0;
    }
#else
    iot_wic_release_all_target();
#endif

    wic_event = os_create_event(IOT_DRIVER_MID);
    if (!wic_event) {
        return RET_FAIL;
    }

    for (i = 0; i < NUM_WIC_PCORE; i++) {
        wic_cpu_query_int_enable((WIC_REG)i);
        wic_cpu_wakeup_int_enable((WIC_REG)i);
    }

#if defined(BUILD_CORE_CORE0)
    iot_wic_query_int_irq1 =
        iot_irq_create(WIC_QUERY_INT_DCORE2BT, WIC_REG_BT, iot_wic_query_isr_handler);
    iot_wic_query_int_irq2 = iot_irq_create(WIC_QUERY_INT_DCORE2AUD,
                                            WIC_REG_AUDIO, iot_wic_query_isr_handler);
    iot_wic_wakeup_int_irq1 = iot_irq_create(WIC_WAKEN_UP_INT_BT2DCORE,
                                             WIC_REG_BT, iot_wic_wakeup_isr_handler);
    iot_wic_wakeup_int_irq2 = iot_irq_create(WIC_WAKEN_UP_INT_AUD2DCORE,
                                             WIC_REG_AUDIO, iot_wic_wakeup_isr_handler);

#elif defined(BUILD_CORE_CORE1)
    iot_wic_query_int_irq1 =
        iot_irq_create(WIC_QUERY_INT_BT2DCORE, WIC_REG_DTOP, iot_wic_query_isr_handler);
    iot_wic_query_int_irq2 =
        iot_irq_create(WIC_QUERY_INT_BT2AUD, WIC_REG_AUDIO, iot_wic_query_isr_handler);
    iot_wic_wakeup_int_irq1 = iot_irq_create(WIC_WAKEN_UP_INT_DCORE2BT,
                                             WIC_REG_DTOP, iot_wic_wakeup_isr_handler);
    iot_wic_wakeup_int_irq2 = iot_irq_create(WIC_WAKEN_UP_INT_AUD2BT,
                                             WIC_REG_AUDIO, iot_wic_wakeup_isr_handler);

#elif defined(BUILD_CORE_DSP)
    iot_wic_query_int_irq1 = iot_irq_create(WIC_QUERY_INT_AUD2DCORE,
                                            WIC_REG_DTOP, iot_wic_query_isr_handler);
    iot_wic_query_int_irq2 =
        iot_irq_create(WIC_QUERY_INT_AUD2BT, WIC_REG_BT, iot_wic_query_isr_handler);
    iot_wic_wakeup_int_irq1 = iot_irq_create(WIC_WAKEN_UP_INT_DCORE2AUD,
                                             WIC_REG_DTOP, iot_wic_wakeup_isr_handler);
    iot_wic_wakeup_int_irq2 =
        iot_irq_create(WIC_WAKEN_UP_INT_BT2AUD, WIC_REG_BT, iot_wic_wakeup_isr_handler);

#endif

    /* enable WIC wakeup src by default */
#if defined(BUILD_CORE_CORE0)
    intc_set_wakeup_enable(WAKEUP_WIC_BT2DCORE);
    intc_set_wakeup_enable(WAKEUP_WIC_AUD2DCORE);
#elif defined(BUILD_CORE_CORE1)
    intc_set_wakeup_enable(WAKEUP_WIC_DCORE2BT);
    intc_set_wakeup_enable(WAKEUP_WIC_AUD2BT);
#elif defined(BUILD_CORE_DSP)
    intc_set_wakeup_enable(WAKEUP_WIC_DCORE2AUD);
    intc_set_wakeup_enable(WAKEUP_WIC_BT2AUD);
#endif

    iot_irq_unmask(iot_wic_query_int_irq1);
    iot_irq_unmask(iot_wic_query_int_irq2);
    iot_irq_unmask(iot_wic_wakeup_int_irq1);
    iot_irq_unmask(iot_wic_wakeup_int_irq2);

#ifdef LOW_POWER_ENABLE
    iot_dev_pm_register(&wic_pm);
#endif

    return 0;
}
