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

#ifndef __GENERIC_TRANSMISSION_PROTOCOL_H__
#define __GENERIC_TRANSMISSION_PROTOCOL_H__

/*
 *  ----------------------------------------------------------------------------------------------------------
 * | preamble   | version | pkt_len | type | fc   | seq   | hec  | payload                             | crc  |
 *  ----------------------------------------------------------------------------------------------------------
 * | 32bit      | 8bit    | 16bit   | 8bit | 8bit | 16bit | 8bit |                                     | 32bit|
 *  ----------------------------------------------------------------------------------------------------------
 *                                                              /                                       \
 *                                                             /                                         \
 *                                                             -------------------------------------------
 *                                                            |    Every Type has individual format       |
 *                                                             -------------------------------------------
 */

/* Simple Description:
 * 1. Each "TID" use individual sequence. TID only for data type.
 * 2. "Control" and "data" has individual sequence.
 * 3. Any pakcet type related to data and data ack, should fill TID.
 */

//uart transport layer pkt type
#define GTP_PKT_PREAMBLE_SYNC_LEN           4U
#define GTP_PKT_PREAMBLE_SYNC_WORD          0xc1c5d2d0
#define GTP_PKT_PREAMBLE_SYNC_VAL           "\xD0\xD2\xC5\xC2"

#define GTP_PKT_VERSION_MAJOR_SHIFT         6U
#define GTP_PKT_VERSION_MAJOR_MASK          0xc0
#define GTP_PKT_VERSION_SUB_SHIFT           0U
#define GTP_PKT_VERSION_SUB_MASK            0x3f

/*lint -emacro(835, GTP_PKT_VERSION_BUILD) '<< 0' */
#define GTP_PKT_VERSION_BUILD(_major, _sub)                 \
            (((_major) << GTP_PKT_VERSION_MAJOR_SHIFT) |    \
             ((_sub) << GTP_PKT_VERSION_SUB_SHIFT))

#define GTP_PKT_VERSION_MAJOR               0
#define GTP_PKT_VERSION_SUB                 1
#define GTP_PKT_VERSION                     GTP_PKT_VERSION_BUILD(GTP_PKT_VERSION_MAJOR, GTP_PKT_VERSION_SUB)

#define GTP_PKT_TYPE_MAJOR_SHIFT            6
#define GTP_PKT_TYPE_MAJOR_MASK             0xc0
#define GTP_PKT_TYPE_SUB_SHIFT              0
#define GTP_PKT_TYPE_SUB_MASK               0x3f

/*lint -emacro(835, GTP_PKT_TYPE_BUILD) '<< 0' */
#define GTP_PKT_TYPE_BUILD(_major, _sub)                   \
            (((_major) << GTP_PKT_TYPE_MAJOR_SHIFT) |      \
             ((_sub) << GTP_PKT_TYPE_SUB_SHIFT))

#define GTP_PKT_TYPE_MAJOR_GET(_type)       (((_type) & GTP_PKT_TYPE_MAJOR_MASK) >> GTP_PKT_TYPE_MAJOR_SHIFT)

/*lint -emacro(835, GTP_PKT_TYPE_SUB_GET) '>> 0' */
#define GTP_PKT_TYPE_SUB_GET(_type)         (((_type) & GTP_PKT_TYPE_SUB_MASK) >> GTP_PKT_TYPE_SUB_SHIFT)

/* The same TID data cannot support both ack and noack */
#define GTP_PKT_TYPE_MAJOR_DATA             0
#define GTP_PKT_TYPE_DATA_SUB_DFT           0x0
#define GTP_PKT_TYPE_DATA_SUB_STREAM_LOG    0x1
#define GTP_PKT_TYPE_DATA_SUB_RAW_LOG       0x2
#define GTP_PKT_TYPE_DATA_SUB_CLI           0x3
#define GTP_PKT_TYPE_DATA_SUB_AUDIO_DUMP    0x4
#define GTP_PKT_TYPE_DATA_SUB_PANIC_LOG     0x5

/* Ctrl Packet do not support ack */
#define GTP_PKT_TYPE_MAJOR_CTRL             1
#define GTP_PKT_TYPE_CTRL_SUB_ACK_REQ       0x0
#define GTP_PKT_TYPE_CTRL_SUB_SINGLE_ACK    0x1
#define GTP_PKT_TYPE_CTRL_SUB_BLOCK_ACK     0x2

