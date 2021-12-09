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

#ifndef LIB_COMMON_LIB_DBGLOG_H
#define LIB_COMMON_LIB_DBGLOG_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef LIB_DBGLOG_ENABLE
#include "dbglog.h"
#include "modules.h"
#define DBGLOG_LIB_RAW(fmt, arg...)      DBGLOG_LOG(UNKNOWN_MID, DBGLOG_LEVEL_VERBOSE, fmt, ##arg)
#define DBGLOG_LIB_LOG(lvl, fmt, arg...) DBGLOG_STREAM_LOG(UNKNOWN_MID, lvl, fmt, ##arg)
#define DBGLOG_LIB_INFO(fmt, arg...)     DBGLOG_STREAM_INFO(UNKNOWN_MID, fmt, ##arg)
#define DBGLOG_LIB_WARNING(fmt, arg...)  DBGLOG_STREAM_WARNING(UNKNOWN_MID,  fmt, ##arg)
#define DBGLOG_LIB_ERROR(fmt, arg...)    DBGLOG_STREAM_ERROR(UNKNOWN_MID, fmt, ##arg)
#else
#include "stdio.h"
#define DBGLOG_LIB_RAW(fmt, arg...)      printf(fmt, ##arg)
#define DBGLOG_LIB_LOG(lvl, fmt, arg...) printf(fmt, ##arg)
#define DBGLOG_LIB_INFO(fmt, arg...)     printf(fmt, ##arg)
#define DBGLOG_LIB_WARNING(fmt, arg...)  printf(fmt, ##arg)
#define DBGLOG_LIB_ERROR(fmt, arg...)    printf(fmt, ##arg)
#endif

#ifdef __cplusplus
}
#endif

#endif /* LIB_COMMON_LIB_DBGLOG_H */
