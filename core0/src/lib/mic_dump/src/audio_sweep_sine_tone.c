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

//common
#include "types.h"
#include "string.h"
#include "os_lock.h"
#include "os_utils.h"

//hal
#include "iot_asrc.h"
#include "iot_audio.h"
#include "iot_audio_dac.h"
#include "iot_share_task.h"

//lib
#include "math.h"
#include "lib_dbglog.h"
#include "mic_dump.h"
#include "m_dtop_ringmap.h"
#include "audio_sweep_sine_tone.h"
#include "audio_anc_sweep_chirp.h"
#include "audio_eq.h"
#include "dtop_app.h"
#include "audio_record.h"
#include "nplayer_task_itl.h"
#include "m_datapath.h"

#define FLOAT_ROUND_POS(x) ((uint16_t)((float)(x) + 0.5f))
#define FLOAT_ABS(x) ((x)>0 ? (x): -(x))

#define AUDIO_SWEEP_PUT_ASRC_BUF_CNT  2
#define AUDIO_BROADCAST_TONE_FREQ_MIN 20
#define AUDIO_BROADCAST_TONE_FREQ_MAX 8000
#define AUDIO_BROADCAST_TONE_LEN_MIN  4000
#define AUDIO_BROADCAST_TONE_LEN_MAX  5000
#define AUDIO_SPK_RECORD_ONE_BUF_LEN  640

#define SWEEP_AUDIO_DAC_SPK_RANGE_OFFSET         -18

#define MIC_DUMP_RING_OFFSET 16

typedef struct audio_sweep_env {
    void     *mutex;
    uint16_t *sweep_play;
    uint16_t play_length;
    uint16_t first_init_flag     :1;
    uint16_t spk_stop_flag       :1;
    uint16_t loopback_flag       :1;
    uint16_t sine_tone_flag      :1;
    uint16_t mic_bitmap          :5;
    uint16_t cur_mic_idx         :3;
    uint16_t cur_mic_cnt         :3;
    uint16_t spk_asrc_start_flag :1;
}audio_sweep_env_t;

typedef struct audio_sweep_msg {
    uint8_t *addr;
    uint32_t len;
}audio_sweep_msg_t;

static audio_sweep_env_t sweep_env;

static int16_t* gen_sine_tone(uint16_t freq, uint16_t range, uint16_t *len)
{
    uint16_t num = 0xff;
    int16_t * wave_data;
    float data_temp;

    if (freq < AUDIO_BROADCAST_TONE_FREQ_MIN) {
        freq = AUDIO_BROADCAST_TONE_FREQ_MIN;
    } else if (freq > AUDIO_BROADCAST_TONE_FREQ_MAX) {
        freq = AUDIO_BROADCAST_TONE_FREQ_MAX;
    }

    float freq_temp = freq * 0.0000625f;

    for (uint16_t i = 2; i < 4096; i++) {
        float temp = i * freq_temp;
        float temp_res = 0;
        if (i == 2) {
            temp_res = (temp - FLOAT_ROUND_POS(temp));
            num = 2;
        }

        if (temp - FLOAT_ROUND_POS(temp) == 0) {
            num = i;
            break;
        } else if (FLOAT_ABS(temp - FLOAT_ROUND_POS(temp)) < temp_res) {
            temp_res = FLOAT_ABS(temp - FLOAT_ROUND_POS(temp));
            num = i;
            if (temp_res < 0.000001f) {
                break;
            }
        }
    }

    if (num < AUDIO_BROADCAST_TONE_LEN_MIN) {
        num = (uint16_t)(((uint16_t)AUDIO_BROADCAST_TONE_LEN_MIN / num + 1) * num);
    } else if (num > AUDIO_BROADCAST_TONE_LEN_MAX) {
        num = (uint16_t)(((uint16_t)AUDIO_BROADCAST_TONE_LEN_MAX / num - 1) * num);
    }

    assert(num <= (RING_ORIGIN_LENGTH / 2));

    wave_data = (int16_t *)(RING_ORIGIN_START + MIC_DUMP_RING_OFFSET);

    for (uint16_t i = 0; i < num; i++) {
        data_temp = sin_float(i * freq_temp * (float)TWO_PI + (float)HALF_PI) * range;
        if (data_temp > 0) {
            wave_data[i] = (int16_t)(data_temp + 0.5);
        } else {
            wave_data[i] = (int16_t)(data_temp - 0.5);
        }
    }

    *len = num;

    return wave_data;
}

static void rx_fifo_to_mem_done_cb(void *buf, uint32_t length) IRAM_TEXT(rx_fifo_to_mem_done_cb);
static void rx_fifo_to_mem_done_cb(void *buf, uint32_t length)
{
    dp_msg_t msg;
    audio_sweep_msg_t *p_dmsg = (audio_sweep_msg_t *)(&msg.data[0]);

    p_dmsg->len = length;
    p_dmsg->addr = buf;

    player_task_send_msg(MSG_LOOPBACK_CFG, &msg, true);
}

