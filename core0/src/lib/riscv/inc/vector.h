#ifndef _VECTOR_H
#define _VECTOR_H

/**
 * Prototype for interrupt handler functions
 */
typedef void(*isr_handler)(void);

enum {
    ISR_EXCEPTION,
    ISR_S_SOFT,
    ISR_H_SOFT,
    ISR_M_SOFT,
    RESERVED_0,
    ISR_S_TIMER,
    ISR_H_TIMER,
    ISR_M_TIMER,
    RESERVED_1,
    ISR_S_EXT,
    ISR_H_EXT,
    ISR_M_EXT,
    ISR_COP,
    ISR_HOST,
    ISR_INTERRUPT_MAX,
};

#endif /* _VECTOR_H */
