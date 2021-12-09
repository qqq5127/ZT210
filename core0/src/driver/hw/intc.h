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
#ifndef _DRIVER_HW_INTC_H
#define _DRIVER_HW_INTC_H

#ifdef __cplusplus
extern "C" {
#endif

#include "types.h"

typedef enum {
    WAKEUP_WIC_DCORE2BT,
    WAKEUP_WIC_AUD2BT,
    WAKEUP_WIC_BT2DCORE,
    WAKEUP_WIC_AUD2DCORE,
    WAKEUP_WIC_BT2AUD,
    WAKEUP_WIC_DCORE2AUD,
    WAKEUP_RTC_TMR2_INT0,
    WAKEUP_RTC_TMR1_INT0,
    WAKEUP_WDG1_INT,
    WAKEUP_DM_SLP_INT,
    WAKEUP_PMM_GPIO_INT0,
    WAKEUP_PMM_GPIO_INT1,
    WAKEUP_PMM_WDG_DTOP_INT,
    WAKEUP_DIG_GPIO,
    WAKEUP_PMM_BOD_INT,
    WAKEUP_PMM_GPIO_DEB_INT_ALL,
    WAKEUP_CHARGER_ON_FLAG_INT,
    WAKEUP_TK_INT,
    WAKEUP_VBATT_RESUME_INT,
    WAKEUP_MAILBOX0_INT2,
    WAKEUP_MAILBOX0_INT1,
    WAKEUP_MAILBOX0_INT0,
    WAKEUP_VAD_TDVAD_INT,
    WAKEUP_BT_PD_EARLY_BY_BT_TMR,
    WAKEUP_WDG0_INT,
    WAKEUP_FLAG_VBUS_PTRN2P5_CHG_INT,
    WAKEUP_GPIO_DEB_INT_ALL,
    WAKEUP_PMM_RTC_TIMER_INT1,
    WAKEUP_PMM_RTC_TIMER_INT0,
    WAKEUP_I2C0_INT,
    WAKEUP_GPIO_WAKEUP,
    WAKEUP_RTC_TMR0_INT0,
} INTC_WAKEUP_SRC;

uint32_t intc_get_interrupt_id(uint32_t cpu);

void intc_init(uint32_t cpu);

void intc_set_priority(uint32_t cpu, uint32_t vector, uint32_t priority);

void intc_mask(uint32_t cpu, uint32_t vector);

void intc_unmask(uint32_t cpu, uint32_t vector);

void intc_config(uint32_t cpu, uint32_t vector, uint32_t level, uint32_t up);

void intc_ack(uint32_t cpu, uint32_t vector);

void intc_handler(uint32_t cpu);

/*
 * intc_force_mtip - Force MTIP which used in OS's Yield function
*/
void intc_force_mtip(uint32_t cpu);

/*
 * intc_get_force_mtip - Get MTIP's force status
*/
uint32_t intc_get_force_mtip(uint32_t cpu);

/*
 * intc_clear_force_mtip - Clear force MTIP's interrupt status
*/
void intc_clear_force_mtip(uint32_t cpu);

/*
 * intc_systick_clear - Clear systick interrupt status
*/
void intc_systick_clear(uint32_t cpu);

/*
 * intc_systick_config - config OS's systick
 *
*/
void intc_systick_config(uint32_t cpu, uint32_t clock, uint32_t rate);

/*
 * intc_systick_enable -- enable OS's systick timer;
 *
*/
void intc_systick_enable(uint32_t cpu);

/*
 * intc_systick_disable -- enable OS's systick timer;
 *
*/
void intc_systick_disable(uint32_t cpu);

/*
 * intc_systick_update -- update OS's systick timer;
 *
*/
void intc_systick_update(uint32_t cpu, uint32_t clock);

uint32_t intc_get_software_int_status(uint32_t cpu);
void intc_send_software_int(uint32_t dst_cpu);
void intc_clear_software_int(uint32_t dst_cpu);
void intc_clear_all_software_int(void);
void intc_set_wakeup_enable(INTC_WAKEUP_SRC wakeup_src);
void intc_set_wakeup_disable(INTC_WAKEUP_SRC wakeup_src);
uint32_t intc_get_wakeup_state(void);
uint32_t intc_save(uint32_t data);
uint32_t intc_restore(uint32_t data);
uint32_t intc_get_interrupt_prio(uint32_t cpu);
uint32_t intc_get_interrupt_threshold(uint32_t cpu);
void intc_set_interrupt_threshold(uint32_t cpu, uint32_t thd);
uint32_t intc_get_wakeup_source(void);
uint32_t intc_get_interrupt_valid(uint32_t cpu);
uint32_t intc_get_source(uint32_t irq);
uint32_t intc_get_status(uint32_t irq);
uint32_t intc_get_enable(uint32_t irq);
uint32_t intc_get_prio_config(uint32_t irq);
void intc_get_all_status(uint32_t *record);

#ifdef __cplusplus
}
#endif

#endif /* _DRIVER_HW_INTC_H */
