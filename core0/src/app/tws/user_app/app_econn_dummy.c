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

#include "app_econn.h"

/**
 * @brief some funnctions of econn wich is non-essential
 */

#ifdef ECONN

bool_t __attribute__((weak)) app_econn_handle_event_led(uint32_t event)
{
    UNUSED(event);
    return false;
}

bool_t __attribute__((weak)) app_econn_handle_state_led(uint32_t state)
{
    UNUSED(state);
    return false;
}

bool_t __attribute__((weak)) app_econn_handle_key_pressed(key_pressed_info_t *info)
{
    UNUSED(info);
    return false;
}

uint8_t __attribute__((weak)) app_econn_get_bat_level(void)
{
    return BAT_LEVEL_INVALID;
}

bool_t __attribute__((weak)) app_econn_handle_charger_evt(charger_evt_t evt, void *param)
{
    UNUSED(evt);
    UNUSED(param);
    return false;
}

uint16_t __attribute__((weak)) app_econn_get_vid(void)
{
    return ro_gen_cfg()->vid;
}

uint16_t __attribute__((weak)) app_econn_get_pid(void)
{
    return ro_gen_cfg()->pid;
}

uint8_t __attribute__((weak)) app_econn_get_tws_pair_magic(void)
{
    return WWS_DEFAULT_PAIR_MAGIC;
}

uint8_t __attribute__((weak)) app_econn_handle_volume(audio_volume_type_t type, uint8_t level)
{
    UNUSED(type);
    return level;
}

#endif   //def ECONN
