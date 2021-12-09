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

#ifndef _DRIVER_NON_OS_IRQ_H
#define _DRIVER_NON_OS_IRQ_H

/**
 * @addtogroup HAL
 * @{
 * @addtogroup IRQ
 * @{
 * This section introduces the IRQ module's enum, structure, functions and how to use this driver.
 */

#include "types.h"
#include "chip_irq_vector.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_SAVE_ISR_CNT    5

/** @brief IRQ priority. */
typedef enum {
    IOT_INTR_PRI_0 = 0,
    IOT_INTR_PRI_1,
    IOT_INTR_PRI_2,
    IOT_INTR_PRI_3,
    IOT_INTR_PRI_4,
    IOT_INTR_PRI_5,
    IOT_INTR_PRI_6,
    IOT_INTR_PRI_7,

    IOT_INTR_PRI_MAX
} IOT_INTR_PRIORITY;

/** @brief IRQ cpu core. */
typedef enum {
    IOT_INTR_CPU_0 = 0,
    IOT_INTR_CPU_1,
    IOT_INTR_CPU_2,
    IOT_INTR_CPU_MAX
} IOT_INTR_CPU;

typedef uint32_t iot_irq_t;

/** @brief interrupt service routine prototype. */
typedef uint32_t (*iot_isr_t)(uint32_t vector, uint32_t data);

/** @brief ISR's data save structure. */
typedef struct
{
    uint32_t vector_ra;
    uint16_t dur_us;
}isr_usage_param_t;

/** @brief ISR usage manage data struct. */
typedef struct
{
    isr_usage_param_t critical[MAX_SAVE_ISR_CNT];
    isr_usage_param_t isr[MAX_SAVE_ISR_CNT];
    isr_usage_param_t timer[MAX_SAVE_ISR_CNT];
    uint64_t  global_int_disable_time;
    uint32_t  last_display_time;
    uint32_t  isr_time_total_us;
    uint8_t   isr_duty;
}iot_isr_usage_t;

//data of iot_isr_usage
extern iot_isr_usage_t iot_isr_usage;

/**
 * @brief This function is to check os timer handler duration and insert to queue.
 *
 * @param[in] duration_us Duration of ISR (us).
 * @param[in] vector_ra ISR vector or ra regisiter.
 */
void iot_irq_timer_usage_insert(uint16_t duration_us, uint32_t vector_ra);

/**
 * @brief This function is to check this duration and insert to queue.
 *
 * @param[in] duration_us Duration of ISR (us).
 * @param[in] vector_ra ISR vector or ra regisiter.
 */
void iot_irq_usage_insert(uint16_t duration_us, uint32_t vector_ra);

/**
 * @brief This function is to check isr duration and insert to queue.
 *
 * @param[in] duration_us Duration of ISR (us).
 * @param[in] vector_ra ISR vector or ra regisiter.
 */
void iot_irq_deliver_usage_insert(uint16_t duration_us, uint32_t vector_ra);

/**
 * @brief This function is an interrupt handler for exterbal machine.
 *
 */
void machine_external_interrupt_handler(void);

/**
 * @brief This function is to create an interrupt for vector.
 *
 * @param vector Interrupt vector.
 * @param data ISR's data.
 * @param isr ISR function.
 * @return iot_irq_t success for interrupt handle else NULL.
 */
iot_irq_t iot_irq_create(uint32_t vector, uint32_t data, iot_isr_t isr);

/**
 * @brief This function is to set interrupt handler parameter.
 *
 * @param vector Tnterrupt vector.
 * @param data The value of the parameter.
 */
void iot_irq_set_data(uint32_t vector, uint32_t data);

/**
 * @brief This function is to delete an interrupt for vector.
 *
 * @param irq Interrupt handle.
 */
void iot_irq_delete(iot_irq_t irq);

/**
 * @brief This function is to mask this interrupt.
 *
 * @param irq Interrupt handle.
 */
void iot_irq_mask(iot_irq_t irq);

/**
 * @brief This function is to unmask this interrupt.
 *
 * @param irq Interrupt handle.
 */
void iot_irq_unmask(iot_irq_t irq);

/**
 * @brief This function is to acknowledge this interrupt.
 *
 * @param irq Interrupt handle.
 */
void iot_irq_acknowledge(iot_irq_t irq);

/**
 * @brief This function is to configure this interrupt.
 *
 * @param irq Interrupt handle.
 * @param level Level trigger.
 * @param up Edge trigger.
 */
void iot_irq_configure(iot_irq_t irq, uint32_t level, uint32_t up);

/**
 * @brief This function is to set this interrupt's priority.
 *
 * @param irq Interrupt handle.
 * @param priority
 */
void iot_irq_priority(iot_irq_t irq, uint32_t priority);

/**
 * @brief This function is to set cpu's id for this interrupt.
 *
 * @param irq Interrupt handle.
 * @param cpu is the cpu id to set.
 */
void iot_irq_set_cpu(iot_irq_t irq, uint32_t cpu);

/**
 * @brief This function is to get cpu's id for this interrupt.
 *
 * @param irq Interrupt handle.
 * @return uint32_t is cpu id.
 */
uint32_t iot_irq_get_cpu(iot_irq_t irq);

/**
 * @brief This function is to init interrupt controller and install default isr.
 *
 * @return uint32_t 0 for success else error.
 */
uint32_t iot_irq_init(void);

/**
 * @brief This function is to init systick.
 *
 * @param rate is the rate for systick to set.
 */
void iot_systick_init(uint32_t rate);

/**
 * @brief This function is to enable the systick.
 *
 */
void iot_systick_enable(void);

/**
 * @brief This function is to clear the systick.
 *
 */
void iot_systick_clear(void);

/**
 * @brief This function is an interrupt handler for timer machine.
 *
 */
void machine_timer_interrupt_handler(void);

/**
 * @brief This function is to deliver irq.
 *
 * @param vector Interrupt vector.
 */
void iot_irq_deliver(uint32_t vector);

/**
 * @brief This function is to restore systick.
 *
 * @param data is the data to restore.
 * @return uint32_t RET_OK for success else error.
 */
uint32_t iot_systick_restore(uint32_t data);

/**
 * @brief This function is to dump current isr top10.
 *
 * @param fun is the dump function.
 */
void iot_irq_assert_dump(void* fun);

/**
 * @brief This function is to return soft-irq status.
 *
 */
uint32_t iot_irq_get_software_int_status(void);

uint32_t iot_irq_get_wakeup_source(void);

/**
 * @brief This function is to return wakeup state.
 *
 */
uint32_t iot_irq_get_wakeup_state(void);

#ifdef __cplusplus
}
#endif

/**
 * @}
 * addtogroup IRQ
 * @}
 * addtogroup HAL
 */

#endif /* _DRIVER_NON_OS_IRQ_H */
