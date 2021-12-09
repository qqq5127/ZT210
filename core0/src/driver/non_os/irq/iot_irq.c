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
/* common includes */
#include "types.h"
#include "riscv_cpu.h"
#include "chip_irq_vector.h"
#include "irq_priority.h"


/* hw includes */
#include "intc.h"
#ifdef LOW_POWER_ENABLE
#include "dev_pm.h"
#endif
#ifdef CHECK_ISR_USAGE
#include "iot_rtc.h"
#include "iot_timer.h"
#include "clock.h"
#include "driver_dbglog.h"
#endif
#ifdef CHECK_ISR_FLASH
#include "iot_soc.h"
#endif

//storm interrupt debug, uncomment it to disable it.
#define DEBUG_CHECK_INTERRUPT_STORM
#define IOT_IRQ_INTERRUPT_STORM_MAX 1000

#define IOT_IRQ_SYS_TICK_CLOCK 16000000U

uint32_t irq_nest = 0;

#ifdef CHECK_ISR_USAGE
#define IOT_EXT_INT_HANDLER_TIME_LIMIT_US 250
iot_isr_usage_t iot_isr_usage;
#endif

#ifdef LOW_POWER_ENABLE
struct pm_operation tick_pm;
#endif

typedef struct isr_handler {
    iot_isr_t isr;
    uint32_t param;
    uint8_t priority;
    uint8_t vector;
} isr_handler_t;

static isr_handler_t iot_isr_handlers[CHIP_IRQ_VECTOR_MAX];

static uint32_t hal_default_isr(uint32_t vector, uint32_t data)
{
    UNUSED(vector);
    UNUSED(data);
    return 0;
}

#ifdef CHECK_ISR_USAGE
static void iot_irq_usage_update(isr_usage_param_t *param, uint16_t duration_us, uint32_t vector_ra) IRAM_TEXT(iot_irq_usage_update);
static void iot_irq_usage_update(isr_usage_param_t *param, uint16_t duration_us, uint32_t vector_ra)
{
    if (duration_us > param[MAX_SAVE_ISR_CNT - 1].dur_us) {
        uint8_t i = 0, j = 0;
        //check current index
        for (i = 0; i < MAX_SAVE_ISR_CNT; i++) {
            if (duration_us > param[i].dur_us) {
                break;
            }
        }
        //need insert
        if (i < MAX_SAVE_ISR_CNT) {
            //move
            for (j = MAX_SAVE_ISR_CNT - 1; j > i; j--) {
                param[j] = param[j - 1];
            }
            //insert
            param[i].dur_us = duration_us;
            param[i].vector_ra = vector_ra;
        }
    }
    iot_isr_usage.isr_time_total_us += duration_us;
}
#endif

void iot_irq_timer_usage_insert(uint16_t duration_us, uint32_t vector_ra) IRAM_TEXT(iot_irq_usageiot_irq_timer_usage_insert_insert);
void iot_irq_timer_usage_insert(uint16_t duration_us, uint32_t vector_ra)
{
#ifdef CHECK_ISR_USAGE
    iot_irq_usage_update(iot_isr_usage.timer, duration_us, vector_ra);
#else
    UNUSED(duration_us);
    UNUSED(vector_ra);
#endif
}

void iot_irq_usage_insert(uint16_t duration_us, uint32_t vector_ra) IRAM_TEXT(iot_irq_usage_insert);
void iot_irq_usage_insert(uint16_t duration_us, uint32_t vector_ra)
{
#ifdef CHECK_ISR_USAGE
    iot_irq_usage_update(iot_isr_usage.critical, duration_us, vector_ra);
#else
    UNUSED(duration_us);
    UNUSED(vector_ra);
#endif
}

void iot_irq_deliver_usage_insert(uint16_t duration_us, uint32_t vector_ra)
    IRAM_TEXT(iot_irq_deliver_usage_insert);
void iot_irq_deliver_usage_insert(uint16_t duration_us, uint32_t vector_ra)
{
#ifdef CHECK_ISR_USAGE
    iot_irq_usage_update(iot_isr_usage.isr, duration_us, vector_ra);
#else
    UNUSED(duration_us);
    UNUSED(vector_ra);
#endif
}

static void iot_irq_deliver_isr(uint32_t vector) IRAM_TEXT(iot_irq_deliver_isr);
static void iot_irq_deliver_isr(uint32_t vector)
{
    iot_isr_t isr = iot_isr_handlers[vector].isr;
    uint32_t param = (uint32_t)iot_isr_handlers[vector].param;

#ifdef CHECK_ISR_USAGE
    uint64_t start = iot_timer_get_time();
#endif

    // Call the ISR
    isr(vector, param);

#ifdef CHECK_ISR_USAGE
    uint64_t stop = iot_timer_get_time();
    uint32_t duration_us = (stop - start);
    iot_irq_deliver_usage_insert(duration_us, vector);

    //int time limit
    //fixme:should decrease the time cost of these interrupts
    if ((vector != UART1_INT) && (vector != DM_INT) && duration_us > IOT_EXT_INT_HANDLER_TIME_LIMIT_US) {
        DBGLOG_DRIVER_WARNING("[WARNING]vector:%d time %d",vector, duration_us);
    }

#endif
}

