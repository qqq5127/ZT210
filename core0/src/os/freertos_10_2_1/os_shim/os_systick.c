/****************************************************************************

Copyright(c) 2016 by WuQi Technologies. ALL RIGHTS RESERVED.

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
#include "os_systick.h"

static os_systick_init_callback tick_init_cb;
static os_systick_clear_callback tick_clear_cb;

void vPortSystickInit(uint32_t rate)
{
    if (tick_init_cb != NULL) {
        tick_init_cb(rate);
    }
}

void vPortSystickEnable(void)
{

}

void vPortSysTickClear(void)
{
    if (tick_clear_cb != NULL) {
        tick_clear_cb();
    }
}

uint8_t os_systick_register_callback(os_systick_init_callback init_cb,
                                     os_systick_clear_callback clear_cb)
{
    tick_init_cb = init_cb;
    tick_clear_cb = clear_cb;

    return RET_OK;
}
