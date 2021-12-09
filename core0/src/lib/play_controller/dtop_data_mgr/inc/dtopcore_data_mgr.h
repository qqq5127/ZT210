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

#ifndef _SRC_APP_TWS_APP_DTOPCORE_DATA_MGR_INC_DTOPCORE_DATA_MGR_H_
#define _SRC_APP_TWS_APP_DTOPCORE_DATA_MGR_INC_DTOPCORE_DATA_MGR_H_

/**
 * @addtogroup LIB
 * @{
 */

/**
 * @addtogroup PLAY_CONTROLLER
 * @{
 * This section introduces the LIB PLAY_CONTROLLER module's enum, structure,
 * functions and how to use this module.
 */

/**
 * @addtogroup DTOPCORE_DATA_MGR
 * @{
 * This section introduces the LIB PLAY_CONTROLLER DTOPCORE_DATA_MGR module's enum,
 * structure, functions and how to use this module.
 * @brief Bluetooth dtopcore_data_mgr  API
 */

#ifdef NEW_ARCH
// please put new arch code into corresponding file
// here is for compilation only when enable NEW_ARCH
#include "nplayer_datamgr_core.h"

#else

/*
 * INCLUDE FILES
 ****************************************************************************
 */
#include "types.h"
#include "player_audio_config_define.h"

/*
 * MACROS
 ****************************************************************************
 */

/* min time in ms to start HW timer before play sync tone. */
#ifndef PLAY_TONE_MIN_RESERVED_US
#define PLAY_TONE_MIN_RESERVED_US (30*1000)
#endif

/* max time in ms to start HW timer before play sync tone.
 * tone module won't start HW timer with duration larger than this value.
 */
#ifndef PLAY_TONE_MAX_RESERVED_US
#define PLAY_TONE_MAX_RESERVED_US (600*1000)
#endif

/*
 * DEFINES
 ****************************************************************************
 */
#define EQ_TEST
/** @defgroup lib_play_controller_dtopcore_data_mgr_enum Enum
 * @{
 */
/*
 * ENUMERATIONS
 ****************************************************************************
 */
/// asrc state
enum {
    //asrc is do nothing, in idle state. default.
    ASRC_ST_IDLE = 1,
    //asrc is transmitting pcm data.
    ASRC_ST_ONGOING,
    //we want to stop asrc,when it's transmitting pcm data.
    ASRC_ST_WAIT_STOP,
    //we want to start asrc,when it's transmitting pcm data.
    ASRC_ST_WAIT_START
};
///asrc cmd
enum {
    // we want to stop asrc
    ASRC_CMD_STOP = 1,
    // we want to start asrc
    ASRC_CMD_START,
    // we want to stop spk
    ASRC_CMD_CLOSE_SPK,
    // asrc transmission pcm data complete
    ASRC_CMD_DONE,
};

///player restart reason
enum {
    PLAYER_RESTART_REASON_DTOP_OFFSET = 0x80,
    PLAYER_RESTART_REASON_DTOP_TONE = 0x81,
};

/**
 * @}
 */
/**
 * @brief player_restart_req_done_cb.
 *     player restart req done event callback function.
 *
 * @param[in]   reason      The reason of the player start req.
 */
typedef void (*player_restart_req_done_cb)(uint8_t reason);

/**
 * @brief dtopcore_datapath_init
 *
 * @param[in]   p_cfg       The pointer of btcore config.
 */
void dtopcore_datapath_init(void *p_cfg);

/**
 * @brief dtopcore_datapath_msg_handler
 *
 * @param[in] p_msg_data    The message of datapath.
 */
void dtopcore_datapath_msg_handler(void *p_msg_data);

/**
 * @brief dtop_asrc_evt_handler.
 *
 * @param[in] p_msg_data    The message of asrc.
 */
void dtop_asrc_evt_handler(void *p_msg_data);

/**
 * @brief player_asrc_state_update.
 *
 * @param[in] cmd    The cmd update asrc.
 *     ASRC_CMD_START,
 *     ASRC_CMD_STOP,
 *     ASRC_CMD_DONE,
 * @param[in] stream    The stream id.
 */
void player_asrc_state_update(uint8_t cmd, uint8_t stream);

/**
 * @brief This function is to config player asrc frequency.
 *
 * @param freq_in is the input frequency.
 * @param freq_out is the output frequency.
 * @param ppm is the num of asrc_ppm_unit.
 */
void player_asrc_config_frequency(uint32_t freq_in, uint32_t freq_out, int16_t ppm);

/**
 * @brief set trigger rtc time for HW timer to trigger asrc in ms.
 * @param target_rtc_ms to trigger asrc.
 * @param clk_trigger_pending use free run timer for trigger asrc.
 */
void player_asrc_set_start_rtc_time(uint32_t target_rtc_ms, uint8_t clk_trigger_pending);


/**
 * @brief dtop_record_msg_handler
 *
 * @param[in] p_msg_data    pointer to the message of cfg mic.
 */
