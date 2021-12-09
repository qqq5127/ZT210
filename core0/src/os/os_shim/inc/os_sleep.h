/****************************************************************************

Copyright(c) 2016 by WuQi Technologies. ALL RIGHTS RESERVED.

This Information is proprietary to WuQi Technologies and MAY NOT
be copied by any method or incorporated into another program without
the express written consent of WuQi. This Information or any portion
thereof remains the property of WuQi. The Information contained herein
is believed to be accurate and WuQi assumes no responsibility or
liability for its use in any way and conveys no license or title under
any patent or copyright and makes no representation or warranty that this
Information is free from patent or copyright infringement.

****************************************************************************/

#ifndef OS_SHIM_SLEEP_H
#define OS_SHIM_SLEEP_H
/**
 * @addtogroup OS
 * @{
 * @addtogroup OS_SHIM
 * @{
 * @addtogroup OS_SLEEP
 * @{
 * This section introduces os sleep reference api.
 */
#ifdef __cplusplus
extern "C" {
#endif

/** @brief os sleep status.*/
typedef enum {
    /* abort entering a sleep mode. */
    ABORT_SLEEP = 0,
    /* Enter a sleep mode that will not last any longer than the expected idle time. */
    STANDARD_SLEEP,
    /* No tasks are waiting for a timeout so it is safe to enter a sleep mode
       that can only be exited by an external interrupt. */
    NO_TIMEOUT_SLEEP,
} OS_SLEEP_STATUS;

/**
 * @brief This function is used to  get os sleep status.
 *
 * @return OS_SLEEP_STATUS status indicate if os can sleep.
 */
OS_SLEEP_STATUS os_get_sleep_status(void);

/**
 * @brief This function is used to compensate the os tick.
 *
 * @param tick_cnt is the tick cnt need be added.
 */
void os_tick_compensate(const uint32_t tick_cnt);

#ifdef __cplusplus
}
#endif

/**
 * @}
 * addtogroup OS_SLEEP
 * @}
 * addtogroup OS_SHIM
 * @}
 * addtogroup OS
 */

#endif /* OS_SHIM_SLEEP_H */
