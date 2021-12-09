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

#ifndef _SRC_LIB_PLAYER_API_NPLAYER_API_H_
#define _SRC_LIB_PLAYER_API_NPLAYER_API_H_

#include "types.h"
#include "modules.h"
#include "nplayer_utils.h"

/*
 * MACROS
 ****************************************************************************
 */

/*
 * DEFINES
 ****************************************************************************
 */
/** @brief codec parameters */
typedef struct _player_codec_t {
    //sample freq 44100/48000/...
    uint32_t sf;
    //codec type @player_codec_type_t
    uint8_t type;
} player_codec_t;

/** @brief stream type */
typedef enum {
    /* player idl type*/
    STREAM_IDLE = 0,
    /* player music type*/
    STREAM_MUSIC,
    /* player voice type*/
    STREAM_VOICE,
    /* player tone type*/
    STREAM_TONE,
    STREAM_MAX,
} stream_id_t;

/** @brief player tws mode */
typedef enum {
    //tws only one headset is at work
    PLAYER_TWS_MD_SINGLE = 0,
    //tws two headset are at work
    PLAYER_TWS_MD_DOUBLE,
} player_tws_md_t;

/** @brief player channel */
typedef enum {
    /* player should only play left channel data*/
    PLAYER_CH_LEFT = 0,
    /* player should only play right channel data*/
    PLAYER_CH_RIGHT = 1,
    /* player can play both left and right channel data*/
    PLAYER_CH_STEREO = 2,
    PLAYER_CH_RESERVE = 3,
} player_channel_t;

/** @brief player role */
typedef enum {
    //main ear
    PLAYER_ROLE_M = 0,
    //auxiliary ear
    PLAYER_ROLE_A,
} player_role_t;

/** @brief codec type */
typedef enum {
    /* codec type */
    PLAYER_SBC = 0,
    PLAYER_AAC,
    PLAYER_mSBC,
    PLAYER_CVSD,
    PLAYER_PCM,
} player_codec_type_t;

/** @brief player speaker out select */
typedef enum {
    /* player should only play on left headset speaker */
    PLAYER_SPK_LEFT = 0,
    /* player should only play on right headset speaker */
    PLAYER_SPK_RIGHT = 1,
    /* player should play on both left and right headset speaker */
    PLAYER_SPK_BOTH = 2,
    /* player should only play on self headset speaker */
    PLAYER_SPK_SELF = 3,
    PLAYER_SPK_RESERVE,
} player_spk_out_t;

/** @brief player config default parameters */
typedef struct _player_config_t {
    /*@see player_channel_t */
    uint8_t channel;
    /* @see player_role_t */
    uint8_t role;
    //@see player_tws_md_t
    uint8_t tws_mode;
    //@see player_tone_vol_md_t
    uint8_t tone_vol_mode;
} player_config_t;

/** @brief player err id */
typedef enum {
    PLAYER_ERR_SPK = 0,
    PLAYER_ERR_MIC,
} player_err_id_t;

/** @brief player event type. (player -> app) */
typedef enum {
    EVT_INIT_DONE = 0,
    EVT_MUSIC,
    EVT_VOICE,
    EVT_TONE,
    EVT_ERR,
    EVT_PLAYER_MAX,
} player_event_t;

/** @brief player event type. */
typedef enum {
    ST_PLAY_DONE = 0,
    ST_PLAY_BUSY,
    //TODO: add
} player_ret_state_t;

/** @brief player mode */
typedef enum {
    ERR_UNDERRUN = 0,
    ERR_NO_MEM,
} player_err_reason_t;

/** @brief player app event type. (app -> player) */
typedef enum {
    // phone connected
    A_EVT_CONNECTED = 0,
    // phone disconnected
    A_EVT_DISCONNECTED,
    // tws peer connected
    A_EVT_TWS_CON,
    // tws peer disconnected
    A_EVT_TWS_DISCON,
    // tws mode update (single/double/...)
    A_EVT_TWS_MD_UPDATE,
    //tws role switch
    A_EVT_ROLE_UPDATE,
    //TODO: add
    EVT_PLAYER_APP_MAX,
} player_app_event_t;

/// player mode
typedef enum {
    //player delay game music mode
    PLAYER_DLY_MD_GAME_MUSIC = 1,
    //player fix delay music mode
    PLAYER_DLY_MD_FIX_MUSIC = 2,
    //player dynamic delay music mode
    PLAYER_DLY_MD_DYNAMIC_MUSIC = 3,
} player_delay_mode_t;

/** @brief event param */
typedef struct _player_tone_evt_t {
    //tone id
    uint16_t tone_id;
    //tone state @see player_ret_state_t
    uint8_t state;
    //speaker out channel @see player_spk_out_t
    uint8_t spk;
    //volume unit db
    int8_t vol;
} player_tone_evt_t;

