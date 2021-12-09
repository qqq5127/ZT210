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
 * @addtogroup APP_OTA_TASK
 * @{
 * This section introduces the APP OTA TASK module's enum, structure, functions and how to use this module.
 */

#ifndef _OTA_TASK_H__
#define _OTA_TASK_H__
#include "types.h"
#include "errno.h"
#include "userapp_dbglog.h"

/** @defgroup ota_task_enum Enum
 * @{
 */

typedef enum {
    OTA_TASK_EVT_STARTED = 0,
    OTA_TASK_EVT_DATA_HANDLED = 1,
    OTA_TASK_EVT_FINISH = 2,
} ota_task_evt_t;
/**
 * @}
 */

/**
 * @brief callback to handle ota event
 *
 * @param evt ota event
 * @param result event result
 * @param data event data
 */
typedef void (*ota_task_callback_t)(ota_task_evt_t evt, int result, void *data);

/**
 * @brief resume the ota task
 *
 * @param callback the callback to handle ota_task_evt_t
 *
 * @return 0 for success, else for fail.
 */
int ota_task_resume(ota_task_callback_t callback);

/**
 * @brief start the ota task
 *
 * @param callback the callback to handle ota_task_evt_t
 * @param image_size
 *
 * @return 0 for success, else for fail.
 */
int ota_task_start(ota_task_callback_t callback, uint32_t image_size);

/**
 * @brief write ota data
 *
 * @param data the data to write to flash, shuold bee freed when received OTA_TASK_EVT_DATA_HANDLED
 * @param len length of the ota data
 *
 * @return 0 for success, else for fail.
 */
int ota_task_write(uint8_t *data, uint16_t len);

/**
 * @brief write ota data
 *
 * @param data the data to write to flash, shuold bee freed when received OTA_TASK_EVT_DATA_HANDLED
 * @param len length of the ota data
 * @param offset offset of the ota data
 *
 * @return 0 for success, else for fail.
 */
int ota_task_write_ext(uint8_t *data, uint16_t len, uint32_t offset);

/**
 * @brief read ota data
 *
 * @param data the data read form flash
 * @param len length of the ota data
 * @param offset offset of the ota data
 *
 * @return 0 for success, else for fail.
 */
int ota_task_read_ext(uint8_t *data, uint16_t len, uint32_t offset);

/**
 * @brief get wirte offset
 *
 * @return return write offset.
 */
uint32_t ota_task_get_write_offset(void);

/**
 * @brief finish the ota procedure, OTA_TASK_EVT_FINISH will bee send over callback
 *
 * @return 0 for success, else for fail.
 */
int ota_task_finish(void);

/**
 * @brief finish the ota procedure, OTA_TASK_EVT_FINISH will bee send over callback, no commit
 *
 * @return 0 for success, else for fail.
 */
int ota_task_finish_without_commit(void);

/**
 * @brief commit the ota package
 *
 * @return 0 for success, else for fail.
 */
int ota_task_commit(void);

/**
 * @brief check if ota is running
 *
 * @return true if ota is running, false if not
 */
bool_t ota_task_is_running(void);

/**
 * @}
 * addtogroup APP_OTA_TASK
 */

/**
 * @}
 * addtogroup APP
 */

#endif