/* MGMT Packet must support ack */
#define GTP_PKT_TYPE_MAJOR_MGMT             2
//TODO in future

#define GTP_PKT_FC_FRAG_COMPLETE            0x0
#define GTP_PKT_FC_FRAG_FIRST               0x1
#define GTP_PKT_FC_FRAG_CONTINUE            0x2
#define GTP_PKT_FC_FRAG_END                 0x3
#define GTP_PKT_FC_FRAG_INVALID             0x4

/* Packet type */
/* data type */
#define GTP_PKT_TYPE_DATA_DFT               (GTP_PKT_TYPE_BUILD(GTP_PKT_TYPE_MAJOR_DATA, GTP_PKT_TYPE_DATA_SUB_DFT))
#define GTP_PKT_TYPE_DATA_STREAM_LOG        (GTP_PKT_TYPE_BUILD(GTP_PKT_TYPE_MAJOR_DATA, GTP_PKT_TYPE_DATA_SUB_STREAM_LOG))
#define GTP_PKT_TYPE_DATA_RAW_LOG           (GTP_PKT_TYPE_BUILD(GTP_PKT_TYPE_MAJOR_DATA, GTP_PKT_TYPE_DATA_SUB_RAW_LOG))
#define GTP_PKT_TYPE_DATA_CLI               (GTP_PKT_TYPE_BUILD(GTP_PKT_TYPE_MAJOR_DATA, GTP_PKT_TYPE_DATA_SUB_CLI))
#define GTP_PKT_TYPE_DATA_AUDIO_DUMP        (GTP_PKT_TYPE_BUILD(GTP_PKT_TYPE_MAJOR_DATA, GTP_PKT_TYPE_DATA_SUB_AUDIO_DUMP))
#define GTP_PKT_TYPE_DATA_PANIC_LOG         (GTP_PKT_TYPE_BUILD(GTP_PKT_TYPE_MAJOR_DATA, GTP_PKT_TYPE_DATA_SUB_PANIC_LOG)

/* control type */
#define GTP_PKT_TYPE_CTRL_ACK_REQ           (GTP_PKT_TYPE_BUILD(GTP_PKT_TYPE_MAJOR_CTRL, GTP_PKT_TYPE_CTRL_SUB_ACK_REQ))
#define GTP_PKT_TYPE_CTRL_SINGLE_ACK        (GTP_PKT_TYPE_BUILD(GTP_PKT_TYPE_MAJOR_CTRL, GTP_PKT_TYPE_CTRL_SUB_SINGLE_ACK))
#define GTP_PKT_TYPE_CTRL_BLOCK_REQ         (GTP_PKT_TYPE_BUILD(GTP_PKT_TYPE_MAJOR_CTRL, GTP_PKT_TYPE_CTRL_SUB_BLOCK_ACK))

typedef struct {
    uint8_t version;
    uint16_t pkt_len;               //pkt length, include it self, all the header and crc if it existed
    uint8_t type;
    struct {
        uint8_t tid:3;              //transport id
        uint8_t frag:2;             //frag type
        uint8_t reserved:2;
        uint8_t need_ack:1;         //not every pack need ack ,this bit indicate whether this packet need ack
    } fc;
    uint16_t seq;                   //sequence
    uint8_t hec;                    //crc8
} __attribute__((packed)) gtp_pkt_hdr_t;

typedef struct {
    uint8_t sync[GTP_PKT_PREAMBLE_SYNC_LEN];
} __attribute__((packed)) gtp_pkt_preamble_t;

typedef struct {
    gtp_pkt_hdr_t hdr;
    uint8_t pld[];
} __attribute__((packed)) gtp_pkt_data_t;

#define GTP_ACK_ST_OK             0x0
#define GTP_ACK_ST_CRC_ERR        0x1
#define GTP_ACK_ST_FORCE_RETRY    0x2
#define GTP_ACK_ST_LT_EXPECT      0x3
#define GTP_ACK_ST_GT_EXPECT      0x4