/** @brief language info */
typedef struct _player_music_evt_t {
    // @see player_ret_state_t
    uint8_t state;
    //volume unit db
    int8_t vol;
} player_music_evt_t;

typedef struct _player_voice_call_evt_t {
    // @see player_ret_state_t
    uint8_t state;
    //volume unit db
    int8_t vol;
} player_voice_call_evt_t;

typedef struct _player_err_param_t {
    //@see player_err_reason_t
    uint32_t reason;
    // asrc count
    uint32_t count;
} player_err_param_t;

typedef union _player_evt_param_t {
    player_tone_evt_t tone;
    player_music_evt_t music;
    player_voice_call_evt_t voice_call;
    player_err_param_t err;
} player_evt_param_t;

/** @brief player delay mode param */
typedef struct _player_delay_mode_param_t {
    //default delay mode @see player_delay_mode_t
    uint8_t mode;
    //voice dly ms ( max delay 255ms)
    uint8_t voice_dly_ms;
   //music game delay ms ( 20ms -> 100ms)
    uint16_t music_game_dly_ms;
    //music fix delay ms ( 100ms-> 500ms)
    uint16_t music_fix_dly_ms;
    //music dynamic min delay ms ( 100ms-> 500ms)
    uint16_t music_dyn_dly_min_ms;
    //music dynamic target delay ms ( 100ms-> 500ms)
    uint16_t music_dyn_dly_tgt_ms;
    //music dynamic aac max delay ms ( 100ms-> 500ms)
    uint16_t music_dyn_dly_aac_max_ms;
    //music dynamic sbc max delay ms ( 100ms-> 400ms)
    uint16_t music_dyn_dly_sbc_max_ms;
} player_delay_mode_param_t;

/**
 * @brief player_event_callback.
 *     player event callback function.
 *
 * @param[in]   evtid       The event of the player.
 * @param[in]   p_param     The pointer of the event paramater.
 */
typedef void (*player_event_callback)(player_event_t evtid, player_evt_param_t *p_param);
#ifdef NEW_ARCH

/**
 * @brief  player_init
 *    config player config parameters (role/channel/...),
 *      and register the event callback handler.
 *
 * @param[in]     p_cfg         The pointer of the player config param.
 * @return  RET_OK when it's success.
 *          RET_INVAL when parameters invalid. @see RET_TYPE .
 */
uint8_t player_init(player_config_t *p_cfg);

/**
 * @brief This function is to register the event callback handler.
 * @param func is the user event callback.
 * @return uint8_t RET_OK for success else error.
 */
uint8_t player_register_event_callback(player_event_callback func);

/**
 * @brief player_get_role.
 *
 * @return  The current player role. @see player_role_t.
 */
player_role_t player_get_role(void);

/**
 * @brief player_set_role.
 *
 * @param[in]   role    The current player role. @see player_role_t.
 * @return uint8_t RET_OK for success else error
 */
uint8_t player_set_role(player_role_t role);

/**
 * @brief player_set_channel.
 *
 * @param[in]   channel    The current player role. @see player_channel_t.
 *
 *    PLAYER_CH_LEFT should only play left channel data,when tws is double mode
 *    PLAYER_CH_RIGHT player should only play right channel data,
 *                          when tws is double mode
 *    PLAYER_CH_STEREO player can play both left and right channel data
 * @return uint8_t RET_OK for success else error.
 */
uint8_t player_set_channel(player_channel_t channel);

/**
 * @brief player_get_channel.
 *
 * @return  The current player channel. @see player_channel_t.
 *    PLAYER_CH_LEFT should only play left channel data,when tws is double mode
 *    PLAYER_CH_RIGHT player should only play right channel data,
 *                          when tws is double mode
 *    PLAYER_CH_STEREO player can play both left and right channel data
 */
player_channel_t player_get_channel(void);

/**
 * @brief player_play_set_tws_mode.
 *
 * @param[in]   mode    The current player tws mode. @see player_tws_md_t.
 *
 *    PLAYER_TWS_MD_SINGLE  tws only one headset is at work
 *    PLAYER_TWS_MD_DOUBLE tws two headset are at work
 * @return uint8_t RET_OK for success else error.
 */
uint8_t player_set_tws_mode(player_tws_md_t mode);

/**
 * @brief player_get_tws_mode.
 *
 * @return  The current player tws mode. @see player_tws_md_t.
 *    PLAYER_TWS_MD_SINGLE  tws only one headset is at work
 *    PLAYER_TWS_MD_DOUBLE tws two headset are at work
 */
player_tws_md_t player_get_tws_mode(void);
#endif // NEW_ARCH
#endif /* _SRC_LIB_PLAYER_API_NPLAYER_API_H_ */
