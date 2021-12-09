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

#include "os_syscall_num.h"

#define SYS_ENTRY(name)     \
    .global os_##name;      \
    os_##name:              \
        li a6, SYS_##name;  \
        ecall;              \
        ret;

.text
    SYS_ENTRY(yield);
    SYS_ENTRY(suspend);
