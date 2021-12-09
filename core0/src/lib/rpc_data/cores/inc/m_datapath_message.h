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

#ifndef _SRC_LIB_MULTICORE_DATAPATH_INC_MESSAGE_H_
#define _SRC_LIB_MULTICORE_DATAPATH_INC_MESSAGE_H_

/**
 * @addtogroup LIB
 * @{
 */

/**
 * @addtogroup RPC_DATA
 * @{
 * This section introduces the LIB RPC_DATA module's enum, structure, functions and how to use this module.
 */

/**
 * @addtogroup MULTICORE_DATAPATH_MESSAGE
 * @{
 * This section introduces the LIB RPC_DATA MULTICORE_DATAPATH_MESSAGE module's enum, structure, functions and how to use this module.
 * @brief Bluetooth multicore_datapath_message  API
 */
/** @defgroup lib_rpc_data_multicore_datapath_message_struct Struct
 * @{
 */
union _dp_msg_data {
    uint32_t rd;
    uint32_t wr;
};

typedef struct _datapath_msg_t {
#define DP_MSG_INVALID     0u
#define DP_MSG_PUSH_DONE   1u
#define DP_MSG_PUSH_REQ    2u
#define DP_MSG_PUSH_UPDATE 3u
#define DP_MSG_PULL_DONE   4u
#define DP_MSG_PULL_REQ    5u
#define DP_MSG_FLUSH_T_REQ 6u
#define DP_MSG_FLUSH_R_REQ 7u
    uint8_t type; /* message type */
    uint8_t reserved;
    uint16_t len; /* data len*/
    union _dp_msg_data data;
} datapath_msg_t;
/**
 * @}
 */

/**
 * @}
 * addtogroup MULTICORE_DATAPATH_MESSAGE
 */

/**
 * @}
 * addtogroup RPC_DATA
 */

/**
 * @}
 * addtogroup LIB
 */
#endif /* _SRC_LIB_MULTICORE_DATAPATH_INC_MESSAGE_H_ */