static void enter_interrupt_nest(uint32_t thd) IRAM_TEXT(enter_interrupt_nest);
static void enter_interrupt_nest(uint32_t thd)
{
    uint32_t cpu = cpu_get_mhartid();

    if (!irq_nest) {
        cpu_disable_timer_irq();
        cpu_disable_software_irq();
    }

    irq_nest++;
    intc_set_interrupt_threshold(cpu, thd);
    cpu_enable_irq();
}

static void exit_interrupt_nest(uint32_t thd) IRAM_TEXT(exit_interrupt_nest);
static void exit_interrupt_nest(uint32_t thd)
{
    uint32_t cpu = cpu_get_mhartid();

    cpu_disable_irq();
    intc_set_interrupt_threshold(cpu, thd);
    irq_nest--;

    if (!irq_nest) {
        cpu_enable_timer_irq();
        cpu_enable_software_irq();
    }
}   //lint !e454 exit interrupt nest, should clear MIE bit before call `mret`

void iot_irq_deliver(uint32_t vector) IRAM_TEXT(iot_irq_deliver);
void iot_irq_deliver(uint32_t vector)
{
    uint32_t cpu = cpu_get_mhartid();
    uint32_t prio = intc_get_interrupt_prio(cpu);
    uint32_t cur_thd = intc_get_interrupt_threshold(cpu);

    if (prio < (IOT_INTR_PRI_MAX - 1)) {
        // TODO: As so far, only allow prio_level 7 to preempt other irqs
        // enter_interrupt_nest(prio + 1);
        enter_interrupt_nest(IOT_INTR_PRI_7);
        iot_irq_deliver_isr(vector);
        exit_interrupt_nest(cur_thd);
    } else {
        iot_irq_deliver_isr(vector);
    }
}

void machine_external_interrupt_handler(void) IRAM_TEXT(machine_external_interrupt_handler);
void machine_external_interrupt_handler(void)
{
    uint32_t cpu = cpu_get_mhartid();
#ifdef DEBUG_CHECK_INTERRUPT_STORM
    uint32_t count = 0;
#endif

#ifdef CHECK_ISR_FLASH
    iot_soc_cpu_access_enable(IOT_SOC_CPU_ACCESS_DTOP_FLASH, false);
#endif

    while (intc_get_interrupt_valid(cpu)) {
        iot_irq_deliver(intc_get_interrupt_id(cpu));
#ifdef DEBUG_CHECK_INTERRUPT_STORM
        count++;
        if (count > IOT_IRQ_INTERRUPT_STORM_MAX) {
            assert(0);
        }
#endif
    }

#ifdef CHECK_ISR_FLASH
    iot_soc_cpu_access_enable(IOT_SOC_CPU_ACCESS_DTOP_FLASH, true);
#endif
}

iot_irq_t iot_irq_create(uint32_t vector, uint32_t data, iot_isr_t isr)
{
    iot_isr_handlers[vector].isr = isr;
    iot_isr_handlers[vector].param = data;
    iot_isr_handlers[vector].priority = irq_int_priority[vector];
    iot_isr_handlers[vector].vector = (uint8_t)vector;

    intc_set_priority(cpu_get_mhartid(), vector, iot_isr_handlers[vector].priority);

    return (iot_irq_t)&iot_isr_handlers[vector];
}

void iot_irq_set_data(uint32_t vector, uint32_t data) IRAM_TEXT(iot_irq_set_data);
void iot_irq_set_data(uint32_t vector, uint32_t data)
{
    iot_isr_handlers[vector].param = data;
}

void iot_irq_delete(iot_irq_t irq)
{
    isr_handler_t *isr = (isr_handler_t *)irq;

    if (isr == NULL) {
        return;
    }

    intc_set_priority(cpu_get_mhartid(), isr->vector, IOT_INTR_PRI_0);

    isr->isr = hal_default_isr;
    isr->param = 0;
    isr->priority = IOT_INTR_PRI_0;
}

void iot_irq_mask(iot_irq_t irq) IRAM_TEXT(iot_irq_mask);
void iot_irq_mask(iot_irq_t irq)
{
    isr_handler_t *isr = (isr_handler_t *)irq;

    if (isr == NULL) {
        return;
    }

    uint32_t cpu = cpu_get_mhartid();
    uint32_t vector = isr->vector;
    uint32_t old_ints;

    old_ints = cpu_disable_irq();
    intc_mask(cpu, vector);
    cpu_restore_irq(old_ints);
}

