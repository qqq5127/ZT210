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

#ifndef DEV_PM_H
#define DEV_PM_H

#include "types.h"
#include "generic_list.h"

struct pm_operation {
    struct list_head node;
    uint32_t (*save)(uint32_t data);
    uint32_t (*restore)(uint32_t data);
    uint32_t data;
};

/**
 * @brief This function is to init device pm.
 *
 * @return uint32_t RET_OK for success else error.
 */
uint32_t iot_dev_pm_init(void);

/**
 * @brief This function is to init device pm mode.
 *
 * @param obj is the pm operation code.
 */
void iot_dev_pm_node_init(struct pm_operation *obj);

/**
 * @brief This function is to save in device.
 *
 * @return int32_t is the time for saving.
 */
uint32_t iot_dev_save(void);

/**
 * @brief This function is to restore in device.
 *
 * @return int32_t is the time for restoring.
 */
uint32_t iot_dev_restore(void);

/**
 * @brief This function is to register device pm.
 *
 * @param obj
 * @return int32_t RET_OK for success else error.
 */
int32_t iot_dev_pm_register(struct pm_operation *obj);

/**
 * @brief This function is to unregister device pm.
 *
 * @param obj
 * @return int32_t RET_OK for success else error.
 */
int32_t iot_dev_pm_unregister(struct pm_operation *obj);

/**
 * @brief This function is to get device pm sleep time.
 *
 * @return int32_t is deep sleep rtc.
 */
uint32_t iot_dev_pm_sleep_time_get(void);

/**
 * @brief This function is to clear device pm sleep time.
 *
 */
void iot_dev_pm_sleep_time_clear(void);

/**
 * @brief This function is to get the save/restore timing info
 *
 * @param st0, st1  save start -> end rtc clock
 * @param lrt       last restore start rtc clock
 * @param rt0, rt1  restore start -> end rtc clock
 * @return RET_OK
 *
 */
uint32_t iot_dev_get_last_timing_info(uint32_t *st0,
        uint32_t *st1, uint32_t *lrt, uint32_t *rt0, uint32_t *rt1);

#endif
