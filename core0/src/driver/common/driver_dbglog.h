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

#ifndef DRIVER_COMMON_DRIVER_DBGLOG_H
#define DRIVER_COMMON_DRIVER_DBGLOG_H

#ifdef __cplusplus
extern "C" {
#endif
#ifdef LIB_DBGLOG_ENABLE
#include "dbglog.h"
#include "modules.h"
#define DBGLOG_DRIVER_RAW(fmt, arg...) DBGLOG_LOG(IOT_DRIVER_MID, DBGLOG_LEVEL_VERBOSE, fmt, ##arg)
#define DBGLOG_DRIVER_LOG(lvl, fmt, arg...) DBGLOG_STREAM_LOG(IOT_DRIVER_MID, lvl, fmt, ##arg)
#define DBGLOG_DRIVER_VERBOSE(fmt, arg...)  DBGLOG_STREAM_VERBOSE(IOT_DRIVER_MID, fmt, ##arg)
#define DBGLOG_DRIVER_DEBUG(fmt, arg...)    DBGLOG_STREAM_DEBUG(IOT_DRIVER_MID, fmt, ##arg)
#define DBGLOG_DRIVER_INFO(fmt, arg...)     DBGLOG_STREAM_INFO(IOT_DRIVER_MID, fmt, ##arg)
#define DBGLOG_DRIVER_WARNING(fmt, arg...)  DBGLOG_STREAM_WARNING(IOT_DRIVER_MID, fmt, ##arg)
#define DBGLOG_DRIVER_ERROR(fmt, arg...)    DBGLOG_STREAM_ERROR(IOT_DRIVER_MID, fmt, ##arg)
#else
#include "stdio.h"
#define DBGLOG_DRIVER_RAW(fmt, arg...)      printf(fmt, ##arg)
#define DBGLOG_DRIVER_LOG(lvl, fmt, arg...) printf(fmt, ##arg)
#define DBGLOG_DRIVER_VERBOSE(fmt, arg...)  printf(fmt, ##arg)
#define DBGLOG_DRIVER_DEBUG(fmt, arg...)    printf(fmt, ##arg)
#define DBGLOG_DRIVER_INFO(fmt, arg...)     printf(fmt, ##arg)
#define DBGLOG_DRIVER_WARNING(fmt, arg...)  printf(fmt, ##arg)
#define DBGLOG_DRIVER_ERROR(fmt, arg...)    printf(fmt, ##arg)
#endif

#ifdef __cplusplus
}
#endif

#endif /* DRIVER_COMMON_DRIVER_DBGLOG_H */
