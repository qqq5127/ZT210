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
#ifndef _SRC_LIB_COMMON_INC_NPLAYER_UTILS_H_
#define _SRC_LIB_COMMON_INC_NPLAYER_UTILS_H_

//#ifdef NEW_ARCH
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
#define DBGLOG_LIB_PLAYER_INFO(fmt, arg...)     DBGLOG_STREAM_INFO(PLAYER_MID, fmt, ##arg)
#define DBGLOG_LIB_PLAYER_WARNING(fmt, arg...)  DBGLOG_STREAM_WARNING(PLAYER_MID, fmt, ##arg)
#define DBGLOG_LIB_PLAYER_ERROR(fmt, arg...)    DBGLOG_STREAM_ERROR(PLAYER_MID, fmt, ##arg)

/*
 * ENUMERATIONS
 ****************************************************************************
 */

/*
 * TYPE DEFINITIONS
 ****************************************************************************
 */

//#endif // NEW_ARCH
#endif // _SRC_LIB_COMMON_INC_NPLAYER_UTILS_H_

