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

#ifndef _SRC_APP_COMMON_MULTICORE_DATA_PACK_INC_MULTICORE_DATA_PACK_H_
#define _SRC_APP_COMMON_MULTICORE_DATA_PACK_INC_MULTICORE_DATA_PACK_H_

/**
 * @addtogroup LIB
 * @{
 */

/**
 * @addtogroup PLAY_CONTROLLER
 * @{
 * This section introduces the LIB PLAY_CONTROLLER module's enum, structure, functions and how to use this module.
 */

/**
 * @addtogroup MULTICORE_DATA_PACK
 * @{
 * This section introduces the LIB PLAY_CONTROLLER MULTICORE_DATA_PACK module's enum, structure, functions and how to use this module.
 * @brief Bluetooth multicore_data_pack  API
 */
/*
 * INCLUDE FILES
 */
#include "types.h"
#include "iot_dma.h"
/*
 * DEFINES
 */
// disable test
//#define DATAPATH_PKT_ALIGN_TEST

///  tag current value
#define TAG_VERSION     0x0
#define TAG_RESERVE_VAL 0x00
/// | hdr | payload |
#define DATAPATH_PKT_SBCHDR_LEN     8u   //the packet header length.(unit byte)
#define DATAPATH_PKT_AACHDR_LEN     8u   //the packet header length.(unit byte)
#define DATAPATH_PKT_MSBCHDR_LEN    8u   //the packet header length.(unit byte)
#define DATAPATH_PKT_CVSDHDR_LEN    8u   //the packet header length.(unit byte)
#define DATAPATH_PKT_PCMHDR_LEN     8u   //the packet header length.(unit byte)
#define DATAPATH_PKT_TAG_LEN        2u   //the packet tag length.(unit byte)
#define DATAPATH_PKT_PAD_NUM_LEN    1u   //the packet padding number length.(unit byte)

#define DATAPATH_PKT_DOWN_ALIGN_LEN 2u   //(max:4)the down stream packet allignment length.(unit byte)
#define DATAPATH_PKT_UP_ALIGN_LEN   16u  //the up stream packet allignment length.(unit byte)

#define DATAPATH_PKT_PAD_NUM_OFFSET 8u   //the up stream packet pad number offset.(unit byte)

/*
 * MACROS
 */
/** @defgroup lib_play_controller_multicore_data_pack_enum Enum
 * @{
 */
/*
 * ENUMERATIONS
 */
/// data path stream id.
/* max support 8 stream id.
   And we can add SM_MUSIC_2 to support two music link. */
typedef enum {
    /*  up stream
        wo should use the mic_map to match how many mic stream.
    */
    SM_MIC = 0,
    /* down stream
    music stream id,default use asrc 0 to play.
    1. When we use the neckbank wireless headset,we shoul set channel id is CH_STEREO.
        And the stream packet should include both left and right stream data.
    2. When we use the true wireless headset.
        <1> when both ear are connected,should send only left channel data to left ear.
            send only right channel data to right ear.
        <2> when only one ear is connected,we should set channel id is CH_STEREO,
                and send mixed data.
    */
    SM_MUSIC = 1,
    /* down stream
    voice stream id,default use asrc 0 to play.
    (now voice stream data is mono)
    we shoul set channel id is CH_STEREO, and send mixed data.
    */
    SM_VOICE = 2,/* default use asrc 0 */
    /*  down stream
        tone steam id,default use asrc1 to play.
        when send to left ear,channel id should be left,
        when send to right ear,channel id should be right,
    */
    SM_TONE = 3,
    /* up stream*/
    SM_VAD = 4,
    /* up stream*/
    SM_ANC = 5,
    SM_RESERVE = 7
} data_tag_stream_id_t;
typedef enum {
    MIC_MAP_VOICE = 0x01, /* voice / vad mic*/
    MIC_MAP_BONE = 0x02,  /* bone mic*/
    /* anc mic
        1. When we use the neckbank wireless headset wo may use 4  anc mic
            left is anc 1 2, right is anc 3 4.
        2. When we use the True wireless headset,we may use 2 anc mic.
            both ear anc mic id are 1 and 2.(anc 1 and anc 2)
    */
    MIC_MAP_ANC_1 = 0x04, /* anc mic 1*/
    MIC_MAP_ANC_2 = 0x08, /* anc mic 2*/
    MIC_MAP_ANC_3 = 0x10, /* anc mic 3*/
    MIC_MAP_ANC_4 = 0x20, /* anc mic 4*/
} data_tag_mic_map_t;
/// data path channel id.
typedef enum {
    CH_LEFT = 0,   /* dsp decode and only send left channel data*/
    CH_RIGHT = 1,  /* dsp decode and only send right channel data*/
    CH_STEREO = 2, /* dsp decode and send both left and right channel data*/
    CH_RESERVE = 3
} data_tag_channel_id_t;
/// data path packet is valid.
typedef enum {
    /* down stream
    1. When we need dsp plc, and the stream data is invalid,
        this bit is set INVALID.
    2. When we don't need dsp plc, and the stream data is valid,
        this bit is set VALID.
    */
    INVALID = 0,
    VALID = 1
} data_tag_pkt_valid_t;
/// data path data type
typedef enum {
    /* codec type */
    SBC = 0,
    AAC,
    mSBC,
    CVSD,
    PCM,
    BLE_RAW   //ble raw data
} data_tag_type_t;