void dtop_record_msg_handler(void *p_msg_data);

/**
 * @brief player_asrc_get_offset_cnt
 *  because asrc underrun ,cnt will reset, so for continue,we need add offset
 *
 * @return asrc offset cnt num
 */
uint32_t player_asrc_get_offset_cnt(void);

/**
 * @brief player_asrc_get_current_ts
 *   get asrc current play done frame timestamp
 *
 * @return asrc last play done frame timestamp
 */
uint32_t player_asrc_get_current_ts(void);

/**
 * @brief get the number of total asrc sample
 *
 * @return asrc total sample number
 */
uint32_t player_asrc_get_total_sample_num(void);

/**
 * @brief player_get_tone_ts
 *   get tone start timestamp to mix with music / voice
 * @param delay_ms: time to delay for getting sn.
 *      0 to use default delay for local tone playing.
 * @return the timestamp tone should to start
 */
uint32_t player_get_tone_start_ts(uint32_t delay_ms);

/**
 * @brief player_asrc_state_get.
 *
 * @return the current asrc state
 * ASRC_ST_IDLE / ASRC_ST_ONGOING /ASRC_ST_WAIT_STOP /ASRC_ST_WAIT_START
 */
uint8_t player_asrc_state_get(void);

/*
 * @brief create a tone stream.
 * @return: RET_OK if succeed, other value if failed.
 */
uint8_t player_tone_create_stream(void);

/*
 * @brief destroy a tone stream.
 * @return: RET_OK if succeed, other value if failed.
 */
uint8_t player_tone_destroy_stream(void);

/*
 * @brief check if tone strema exist.
 * @return 1 if tone stream exist, 0 if not exist.
 */
uint8_t player_tone_stream_exist(void);

#ifndef NEW_ARCH
/**
 * @brief config EQ coeff.
 * @param[in] param    pointer to the message of eq param.
 */
void player_eq_coeff_try(eq_band_cfg_t *param);
#endif

/**
 * @brief: check if music or voice is playing.
 * @return: true if music or voice is playing, false if not.
 */
bool is_music_voice_stream(void);

/**
 * @brief is_music_stream
 *
 * @return True is music stream,false is other.
 */
bool is_music_stream(void);

/**
 * @brief switch power balance mode.
 * @param[in] zero_data 0 close power balance, multiple 1 in a row swtich to
 *                      power balance mode.
 * @param close_spk does the audio spk need to be closed
 *
 *  @return if close_spk is true and return is 15, force close power balance.
 */
uint8_t dtopcore_check_switch_power_balance(bool zero_data, bool close_spk);

/**
 * @brief dtopcore_speaker_config
 *
 * @param[in] stream_id    The create stream id.
 *      SM_MUSIC / SM_VOICE / SM_TONE
 * @param[in] open    True is open, false is close.
 * @param[in] freq    The freq of the stream.
 * @param[in] ppm is the num of asrc_ppm_unit.
 */
void dtopcore_speaker_config(uint8_t stream_id, uint8_t open, uint32_t freq, int32_t ppm);

/**
 * @brief player_set_eq_coeff_gain
 *
 * @param[in] size   the eq gain list size
 * @param[in] eq_gain_list  the eq coeff gain list to be configured. unit 0.01db for each gain.
 *
 */
uint8_t player_set_eq_coeff_gain(uint8_t size, int16_t *eq_gain_list);

/**
 * @brief player_audio_check_tone_playing
 * @return: positive number is tone playing, 0 if not.
 *
 */
uint8_t player_audio_check_tone_playing(void);

/**

 * @brief player_stream_restart_req
 *
 * @param[in] reason       The restart req reason.
 * @param[in] cb           The restart stream destroy done cb.
 *
 * @return RET_OK is restart req ok
 *         RET_INVAL the param is invalid
 *         RET_BUSY is restarting.
 */
uint8_t player_stream_restart_req(uint8_t reason, player_restart_req_done_cb cb);

/**

 * @brief audio_spk_mic_dump_start
 *
 * @param[in] dump 0 stop dump, othor bit is start spk or mic dump.
 *
 * @return RET_OK is ok, else is not.
 */
uint8_t audio_spk_mic_dump_start(uint8_t dump);
/**
 * @}
 * addtogroup DTOPCORE_DATA_MGR
 */

/**
 * @brief return player stream spk status
 *
 * @return uint8_t spk status
 */
uint8_t player_get_spk_status(void);

/**
 * @brief player_cfg_anc_mic
 * @param cfg config start/stop start anc mic record data.
 * @param create_stream_flag need to create/destory dsp stream flag.
 */
void player_cfg_anc_mic(uint32_t cfg, uint8_t create_stream_flag);
/**
 * @}
 * addtogroup PLAY_CONTROLLER
 */

/**
 * @}
 * addtogroup LIB
 */

#endif // NEW_ARCH
#endif /* _SRC_APP_TWS_APP_DTOPCORE_DATA_MGR_INC_DTOPCORE_DATA_MGR_H_ */
