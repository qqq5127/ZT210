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
#include "lib_dbglog.h"
#include "string.h"
#include "os_mem.h"
#include "modules.h"
#include "bt_rpc_api.h"

static bt_evt_cb_t bt_evt_cb = NULL;
static bool_t bt_ready = false;

void bt_register_evt_cb(bt_evt_cb_t cb)
{
    bt_evt_cb = cb;
    if (bt_ready) {
        //bt ready when register, report to upper layer.
        bt_evt_enable_state_changed_t evt;
        evt.enabled = false;
        bt_evt_cb(BT_EVT_ENABLE_STATE_CHANGED, (void *)&evt, sizeof(evt));
    }
}

bt_result_t user_handle_bt_evt(bt_evt_t evt, void *param, uint32_t param_len)
{
    if ((evt == BT_EVT_ENABLE_STATE_CHANGED) && (!bt_ready)) {
        bt_ready = true;
    }

    if (bt_evt_cb) {
        return bt_evt_cb(evt, param, param_len);
    }

    return BT_RESULT_DISABLED;
}
