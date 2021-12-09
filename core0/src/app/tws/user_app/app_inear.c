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

#include "app_inear.h"
#include "app_main.h"
#include "app_evt.h"
#include "inear_sensor.h"
#include "app_wws.h"
#include "app_econn.h"
#include "app_charger.h"
#include "usr_cfg.h"

#define APP_INEAR_MSG_ID_INEAR_CHANGED 1
#define APP_INEAR_MSG_ID_OPEN          2

static bool_t is_inear = false;

static void inear_change_callback(bool_t inear)
{
    app_send_msg(MSG_TYPE_INEAR, APP_INEAR_MSG_ID_INEAR_CHANGED, &inear, 1);
}

static void inear_state_handler(bool_t inear)
{
    is_inear = inear;

    if (!app_inear_is_enabled() || app_charger_is_charging()) {
        return;
    }

    app_wws_send_in_ear(app_inear_is_enabled(), is_inear);

    if (inear) {
        app_evt_send(EVTSYS_IN_EAR);
    } else {
        app_evt_send(EVTSYS_OUT_OF_EAR);
    }

    app_econn_handle_in_ear_changed(inear);
}

static void app_inear_handle_msg(uint16_t msg_id, void *param)
{
    switch (msg_id) {
        case APP_INEAR_MSG_ID_INEAR_CHANGED:
            inear_state_handler(*((bool_t *)param));
            break;
        case APP_INEAR_MSG_ID_OPEN:
            inear_sensor_open();
            break;

        default:
            break;
    }
}

bool_t app_inear_get(void)
{
    if (!app_inear_is_enabled()) {
        return true;
    } else {
        return is_inear;
    }
}

bool_t app_inear_is_enabled(void)
{
    return usr_cfg_get_inear_enabled();
}

void app_inear_set_enabled(bool_t enabled)
{
    DBGLOG_INEAR_DBG("set inear enabled %d \n", enabled);
    if (enabled != app_inear_is_enabled()) {
        usr_cfg_set_inear_enabled(enabled);
        app_evt_send((enabled == true) ? EVTSYS_INEAR_ENABLE : EVTSYS_INEAR_DISABLE);
    }

    if (app_wws_is_connected_master()) {
        app_wws_send_in_ear(enabled, is_inear);
    }
}

void app_inear_enable_set_toggle(void)
{
    if (app_wws_is_master()) {
        app_inear_set_enabled(!app_inear_is_enabled());
    } else if (app_wws_is_connected_slave()) {
        app_wws_send_in_ear_enable_request(!app_inear_is_enabled());
    }
}

void app_inear_init(void)
{
    DBGLOG_INEAR_DBG("app_inear init, enabled %d\n", usr_cfg_get_inear_enabled());
    app_register_msg_handler(MSG_TYPE_INEAR, app_inear_handle_msg);
    inear_sensor_init(inear_change_callback);

    app_send_msg(MSG_TYPE_INEAR, APP_INEAR_MSG_ID_OPEN, NULL, 0);
}

void app_inear_deinit(void)
{
    inear_sensor_deinit();
}

void app_inear_cfg_reset(void)
{
    inear_sensor_cfg_reset();
}