typedef struct {
    uint16_t ack_seq;
    uint16_t ack_status;          //only 0 meas ack success, other value means error
    uint8_t ack_type;             //packet major type
    uint8_t ack_tid;
} __attribute__((packed)) gtp_pld_single_ack_t;

typedef struct {
    gtp_pkt_hdr_t hdr;
    gtp_pld_single_ack_t pld;
} __attribute__((packed)) gtp_pkt_single_ack_t;

// PKT Length definition
#define GTP_PKT_PREAMBLE_LEN       sizeof(gtp_pkt_preamble_t)
#define GTP_PKT_HDR_LEN            sizeof(gtp_pkt_hdr_t)
#define GTP_PKT_HEC_LEN            (1)
#define GTP_PKT_CRC_LEN            (4)
#define GTP_PKT_HEC_LEN            (1)

#define GTP_PKT_SINGLE_ACK_LEN     (sizeof(gtp_pkt_single_ack_t))
#define GTP_PLD_SINGLE_ACK_LEN     (sizeof(gtp_pld_single_ack_t))
/*
 *  PC                      DEV
 *  |    ack req cnt "x"     |
 *  | <--------------------  |
 *  |         PDU0           |
 *  | <--------------------  |
 *  |         PDU1           |
 *  | <--------------------  |
 *  |         ...            |
 *  | <--------------------  |
 *  |         PDUx           |
 *  | <--------------------  |
 *  |         ack            |
 *  | -------------------->  |
 *  |                        |
 *
 */

// PKT PREAMBLE PACK
#define GTP_PROTO_PKT_PREAMBLE_SYNC_SET(_buf)                               \
    memcpy(((gtp_pkt_preamble_t *)(_buf))->sync, GTP_PKT_PREAMBLE_SYNC_VAL, \
           GTP_PKT_PREAMBLE_SYNC_LEN)

#define GTP_PROTO_PKT_PREAMBLE_PACK(_buf)                                                  \
            do {                                                                           \
                GTP_PROTO_PKT_PREAMBLE_SYNC_SET(_buf);                                     \
            } while (0)

// PKT HDR PACK
#define GTP_PROTO_PKT_HDR_VERSION_SET(_buf, _version)      ((gtp_pkt_hdr_t *)(_buf))->version = (_version)
#define GTP_PROTO_PKT_HDR_LEN_SET(_buf, _pkt_len)          ((gtp_pkt_hdr_t *)(_buf))->pkt_len = (_pkt_len)
#define GTP_PROTO_PKT_HDR_TYPE_SET(_buf, _type)            ((gtp_pkt_hdr_t *)(_buf))->type = (_type)
#define GTP_PROTO_PKT_HDR_FC_TID_SET(_buf, _tid)           ((gtp_pkt_hdr_t *)(_buf))->fc.tid = (_tid)
#define GTP_PROTO_PKT_HDR_FC_FRAG_SET(_buf, _frag)         ((gtp_pkt_hdr_t *)(_buf))->fc.frag = (_frag)
#define GTP_PROTO_PKT_HDR_FC_RESERVED_SET(_buf, _val)      ((gtp_pkt_hdr_t *)(_buf))->fc.reserved = (_val)
#define GTP_PROTO_PKT_HDR_FC_NEED_ACK_SET(_buf, _need_ack) ((gtp_pkt_hdr_t *)(_buf))->fc.need_ack = (_need_ack)
#define GTP_PROTO_PKT_HDR_SEQ_SET(_buf, _seq)              ((gtp_pkt_hdr_t *)(_buf))->seq = (_seq)
#define GTP_PROTO_PKT_HDR_HEC_SET(_buf, _hec)              ((gtp_pkt_hdr_t *)(_buf))->hec = (_hec)

#define GTP_PROTO_PKT_HDR_PACK(_buf, _pkt_len, _type, _tid, _frag, _need_ack, _seq)        \
            do {                                                                           \
                GTP_PROTO_PKT_HDR_VERSION_SET(_buf, GTP_PKT_VERSION);                      \
                GTP_PROTO_PKT_HDR_LEN_SET(_buf, _pkt_len);                                 \
                GTP_PROTO_PKT_HDR_TYPE_SET(_buf, _type);                                   \
                GTP_PROTO_PKT_HDR_FC_TID_SET(_buf, _tid);                                  \
                GTP_PROTO_PKT_HDR_FC_FRAG_SET(_buf, _frag);                                \
                GTP_PROTO_PKT_HDR_FC_RESERVED_SET(_buf, 0);                                \
                GTP_PROTO_PKT_HDR_FC_NEED_ACK_SET(_buf, _need_ack);                        \
                GTP_PROTO_PKT_HDR_SEQ_SET(_buf, _seq);                                     \
            } while (0)


