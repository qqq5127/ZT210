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

#ifndef SRC_APP_TWS_COMMON_USERAPP_DBGLOG_H_
#define SRC_APP_TWS_COMMON_USERAPP_DBGLOG_H_

/**
 * @addtogroup APP
 * @{
 */

/**
 * @addtogroup APP_DBGLOG
 * @{
 * This section introduces the LIB APP_DBGLOG module's enum, structure, functions and how to use this module.
 * @brief Bluetooth app_dbglog  API
 */
/*
 * INCLUDE FILES
 ****************************************************************************
 */
#include "types.h"
#include "modules.h"
#include "dbglog.h"
/*
 * MACROS
 ****************************************************************************
 */

/*
 * DEFINES
 ****************************************************************************
 */
///-----------------USR APP USER DEBUGE LOG [DTOP CORE] -----------------
#ifdef LIB_DBGLOG_ENABLE
#define DBGLOG_USER_APP_RAW(fmt, arg...)      \
    DBGLOG_LOG(IOT_APP_MID, DBGLOG_LEVEL_VERBOSE, fmt, ##arg)
#define DBGLOG_USER_APP_LOG(lvl, fmt, arg...) \
    DBGLOG_STREAM_LOG(IOT_APP_MID, lvl, fmt, ##arg)
#define DBGLOG_USER_APP_DEBUG(fmt, arg...) \
    DBGLOG_STREAM_LOG(IOT_APP_MID, DBGLOG_LEVEL_DEBUG, fmt, ##arg)
#define DBGLOG_USER_APP_INFO(fmt, arg...)     \
    DBGLOG_STREAM_INFO(IOT_APP_MID, fmt, ##arg)
#define DBGLOG_USER_APP_WARNING(fmt, arg...)  \
    DBGLOG_STREAM_WARNING(IOT_APP_MID,  fmt, ##arg)
#define DBGLOG_USER_APP_ERROR(fmt, arg...)    \
    DBGLOG_STREAM_ERROR(IOT_APP_MID, fmt, ##arg)
#else
#define DBGLOG_USER_APP_RAW(fmt, arg...) do{}while(0)
#define DBGLOG_USER_APP_LOG(lvl, fmt, arg...) do{}while(0)
#define DBGLOG_USER_APP_DEBUG(fmt, arg...) do{}while(0)
#define DBGLOG_USER_APP_INFO(fmt, arg...) do{}while(0)
#define DBGLOG_USER_APP_WARNING(fmt, arg...) do{}while(0)
#define DBGLOG_USER_APP_ERROR(fmt, arg...) do{}while(0)
#endif

/*
 * ENUMERATIONS
 ****************************************************************************
 */

/*
 * TYPE DEFINITIONS
 ****************************************************************************
 */

/*
 * GLOBAL VARIABLE DECLARATIONS
 ****************************************************************************
 */

/*
 * FUNCTION DECLARATIONS
 ****************************************************************************
 */

/**
 * @}
 * addtogroup APP_DBGLOG
 */

/**
 * @}
 * addtogroup APP
 */
#endif /* SRC_APP_TWS_COMMON_USERAPP_DBGLOG_H_ */
