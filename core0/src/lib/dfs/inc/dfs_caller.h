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
#ifndef __DFS_CALLER_H__
#define __DFS_CALLER_H__

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief This function is to update cpu frequence of specified object
 *        It should be called after BT/DSP core initialize complete, because
 *        it use IPC/RPC.
 *
 * @param obj which object, such as dsp stream create.
 * @param timeout_ms millisecond timeout to stop automatically.
 *        range [0, 0x7FFFFFFF], 0 means no timeout, max value is 0x7FFFFFFF.
 *        other value is invalid.
 * @return uint32_t RET_OK for success else error.
 */
uint32_t dfs_start(DFS_OBJ obj, uint32_t timeout_ms);


/**
 * @brief This function is to stop current cpu frequence of specified object
 *         and recovery previous cpu frequence.
 *
 * @param obj which object, such as dsp stream create.
 * @return uint32_t RET_OK for success else error.
 */
uint32_t dfs_stop(DFS_OBJ obj);

#ifdef __cplusplus
}
#endif

#endif /* __DFS_CALLER_H__ */
