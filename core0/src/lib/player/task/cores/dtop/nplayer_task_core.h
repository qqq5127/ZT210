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

#ifndef _SRC_LIB_PLAYER_TASK_CORES_NPLAYER_TASK_CORE_H_
#define _SRC_LIB_PLAYER_TASK_CORES_NPLAYER_TASK_CORE_H_

/*
 * INCLUDE FILES
 ****************************************************************************
 */

/*
 * MACROS
 ****************************************************************************
 */

/*
 * DEFINES
 ****************************************************************************
 */

/*
 * ENUMERATIONS
 ****************************************************************************
 */
///player task event type
enum {
    MSG_DATAPATH = 0,
    MSG_ASRC,
    MSG_RECORD,
    MSG_CFG_MIC,
    MSG_VAD,
    MSG_ADJ_VOL,
    MSG_TONE,
    MSG_STREAM,
    MSG_PLAYER_ERR,
    MSG_MUSIC,
    MSG_VOICE,
    MSG_OPEN_SWEEP_CHIRP_MIC,
    MSG_CFG_SPK,
    MSG_CFG_EQ,
    MSG_RESTART,
    MSG_LOOPBACK_CFG,
    PLAYER_MSG_MAX,
};

///dtop core msg id
typedef enum {
    ASRC_UNDERRUN = 0,
    ASRC_REACHCNT,
    ASRC_DMA_STAGE,
    ASRC_DMA_DONE,
    ASRC_MAX
} data_asrc_reason_t;

/*
 * TYPE DEFINITIONS
 ****************************************************************************
 */

typedef struct _dtop_datapath_msg_t {
    uint32_t param;
    uint8_t evt;
    uint8_t remoteid;
} dtop_datapath_msg_t;

typedef struct _dtop_asrc_msg_t {
    uint32_t channel_id;
    //latch asrc sample count.
    uint32_t cnt;
    void *p_data;
    uint8_t reason;
} dtop_asrc_msg_t;

typedef struct _dtop_record_msg_t {
    uint8_t *addr;
    uint16_t len;
    //which core does the data send to
    uint8_t remoteid;
    uint8_t streamid;
    uint8_t mic_map;
} dtop_record_msg_t;

// unused for new arch
typedef struct _dtop_cfg_mic_msg_t {
    uint32_t cfg;
} dtop_cfg_mic_msg_t;

/**
 * @brief player_close_spk_cb.
 *      close-speaker callback function that is used to
 *      close music/voice speaker before tone start playing.
 */
typedef uint8_t (*player_close_spk_cb)(void);

typedef struct _dtop_tone_msg_t {
    uint8_t evt;
    uint8_t state;
    uint8_t last_stream;
    int8_t vol;
    uint8_t spk;
    uint8_t vol_md;
    uint16_t tone_id;
    uint32_t start_ts;
    player_close_spk_cb close_spk_cb;
    uint8_t resume_stage;
} dtop_tone_msg_t;

typedef struct _dtop_adj_vol_msg_t {
    uint8_t stream;
    int8_t db;
} dtop_adj_vol_msg_t;

typedef struct _dtop_vad_msg_t {
    uint32_t *buffer;
    uint32_t length;
    uint32_t callback_id;
} dtop_vad_msg_t;

typedef struct _dtop_player_err_msg_t {
    uint8_t id;
    uint8_t status;
    uint8_t reason;
    uint8_t stream_mask;
} dtop_player_err_msg_t;

typedef struct _dtop_music_msg_t {
    uint8_t evt;
    uint8_t state;
} dtop_music_msg_t;

typedef struct _dtop_voice_msg_t {
    uint8_t evt;
    uint8_t state;
} dtop_voice_msg_t;

//new arch will remove this
typedef struct _dtop_cfg_spk_msg_t {
    uint32_t freq;
    int32_t ppm;
    uint8_t stream_id;
    uint8_t open;
} dtop_cfg_spk_msg_t;

//new arch will remove this
typedef struct _dtop_eq_cfg_msg_t {
    uint32_t freq_in;
} dtop_eq_cfg_msg_t;

typedef struct _dtop_resart_req_msg_t {
    uint32_t done_cb;
    uint8_t evt;
    uint8_t reason;
    uint8_t stream_id;
} dtop_resart_req_msg_t;

typedef struct _dtop_stream_msg_t {
    uint8_t stream_id;
    uint8_t cmd;
} dtop_stream_msg_t;

/*
 * GLOBAL VARIABLE DECLARATIONS
 ****************************************************************************
 */

/*
 * FUNCTION DECLARATIONS
 ****************************************************************************
 */

#endif /* _SRC_LIB_PLAYER_TASK_CORES_NPLAYER_TASK_CORE_H_ */
