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

#ifndef _DRIVER_HW_SPINLOCK_H
#define _DRIVER_HW_SPINLOCK_H

#ifdef __cplusplus
extern "C" {
#endif

#include "types.h"

typedef enum {
    ATOMIC_SMP_LOCK,
    PMM_WAKEUP_RTC_LOCK,
    SUSPEND_TASK_LOCK,
    SPINLOCK_ID_MAX = 16,
} DEV_LOCK_ID;

unsigned long dev_spinlock_lock(DEV_LOCK_ID id);
void dev_spinlock_unlock(DEV_LOCK_ID id, unsigned long level);
int dev_spinlock_init(void);

#ifdef __cplusplus
}
#endif

#endif /* _DRIVER_HW_SPINLOCK_H */