static void tx_fifo_to_mem_done_cb(void *buf, uint32_t length) IRAM_TEXT(tx_fifo_to_mem_done_cb);
static void tx_fifo_to_mem_done_cb(void *buf, uint32_t length)
{
    UNUSED(buf);
    UNUSED(length);

    if (sweep_env.loopback_flag == 0) {
        iot_share_task_post_event_from_isr(IOT_SHARE_TASK_QUEUE_LP, IOT_SHARE_EVENT_SPK_SINE_TONE_EVENT);
    }
}

static void audio_sweep_put_buf_to_asrc(void *p_data)
{
    UNUSED(p_data);

    os_acquire_mutex((os_mutex_h)(sweep_env.mutex));
    if (sweep_env.spk_stop_flag == 0) {
        if (sweep_env.loopback_flag == 0) {
            iot_tx_asrc_from_mem_mount_dma(IOT_ASRC_CHANNEL_1, (char *)sweep_env.sweep_play,
                    sweep_env.play_length * 2, tx_fifo_to_mem_done_cb);
        } else {
            audio_sweep_msg_t *msg = (audio_sweep_msg_t *)p_data;
            iot_tx_asrc_from_mem_mount_dma(IOT_ASRC_CHANNEL_1,
                    (char *)msg->addr - ((sweep_env.cur_mic_cnt - 1) - sweep_env.cur_mic_idx) * AUDIO_SPK_RECORD_ONE_BUF_LEN,
                        AUDIO_SPK_RECORD_ONE_BUF_LEN, tx_fifo_to_mem_done_cb);
            audio_record_run();
        }
    }
    os_release_mutex((os_mutex_h)(sweep_env.mutex));
}

static void audio_sweep_transmission(void)
{
    for (uint8_t i = 0; i < AUDIO_SWEEP_PUT_ASRC_BUF_CNT; i++) {
        iot_tx_asrc_from_mem_mount_dma(IOT_ASRC_CHANNEL_1, (char *)sweep_env.sweep_play,
                                       sweep_env.play_length * 2, tx_fifo_to_mem_done_cb);
    }
}

static void audio_sweep_init(void)
{
    iot_share_task_event_register(IOT_SHARE_TASK_QUEUE_LP, IOT_SHARE_EVENT_SPK_SINE_TONE_EVENT,
            audio_sweep_put_buf_to_asrc, NULL);

    player_task_msg_handler_register(MSG_LOOPBACK_CFG, audio_sweep_put_buf_to_asrc);

    sweep_env.mutex = (void *)os_create_mutex(LIB_MICDUMP_MID);

    audio_spk_adjust_gain_target(SWEEP_AUDIO_DAC_SPK_RANGE_OFFSET);//set to 16bit data 0db

    sweep_env.first_init_flag = 1;
}

void audio_sweep_sine_tone_start(uint16_t freq, uint16_t range)
{
    DBGLOG_MIC_DUMP_INFO("[DUMP] audio_sweep_sine_tone_start freq:%d range:%d\n", freq, range);

    if (sweep_env.first_init_flag == 0) {
        audio_sweep_init();
    }

    if (sweep_env.sine_tone_flag == 1) {
        DBGLOG_MIC_DUMP_INFO("[DUMP] ERR need call audio_sweep_sine_tone_stop first\n");
        return;
    }

    if (sweep_env.spk_stop_flag == 1) {
        if (sweep_env.spk_asrc_start_flag == 1) {
            sweep_env.spk_asrc_start_flag = 0;
            //close asrc
            iot_asrc_stop((uint8_t)BIT(IOT_ASRC_CHANNEL_1));
            iot_asrc_close((IOT_ASRC_CHANNEL_ID)IOT_ASRC_CHANNEL_1);
        }
        audio_sweep_spk_deinit(IOT_ASRC_CHANNEL_1, IOT_AUDIO_DAC_CHN_MONO_R);
    }

    sweep_env.sine_tone_flag = 1;
    sweep_env.spk_stop_flag = 0;
    sweep_env.spk_asrc_start_flag = 1;

    sweep_env.sweep_play = (uint16_t *)gen_sine_tone(freq, range, &sweep_env.play_length);

    audio_sweep_spk_config(IOT_AUDIO_DAC_SRC_ANC);
    audio_sweep_transmission();

    iot_audio_dac_multipath_sync();

    iot_asrc_overwrite_start(3);
    iot_asrc_overwrite_start(0);

    iot_audio_dac_start(IOT_AUDIO_DAC_CHN_MONO_R);
    iot_audio_dac_unmute(IOT_AUDIO_DAC_CHN_MONO_R, IOT_AUDIO_DAC_GAIN_DIRECTLY);
}

