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

#include "os_sys_restore.h"
#include "riscv_cpu.h"
#include "os_hook.h"

/*lint -esym(526, processed_source) defined at portASM.S */
extern void processed_source(void);

void os_sys_suspend(void)
{
    os_hook_set_sleep_flags(1);
    save_cpu_context();
    if (vPortSaveHook() == RET_OK) {
        cpu_enter_wfi();
    }
    os_hook_sleep_fail_handler();
    // for the sleep not enter case
    os_hook_set_sleep_flags(0);
}

void os_sys_restore(void)
{
    restore_cpu_context();
    vPortRestoreHook();
#ifdef LOW_POWER_ENABLE
    if (os_hook_get_sleep_flags() == 0) {
        vPortFailedDumpHook();
    }
#endif
    //assert(os_hook_get_sleep_flags() == 1);
    os_hook_set_sleep_flags(0);
    processed_source();
    // never return
}