// PKT HDR UNPACK
#define GTP_PROTO_PKT_HDR_VERSION_GET(_buf)     (((gtp_pkt_hdr_t *)(_buf))->version)
#define GTP_PROTO_PKT_HDR_LEN_GET(_buf)         (((gtp_pkt_hdr_t *)(_buf))->pkt_len)
#define GTP_PROTO_PKT_HDR_TYPE_GET(_buf)        (((gtp_pkt_hdr_t *)(_buf))->type)
#define GTP_PROTO_PKT_HDR_FC_TID_GET(_buf)      (((gtp_pkt_hdr_t *)(_buf))->fc.tid)
#define GTP_PROTO_PKT_HDR_FC_FRAG_GET(_buf)     (((gtp_pkt_hdr_t *)(_buf))->fc.frag)
#define GTP_PROTO_PKT_HDR_FC_NEED_ACK_GET(_buf) (((gtp_pkt_hdr_t *)(_buf))->fc.need_ack)
#define GTP_PROTO_PKT_HDR_SEQ_GET(_buf)         (((gtp_pkt_hdr_t *)(_buf))->seq)
#define GTP_PROTO_PKT_HDR_HEC_GET(_buf)         (((gtp_pkt_hdr_t *)(_buf))->hec)

// PKT DATA SET
#define GTP_PROTO_PKT_DATA_SET(_buf, _data, _data_len) memcpy(((gtp_pkt_data_t *)(_buf))->pld, _data, _data_len)

#define GTP_PROTO_PKT_DATA_PACK(_buf, data, data_len)                                      \
            do {                                                                           \
                GTP_PROTO_PKT_DATA_SET(_buf, data, data_len);                              \
            } while (0)

// PKT DATA GET
#define GTP_PROTO_PKT_DATA_START_GET(_buf) (((gtp_pkt_data_t *)(_buf))->pld)
#define GTP_PROTO_PKT_DATA_LEN_GET(_buf)   ((GTP_PROTO_PKT_HDR_LEN_GET(_buf) - GTP_PKT_HDR_LEN) - GTP_PKT_CRC_LEN)

// PKT SINGLE_ACK SET
#define GTP_PROTO_PLD_CTRL_SINGLE_ACK_SEQ_SET(_buf, _seq)       ((gtp_pld_single_ack_t *)(_buf))->ack_seq = (_seq)
#define GTP_PROTO_PLD_CTRL_SINGLE_ACK_STATUS_SET(_buf, _status) ((gtp_pld_single_ack_t *)(_buf))->ack_status = (_status)
#define GTP_PROTO_PLD_CTRL_SINGLE_ACK_TYPE_SET(_buf, _type)     ((gtp_pld_single_ack_t *)(_buf))->ack_type = (_type)
#define GTP_PROTO_PLD_CTRL_SINGLE_ACK_TID_SET(_buf, _tid)       ((gtp_pld_single_ack_t *)(_buf))->ack_tid = (_tid)

#define GTP_PROTO_PLD_CTRL_SINGLE_ACK_PACK(_buf, _seq, _status, _type, _tid)               \
            do {                                                                           \
                GTP_PROTO_PLD_CTRL_SINGLE_ACK_SEQ_SET(_buf, _seq);                         \
                GTP_PROTO_PLD_CTRL_SINGLE_ACK_STATUS_SET(_buf, _status);                   \
                GTP_PROTO_PLD_CTRL_SINGLE_ACK_TYPE_SET(_buf, _type);                       \
                GTP_PROTO_PLD_CTRL_SINGLE_ACK_TID_SET(_buf, _tid);                         \
            } while (0)

