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

#ifndef _SRC_LIB_PLAYER_DATAMGR_CORES_NPLAYER_DATAMGR_CORE_H_
#define _SRC_LIB_PLAYER_DATAMGR_CORES_NPLAYER_DATAMGR_CORE_H_
#ifdef NEW_ARCH

/// asrc state
enum {
    //asrc is do nothing, in idle state. default.
    ASRC_ST_IDLE = 0,
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
    ASRC_CMD_STOP = 0,
    // we want to start asrc
    ASRC_CMD_START,
    // asrc transmission pcm data complete
    ASRC_CMD_DONE,
};

/**
 * @brief player_datamgr_recv_done_cb.
 *
 * @param[in] arg    mic data address.
 * @param[in] length mic data length.
 *
 * @return void.
 */
void player_datamgr_recv_done_cb(void *arg, uint32_t length);

/**
 * @brief player_datamgr_record_start
 *
 * @return void.
 *
 */
void player_datamgr_record_start(void);

/**
 * @brief player_datamgr_record_stop
 *
 * @return void.
 *
 */
void player_datamgr_record_stop(void);

/**
 * @brief player_datamgr_pcm_data_send_done_cb.
 *
 * @param[in] p_data  The pointer of data.
 * @param[in] len     The length of data.
 */
void player_datamgr_pcm_data_send_done_cb(void *p_data, uint32_t len);

/*
 * the follow functions will be moved out later
 */
/**
 * @brief player_get_tone_ts
 *   get tone start timestamp to mix with music / voice
 *
 * @return the timestamp tone should to start
 */
uint32_t nplayer_get_tone_start_ts(void);

/**
 * @brief player_asrc_get_current_ts
 *   get asrc current play done frame timestamp
 *
 * @return asrc last play done frame timestamp
 */
uint32_t nplayer_asrc_get_current_ts(void);

/**
 * @brief get the number of total asrc sample
 *
 * @return asrc total sample number
 */
uint32_t nplayer_asrc_get_total_sample_num(void);

/**
 * @brief  player_vol_force_gain_set
 *
 * @param[in] force true is set gain inoperative, false is operative.
 */
void player_vol_force_gain_set(bool force);

#ifdef NEW_ARCH
/**
 * @brief  player_current_vol_set
 *      set the volume of the stream.
 *
 * @param[in] id The player stream id of the player volume. @see stream_id_t
 *           STREAM_MUSIC / STREAM_VOICE / STREAM_TONE /
 * @param[in]    db     The decibels of the player volume.
 *
 * @return 0 for success, else for the error code.
 */
uint8_t player_current_vol_set(uint8_t id, int8_t db);
/**
 * @brief cli config eq
 *
 */
void player_eq_coeff_try(void *param);
#endif

/**
 * @brief This function is to config eq ,
 *
 * @param sample_rate  The asrc input sample rate.
 */
void player_eq_coeff_update(uint32_t sample_rate);


/**
 * @brief player_set_eq_coeff_gain
 *
 * @param[in] size   the eq gain list size
 * @param[in] eq_gain_list  the eq coeff gain list to be configured. unit 0.01db for each gain.
 *
 */
uint8_t player_set_eq_coeff_gain(uint8_t size, int16_t *eq_gain_list);

/**
 * @brief player_datamgr_flush_read.
 * flush read buffer
 */
void player_datamgr_flush_read(void);

/**

 * @brief audio_spk_mic_dump_start
 *
 * @param[in] dump 0 stop dump, othor bit is start spk or mic dump.
 *
 * @return RET_OK is ok, else is not.
 */
uint8_t audio_spk_mic_dump_start(uint8_t dump);
#endif //NEW_ARCH
#endif /* _SRC_LIB_PLAYER_DATAMGR_CORES_NPLAYER_DATAMGR_CORE_H_ */
