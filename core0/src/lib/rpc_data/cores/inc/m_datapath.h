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

#ifndef _SRC_LIB_MULTICORE_DATAPATH_INC_MULTICORE_DATAPATH_H_
#define _SRC_LIB_MULTICORE_DATAPATH_INC_MULTICORE_DATAPATH_H_

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
 * @addtogroup MULTICORE_DATAPATH
 * @{
 * This section introduces the LIB RPC_DATA MULTICORE_DATAPATH module's enum, structure, functions and how to use this module.
 * @brief Bluetooth multicore_datapath  API
 */
/*
 *   ---------r*************f****************w-----------------
 *            |             |                |
 *         readindex      fetchindex      writeindex
 */

#include "types.h"
#include "iot_dma.h"
#include "iot_ipc.h"
#include "m_datapath_message.h"
#include "m_dtop_ringmap.h"
#include "m_audio_ringmap.h"
#include "modules.h"

/// debug dp task msg process time ,add log
//#define DP_MSG_RPOCESS_DBG        1
//#define DP_MSG_RPOCESS_MAX_MS     30

#ifdef BUILD_CORE_CORE0
#define DP_MSG_DATA_MAX 20
#elif BUILD_CORE_CORE1
#define DP_MSG_DATA_MAX 40
#else
#define DP_MSG_DATA_MAX 8
#endif

/// multicore data path write type.
#define DP_PULL false
#define DP_PUSH true

typedef IPC_CORES coreid_t;

/** @defgroup lib_rpc_data_multicore_datapath_enum Enum
 * @{
 */
/// multicore data path mode.
enum {
    DP_MD_MUSIC = 0,   //default
    DP_MD_VOICE,
    DP_MD_MAX,
};
/// multicore data path msg id.
enum {
    DP_MSG_DATAPATH = 0,
    DP_MSG_PLAYER,
    DP_MSG_MAX,
};

/// multicore data path write type.
enum {
    DP_ISR_DMA_PULL = 0,
    DP_ISR_DMA_PUSH,
    DP_ISR_IPC,
};
///multicore data path event type.
typedef enum {
    DATA_RX = 0,
    WRITE_DONE,
    READ_DONE,
    WRITE_BUF_UPDATE,
    REQ_DATA,
    FLUSH_WRITE_REQ,
    FLUSH_READ_REQ,
    DP_EVT_MAX,
} multicore_datapath_event_t;
/**
 * @}
 */

/** @defgroup lib_rpc_data_multicore_datapath_struct Struct
 * @{
 */
/// datapath task msg handler callback function
typedef void (*dp_task_msg_hdl_func_t)(void *param);
/// datapath task msg handler
typedef struct _dp_task_msg_hdl {
    dp_task_msg_hdl_func_t hdl;
} dp_task_msg_hdl;

typedef struct _dp_msg_t {
    uint8_t msg_id;
    uint8_t evt_id;
    uint16_t reserve;
    uint8_t data[DP_MSG_DATA_MAX];
} dp_msg_t;

/// multicore dma isr parameter
typedef struct {
    void *dst;
    void *src;
    uint32_t len;
    void *param;
} multicore_dma_isr_param_t;

/// multicore ipc isr parameter
typedef struct {
    datapath_msg_t msg;
    uint8_t core_id;
} multicore_ipc_isr_param_t;

/// multicore isr parameter
typedef union {
    multicore_dma_isr_param_t dma;
    multicore_ipc_isr_param_t ipc;
} multicore_isr_param_t;

/// multicore isr msg
typedef struct {
    multicore_isr_param_t isr;
} multicore_isr_msg_t;

/// multicore data path callback msg parameter type
typedef struct {
    uint32_t param;
    multicore_datapath_event_t evtid;
    uint8_t remoteid;
} multicore_cb_msg_param_t;

/// multicore data path basic information.
typedef struct _multicore_dp_info_t {
    uint32_t rd_index;   //
    uint32_t wr_index;
    uint32_t ring_addr;
    uint32_t size;
    uint32_t used_len;
} multicore_dp_info_t;

/// -> config param.
/// multicore data path ring config.
typedef struct _multicore_dp_ring_cfg_t {
    uint32_t rd_addr;
    uint32_t wr_addr;
    uint32_t ring_addr;
    uint32_t size;
} multicore_dp_ring_cfg_t;

/// multicore data path config parameter.
typedef struct _multicore_datapath_cfg_t {
    multicore_dp_ring_cfg_t dta_r;   //dtop -> audio
    multicore_dp_ring_cfg_t dtb_r;   //dtop -> bt
    multicore_dp_ring_cfg_t atb_r;   //audio -> bt
    multicore_dp_ring_cfg_t atd_r;   //auido -> dtop
    multicore_dp_ring_cfg_t bta_r;   //bt -> audio
    multicore_dp_ring_cfg_t btd_r;   //bt -> dtop
} multicore_datapath_cfg_t;
/**
 * @}
 */

/**
 ******************************************************************************
 * @brief multicore_datapath_event_f.
 *      multicore datapath event callback function.
 *
 * @param[in]   evtid       The event of the datapath.
 * @param[in]   remoteid    The remote core id of event.
 * @param[in]   param       The paramater of event.
 *
 ******************************************************************************
 */
typedef void (*multicore_datapath_event_callback)(multicore_datapath_event_t evtid,
                                                  uint8_t remoteid, uint32_t param);

/**
 * @brief   multicore_datapath_write true write data in low layer ring buffer.
 *
 * @param[in]         remoteid      The remote core id.
 * @param[in]         p_buflist     The Pointer of the buflist.
 *
 * @return  The result status.
 */
uint32_t multicore_datapath_write(uint8_t remoteid, const iot_dma_buf_entry_t *p_buflist);

/**
 * @brief   multicore_datapath_fetch not only  get list -> data address and
 *          length,but also update the read buffer fetch index.
 *          [not update read index]
 *
 * @param[in]         remoteid      The remote core id.
 * @param[out]        p_buflist     The Pointer of the buflist.
 * @param[in]         len           The length of the total read data.
 *
 * @return  The result status.
 */
uint32_t multicore_datapath_fetch(uint8_t remoteid, iot_dma_buf_entry_t *p_buflist, uint32_t len);

/**
 * @brief   multicore_datapath_read not only  get list -> data address and
 *          lenhth,but also update the read buffer index.
 *
 * @param[in]         remoteid      The remote core id.
 * @param[out]        p_buflist     The Pointer of the buflist.[no used]
 * @param[in]         len           The length of the total read data.
 *
 * @return  The result status.
 */
uint32_t multicore_datapath_read(uint8_t remoteid, iot_dma_buf_entry_t *p_buflist, uint32_t len);

/**
 * @brief   multicore_datapath_init init datapath core id and low layer rings.
 *          register the user event callback function.
 *
 * @param[in]         p_cfg         The point of config parameter.
 * @param[in]         func          The user event callback.
 *
 * @return  RET_OK when it's success.
 *          RET_INVAL when parameter is invalid.
 */
uint32_t multicore_datapath_init(const multicore_datapath_cfg_t *p_cfg,
                                 multicore_datapath_event_callback func);

/**
* @brief   multicore_datapath_flush
*          flush the datapath ring buffer
*
* @param[in]    remoteid      The remote core id.
* @param[in]    is_write      The direct of the datapath ,true is write
*                                  false is read.
* @param[in]    peer_req      If the flush is from peer req,should set true,
*                             else should set false.
*
* @return  RET_OK when it's success.
*          RET_BUSY  when the ring is busy.
*          RET_INVAL when parameter is invalid.
*/
uint32_t multicore_datapath_flush(uint8_t remoteid, bool is_write, bool peer_req);

/**
 * @brief   multicore_datapath_get_info
 *    get the datapath information fo the low layer ring buffer.
 *
 * @param[in]         remoteid      The remote core id.
 * @param[in]         p_info        The info of the low layer ring buffer.
 * @param[in]         is_write      The direct of the datapath ,true is write
 *                                  false is read.
 *
 * @return  RET_OK when it's success,others is RET_INVAL.
 */
uint32_t multicore_datapath_get_info(uint8_t remoteid, multicore_dp_info_t *p_info, bool is_write);

/**
 * @brief   multicore_datapath_write_malloc
 *    malloc the memory to write data in low layer ring buffer.
 *
 * @param[in]         remoteid      The remote core id.
 * @param[in]         len           The lenght of the low layer ring buffer.
 * @param[in,out]     p_buflist     The Pointer of the buflist array[2].
 *
 * @return  RET_OK when it's success,others is RET_NOMEM.
 */
uint32_t multicore_datapath_write_malloc(uint8_t remoteid, uint32_t len,
                                         iot_dma_buf_entry_t *p_buflist);

/**
 * @brief   multicore_datapath_set_mode
 *
 * @param[in]         mode      The datapath mode.
 *     DP_MD_MUSIC(default)/DP_MD_VOICE/DP_MD_MAX,
 *
 * @return  RET_OK when it's success,others is RET_INVAL.
 */
uint32_t multicore_datapath_set_mode(uint8_t mode);

/**
 * @brief default_datapath_cfg_get
 *      todo: remove when config get from flash.
 * @param[in]    p_cfg       The pointer of btcore config.
 * @param[out]   p_dp_cfg    The pointer of btcore datapath config.
 */
void default_datapath_cfg_get(void *p_cfg, multicore_datapath_cfg_t *p_dp_cfg);

/**
 * @brief multicore_dp_task_msg_hdl_register.
 *
 * @param[in] msg_id      The msg id of the message.
 * @param[in] hdl_func    Pointer to datapath task message handler function.
 *
 * @return  RET_OK when it's success,others is RET_INVAL.
 */
uint32_t multicore_dp_task_msg_hdl_register(uint8_t msg_id, dp_task_msg_hdl_func_t hdl_func);

/**
 * @brief multicore_dp_task_send_msg
 *
 * @param[in] p_msg    The pointer of message.
 * @param[in] in_isr   Is the send messgae in isr.
 *
 * @return  True when it's send success.
 */
bool multicore_dp_task_send_msg(void *p_msg, bool in_isr);

/**
 * @}
 * addtogroup MULTICORE_DATAPATH
 */

/**
 * @}
 * addtogroup RPC_DATA
 */

/**
 * @}
 * addtogroup LIB
 */
#endif /* _SRC_LIB_MULTICORE_DATAPATH_INC_MULTICORE_DATAPATH_H_ */
