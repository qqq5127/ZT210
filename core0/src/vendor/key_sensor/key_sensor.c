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
#include "key_sensor.h"
#include "key_base.h"
#include "debounce_io/key_debounce_io.h"
#include "wq_touch/key_wq_touch.h"
#include "simple_io/key_simple_io.h"
#include "sc7a20/sc7a20.h"
#include "df100/df100.h"
#include "df230/df230.h"
#include "ext_touch/key_ext_touch.h"
#include "aw8686x/aw8686x_demo.h"

#if KEY_DRIVER_SELECTION != KEY_DRIVER_CUSTOMIZE

void key_sensor_init(const key_id_cfg_t *key_id, key_callback_t callback)
{
    UNUSED(key_id);
    UNUSED(callback);

#if KEY_DRIVER_SELECTION == KEY_DRIVER_SIMPLE_IO
    key_base_init(callback);
    key_simple_io_init();
#elif KEY_DRIVER_SELECTION == KEY_DRIVER_DEBOUNCE_IO
    key_base_init(callback);
    key_debounce_io_init();
#elif KEY_DRIVER_SELECTION == KEY_DRIVER_WQ_TOUCH
    key_base_init(callback);
    key_wq_touch_init(key_id);
#elif KEY_DRIVER_SELECTION == KEY_DRIVER_SC7A20
    sc7a20_init(callback);
#elif KEY_DRIVER_SELECTION == KEY_DRIVER_DF100
    df100_init(callback);
#elif KEY_DRIVER_SELECTION == KEY_DRIVER_DF230
    df230_init(callback);
#elif KEY_DRIVER_SELECTION == KEY_DRIVER_EXT_TOUCH
    key_base_init(callback);
    key_ext_touch_init();
#elif KEY_DRIVER_SELECTION == KEY_DRIVER_NDT_PT135
	pt135_init(callback);	
#elif KEY_DRIVER_SELECTION == KEY_DRIVER_AW8686X
	aw8686x_init(callback);
#else
#endif
}

void key_sensor_deinit(bool_t wakeup_enable)
{
#if KEY_DRIVER_SELECTION == KEY_DRIVER_SIMPLE_IO
    key_base_deinit();
    key_simple_io_deinit(wakeup_enable);
#elif KEY_DRIVER_SELECTION == KEY_DRIVER_DEBOUNCE_IO
    key_base_deinit();
    key_debounce_io_deinit(wakeup_enable);
#elif KEY_DRIVER_SELECTION == KEY_DRIVER_WQ_TOUCH
    key_base_deinit();
    key_wq_touch_deinit(wakeup_enable);
#elif KEY_DRIVER_SELECTION == KEY_DRIVER_SC7A20
    sc7a20_deinit(wakeup_enable);
#elif KEY_DRIVER_SELECTION == KEY_DRIVER_DF100
    df100_deinit(wakeup_enable);
#elif KEY_DRIVER_SELECTION == KEY_DRIVER_DF230
    df230_deinit(wakeup_enable);
#elif KEY_DRIVER_SELECTION == KEY_DRIVER_EXT_TOUCH
    key_base_deinit();
    key_ext_touch_deinit(wakeup_enable);
#elif KEY_DRIVER_SELECTION == KEY_DRIVER_NDT_PT135
	pt135_deinit(wakeup_enable);
#elif KEY_DRIVER_SELECTION == KEY_DRIVER_AW8686X
	aw8686x_deinit(wakeup_enable);
#else
    UNUSED(wakeup_enable);
#endif
}

