/****************************************************************************

Copyright(c) 2021 by WuQi Technologies. ALL RIGHTS RESERVED.

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
#include "riscv_cpu.h"

#include "dfs.h"

uint32_t dfs_start(DFS_OBJ obj, uint32_t timeout_ms)
{
    return dfs_request_impl(cpu_get_mhartid(), obj, DFS_ACT_START, timeout_ms);
}

uint32_t dfs_stop(DFS_OBJ obj)
{
    return dfs_request_impl(cpu_get_mhartid(), obj, DFS_ACT_STOP, 0);
}
