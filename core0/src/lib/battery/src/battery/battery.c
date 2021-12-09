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

#include "battery.h"
#include "iot_charger.h"

typedef struct {
    bool_t bat_bad;
} battery_info_t;

static battery_info_t bat_info;

uint16_t battery_get_voltage_mv(void)
{
    return iot_charger_get_vbat_mv();
}

void battery_set_bat_bad(bool_t bad)
{
    bat_info.bat_bad = bad;
}

bool_t battery_bat_is_bad(void)
{
    return bat_info.bat_bad;
}

void battery_init(void)
{
    bat_info.bat_bad = false;
    battery_charger_init();
}
