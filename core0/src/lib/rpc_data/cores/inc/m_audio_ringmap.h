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

#ifndef _SRC_LIB_MULTICORE_DATAPATH_INC_MULTICORE_AUDIO_RINGMAP_H_
#define _SRC_LIB_MULTICORE_DATAPATH_INC_MULTICORE_AUDIO_RINGMAP_H_

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
 * @addtogroup MULTICORE_AUDIO_RINGMAP
 * @{
 * This section introduces the LIB RPC_DATA MULTICORE_AUDIO_RINGMAP module's enum, structure, functions and how to use this module.
 * @brief Bluetooth multicore_audio_ringmap  API
 */

/*
 * DEFINES
 ****************************************************************************
 */
///--------------------------- ring config
// audio core ring attr     config [addr len idx]

#define     AUD_RING_TOP_ADDR             0x26420000

/// [BT] -- pull -> [AUDIO]
/// the ring  bt core pull from audio core
#define     AUD_RING_TO_BT_TAIL           AUD_RING_TOP_ADDR
#define     AUD_RING_TO_BT_MAX_LEN        0x0c00  //3k bytes
#define     AUD_RING_TO_BT_PAD_LEN        0x0400  //1k bytes (reserved 248 bytes for tone)
#define     AUD_RING_TO_BT_SIZE           (AUD_RING_TO_BT_MAX_LEN + AUD_RING_TO_BT_PAD_LEN)

#define     AUD_RING_TO_BT_ADDR           (AUD_RING_TO_BT_TAIL - AUD_RING_TO_BT_SIZE)
#define     AUD_RING_TO_BT_RD_ADDR        (AUD_RING_TO_BT_TAIL - 8)
#define     AUD_RING_TO_BT_WR_ADDR        (AUD_RING_TO_BT_TAIL - 4)

#define     AUD_TONE_BUF_INFO_ADDR        (AUD_RING_TO_BT_TAIL - 0x100)

/// [BT] -- push -> [AUDIO]
///the ring  bt core push to audio core
#define     AUD_RING_FROM_BT_TAIL         AUD_RING_TO_BT_ADDR
#define     AUD_RING_FROM_BT_MAX_LEN      0x1800  //6k bytes
#define     AUD_RING_FROM_BT_PAD_LEN      0x0800  //2k bytes
#define     AUD_RING_FROM_BT_SIZE         (AUD_RING_FROM_BT_MAX_LEN + AUD_RING_FROM_BT_PAD_LEN)

#define     AUD_RING_FROM_BT_ADDR         (AUD_RING_FROM_BT_TAIL - AUD_RING_FROM_BT_SIZE)
#define     AUD_RING_FROM_BT_RD_ADDR      (AUD_RING_FROM_BT_TAIL - 8)
#define     AUD_RING_FROM_BT_WR_ADDR      (AUD_RING_FROM_BT_TAIL - 4)


/// [DTOP] -- push -> [AUDIO]
/// the ring  dtop core push to audio core
#define     AUD_RING_FROM_DTOP_TAIL       AUD_RING_FROM_BT_ADDR
#define     AUD_RING_FROM_DTOP_MAX_LEN    0x3000  //12k bytes
#define     AUD_RING_FROM_DTOP_PAD_LEN    0x1000  //4k bytes
#define     AUD_RING_FROM_DTOP_SIZE       (AUD_RING_FROM_DTOP_MAX_LEN + AUD_RING_FROM_DTOP_PAD_LEN)

#define     AUD_RING_FROM_DTOP_ADDR       (AUD_RING_FROM_DTOP_TAIL - AUD_RING_FROM_DTOP_SIZE)
#define     AUD_RING_FROM_DTOP_RD_ADDR    (AUD_RING_FROM_DTOP_TAIL - 8)
#define     AUD_RING_FROM_DTOP_WR_ADDR    (AUD_RING_FROM_DTOP_TAIL - 4)


/// 4k reserved


/// total 32k reserved for ring buffer

/**
 * @}
 * addtogroup MULTICORE_AUDIO_RINGMAP
 */

/**
 * @}
 * addtogroup RPC_DATA
 */

/**
 * @}
 * addtogroup LIB
 */
#endif /* _SRC_LIB_MULTICORE_DATAPATH_INC_MULTICORE_AUDIO_RINGMAP_H_ */
