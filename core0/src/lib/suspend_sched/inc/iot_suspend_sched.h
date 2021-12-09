#ifndef IOT_SUSPEND_SCHED_H
#define IOT_SUSPEND_SCHED_H

#include "iot_ipc.h"

/**
 * @brief This function is to initialize suspend scheduler lib.
 *
 * @return int RET_OK for success else error.
 */
int32_t iot_suspend_sched_init(void);

/**
 * @brief This function is busy to check flag until target core has been suspended.
 *
 * @param core is the target core ID.
 */
void iot_suspend_sched_wait_core_suspend(IPC_CORES core);

/**
 * @brief This function is to notify target core to resume its own scheduler.
 *
 * @param core is the target core ID.
 */
void iot_suspend_sched_notify_core_resume(IPC_CORES core);

#endif
