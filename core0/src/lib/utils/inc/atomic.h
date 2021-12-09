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

#ifndef _BSP_ATOMIC_H
#define _BSP_ATOMIC_H

#include "spinlock.h"

#ifdef __cplusplus
extern "C" {
#endif

// #define SPINLOCK_DEBUG

/* Defination of memory barrier macro */
#define mb() { asm volatile("fence" ::: "memory"); }

typedef struct _spinlock {
    int locked;

#ifdef SPINLOCK_DEBUG
#define DEBUG_PCS   16
    // For debugging:
    char *name;                 // Name of lock.
    int cpu;
    uintptr_t pcs[DEBUG_PCS];   // The call stack (an array of program counters) that locked the lock.
#endif
} spinlock_t;

#ifdef SPINLOCK_DEBUG
void __spin_initlock(spinlock_t *lock, char *name);
#define spin_initlock(lock)     __spin_initlock(lock, #lock)
#else
/**
 * @brief This function is to init spinlock.
 *
 * @param lock is spinlock
 */
void spin_initlock(spinlock_t *lock);
#endif

/**
 * @brief This function is to lock a spinlock.
 *
 * @param lock is spinlock
 */
void spinlock_lock(spinlock_t *lock);

/**
 * @brief This function is to unlock a spinlock.
 *
 * @param lock is spinlock
 */
void spinlock_unlock(spinlock_t *lock);

#ifdef __cplusplus
}
#endif

#endif /* _BSP_ATOMIC_H */
