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

/**
 * @addtogroup APP
 * @{
 */

/**
 * @addtogroup APP_WQOTA
 * @{
 * This section introduces the APP WQOTA module's enum, structure, functions and how to use this module.
 */

#ifndef _APP_WQOTA_H_
#define _APP_WQOTA_H_
/**
 * @brief easy connection module
 */

#include "types.h"
#include "userapp_dbglog.h"

#ifndef WQOTA_ENABLED
#define WQOTA_ENABLED 1
#endif

#if WQOTA_ENABLED
/**
 * @brief init the wq ota module
 */
void app_wqota_init(void);

/**
 * @brief deinit the wq ota module
 */
void app_wqota_deinit(void);

/**
 * @brief start ota
 */
void app_wqota_start_adv(void);

/**
 * @brief private function to handle tws state changed event
 *
 * @param connected true if connected, false if not
 */
void app_wqota_handle_tws_state_changed(bool_t connected);
#else
static inline void app_wqota_init(void)
{
}
static inline void app_wqota_deinit(void)
{
}
static inline void app_wqota_start_adv(void)
{
}

static inline void app_wqota_handle_tws_state_changed(bool_t connected)
{
    UNUSED(connected);
}
#endif

/**
 * @}
 * addtogroup APP_WQOTA
 */

/**
 * @}
 * addtogroup APP
 */

#endif   //_APP_WQOTA_H_
