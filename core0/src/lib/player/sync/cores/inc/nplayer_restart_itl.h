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
#ifndef _SRC_LIB_PLAYER_SYNC_CORES_INC_NPLAYER_RESTART_ITL_H_
#define _SRC_LIB_PLAYER_SYNC_CORES_INC_NPLAYER_RESTART_ITL_H_

#ifdef NEW_ARCH
/*
 * INCLUDE FILES
 ****************************************************************************
 */
#include "types.h"

/*
 * MACROS
 ****************************************************************************
 */
typedef enum {
    //bt core restart req
    PLAYER_RESTART_REASON_CONTINUE_PLC_TOO_MUCH = 1,
    PLAYER_RESTART_REASON_FRAME_LOST_TOO_MUCH = 2,
    PLAYER_RESTART_REASON_PRI_SEC_TIME_DIFF_TOO_MUCH = 3,
    PLAYER_RESTART_REASON_JITTER_BUFFER_OVERFLOW = 4,
    PLAYER_RESTART_REASON_UNDERRUN = 5,
    PLAYER_RESTART_REASON_REMOTE = 6,
    PLAYER_RESTART_REASON_TIMEOUT = 7,
    PLAYER_RESTART_SCO_ROLE_SWITCH = 8,
    //dtop core restart req
    PLAYER_RESTART_DTOP_OFFSET = 0x80,
    PLAYER_RESTART_TONE_SYNC,
} player_restart_reason_t;

/*
 * DEFINES
 ****************************************************************************
 */

/*
 * ENUMERATIONS
 ****************************************************************************
 */

/*
 * TYPE DEFINITIONS
 ****************************************************************************
 */

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************
 */
/*
 * FUNCTIONS DECLARATIONS
 ****************************************************************************
 */

#endif // NEW_ARCH
#endif // _SRC_LIB_PLAYER_SYNC_CORES_INC_NPLAYER_RESTART_ITL_H_