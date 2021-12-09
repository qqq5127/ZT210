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

#ifndef OS_SYS_RESTORE_H
#define OS_SYS_RESTORE_H

/**
 * @addtogroup OS_SHIM
 * @{
 * @addtogroup OS_SYS_RESTORE
 * @{
 * This section introduces the SYS_RESTORE module's functions and how to use this driver.
 */

#include "types.h"

/**
 * @brief This function is used to suspend system.
 *
 */
void os_sys_suspend(void);

/**
 * @brief This function is used to restore system.
 *
 */
void os_sys_restore(void);
void os_sys_save_restore_hook(void);

/**
 * @}
 * @}
 */
#endif
