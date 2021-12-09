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
#include "bt_rpc_api.h"
#ifndef BUILD_ROM_LIB
#include "rpc_caller.h"
#endif

bt_result_t bt_handle_user_cmd(bt_cmd_t cmd, void *param, uint32_t param_len)
{
    assert(param);

    return rpc_bt_handle_user_cmd((uint32_t)cmd, (uint32_t)param, param_len);
}

void bt_handle_user_shutdown(void)
{
    rpc_bt_handle_user_shutdown();
}
