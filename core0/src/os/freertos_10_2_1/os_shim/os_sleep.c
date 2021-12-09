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

/* FreeRTOS includes */
#include "FreeRTOS.h"
#include "task.h"

/* os shim includes */
#include "os_sleep.h"

/* common includes */

OS_SLEEP_STATUS os_get_sleep_status(void) IRAM_TEXT(os_get_sleep_status);
OS_SLEEP_STATUS os_get_sleep_status(void)
{
    return (OS_SLEEP_STATUS)eTaskConfirmSleepModeStatus();
}

void os_tick_compensate(const uint32_t tick_cnt) IRAM_TEXT(os_tick_compensate);
void os_tick_compensate(const uint32_t tick_cnt)
{
    vTaskStepTick(tick_cnt);
}
