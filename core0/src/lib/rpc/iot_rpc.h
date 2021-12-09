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

#ifndef IOT_RPC_H
#define IOT_RPC_H

#include "types.h"
#ifdef LIB_DBGLOG_ENABLE
#include "dbglog.h"
#include "modules.h"
#define DBGLOG_LIB_RPC_INFO(fmt, arg...)     DBGLOG_STREAM_INFO(IOT_RPC_MID, fmt, ##arg)
#define DBGLOG_LIB_RPC_ERROR(fmt, arg...)    DBGLOG_STREAM_ERROR(IOT_RPC_MID, fmt, ##arg)
#define DBGLOG_LIB_RPC_DEBUG(fmt, arg...)    DBGLOG_STREAM_LOG(IOT_RPC_MID, DBGLOG_LEVEL_DEBUG, fmt, ##arg)
#else
#include "stdio.h"
#define DBGLOG_LIB_RPC_INFO(fmt, arg...)     printf(fmt, ##arg)
#define DBGLOG_LIB_RPC_ERROR(fmt, arg...)    printf(fmt, ##arg)
#define DBGLOG_LIB_RPC_DEBUG(fmt, arg...)    printf(fmt, ##arg)
#endif

typedef enum {
    DTOP_DOMAIN,
    BT_DOMAIN,
    DSP_DOMAIN,
    MAX_DOMAIN,
} RPC_DOMAIN;

extern void * rpc_handlers[];

/**
 * @brief This function is to commit rpc.
 *
 * @param dst is specify target domain.
 * @param cmd is handler function id.
 * @param num_params is of arguments (includes return-value).
 * @param params is array of arguments, first one acts as return value,transfer 6 parameters and 1 return-value back in Maximum.
 * @return int32_t RET_OK for success else error.
 */
int32_t iot_rpc_commit(RPC_DOMAIN dst, uint32_t cmd, uint32_t num_params, uint32_t params[]);

/**
 * @brief This function is to initialize only caller side resource.
 *
 * @return int32_t RET_OK for success else error.
 */
int32_t iot_rpc_caller_init(void);

/**
 * @brief This function is to initialize only callee side resource.
 *
 * @return int32_t RET_OK for success else error.
 */
int32_t iot_rpc_callee_init(void);

/**
 * @brief This function is to initialize both (caller & callee) side resource.
 *
 * @return int32_t RET_OK for success else error.
 */
int32_t iot_rpc_init(void);

void iot_rpc_freq_detect(void);

#endif
