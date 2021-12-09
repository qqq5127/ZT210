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

#ifndef _SRC_LIB_MULTICORE_DATAPATH_INC_MULTICORE_DTOP_RINGMAP_H_
#define _SRC_LIB_MULTICORE_DATAPATH_INC_MULTICORE_DTOP_RINGMAP_H_

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
 * @addtogroup MULTICORE_DTOP_RINGMAP
 * @{
 * This section introduces the LIB RPC_DATA MULTICORE_DTOP_RINGMAP module's enum, structure, functions and how to use this module.
 * @brief Bluetooth multicore_dtop_ringmap  API
*/
/*
 * INCLUDE FILES
 ****************************************************************************
 */
#include "iot_memory_config.h"
/*
 * DEFINES
 ****************************************************************************
 */
///--------------------------- ring config
//origiin  [audio] - push -> [dtop]  ring 30k layout
//new   [AUDIO] -- push -> [DTOP]  18k
//add   DTOP MIC record buffer     8k [16K*20ms*2(byte)*6(max 6 mic)*3(buffer num)]
//add   DTOP TONE record buffer    4K

#define     RING_DFA_LENGTH_NEW           0x6800 //26k
#define     MIC_DFA_LENGTH_NEW            0x1800 //6k

#define     DTOP_MIC_BUF_ADDR             (RING_DFA_START + RING_DFA_LENGTH_NEW)
#define     DTOP_MIC_BUF_MAX_LEN          (MIC_DFA_LENGTH_NEW)

// dtop core ring attr     config [addr len idx]
/// [AUDIO] -- push -> [DTOP]
///the ring  bt core push to audio core
#define     DTOP_RING_FROM_AUD_ADDR       (RING_DFA_START + 8)//ring start address
#define     DTOP_RING_FROM_AUD_MAX_LEN    (RING_DFA_LENGTH_NEW - 8)//18k-8 byte ring max length
#define     DTOP_RING_FROM_AUD_RD_ADDR    RING_DFA_START//ring read index address
#define     DTOP_RING_FROM_AUD_WR_ADDR    (RING_DFA_START + 4)//ring write index address
/// [BT] -- push -> [DTOP]
/// the ring  dtop core push to audio core
#define     DTOP_RING_FROM_BT_ADDR        (RING_DFB_START + 8)//ring start address
#define     DTOP_RING_FROM_BT_MAX_LEN     (RING_DFB_LENGTH - 8)//1k-8 byte ring max length
#define     DTOP_RING_FROM_BT_RD_ADDR     RING_DFB_START//ring read index address
#define     DTOP_RING_FROM_BT_WR_ADDR     (RING_DFB_START + 4)//ring write index address
/// [BT] -- pull -> [DTOP]
/// the ring  bt core pull from audio core
#define     DTOP_RING_TO_BT_ADDR          (RING_DTB_START+8)//ring start address
#define     DTOP_RING_TO_BT_MAX_LEN       (RING_DTB_LENGTH -8)//1k-8 byte ring max length
#define     DTOP_RING_TO_BT_RD_ADDR       RING_DTB_START//ring read index address
#define     DTOP_RING_TO_BT_WR_ADDR       (RING_DTB_START + 4)//ring write index address

/**
 * @}
 * addtogroup MULTICORE_DTOP_RINGMAP
 */

/**
 * @}
 * addtogroup RPC_DATA
 */

/**
 * @}
 * addtogroup LIB
 */
#endif /* _SRC_LIB_MULTICORE_DATAPATH_INC_MULTICORE_DTOP_RINGMAP_H_ */
