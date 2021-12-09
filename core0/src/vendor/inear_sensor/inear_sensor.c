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
#include "inear_sensor.h"
#include "wq_touch/inear_wq_touch.h"
#include "hy2751/hy2751.h"

#if INEAR_DRIVER_SELECTION != INEAR_DRIVER_CUSTOMIZE

void inear_sensor_init(inear_callback_t callback)
{
#if INEAR_DRIVER_SELECTION == INEAR_DRIVER_SIMPLE_IO
#elif INEAR_DRIVER_SELECTION == INEAR_DRIVER_DEBOUNCE_IO
#elif INEAR_DRIVER_SELECTION == INEAR_DRIVER_WQ_TOUCH
    inear_wuqi_touch_init(callback);
#elif INEAR_DRIVER_SELECTION == INEAR_DRIVER_LIGHT_HY2751
    inear_hy2751_init(callback);
#else
    callback(true);
#endif
}

void inear_sensor_deinit(void)
{
#if INEAR_DRIVER_SELECTION == INEAR_DRIVER_SIMPLE_IO
#elif INEAR_DRIVER_SELECTION == INEAR_DRIVER_DEBOUNCE_IO
#elif INEAR_DRIVER_SELECTION == INEAR_DRIVER_WQ_TOUCH
    inear_wuqi_touch_deinit();
#elif INEAR_DRIVER_SELECTION == INEAR_DRIVER_LIGHT_HY2751
    inear_hy2751_deinit();
#else
#endif
}

void inear_sensor_open(void)
{
#if INEAR_DRIVER_SELECTION == INEAR_DRIVER_SIMPLE_IO
#elif INEAR_DRIVER_SELECTION == INEAR_DRIVER_DEBOUNCE_IO
#elif INEAR_DRIVER_SELECTION == INEAR_DRIVER_WQ_TOUCH
    inear_wuqi_touch_open();
#else

#endif
}

void inear_sensor_cfg_reset(void)
{
#if INEAR_DRIVER_SELECTION == INEAR_DRIVER_SIMPLE_IO
#elif INEAR_DRIVER_SELECTION == INEAR_DRIVER_DEBOUNCE_IO
#elif INEAR_DRIVER_SELECTION == INEAR_DRIVER_WQ_TOUCH
    inear_wuqi_touch_cfg_reset();
#else

#endif
}

#endif   //INEAR_DRIVER_SELECTION != INEAR_DRIVER_CUSTOMIZE
