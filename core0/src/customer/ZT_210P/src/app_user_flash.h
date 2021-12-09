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
 * @addtogroup _APP_USER_SPP_H_
 * @{
 * This section introduces the APP _APP_USER_SPP_H_ module's enum, structure, functions and how to use this module.
 */

#ifndef _APP_USER_FLASH_H_
#define _APP_USER_FLASH_H_
#include "types.h"
#include "userapp_dbglog.h"

#define DBGLOG_USER_FLASH_DBG(fmt, ...) DBGLOG_USER_APP_INFO("[user flash] " fmt, ##__VA_ARGS__)
#define DBGLOG_USER_FLASH_ERR(fmt, ...) DBGLOG_USER_APP_ERROR("[user flash] " fmt, ##__VA_ARGS__)

/** @defgroup app_cli_enum Enum
 * @{
 */

//4字节对齐
typedef struct {
    uint8_t earside;


} app_flash_type_t;


void app_user_read_data(void);
void app_user_write_data(app_flash_type_t flash_data);

extern app_flash_type_t	app_flash_data;

#endif
