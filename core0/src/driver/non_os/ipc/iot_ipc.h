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

#ifndef _DRIVER_NON_OS_IPC_H
#define _DRIVER_NON_OS_IPC_H

/**
 * @addtogroup HAL
 * @{
 * @addtogroup IPC
 * @{
 * This section introduces the IPC module's enum, functions and how to use this driver.
 */

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief IPC type. */
typedef enum {
    IPC_TYPE_CORE_STARTED,     //!< Sent to the main if another core has started
    IPC_TYPE_SYS_REBOOT_REQ,   //!< Request a complete system reboot.
    IPC_TYPE_GENERIC_TRANSMISSION,         //!< Inform there are GTP available.
    IPC_TYPE_RPC_REQ,   //!< Used to send rpc style request between the cores.
    IPC_TYPE_RPC_ACK,   //!< Used to ack rpc stple request if needed.
    IPC_TYPE_SMP_WAKEUP,
    IPC_TYPE_IPC_ACK,
    IPC_TYPE_DATAPATH,
    IPC_TYPE_DMA_DONE,
    IPC_TYPE_PANIC_REQ,
    IPC_TYPE_TASK_SUSPEND,
    IPC_TYPE_PM_LOCK,
    IPC_TYPE_MAX,       //!< Used to size arrays.
} IPC_TYPE;

/** @brief IPC cores. */
typedef enum {
    DTOP_CORE = 0,
    BT_CORE,
    BT_CORE2,
    AUD_CORE,
    MAX_CORE,
} IPC_CORES;

typedef int32_t (*iot_ipc_msg_handler)(IPC_CORES src_cpu, const void *payload);

/**
 * @brief iot ipc ack cb type
 */
typedef void (*iot_ipc_ack_cb_t)(IPC_CORES src_cpu);

/**
 * @brief iot ipc register ack callback
 * @param type is ipc type.
 * @param cb is callback function pointer
 */
int32_t iot_ipc_register_ack_cb(IPC_TYPE type, iot_ipc_ack_cb_t cb);

/**
 * @brief iot ipc unregister ack callback
 * @param type is ipc type.
 */
int32_t iot_ipc_unregister_ack_cb(IPC_TYPE type);

/**
 * @brief This function is finish keep wakeup of dst core.
 *
 */
void iot_ipc_finsh_keep_wakeup(IPC_CORES dst_core);

/**
 * @brief This function is to init ipc module.
 *
 */
void iot_ipc_init(void);

/**
 * @brief This function is to deinitialize ipc module.
 *
 */
void iot_ipc_deinit(void);

/**
 * @brief This function is to register handle.
 *
 * @param action is ipc type.
 * @param handler is message handle.
 * @return int32_t RET_OK for success else error.
 */
int32_t iot_ipc_register_handler(IPC_TYPE action, iot_ipc_msg_handler handler);

/**
 * @brief This function is to unregister handle.
 *
 * @param action is ipc type.
 * @return int32_t RET_OK for success else error.
 */
int32_t iot_ipc_unregister_handler(IPC_TYPE action);

/**
 * @brief This function is to send message.
 *
 * @param dst_core Destination core.
 * @param type Message type.
 * @param payload Payload of the message.
 * @param len Length of the message.
 * @param keep_wakeup unused for now.
 * @return int32_t RET_OK for success else error.
 */
int32_t iot_ipc_send_message(IPC_CORES dst_core, IPC_TYPE type,
                             const void *payload, uint16_t len, bool_t keep_wakeup);

/**
 * @brief This function is to receive message.
 *
 * @param src_core Source core.
 * @return int32_t RET_OK for success else error.
 */
int32_t iot_ipc_recv_message(IPC_CORES src_core);

/**
 * @brief This function Interrupt of the machine soft.
 *
 */
void machine_soft_interrupt_handler(void);

/**
 * @brief This function is to dump ipc information.
 *
 */
void iot_ipc_info_dump(void);

/**
 * @brief This function is to dump ipc's frequency.
 *
 */
void iot_ipc_freq_detect(void);

#ifdef __cplusplus
}
#endif

/**
 * @}
 * addtogroup IPC
 * @}
 * addtogroup HAL
 */

#endif /* _DRIVER_NON_OS_IPC_H */
