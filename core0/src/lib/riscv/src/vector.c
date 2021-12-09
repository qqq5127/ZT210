/****************************************************************************
 *
 * Copyright(c) 2020 by WuQi Technologies. ALL RIGHTS RESERVED.
 *
 * This Information is proprietary to WuQi Technologies and MAY NOT
 * be copied by any method or incorporated into another program without
 * the express written consent of WuQi. This Information or any portion
 * thereof remains the property of WuQi. The Information contained herein
 * is believed to be accurate and WuQi assumes no responsibility or
 * liability for its use in any way and conveys no license or title under
 * any patent or copyright and makes no representation or warranty that this
 * Information is free from patent or copyright infringement.
 *
 * ****************************************************************************/

#include "riscv_cpu.h"
#include "vector.h"

/**
 * Default interrupt handler, used for when no specific handler has been implemented.
 * Needed for weak aliasing (an aliased function must have static linkage).
 */
static void isr_not_implemented(void) IRAM_TEXT(isr_not_implemented);
static void isr_not_implemented(void)
{
    volatile uint8_t i = 1;
    // An infinite loop, preserving the system state for examination by a debugger
    while (i) {
    }
}

/**
 * Weak aliased IRQ interrupt handlers
 */
/*lint -esym(526, supervisor_soft_interrupt_handler) */
/*lint -esym(526, hypervisor_soft_interrupt_handler) */
/*lint -esym(526, supervisor_timer_interrupt_handler) */
/*lint -esym(526, hypervisor_timer_interrupt_handler) */
/*lint -esym(526, supervisor_external_interrupt_handler) */
/*lint -esym(526, hypervisor_external_interrupt_handler) */
/*lint -esym(526, cop_interrupt_handler) */
/*lint -esym(526, host_interrupt_handler) */
void supervisor_soft_interrupt_handler(void) __attribute__((weak, alias("isr_not_implemented")));
void hypervisor_soft_interrupt_handler(void) __attribute__((weak, alias("isr_not_implemented")));
void machine_soft_interrupt_handler(void) __attribute__((weak, alias("isr_not_implemented")));
void supervisor_timer_interrupt_handler(void) __attribute__((weak, alias("isr_not_implemented")));
void hypervisor_timer_interrupt_handler(void) __attribute__((weak, alias("isr_not_implemented")));
void machine_timer_interrupt_handler(void) __attribute__((weak, alias("isr_not_implemented")));
void supervisor_external_interrupt_handler(void) __attribute__((weak, alias("isr_not_implemented")));
void hypervisor_external_interrupt_handler(void) __attribute__((weak, alias("isr_not_implemented")));
void machine_external_interrupt_handler(void) __attribute__((weak, alias("isr_not_implemented")));
void cop_interrupt_handler(void) __attribute__((weak, alias("isr_not_implemented")));
void host_interrupt_handler(void) __attribute__((weak, alias("isr_not_implemented")));

extern void machine_soft_interrupt_handler(void);
extern void machine_timer_interrupt_handler(void);
extern void machine_external_interrupt_handler(void);


/**
 * The interrupt vector table
 */
const isr_handler clint_vector_table[ISR_INTERRUPT_MAX] IRAM_RODATA(clint_vector_table)= {
    [ISR_EXCEPTION] = (isr_handler)trap_handler,
    [ISR_S_SOFT] = supervisor_soft_interrupt_handler,
    [ISR_H_SOFT] = hypervisor_soft_interrupt_handler,
#ifndef DISABLE_SOFT_IRQ
    [ISR_M_SOFT] = machine_soft_interrupt_handler,
#endif
    [RESERVED_0] = isr_not_implemented,
    [ISR_S_TIMER] = supervisor_timer_interrupt_handler,
    [ISR_H_TIMER] = hypervisor_timer_interrupt_handler,
    [ISR_M_TIMER] = machine_timer_interrupt_handler,
    [RESERVED_1] = isr_not_implemented,
    [ISR_S_EXT] = supervisor_external_interrupt_handler,
    [ISR_H_EXT] = hypervisor_external_interrupt_handler,
    [ISR_M_EXT] = machine_external_interrupt_handler,
    [ISR_COP] = cop_interrupt_handler,
    [ISR_HOST] = host_interrupt_handler,
};
