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
 * @addtogroup APP_INEAR
 * @{
 * This section introduces the APP INEAR module's enum, structure, functions and how to use this module.
 */

#ifndef _APP_INEAR_H_
#define _APP_INEAR_H_
#include "types.h"
#include "userapp_dbglog.h"

#define DBGLOG_INEAR_DBG(fmt, ...) DBGLOG_USER_APP_INFO("[inear] " fmt, ##__VA_ARGS__)
#define DBGLOG_INEAR_ERR(fmt, ...) DBGLOG_USER_APP_ERROR("[inear] " fmt, ##__VA_ARGS__)

/**
 * @brief init the app inear module
 */
void app_inear_init(void);

/**
 * @brief deinit the app inear module
 */
void app_inear_deinit(void);

/**
 * @brief check if the earbud is currently in ear
 *
 * @return true for in ear, else for out of ear
 */
bool_t app_inear_get(void);

/**
 * @brief check if in ear detection is enabled
 *
 * @return true if enabled, false if not.
 */
bool_t app_inear_is_enabled(void);


/**
 * @brief enable/disable inear detection
 *
 * @param enabled true if enabled, false if not.
 */
void app_inear_set_enabled(bool_t enabled);

/**
 * @brief switch toggle enable/disable inear detection
 *
 */
void app_inear_enable_set_toggle(void);

/**
 * @brief factory reset the app inear cfg
 */
void app_inear_cfg_reset(void);

/**
 * @}
 * addtogroup APP_INEAR
 */

/**
 * @}
 * addtogroup APP
 */

#endif