void iot_irq_unmask(iot_irq_t irq) IRAM_TEXT(iot_irq_unmask);
void iot_irq_unmask(iot_irq_t irq)
{
    isr_handler_t *isr = (isr_handler_t *)irq;

    if (isr == NULL) {
        return;
    }

    uint32_t cpu = cpu_get_mhartid();
    uint32_t vector = isr->vector;
    uint32_t old_ints;

    old_ints = cpu_disable_irq();
    intc_unmask(cpu, vector);
    cpu_restore_irq(old_ints);
}

void iot_irq_acknowledge(iot_irq_t irq)
{
    UNUSED(irq);
}

void iot_irq_configure(iot_irq_t irq, uint32_t level, uint32_t up)
{
    UNUSED(irq);
    UNUSED(level);
    UNUSED(up);
}

void iot_irq_priority(iot_irq_t irq, uint32_t priority)
{
    UNUSED(irq);
    UNUSED(priority);
}

void iot_irq_set_cpu(iot_irq_t irq, uint32_t cpu)
{
    UNUSED(irq);
    UNUSED(cpu);
}

uint32_t iot_irq_get_cpu(iot_irq_t irq)
{
    UNUSED(irq);
    return 0;
}

uint32_t iot_irq_init(void)
{
    uint32_t cpu = cpu_get_mhartid();

#ifdef LOW_POWER_ENABLE
    iot_dev_pm_node_init(&tick_pm);
#endif
    intc_init(cpu);
    for (uint8_t i = 0; i < CHIP_IRQ_VECTOR_MAX; i++) {
        iot_isr_handlers[i].isr = hal_default_isr;
        iot_isr_handlers[i].param = 0;
        iot_isr_handlers[i].priority = IOT_INTR_PRI_0;
        iot_isr_handlers[i].vector = i;
    }

    cpu_enable_exirq();

    return 0;
}

uint32_t iot_systick_restore(uint32_t data)
{
    iot_systick_init(data);
    return RET_OK;
}

void iot_systick_init(uint32_t rate)
{
    uint32_t cpu = cpu_get_mhartid();
    uint32_t clock = IOT_IRQ_SYS_TICK_CLOCK;

    intc_systick_config(cpu, clock, rate);

#ifdef LOW_POWER_ENABLE
    tick_pm.data = rate;
    tick_pm.restore = iot_systick_restore;
    iot_dev_pm_register(&tick_pm);
#endif
    intc_systick_enable(cpu);
    cpu_enable_timer_irq();
}

void iot_systick_enable(void)
{
}

void iot_systick_clear(void) IRAM_TEXT(iot_systick_clear);
void iot_systick_clear(void)
{
    uint32_t cpu = cpu_get_mhartid();
    intc_systick_clear(cpu);
#ifdef BUILD_CORE_CORE0
    /** War for dig gpio interrupt */
    uint32_t unused = *(volatile uint32_t *)0x01440000;
    UNUSED(unused);
#endif
}

void machine_timer_interrupt_handler(void) IRAM_TEXT(machine_timer_interrupt_handler);
void machine_timer_interrupt_handler(void)
{
    iot_systick_clear();
}

void iot_irq_assert_dump(void *fun)
{
    (void)fun;
#ifdef CHECK_ISR_USAGE
    typedef void (*iot_debug_log_print_t)(char *fmt, ...);
    iot_debug_log_print_t func = (iot_debug_log_print_t)fun;

    func("----- os_cpu_usage_assert_dump_isr -----\n");
    func("ISR/Critical time top%d:\n", MAX_SAVE_ISR_CNT);
    func("Top\tTime/us\tVector\tCritical-Time/us\tra\tOS-timer/us\tra\r\n");
    func("----------------------------------------------\n");
    for (int i = 0; i < MAX_SAVE_ISR_CNT; i++) {
        func("%d\t%d\t0%d\t\t%d\t0x%x\t\t%d\t0x%x\n", i, iot_isr_usage.isr[i].dur_us,
             iot_isr_usage.isr[i].vector_ra, iot_isr_usage.critical[i].dur_us,
             iot_isr_usage.critical[i].vector_ra, iot_isr_usage.timer[i].dur_us,
             iot_isr_usage.timer[i].vector_ra);
    }
    //display ISR duty
    uint32_t now = iot_rtc_get_global_time_ms();
    uint32_t display_duration = (now - iot_isr_usage.last_display_time) * 1000;
    iot_isr_usage.isr_duty = 100 * iot_isr_usage.isr_time_total_us / display_duration;
    func("ISR duty = %d%%\n", iot_isr_usage.isr_duty);
    iot_isr_usage.last_display_time = now;
    iot_isr_usage.isr_time_total_us = 0;
#endif
}

uint32_t iot_irq_get_software_int_status(void) IRAM_TEXT(iot_irq_get_software_int_status);
uint32_t iot_irq_get_software_int_status(void)
{
    return intc_get_software_int_status(cpu_get_mhartid());
}

uint32_t iot_irq_get_wakeup_source(void)
{
    return intc_get_wakeup_source();
}

uint32_t iot_irq_get_wakeup_state(void)
{
    return intc_get_wakeup_state();
}
