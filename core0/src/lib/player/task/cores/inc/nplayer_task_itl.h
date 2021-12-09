/****************************************************************************

Copyright(c) 2021 by WuQi Technologies. ALL RIGHTS RESERVED.

This Information is proprietary to WuQi Technologies and MAY NOT
be copied by any method or incorporated into another program without
the express written consent of WuQi. This Information or any portion
thereof remains the property of WuQi. The Information contained herein
is believed to be accurate and WuQi assumes no responsibility or
liability for its use in any way and conveys no license or title under
any patent or copyright and makes no representation or warranty that this
Information is free from patent or copyright infringement.

****************************************************************************/

#ifndef _SRC_LIB_PLAYER_TASK_CORES_INC_NPLAYER_TASK_ITL_H_
#define _SRC_LIB_PLAYER_TASK_CORES_INC_NPLAYER_TASK_ITL_H_

#include "types.h"
#include "modules.h"
#include "nplayer_task_core.h"

#define NEW_PALYER_TASK_API 1

/// player task msg handler callback function
typedef void (*player_task_msg_hdl_func_t)(void *param);
/// player task msg handler
typedef struct _player_task_msg_hdl {
    player_task_msg_hdl_func_t hdl;
} player_task_msg_hdl;

/**
 * @brief player_task_msg_handler_register.
 *
 * @param[in] evt_id    The event id of the message.
 * @param[in] hdl_func  Pointer to player task message handler function.
 */
void player_task_msg_handler_register(uint8_t evt_id, player_task_msg_hdl_func_t hdl_func);

/**
 * @brief   player_task_msg_handler
 *
 * @param[in] evt_id    The event id of the message.
 * @param[in] p_param     Pointer to the parameters of the message.
 */
void player_task_msg_handler(uint8_t evt_id, void *p_param);

/**
 * @brief   player_task_send_msg
 *
 * @param[in] evt_id    The event id of the message.
 * @param[in] p_param     Pointer to the parameters of the message.
 * @param[in] in_isr    whether it's in isr.
 */
void player_task_send_msg(uint8_t evt_id, void *p_param, bool in_isr);

/**
 * @brief   player_task_init
 */
void player_task_init(void);

#endif /* _SRC_LIB_PLAYER_TASK_CORES_INC_NPLAYER_TASK_ITL_H_ */
