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
#ifndef __DFS_CALLEE_H__
#define __DFS_CALLEE_H__

#ifdef __cplusplus
extern "C" {
#endif

#ifdef BUILD_CORE_CORE0

#include "iot_clock.h"

typedef enum {
    DFS_HOOK_TYPE_PRE = 0,
    DFS_HOOK_TYPE_POST,
    DFS_HOOK_TYPE_NUM,
} DFS_HOOK_TYPE;

/**
 * @brief dfs initialize configuration
 * init_obj: initialized object to do dfs_start
 * init_timeout_ms: timeout to do dfs_stop with init_obj
 * you can do dfs_stop(init_obj) later.
 * range [0, 0x7FFFFFFF], 0 means no timeout, max value is 0x7FFFFFFF.
 * other value is invalid.
 *
 */
typedef struct {
    DFS_OBJ init_obj;
    uint32_t init_timeout_ms;
} dfs_init_cfg_t;

/**
 * @brief dfs hook function type
 * @param src_mode source clock core mode
 * @param dst_mode destination clock core mode
 */
typedef void (* dfs_hook_t)(IOT_CLOCK_MODE src_mode, IOT_CLOCK_MODE dst_mode);

/**
 * @brief dfs hook regsiter
 * @param type hook type, previous or post to set clock mode
 * @param hook hook function pointer, can be set to NULL
 */
uint8_t dfs_hook_register(DFS_HOOK_TYPE type, dfs_hook_t hook);

/**
 * @brief Initialise DFS of callee in dtop core
 */
uint8_t dfs_init(dfs_init_cfg_t *cfg);

/**
 * @brief De-initialise DFS of callee in dtop core
 */
void dfs_deinit(void);

/**
 * @brief for rpc implement
 * @param src_core source cpu core id
 * @param obj DFS_OBJ enum type
 * @param act DFS ACT enum type
 * @param param DFS operation parameter
 * @return RET_OK or other failure return value
 */
uint32_t dfs_request_impl(uint32_t src_core, uint32_t obj, uint32_t act, uint32_t param);

#endif

#ifdef __cplusplus
}
#endif

#endif /* __DFS_CALLEE_H__ */
