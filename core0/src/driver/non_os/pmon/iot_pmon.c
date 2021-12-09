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
#include "iot_pmon.h"
#include "pmon.h"

uint32_t iot_pmon_get_score(void)
{
    uint32_t aver_count;
    pmon_enable();
    aver_count = pmon_get_aver_count();
    pmon_disable();
    return aver_count;
}
