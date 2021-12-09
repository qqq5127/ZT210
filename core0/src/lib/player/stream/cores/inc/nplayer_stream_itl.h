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

#ifndef _SRC_LIB_PLAYER_STREAM_CORES_INC_NPLAYER_STREAM_ITL_H_
#define _SRC_LIB_PLAYER_STREAM_CORES_INC_NPLAYER_STREAM_ITL_H_

#include "types.h"
#include "modules.h"

// player stream info
typedef struct _player_stream_info_t {
    uint8_t id;
    //TODO : link id -> stream id
    uint8_t type; //stream type
    uint8_t link_id; //link id
    uint8_t codec_type;
    uint32_t sample_rate;
} player_stream_info_t;

typedef struct _player_stream_t {
    player_stream_info_t info;
    uint8_t (*in)(uint8_t *p_data, uint32_t len);
    uint8_t (*process)(void *p_info);
    uint8_t (*out)(void *p_list);
    //TODO: add create done cb
    uint8_t (*evt)(void *p_info);
} player_stream_t;

typedef struct _player_stream_t *player_stream_hdl_t;

typedef void(*player_stream_event_cb)(uint8_t streamid,
    uint8_t evtid, uint8_t remoteid, uint32_t param);
#ifdef NEW_ARCH

/**
 * @brief player_stream_init.
 *
 * @return ok.
 */
uint8_t player_stream_init(void);

#if 1
/**
 * @brief player_stream_create
 *
 * @param[in] p_info    The info of the stream.
 * @param[out] p_hdl     The handler of the stream.
 *
 * @return uint8_t RET_OK for success else error.
 */
uint8_t player_stream_create(player_stream_info_t *p_info, player_stream_hdl_t p_hdl);

/**
 * @brief audio_sys_destory_stream
 *
 * @param[in] p_hdl    The handler of the stream.
 *
 * @return uint8_t RET_OK for success else error.
 */
uint8_t player_stream_destory(player_stream_hdl_t p_hdl);
#else

/**
 * @brief audio_sys_create_stream
 *
 * @param[in] stream_id    The create stream id.
 *      SM_MUSIC / SM_VOICE / SM_TONE
 * @return uint8_t RET_OK for success else error.
 */
uint8_t player_stream_create(uint8_t stream_id);

/**
 * @brief audio_sys_destory_stream
 *
 * @param[in] stream_id    The create stream id.
 *      SM_MUSIC / SM_VOICE / SM_TONE
 * @return uint8_t RET_OK for success else error.
 */
uint8_t player_stream_destory(uint8_t stream_id);
#endif

/**
 * @brief audio_sys_resume_stream
 *      resume pending stream
 * @return uint8_t RET_OK for success else error.
 */
uint8_t player_stream_resume(void);

/**
 * @brief player_stream_stream_get
 *
 * @return The stream mask.
 */
uint8_t player_stream_mask_get(void);

/**
 * @brief get last stream id
 *
 * @return The last stream id.
 */
uint8_t player_stream_last_get(void);

/**
 * @brief player_stream_streamid_get.
 *
 * @return ok.
 */
uint8_t player_stream_streamid_get(void);

/**
 * @brief is_music_voice_stream
 *
 * @return True is music/voice stream,false is other.
 */
bool is_music_or_voice_stream(void);

/**
 * @brief is_music_stream
 *
 * @return True is music stream, false is other.
 */
bool is_music_stream(void);

/**
 * @brief player_stream_data_send.
 *      multicore datapath event callback function.
 *
 * @param[in]   remoteid    The remote core id of event.
 * @param[in]   buff        The data addres.
 * @param[in]   len         The data length.
 *
 * @return ok.
 */
uint8_t player_stream_data_send(uint8_t remoteid, uint8_t *buff,
                                uint32_t len);

/**
 * @brief player_stream_event_cb_register.
 *
 * @param[in]   cb    stream callback.
 *
 * @return ok.
 */
uint8_t player_stream_event_cb_register(player_stream_event_cb cb);
#endif // NEW_ARCH
#endif /* _SRC_LIB_PLAYER_STREAM_CORES_INC_NPLAYER_STREAM_ITL_H_ */