void key_sensor_open(const key_cfg_t *key_cfg, bool_t can_power_on)
{
    UNUSED(can_power_on);
#if KEY_DRIVER_SELECTION == KEY_DRIVER_SIMPLE_IO
    key_base_set_time_cfg(&key_cfg->time);
    key_simple_io_open(&key_cfg->id, &key_cfg->time);
#elif KEY_DRIVER_SELECTION == KEY_DRIVER_DEBOUNCE_IO
    key_base_set_time_cfg(&key_cfg->time);
    key_debounce_io_open(&key_cfg->id, &key_cfg->time);
#elif KEY_DRIVER_SELECTION == KEY_DRIVER_WQ_TOUCH
    key_base_set_time_cfg(&key_cfg->time);
    key_wq_touch_open(&key_cfg->thres, can_power_on);
#elif KEY_DRIVER_SELECTION == KEY_DRIVER_SC7A20
    sc7a20_set_param(&key_cfg->id, &key_cfg->time);
#elif KEY_DRIVER_SELECTION == KEY_DRIVER_DF100
    UNUSED(key_cfg);
#elif KEY_DRIVER_SELECTION == KEY_DRIVER_DF230
    UNUSED(key_cfg);
#elif KEY_DRIVER_SELECTION == KEY_DRIVER_EXT_TOUCH
    key_base_set_time_cfg(&key_cfg->time);
    key_ext_touch_open(&key_cfg->id, &key_cfg->time);
#elif KEY_DRIVER_SELECTION == KEY_DRIVER_NDT_PT135
    UNUSED(key_cfg);
#elif KEY_DRIVER_SELECTION == KEY_DRIVER_AW8686X
	UNUSED(key_cfg);
#else
    UNUSED(key_cfg);
#endif
}

bool_t key_sensor_all_key_released(void)
{
#if KEY_DRIVER_SELECTION == KEY_DRIVER_SIMPLE_IO
    return key_base_all_key_released();
#elif KEY_DRIVER_SELECTION == KEY_DRIVER_DEBOUNCE_IO
    return key_base_all_key_released();
#elif KEY_DRIVER_SELECTION == KEY_DRIVER_WQ_TOUCH
    return key_base_all_key_released();
#elif KEY_DRIVER_SELECTION == KEY_DRIVER_SC7A20
    return true;
#elif KEY_DRIVER_SELECTION == KEY_DRIVER_DF100
    return true;
#elif KEY_DRIVER_SELECTION == KEY_DRIVER_DF230
    return true;
#elif KEY_DRIVER_SELECTION == KEY_DRIVER_EXT_TOUCH
    return key_base_all_key_released();
#elif KEY_DRIVER_SELECTION == KEY_DRIVER_NDT_PT135
    return true;
#elif KEY_DRIVER_SELECTION == KEY_DRIVER_AW8686X
	return true;
#else
    return true;
#endif
}

bool_t key_sensor_is_key_pressed(uint8_t key_id, uint8_t key_src)
{
    UNUSED(key_id);
    UNUSED(key_src);
#if KEY_DRIVER_SELECTION == KEY_DRIVER_SIMPLE_IO
    return key_base_is_key_pressed(key_id, key_src);
#elif KEY_DRIVER_SELECTION == KEY_DRIVER_DEBOUNCE_IO
    return key_base_is_key_pressed(key_id, key_src);
#elif KEY_DRIVER_SELECTION == KEY_DRIVER_WQ_TOUCH
    return key_base_is_key_pressed(key_id, key_src);
#elif KEY_DRIVER_SELECTION == KEY_DRIVER_SC7A20
    return false;
#elif KEY_DRIVER_SELECTION == KEY_DRIVER_DF100
    return false;
#elif KEY_DRIVER_SELECTION == KEY_DRIVER_DF230
    return false;
#elif KEY_DRIVER_SELECTION == KEY_DRIVER_EXT_TOUCH
    return key_base_is_key_pressed(key_id, key_src);
#elif KEY_DRIVER_SELECTION == KEY_DRIVER_NDT_PT135
    return false;
#elif KEY_DRIVER_SELECTION == KEY_DRIVER_AW8686X
	return false;
#else
    return false;
#endif
}

void key_sensor_set_enabled(bool_t enable)
{
    UNUSED(enable);
#if KEY_DRIVER_SELECTION == KEY_DRIVER_SIMPLE_IO
#elif KEY_DRIVER_SELECTION == KEY_DRIVER_DEBOUNCE_IO
#elif KEY_DRIVER_SELECTION == KEY_DRIVER_WQ_TOUCH
    key_wq_touch_set_enable(enable);
#elif KEY_DRIVER_SELECTION == KEY_DRIVER_SC7A20
#elif KEY_DRIVER_SELECTION == KEY_DRIVER_DF100
#elif KEY_DRIVER_SELECTION == KEY_DRIVER_DF230
#elif KEY_DRIVER_SELECTION == KEY_DRIVER_NDT_PT135
#elif KEY_DRIVER_SELECTION == KEY_DRIVER_EXT_TOUCH
    key_ext_touch_set_enabled(enable);
#else
    return;
#endif
}

#endif   //KEY_DRIVER_SELECTION != KEY_DRIVER_CUSTOMIZE