/// data path packet is valid.
typedef enum {
    /*
    1. When packet length DOES meet the alignment requirement, packet don't need to add padding
        and this bit is set DATA_TAG_NO_NEED_PAD.
    2. When packet length DOES NOT meet the alignment requirement, packet need to add padding
        and this bit is set DATA_TAG_NEED_PAD.
    */
    DATA_TAG_NO_NEED_PAD = 0,
    DATA_TAG_NEED_PAD = 1
} data_tag_need_pad_t;
/**
 * @}
 */


/** @defgroup lib_play_controller_multicore_data_pack_struct Struct
 * @{
 */
/*
 * TYPE DEFINITIONS
 */
#pragma pack(1)
// data tag type
typedef struct {
    uint8_t version : 1;
    uint8_t stream_id : 3;
    uint8_t data_type : 4;
    union {
        /* down stream */
        struct {
            uint8_t pkt_valid : 1;
            uint8_t channel_id : 2;
            uint8_t tone_mix : 1;
            uint8_t zero_data : 1;
            uint8_t reserve_dw : 2;
            uint8_t need_pad_dw : 1;
        };
        /* up stream */
        struct {
            uint8_t mic_map : 6;
            uint8_t reserve_up : 1;
            uint8_t need_pad_up : 1;
        };
    };
} data_tag_t;

///timestamp field-> | sample_cnt | sn |
///            bytes |     2      | 2  |
typedef struct _data_payload_ts_t {
    uint16_t sample_cnt;
    uint16_t sn;   // sequence number
} data_payload_ts_t;

///payload->      | timestamp| payload |
///         bytes |  4       | 0~65529 |
typedef struct _data_payload_sbc_t {
    uint32_t ts;   // timestamp
    uint8_t payload[];
} data_payload_sbc_t;
typedef struct _data_payload_aac_t {
    uint32_t ts;   // timestamp
    uint8_t payload[];
} data_payload_aac_t;
typedef struct _data_payload_msbc_t {
    uint32_t ts;   // timestamp
    uint8_t payload[];
} data_payload_msbc_t;
typedef struct _data_payload_cvsd_t {
    uint32_t ts;   // timestamp
    uint8_t payload[];
} data_payload_cvsd_t;
typedef struct _data_payload_pcm_t {
    uint32_t ts;   // timestamp
    uint8_t payload[];
} data_payload_pcm_t;
typedef union _data_payload_t {
    data_payload_sbc_t sbc;
    data_payload_aac_t aac;
    data_payload_msbc_t msbc;
    data_payload_cvsd_t cvsd;
    data_payload_pcm_t pcm;
} data_payload_t;
///frame->      |tag| len| payload |
///       bytes | 2 | 2  | 0~65533 |
typedef struct _data_pkt_t {
    data_tag_t tag;
    uint16_t len;
    data_payload_t payload;
} data_pkt_t;
#pragma pack()
/**
 * @}
 */

/**
 * @brief   multicore_data_pack
 *
 * @param[in,out]     pp_data       The Pointer of the pointer of the data.
 * @param[in,out]     p_len         The length of the data.
 * @param[in]         p_tag         The Pointer of data packet tag.
 * @param[in]         sample_num    The sample number of data packet.
 *
 * @return  RET_OK when it's success.
 *          RET_INVAL when parameters invalid.
 */
uint8_t multicore_data_pack(uint8_t **pp_data, uint32_t *p_len,
                            data_tag_t *p_tag, uint32_t sample_num);
/**
 * @brief   multicore data unpack
 *
 * @param[in]         remoteid      The remote core id.
 * @param[in,out]     p_buflist     The Pointer of the buflist.
 * @param[out]        p_pkt         The Pointer of the packet.
 * @param[out]        p_pad_num     The Pointer of pad number.
 *
 * @return  RET_OK when it's success.
 *          RET_INVAL when parameters invalid.
 */
uint8_t multicore_data_unpack(uint8_t remoteid, iot_dma_buf_entry_t *p_buflist,
                              data_pkt_t *p_pkt, uint8_t *p_pad_num);
/**
 * @brief multicore_data_get_timestamp.
 *
 * @param[in] p_data  The pointer of data.
 *
 * @return  The timestamp in pack payload of the point of the data.
 */
uint32_t multicore_data_get_timestamp(void *p_data);

/**
 * @}
 * addtogroup MULTICORE_DATA_PACK
 */

/**
 * @}
 * addtogroup PLAY_CONTROLLER
 */

/**
 * @}
 * addtogroup LIB
 */
#endif /* _SRC_APP_COMMON_MULTICORE_DATA_PACK_INC_MULTICORE_DATA_PACK_H_ */
