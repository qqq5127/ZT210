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

#include "bt_rpc_api.h"

uint32_t rpc_user_handle_bt_evt(uint32_t evt, uint32_t param, uint32_t param_len);
void rpc_bt_mgnt_dummy(void);

uint32_t rpc_user_handle_bt_evt(uint32_t evt, uint32_t param, uint32_t param_len)
{
    return user_handle_bt_evt((bt_evt_t)evt, (void *)param, param_len);
}

void rpc_bt_mgnt_dummy(void)
{
    return;
}