#define GTP_PROTO_PKT_CTRL_SINGLE_ACK_SEQ_SET(_buf, _seq)       ((gtp_pkt_single_ack_t *)(_buf))->pld.ack_seq = (_seq)
#define GTP_PROTO_PKT_CTRL_SINGLE_ACK_STATUS_SET(_buf, _status) ((gtp_pkt_single_ack_t *)(_buf))->pld.ack_status = (_status)
#define GTP_PROTO_PKT_CTRL_SINGLE_ACK_TYPE_SET(_buf, _type)     ((gtp_pkt_single_ack_t *)(_buf))->pld.ack_type = (_type)
#define GTP_PROTO_PKT_CTRL_SINGLE_ACK_TID_SET(_buf, _tid)       ((gtp_pkt_single_ack_t *)(_buf))->pld.ack_tid = (_tid)

#define GTP_PROTO_PKT_CTRL_SINGLE_ACK_PACK(_buf, _seq, _status, _type, _tid)               \
            do {                                                                           \
                GTP_PROTO_PKT_CTRL_SINGLE_ACK_SEQ_SET(_buf, _seq);                         \
                GTP_PROTO_PKT_CTRL_SINGLE_ACK_STATUS_SET(_buf, _status);                   \
                GTP_PROTO_PKT_CTRL_SINGLE_ACK_TYPE_SET(_buf, _type);                       \
                GTP_PROTO_PKT_CTRL_SINGLE_ACK_TID_SET(_buf, _tid);                         \
            } while (0)

// PKT SINGLE ACK GET/UNPACK
#define GTP_PROTO_PLD_CTRL_SINGLE_ACK_SEQ_GET(_buf)    (((gtp_pld_single_ack_t *)(_buf))->ack_seq)
#define GTP_PROTO_PLD_CTRL_SINGLE_ACK_STATUS_GET(_buf) (((gtp_pld_single_ack_t *)(_buf))->ack_status)
#define GTP_PROTO_PLD_CTRL_SINGLE_ACK_TYPE_GET(_buf)   (((gtp_pld_single_ack_t *)(_buf))->ack_type)
#define GTP_PROTO_PLD_CTRL_SINGLE_ACK_TID_GET(_buf)    (((gtp_pld_single_ack_t *)(_buf))->ack_tid)

#define GTP_PROTO_PKT_CTRL_SINGLE_ACK_SEQ_GET(_buf)    (((gtp_pkt_single_ack_t *)(_buf))->pld.ack_seq)
#define GTP_PROTO_PKT_CTRL_SINGLE_ACK_STATUS_GET(_buf) (((gtp_pkt_single_ack_t *)(_buf))->pld.ack_status)
#define GTP_PROTO_PKT_CTRL_SINGLE_ACK_TYPE_GET(_buf)   (((gtp_pkt_single_ack_t *)(_buf))->pld.ack_type)
#define GTP_PROTO_PKT_CTRL_SINGLE_ACK_TID_GET(_buf)    (((gtp_pkt_single_ack_t *)(_buf))->pld.ack_tid)

// PKT CRC PACK
#define GTP_PROTO_PKT_CRC_PACK(_buf, _pkt_len, _crc)                                        \
            do {                                                                            \
                uint8_t *crc_offset = ((uint8_t *)(_buf)) + (_pkt_len) - GTP_PKT_CRC_LEN;   \
                crc_offset[0] = ((_crc) & 0xff);                                            \
                crc_offset[1] = (((_crc)>>8) & 0xff);                                       \
                crc_offset[2] = (((_crc)>>16) & 0xff);                                      \
                crc_offset[3] = (((_crc)>>24) & 0xff);                                      \
            } while (0)

// PKT CRC GET
#define GTP_PROTO_PKT_CRC_GET(_buf, _pkt_len)                                               \
            ({                                                                              \
                uint32_t crc;                                                               \
                uint8_t *crc_offset = ((uint8_t *)(_buf)) + (_pkt_len) - GTP_PKT_CRC_LEN;   \
                crc = crc_offset[0] | (crc_offset[1]<<8) |                                  \
                      (crc_offset[2]<<16) | ((unsigned)crc_offset[3]<<24);                            \
                crc;                                                                        \
            })


#endif /* __GENERIC_TRANSMISSION_PROTOCOL_H__ */