void audio_sweep_sine_tone_stop(void)
{
    sweep_env.spk_stop_flag = 1;
    sweep_env.sine_tone_flag = 0;
}

uint8_t audio_sweep_loopback_start(uint8_t bitmap, uint8_t reserved)
{
    uint8_t mic_bitmap = 0;
    UNUSED(reserved);

    DBGLOG_MIC_DUMP_INFO("[DUMP] audio_sweep_loopback_start bitmap:%x\n", bitmap);

    if (sweep_env.first_init_flag == 0) {
        audio_sweep_init();
    }

    if (bitmap == 0) {
        return RET_INVAL;
    }

    if (sweep_env.spk_stop_flag == 1) {
        if (sweep_env.spk_asrc_start_flag == 1) {
            sweep_env.spk_asrc_start_flag = 0;
            //close asrc
            iot_asrc_stop((uint8_t)BIT(IOT_ASRC_CHANNEL_1));
            iot_asrc_close((IOT_ASRC_CHANNEL_ID)IOT_ASRC_CHANNEL_1);
        }
        audio_sweep_spk_deinit(IOT_ASRC_CHANNEL_1, IOT_AUDIO_DAC_CHN_MONO_R);
    }

    for (uint8_t i = MIC_VOICE_MAIN_IDX; i < MIC_VOICE_MAX_IDX; i++) {
        if (BIT(i) & bitmap) {
            mic_bitmap = (uint8_t)BIT(i);
            sweep_env.cur_mic_idx = i;
            break;
        }
    }

    assert(mic_bitmap);

    if (sweep_env.mic_bitmap == mic_bitmap) {
        return RET_BUSY;
    }

    sweep_env.mic_bitmap = mic_bitmap;

    if (sweep_env.loopback_flag == 1) {
        return RET_OK;
    }


    sweep_env.sweep_play = (uint16_t *)(RING_ORIGIN_START + MIC_DUMP_RING_OFFSET);
    sweep_env.play_length = AUDIO_SPK_RECORD_ONE_BUF_LEN / 2;
    sweep_env.loopback_flag = 1;
    sweep_env.spk_stop_flag = 0;
    sweep_env.spk_asrc_start_flag = 1;

    memset(sweep_env.sweep_play, 0, sweep_env.play_length * 2);

    audio_sweep_spk_config(IOT_AUDIO_DAC_SRC_ASRC);
    audio_sweep_transmission();

    uint8_t voice_mic_bitmap = BIT(MIC_VOICE_MAIN_IDX);
#if (MIC_USE_NUM > 1)
    voice_mic_bitmap |= BIT(MIC_VOICE_SECOND_IDX);
#endif /* MIC_USE_NUM > 1 */
#if (MIC_USE_NUM > 2)
    voice_mic_bitmap |= BIT(MIC_VOICE_THIRD_IDX);
#endif /* MIC_USE_NUM > 2 */

    sweep_env.cur_mic_cnt = audio_voice_mic_record_start(voice_mic_bitmap, SM_MIC, IOT_ASRC_BT_TRIGGER, rx_fifo_to_mem_done_cb);
    assert(sweep_env.cur_mic_cnt > 0);

    iot_audio_dac_multipath_sync();

    //for mic open done
    os_delay(60);

    iot_asrc_overwrite_start(3);
    iot_asrc_overwrite_start(0);

    iot_audio_dac_start(IOT_AUDIO_DAC_CHN_MONO_R);
    iot_audio_dac_unmute(IOT_AUDIO_DAC_CHN_MONO_R, IOT_AUDIO_DAC_GAIN_DIRECTLY);

    return RET_OK;
}

uint8_t  audio_sweep_loopback_stop(void)
{
    if (sweep_env.loopback_flag == 0) {
        return RET_BUSY;
    }

    os_acquire_mutex((os_mutex_h)(sweep_env.mutex));
    sweep_env.spk_stop_flag = 1;
    sweep_env.loopback_flag = 0;
    sweep_env.spk_asrc_start_flag = 0;

    uint8_t voice_mic_bitmap = BIT(MIC_VOICE_MAIN_IDX);
#if (MIC_USE_NUM > 1)
        voice_mic_bitmap |= BIT(MIC_VOICE_SECOND_IDX);
#endif /* MIC_USE_NUM > 1 */
#if (MIC_USE_NUM > 2)
        voice_mic_bitmap |= BIT(MIC_VOICE_THIRD_IDX);
#endif /* MIC_USE_NUM > 2 */

    audio_voice_mic_record_stop(voice_mic_bitmap);

    //close asrc
    iot_asrc_stop((uint8_t)BIT(IOT_ASRC_CHANNEL_1));
    iot_asrc_close((IOT_ASRC_CHANNEL_ID)IOT_ASRC_CHANNEL_1);

    os_release_mutex((os_mutex_h)(sweep_env.mutex));

    return RET_OK;
}
