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
#ifndef _SRC_LIB_PLAYER_SYNC_CORES_INC_NPLAYER_RESTART_H_
#define _SRC_LIB_PLAYER_SYNC_CORES_INC_NPLAYER_RESTART_H_

#ifdef NEW_ARCH
/*
 * INCLUDE FILES
 ****************************************************************************
 */
#include "types.h"
#include "nplayer_restart_itl.h"

/*
 * MACROS
 ****************************************************************************
 */

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
/**
 * @brief player_restart_req_done_cb.
 *     player restart req done event callback function.
 *
 * @param[in]   reason      The reason of the player start req.
 */
typedef void (*player_restart_req_done_cb)(uint8_t reason, uint32_t reserved);

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************
 */
/*
 * FUNCTIONS DECLARATIONS
 ****************************************************************************
 */
/**
 ******************************************************************************
 * @brief init context to restart.
 ******************************************************************************
 */
void player_restart_init(void);

/**
 * @brief player_stream_restart_req
 *
 * @param[in] reason       The restart req reason.
 * @param[in] cb           The restart stream destroy done cb.
 *
 * @return RET_OK is restart req ok
 *         RET_INVAL the param is invalid
 *         RET_BUSY is restarting.
 */
uint8_t player_stream_restart_req(uint8_t reason, player_restart_req_done_cb cb);

/**
 * @brief judge whether restart request is handler completly
 *          and start tone reserved
 */
void player_restart_req_is_destroyed(void);

#endif // NEW_ARCH
#endif // _SRC_LIB_PLAYER_SYNC_CORES_INC_NPLAYER_RESTART_H_
