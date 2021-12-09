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

#ifndef LIB_SYSCALL_H
#define LIB_SYSCALL_H

/**
 * @addtogroup OS_SHIM
 * @{
 * @addtogroup OS_SYSCALL
 * @{
 * This section introduces the SYSCALL module's functions and how to use this driver.
 */

void os_yield(void);
void os_suspend(void);
int32_t syscall_handler(void *sp);

/**
 * @}
 * @}
 */
#endif
