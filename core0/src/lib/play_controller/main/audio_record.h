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

#ifndef _AUDIO_RECORD_H_
#define _AUDIO_RECORD_H_
#include "multicore_data_pack.h"

// mic can max used 6 mic, we should config we true use
#define MIC_USE_NUM 3
// max 3 buffer  one is tx,one is done,one is recording.
#define MIC_RECORD_BUF_MAX_NUM        3
#define MIC_RECORD_PCM_LEN_BYTE       2     //16 bit
#define MIC_RECORD_BLK_MS             20u   //20ms
#define MIC_RECORD_SAMPLING_FREQUENCY 16u   //16k hz
#define MIC_RECORD_ONE_BUF_LEN                   \
    (MIC_RECORD_PCM_LEN_BYTE * MIC_RECORD_BLK_MS \
     * MIC_RECORD_SAMPLING_FREQUENCY)
#define MIC_RECORD_ONE_BUF_DATA_LEN (MIC_RECORD_ONE_BUF_LEN / MIC_RECORD_PCM_LEN_BYTE)

#define AUDIO_RECORD_GAIN_MAX  511
#define AUDIO_RECORD_GAIN_MIN  -511

typedef void (*audio_recv_done_handle_cb)(void *arg, uint32_t length);

enum {
    MIC_VOICE_MAIN_IDX = 0, /* voice mic in mic buffer array index */
    MIC_VOICE_SECOND_IDX,   /* voice mic*/
    MIC_VOICE_THIRD_IDX,    /* voice mic*/
    MIC_VOICE_MAX_IDX
};


typedef struct _mic_one_record_buf {
    uint8_t reserved[DATAPATH_PKT_UP_ALIGN_LEN]; // reserved for byte alignment
    data_pkt_t head;
    uint8_t mic_array[MIC_USE_NUM][MIC_RECORD_ONE_BUF_LEN];
} mic_one_record_buf_t;

typedef struct _mic_record_buf {
    mic_one_record_buf_t buf[MIC_RECORD_BUF_MAX_NUM];
} mic_record_buf_t;

typedef struct _record_mic_config {
    uint32_t dfe_freq;
    uint32_t freq_in;
    uint32_t freq_out;
    int16_t adj_gain;
    /* using audio adc id*/
    uint8_t adc_id;
    /* using audio adc dfe id */
    uint8_t chan_id;
    /* using audio adc fifo id */
    uint8_t fifo_id;
    /* record mode by ADC or PDM */
    uint8_t dfe_mode:4;
    uint8_t asrc_id:4;
    uint8_t mic_power_id;
} record_mic_config;

typedef struct _record_mic_env {
    void *mutex;
    audio_recv_done_handle_cb cb;
    mic_record_buf_t *mic_rec_buf;
    record_mic_config mic_cfg[MIC_USE_NUM];
    uint8_t record_mic_start[MIC_USE_NUM];
    uint8_t cfg_mic_bitmap;
    uint8_t start_mic_bitmap;
    uint8_t start_mic_count;
    /* audio mic start record flag */
    uint8_t record_flag:1;
    uint8_t record_buf_idx:3;
    uint8_t cur_buf_idx:3;
    uint8_t timer_first_start:1;
    uint8_t record_mode:4;
    uint8_t trigger_mode:4;
    /* check the mic open done flag
     * 0 is done, other is not*/
    volatile uint8_t wait_mic_open_done;
} record_mic_env;

/**
 * @brief This function is to start voice record.
 * @param mic_bitmap start mic_id bitmap
 * @param record_mode start mic by for call, vad, and so.
 * @param trigger_mode start mic trigger mode.
 * @param cb if need deal record buf self set it, else using default.
 * @return return cur mic count start.
 */
uint8_t audio_voice_mic_record_start(uint8_t mic_bitmap, uint8_t record_mode,
                                    uint8_t trigger_mode, audio_recv_done_handle_cb cb);

/**
 * @brief This function is to stop voice record.
 * @param mic_bitmap start mic_id bitmap
 *
 */
void audio_voice_mic_record_stop(uint8_t mic_bitmap);

/**
 * @brief This function is to config audio record.
 * @param mic_id is will config mic.
 * @param cfg is the pointer of config message.
 */
void audio_system_mic_config(uint8_t mic_id, record_mic_config *cfg);

/**
 * @brief This function is to init mic system.
 */
void audio_mic_init(void);

/**
 * @brief This function is to run audio record.
 *
 */
void audio_record_run(void);

/**
 * @brief This function is get mic wether is running.
 * @return 1 mic is running, 0 mic stop.
 */
uint8_t audio_get_record_run_flag(void);

/**
 * @brief This function is get fifo id through mic id.
 * @param mic_id is current using record mic index.
 * @return the fifo_id corresponding to mic_id.
 */
uint8_t audio_mic_get_fifo_id(uint8_t mic_id);

/**
 * @brief This function is set all record mic gain.
 * @param[in] gain is set mic record gain, the unit of each value is 0.1875dB.
 */
void audio_mic_set_default_gain(int16_t gain);

/**
 * @brief audio_system_mic_switch_config.
 * @param mic_id is will config mic.
 * @param path_id is get path resource from audio map.
 */
void audio_system_mic_switch_config(uint8_t mic_id, uint8_t path_id);

#endif /* _AUDIO_RECORD_H_ */
