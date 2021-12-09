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

#include "types.h"
#include "FreeRTOS.h"
#include "os_sys_restore.h"
#include "os_syscall_num.h"
#include "exception.h"

static int32_t syscall_dispatch(uint32_t syscallno,
                                uint32_t a0, uint32_t a1,
                                uint32_t a2, uint32_t a3,
                                uint32_t a4, uint32_t a5)
{
    UNUSED(a0);
    UNUSED(a1);
    UNUSED(a2);
    UNUSED(a3);
    UNUSED(a4);
    UNUSED(a5);

    switch (syscallno) {
    case SYS_yield:
        vTaskSwitchContext();
        break;

    case SYS_suspend:
        os_sys_suspend();
        break;

    default:
        return RET_INVAL;
    }

    return RET_OK;
}

int32_t syscall_handler(void *sp)
{
    trap_stack_registers_t *frame = sp;
    return syscall_dispatch(frame->a0_7[6], frame->a0_7[0], frame->a0_7[1], frame->a0_7[2],
                            frame->a0_7[3], frame->a0_7[4], frame->a0_7[5]);
}
