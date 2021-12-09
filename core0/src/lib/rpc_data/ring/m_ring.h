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

#ifndef _SRC_LIB_MULTICORE_RING_INC_MULTICORE_RING_H_
#define _SRC_LIB_MULTICORE_RING_INC_MULTICORE_RING_H_

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
 * @addtogroup MULTICORE_RING
 * @{
 * This section introduces the LIB RPC_DATA MULTICORE_RING module's enum, structure, functions and how to use this module.
 * @brief Bluetooth multicore_ring  API
 */
/*
 ****************************************************************************
 *   ---------r*************f****************w-----------------
 *            |             |                |
 *         readindex      fetchindex      writeindex
 ****************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************
 */
#include "types.h"
#include "iot_dma.h"
/*
 * MACROS
 ****************************************************************************
 */

/*
 * DEFINES
 ****************************************************************************
 */
// multi-core ring is local
#define REMOTE_RING false
#define LOCAL_RING true
#ifdef LIB_DBGLOG_ENABLE
#include "dbglog.h"
#include "modules.h"
#define DBGLOG_LIB_DPATH_INFO(fmt, arg...)     DBGLOG_STREAM_INFO(IOT_DATAPATH_MID, fmt, ##arg)
#define DBGLOG_LIB_DPATH_ERROR(fmt, arg...)    DBGLOG_STREAM_ERROR(IOT_DATAPATH_MID, fmt, ##arg)
#else
#include "stdio.h"
#define DBGLOG_LIB_DPATH_INFO(fmt, arg...)     printf(fmt, ##arg)
#define DBGLOG_LIB_DPATH_ERROR(fmt, arg...)    printf(fmt, ##arg)
#endif

/*
 * ENUMERATIONS
 ****************************************************************************
 */
/** @defgroup lib_rpc_data_multicore_ring_struct Struct
 * @{
 */
/*
 * TYPE DEFINITIONS
 ****************************************************************************
 */
// multi-core ring  instance type
typedef struct {
    uint32_t ring_addr;   //ring buffer start address
    uint32_t rd_addr;     //ring read index address
    uint32_t wr_addr;     //ring write index address
    uint32_t rd;          // read index
    uint32_t wr;          // write index
    uint32_t fetch;       //fetch index
    uint32_t size;        // the size of the ring
    uint32_t req_len;
    dma_mem_mem_done_callback f_cb;
    bool is_busy : 1;
    bool is_local : 1;
} multicore_ring_t;
/**
 * @}
 */

/**
 * @brief   Func Init the ring
 *
 * @param[in,out] p_ring       Pointer of the ring.
 */
void multicore_ring_init(multicore_ring_t *p_ring);

/**
 * @brief   Func  Get the used size of the ring.
 *
 * @param[in] p_ring   Pointer of the ring.
 *
 * @return The used size in the ring.
 */
uint32_t multicore_ring_get_used_size(multicore_ring_t *p_ring);

/**
 * @brief   Func  Get the free size of the ring.
 *
 * @param[in] p_ring   Pointer of the ring.
 *
 * @return The free size in the ring.
 */
uint32_t multicore_ring_get_free_size(multicore_ring_t *p_ring);

/**
 * @brief   Func source push elements to the ring
 *
 * @param[in,out] p_ring       Pointer of the ring.
 * @param[in]     p_srclist    Poihter of the source buffer list,
 *                             with (addr=NULL) ending.
 * @param[in]     len          The total length of data.
 * @param[in]     f_cb         Pointer of the user push done function.
 *
 * @return It's true when the push was success,otherwise is false.
 */
bool multicore_ring_remote_push(multicore_ring_t *p_ring,
                                const iot_dma_buf_entry_t *p_srclist, uint32_t len,
                                dma_mem_mem_done_callback f_cb);

/**
 * @brief   Func pull elements from the ring
 *
 * @param[in,out] p_ring       Pointer of the ring.
 * @param[in]     p_dstlist    Poihter of the destination buffer list,
 *                             with (addr=NULL) ending.
 * @param[in]     len          The total length of data.
 * @param[in]     f_cb         Pointer of the user push done function.
 *
 * @return It's true when the pull was success,otherwise is false.
 */
bool multicore_ring_remote_pull(multicore_ring_t *p_ring,
                                const iot_dma_buf_entry_t *p_dstlist, uint32_t len,
                                dma_mem_mem_done_callback f_cb);

/**
 * @brief   Func multicore_ring_local_pop.just update read index from the ring
 *
 * @param[in,out] p_ring       Pointer of the ring.
 * @param[in]     len          The total length of data.
 *
 * @return It's true when the pop was success,otherwise is false.
 */
bool multicore_ring_local_pop(multicore_ring_t *p_ring, uint32_t len);

/**
 * @brief   multicore_ring_fetch not only  get list -> data address and
 *          length,but also update the ring fetch index.
 *          [not update read index]
 *
 * @param[in,out] p_ring       Pointer of the ring.
 * @param[in]     p_srclist    Pointer of the source buffer list,
 *                             with (addr=NULL) ending.
 * @param[in]     len          The total length of data.
 *
 * @return It's true when the fetch was success,otherwise is false.
 */
bool multicore_ring_fetch(multicore_ring_t *p_ring,
                          iot_dma_buf_entry_t *p_srclist, uint32_t len);

/**
 * @brief   Func multicore_ring_set_req_len
 *
 * @param[in,out] p_ring       Pointer of the ring.
 * @param[in]     req_len      The lenght of the ring req data.
 *
 * @return It's true when the sync was success,otherwise is false.
 */
void multicore_ring_set_req_len(multicore_ring_t *p_ring, uint32_t req_len);

/**
 * @brief   Func multicore_ring_get_req_len
 *
 * @param[in,out] p_ring       Pointer of the ring.
 *
 * @return The lenght of the ring req data.
 */
uint32_t multicore_ring_get_req_len(const multicore_ring_t *p_ring);

/**
 * @brief   Func multicore_ring_sync_remote_readindex
 *
 * @param[in,out] p_ring       Pointer of the ring.
 */
void multicore_ring_sync_remote_readindex(multicore_ring_t *p_ring);

/**
 * @brief   Func multicore_ring_update_local_writeindex
 *
 * @param[in,out] p_ring       Pointer of the ring.
 * @param[in]     wr           The write index of the ring.
 */
void multicore_ring_update_local_writeindex(multicore_ring_t *p_ring,
                                            uint32_t wr);

/**
 * @brief   Func multicore_ring_update_local_readindex
 *
 * @param[in,out] p_ring       Pointer of the ring.
 * @param[in]     rd           The read index of the ring.
 */
void multicore_ring_update_local_readindex(multicore_ring_t *p_ring,
                                           uint32_t rd);

/**
 * @brief   Func multicore_ring_local_push.just update wr index from the ring
 *
 * @param[in,out] p_ring       Pointer of the ring.
 * @param[in]     len          The total length of data.
 *
 * @return It's true when the push was success,otherwise is false.
 */
bool multicore_ring_local_push(multicore_ring_t *p_ring, uint32_t len);

/**
 * @brief   multicore_ring_flush
 *          flush the ring buffer
 *
 * @param[in,out] p_ring       Pointer of the ring.
 *
 * @return  RET_OK when it's success.
 *          RET_BUSY  when the ring is busy.
 *          RET_INVAL when parameter is invalid.
*/
uint32_t multicore_ring_flush(multicore_ring_t *p_ring);

/**
 * @}
 * addtogroup MULTICORE_RING
 */

/**
 * @}
 * addtogroup RPC_DATA
 */

/**
 * @}
 * addtogroup LIB
 */
#endif /* _SRC_LIB_MULTICORE_RING_INC_MULTICORE_RING_H_ */
