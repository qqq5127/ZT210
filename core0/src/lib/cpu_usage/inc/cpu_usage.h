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
#ifndef LIB_CPU_USAGE_H
#define LIB_CPU_USAGE_H

/**
 * @addtogroup LIB
 * @{
 */

/**
 * @addtogroup CPU_USAGE
 * @{
 * This section introduces the cpu usage module's functions and how to use this module.
 */

#ifdef __cplusplus
extern "C" {
#endif
#ifdef LIB_DBGLOG_ENABLE
#include "dbglog.h"
#include "modules.h"
#define DBGLOG_LIB_CPU_USAGE_RAW(fmt, arg...)      DBGLOG_LOG(LIB_CPU_USAGE_MID, DBGLOG_LEVEL_VERBOSE, fmt, ##arg)
#define DBGLOG_LIB_CPU_USAGE_DEBUG(fmt, arg...)    DBGLOG_STREAM_LOG(LIB_CPU_USAGE_MID, DBGLOG_LEVEL_DEBUG, fmt, ##arg)
#define DBGLOG_LIB_CPU_USAGE_INFO(fmt, arg...)     DBGLOG_STREAM_INFO(LIB_CPU_USAGE_MID, fmt, ##arg)
#define DBGLOG_LIB_CPU_USAGE_ERROR(fmt, arg...)    DBGLOG_STREAM_ERROR(LIB_CPU_USAGE_MID, fmt, ##arg)
#else
#include "stdio.h"
#define DBGLOG_LIB_CPU_USAGE_RAW(fmt, arg...)      printf(fmt, ##arg)
#define DBGLOG_LIB_CPU_USAGE_DEBUG(fmt, arg...)    printf(fmt, ##arg)
#define DBGLOG_LIB_CPU_USAGE_INFO(fmt, arg...)     printf(fmt, ##arg)
#define DBGLOG_LIB_CPU_USAGE_ERROR(fmt, arg...)    printf(fmt, ##arg)
#endif

/**
 * @brief This function is used to display isr for cpu usage.
 *
 */
void os_cpu_usage_display_isr(void);

/**
 * @brief This function is used to init cpu usage util.
 *
 */
void os_cpu_usage_util_init(void);

/**
 * @brief This function is used to sstart cpu usage util.
 *
 * @param interval is the interval of cpu start.
 */
void os_cpu_usage_util_start(uint32_t interval);

/**
 * @brief This function is used to stop cpu usage util.
 *
 */
void os_cpu_usage_util_stop(void);

#ifdef __cplusplus
}
#endif

/**
 * @}
 * addtogroup CPU_USAGE
 */

/**
* @}
 * addtogroup LIB
*/

#endif /* LIB_CPU_USAGE_H */
