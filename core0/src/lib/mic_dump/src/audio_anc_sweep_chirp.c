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
#include "os_mem.h"

//hal
#include "iot_asrc.h"
#include "iot_audio.h"
#include "iot_audio_adc.h"
#include "iot_audio_dac.h"
#include "iot_share_task.h"
#include "iot_resource.h"
#include "iot_rtc.h"
#include "iot_equaliser.h"

//lib
#include "math.h"
#include "lib_dbglog.h"
#include "audio_record.h"
#include "dtop_app.h"
#include "audio_anc_sweep_chirp.h"
#include "m_dtop_ringmap.h"
#include "mic_dump.h"
#include "dump_resource.h"
#include "os_utils.h"
#include "audio_eq.h"
#include "player_audio_config_define.h"

#define SWEEP_CHIRP_PER_DUMP_LEN     2048
#define SWEEP_CHIRP_PER_IDLE_LEN     1024
#define SWEEP_CHIRP_IDLE_BUF_CNT     6
#define SWEEP_CHIRP_SPK_FREQ_IN      16000

#define SWEEP_CHIRP_AUDIO_DAC_SPK_RANGE_OFFSET         -19
#define SWEEP_CHIRP_AUDIO_EQ_DEAL_BUF_MOVE_DATA_OFFSET 5
#define SWEEP_CHIRP_AUDIO_FULL_SCALE_LIMIT_NUM         0

#define MIC_STORE_DUMP_MAX 2

#define MIC_DUMP_RING_OFFSET 16

typedef struct sweep_chirp_trans {
    uint16_t *sweep_chirp_record[3];
    int32_t *sweep_chirp_play[2];
    uint32_t play_length;
    uint32_t record_length[2];
    uint8_t sweep_save_data[MIC_DUMP_RING_OFFSET];
    int16_t mic_gain;
    uint8_t mic_id[2];
    uint8_t chan_id[2];
    uint8_t fifo_id[2];
    uint8_t record_asrc_id[2];
    uint8_t mic_power_id[2];
    uint8_t sweep_first_init:1;
    uint8_t only_mic_dump_flag:1;
    uint8_t only_two_mic_init_flag:1;
    uint8_t spk_open_flag:1;
    uint8_t mic_dump_cnt:3;
    uint8_t dump_start_flag:1;
} sweep_chirp_trans_t;

static sweep_chirp_trans_t trans;

static int16_t anc_generate_chirp(uint16_t index, uint8_t sine_float_flag)
{
    /* sampling number = rate * time */
    uint16_t number = (uint16_t)(16000 * 0.48f);

    //double beta = t1 / BASE_NUMBER;
    //double beta = 0.05340931211125344849025324893572;
    double beta = 0.3355806051239964071041036028867;   //2*PI*t1/BASE_NUMBER
    int16_t wave_data = 0;
    double data_temp;

    if (index < number) {
        //data_temp = index / (number - 1.0);
        data_temp = index * 0.00013022528975f;
        data_temp = beta * (pow_float((float)data_temp) - 1.0f);
        if (sine_float_flag) {
            data_temp = sin_float((float)data_temp) * 3276;
        } else {
            data_temp = cos_float((float)data_temp) * 3276;
        }

        if (data_temp > 0) {
            wave_data = (int16_t)(data_temp + 0.5f);
        } else {
            wave_data = (int16_t)(data_temp - 0.5f);
        }
    }

    return wave_data;
}

static void sweep_mic_adc_timer_done_callback(void)
{
    IOT_AUDIO_RXFIFO_LINK_ASRC_ID rx_fifo_matrix[8] = {0};   //lint !e64 type mismatch
    for (uint8_t i = 0; i < trans.mic_dump_cnt; i++) {
        rx_fifo_matrix[trans.fifo_id[i]] =
            (IOT_AUDIO_RXFIFO_LINK_ASRC_ID)trans.record_asrc_id[i] + IOT_AUDIO_LINK_ASRC_BASE;
    }

    iot_audio_rx_fifo_link_asrc_multipath(rx_fifo_matrix);
    iot_audio_dac_multipath_sync();

    iot_asrc_overwrite_start(3);
    iot_asrc_overwrite_start(0);

    for (uint8_t i = 0; i < trans.mic_dump_cnt; i++) {
        iot_audio_adc_gain_set((IOT_AUDIO_CHN_ID)trans.chan_id[i], trans.mic_gain);
    }

    if (trans.only_mic_dump_flag == 0) {
        iot_audio_dac_start(IOT_AUDIO_DAC_CHN_MONO_R);
        iot_audio_dac_unmute(IOT_AUDIO_DAC_CHN_MONO_R, IOT_AUDIO_DAC_GAIN_DIRECTLY);
    }
}

static void rx_fifo_to_mem_idle_cb(void *arg, uint32_t length) IRAM_TEXT(rx_fifo_to_mem_idle_cb);
static void rx_fifo_to_mem_idle_cb(void *buf, uint32_t length)
{
    UNUSED(buf);
    UNUSED(length);
    uint8_t asrc_bitmap = 0;

    for (uint8_t i = 0; i < trans.mic_dump_cnt; i++) {
        asrc_bitmap |= (uint8_t)BIT(trans.record_asrc_id[i]);
    }

    iot_asrc_stop(asrc_bitmap);

    iot_share_task_post_event_from_isr(IOT_SHARE_TASK_QUEUE_LP, IOT_SHARE_EVENT_SPK_MIC_DUMP_EVENT);
}

static void sweep_chirp_to_memory_idle_cb(void *arg, uint32_t length)
    IRAM_TEXT(sweep_chirp_to_memory_idle_cb);
static void sweep_chirp_to_memory_idle_cb(void *buf, uint32_t length)
{
    UNUSED(buf);
    UNUSED(length);
}

void audio_sweep_spk_config(uint8_t src)
{
    iot_asrc_config_t tx_asrc_cfg;
    iot_audio_dac_config_t audio_dac_cfg;

    //audio_dac_cfg.src = IOT_AUDIO_DAC_SRC_ASRC;
    audio_dac_cfg.src = (IOT_AUDIO_DAC_SRC_ID)src;
    audio_dac_cfg.fs = IOT_AUDIO_DAC_FS_800K;
    audio_dac_cfg.full_scale_limit = SWEEP_CHIRP_AUDIO_FULL_SCALE_LIMIT_NUM;
    audio_dac_cfg.dc_offset_dig_calibration = 0;

    tx_asrc_cfg.ppm = 0;
    tx_asrc_cfg.freq_in = 16000;
    tx_asrc_cfg.freq_out = 800000;
    tx_asrc_cfg.sync = true;
    tx_asrc_cfg.mode = IOT_ASRC_TX_MODE;
    tx_asrc_cfg.latch = IOT_ASRC_BT_LATCH;
    tx_asrc_cfg.trigger = IOT_ASRC_BT_TRIGGER;
    tx_asrc_cfg.half_word = IOT_ASRC_HALF_WORD_16BIT_PAIR;

    iot_asrc_open(IOT_ASRC_CHANNEL_1, &tx_asrc_cfg);
    iot_audio_dac_open(IOT_AUDIO_DAC_CHN_MONO_R, &audio_dac_cfg);

    iot_asrc_start(IOT_ASRC_CHANNEL_1);
}

static void audio_sweep_mic_config(bool bit16_mode)
{
    uint8_t adc_bitmap = 0;
    uint8_t power_bitmap = 0;
    iot_asrc_config_t rx_asrc_cfg;
    iot_audio_adc_config_t adc_cfg;

    adc_cfg.dfe.mode = IOT_RX_DFE_ADC;
    adc_cfg.dfe.fs = IOT_RX_DFE_FS_16K;

    rx_asrc_cfg.ppm = 0;
    rx_asrc_cfg.freq_in = 16000;
    rx_asrc_cfg.freq_out = 16000;
    rx_asrc_cfg.sync = true;
    rx_asrc_cfg.mode = IOT_ASRC_RX_MODE;
    rx_asrc_cfg.latch = IOT_ASRC_TIMER_LATCH_NONE;
    rx_asrc_cfg.trigger = IOT_ASRC_BT_TRIGGER;
    rx_asrc_cfg.half_word = IOT_ASRC_HALF_WORD_16BIT_PAIR;

    DBGLOG_MIC_DUMP_INFO("[DUMP] audio_sweep_mic_config mic_cnt:%d", trans.mic_dump_cnt);

    for (uint8_t i = 0; i < trans.mic_dump_cnt; i++) {
        iot_asrc_open((IOT_ASRC_CHANNEL_ID)trans.record_asrc_id[i], &rx_asrc_cfg);

        iot_audio_rx_fifo_open(trans.fifo_id[i]);
        iot_audio_rx_fifo_half_word(bit16_mode);
        adc_bitmap |= (uint8_t)BIT(trans.mic_id[i]);
        power_bitmap |= (uint8_t)((unsigned)trans.mic_power_id[i] << trans.mic_id[i]);
    }

    iot_audio_adc_open((uint8_t)adc_bitmap,
                       (uint8_t)power_bitmap,
                       sweep_mic_adc_timer_done_callback);

    for (uint8_t i = 0; i < trans.mic_dump_cnt; i++) {
        iot_asrc_link_rx_dfe((IOT_ASRC_CHANNEL_ID)trans.record_asrc_id[i], trans.chan_id[i]);

        iot_audio_adc_start((IOT_AUDIO_ADC_PORT_ID)trans.mic_id[i], (IOT_AUDIO_CHN_ID)trans.chan_id[i],
                &adc_cfg);

        iot_asrc_start((IOT_ASRC_CHANNEL_ID)trans.record_asrc_id[i]);
    }
}

static void sweep_chirp_transmission(const sweep_chirp_trans_t *trans_cfg)
{
    for (uint8_t i = 0; i < SWEEP_CHIRP_IDLE_BUF_CNT; i++) {
        iot_audio_rx_fifo_to_mem_mount_dma(trans_cfg->fifo_id[0],
                                           (char *)trans_cfg->sweep_chirp_record[2],
                                           SWEEP_CHIRP_PER_IDLE_LEN, sweep_chirp_to_memory_idle_cb);
        iot_tx_asrc_from_mem_mount_dma(IOT_ASRC_CHANNEL_1, (char *)trans_cfg->sweep_chirp_play[1],
                                       SWEEP_CHIRP_PER_IDLE_LEN, sweep_chirp_to_memory_idle_cb);
    }

    iot_audio_rx_fifo_to_mem_mount_dma(trans_cfg->fifo_id[0], (char *)trans_cfg->sweep_chirp_record[0],
                                       trans_cfg->record_length[0] * 2, rx_fifo_to_mem_idle_cb);
    iot_tx_asrc_from_mem_mount_dma(IOT_ASRC_CHANNEL_1, (char *)trans_cfg->sweep_chirp_play[0],
                                   trans_cfg->play_length * 4, sweep_chirp_to_memory_idle_cb);
}

static void audio_anc_sweep_chirp_dump(void *p_data)
{
    UNUSED(p_data);

    //uart dump
    for (uint32_t idx = 0; idx < RING_ORIGIN_LENGTH; idx += SWEEP_CHIRP_PER_DUMP_LEN) {
        audio_dump_2_uart((uint8_t *)trans.sweep_chirp_play[0] + idx, SWEEP_CHIRP_PER_DUMP_LEN);
        os_delay(40);
    }

    if (trans.only_mic_dump_flag) {
        for (uint8_t i = 0; i < trans.mic_dump_cnt; i++) {
            audio_sweep_mic_deinit(trans.fifo_id[i], trans.mic_id[i], trans.chan_id[i], trans.record_asrc_id[i]);
        }
        trans.only_mic_dump_flag = 0;
    } else {
        audio_sweep_mic_deinit(trans.fifo_id[0], trans.mic_id[0], trans.chan_id[0], trans.record_asrc_id[0]);
        iot_asrc_stop((uint8_t)BIT(IOT_ASRC_CHANNEL_1));
        iot_asrc_close(IOT_ASRC_CHANNEL_1);
    }

    memset(trans.sweep_chirp_play[0], 0, MIC_DUMP_RING_OFFSET);

    trans.dump_start_flag = 0;
}

void audio_anc_sweep_chirp_start(void)
{
    DBGLOG_MIC_DUMP_INFO("[DUMP] audio_anc_sweep_chirp start\n");

    if (trans.dump_start_flag == 1) {
        return;
    }

    if (trans.spk_open_flag == 1) {
        audio_sweep_spk_deinit(IOT_ASRC_CHANNEL_1, IOT_AUDIO_DAC_CHN_MONO_R);
    }

    trans.spk_open_flag = 1;
    trans.dump_start_flag = 1;

    memcpy(trans.sweep_chirp_play[0], trans.sweep_save_data, MIC_DUMP_RING_OFFSET);

    audio_sweep_spk_config(IOT_AUDIO_DAC_SRC_ANC);
    audio_sweep_mic_config(true);

    sweep_chirp_transmission(&trans);
}

static void audio_sweep_eq_coeff_config(void)
{
    music_eq_cfg_t *music_eq = (music_eq_cfg_t *)os_mem_malloc(PLAYER_MID, sizeof(music_eq_cfg_t));

    assert(music_eq != NULL);
    if (music_eq == NULL) {   //lint !e774 not always false if assert is empty
        return;
    }

    memset(music_eq, 0, sizeof(music_eq_cfg_t));

    audio_eq_coeff_init(true);
    audio_eq_coeff_spk_config(SWEEP_CHIRP_SPK_FREQ_IN);
    audio_eq_coeff_music_config(IOT_EQ_FIX_POINT, IOT_EQ_32_BIT, SWEEP_CHIRP_SPK_FREQ_IN,
                                (void *)music_eq->eq_band_list);

    os_mem_free(music_eq);
}

static void audio_anc_sweep_dump_first_init(void)
{
    trans.fifo_id[0] = dump_get_fifo_resource();
    trans.chan_id[0] = dump_get_chan_resource();
    trans.record_asrc_id[0] = dump_get_asrc_resource();

    trans.sweep_chirp_play[0] = (int32_t *)(RING_ORIGIN_START);

    trans.sweep_chirp_play[1] =
        (int32_t *)os_mem_malloc(LIB_MICDUMP_MID, SWEEP_CHIRP_PER_IDLE_LEN);
    trans.sweep_chirp_record[2] =
        (uint16_t *)os_mem_malloc(LIB_MICDUMP_MID, SWEEP_CHIRP_PER_IDLE_LEN);
    assert(trans.sweep_chirp_play[1]);
    assert(trans.sweep_chirp_record[2]);

    iot_share_task_event_register(IOT_SHARE_TASK_QUEUE_LP, IOT_SHARE_EVENT_SPK_MIC_DUMP_EVENT,
            audio_anc_sweep_chirp_dump, NULL);

    trans.spk_open_flag = 0;

    trans.sweep_first_init = 1;
}

void audio_anc_sweep_chirp_init(uint8_t mic_bitmap)
{
    uint8_t sine_float_flag = mic_bitmap & 0x80;
    mic_bitmap &= 0x7f;
    DBGLOG_MIC_DUMP_INFO("[DUMP] audio_anc_sweep_chirp init mic_bitmap:%x sin_float_flag:%x\n",
            mic_bitmap, sine_float_flag);

    uint32_t start_time = iot_rtc_get_global_time_ms();

    if (trans.sweep_first_init == 0) {
        audio_anc_sweep_dump_first_init();
    }

    audio_spk_adjust_gain_target(SWEEP_CHIRP_AUDIO_DAC_SPK_RANGE_OFFSET);   //set to 16bit data -1db

    for (uint8_t mic_idx = 0; mic_idx < RESOURCE_AUDIO_MAX; mic_idx++) {
        if (BIT(mic_idx) & mic_bitmap) {
            trans.mic_id[0] = iot_resource_lookup_adc((RESOURCE_AUDIO_PATH_ID)mic_idx);
            trans.mic_power_id[0] = iot_resource_lookup_bias(trans.mic_id[0]);
            trans.mic_dump_cnt = 1;
            break;
        }
    }

    trans.mic_gain = 0;
    trans.play_length = SWEEP_CHIRP_SAMPLING_TOTAL_NUMBER / 2;
    trans.record_length[0] = SWEEP_CHIRP_RECORD_LEN;

    trans.sweep_chirp_record[0] = (uint16_t *)(RING_ORIGIN_START + trans.play_length * 4);
    assert(trans.sweep_chirp_record[0]);

    audio_sweep_eq_coeff_config();

    memset((uint8_t *)trans.sweep_chirp_play[1], 0, SWEEP_CHIRP_PER_IDLE_LEN);

    /** Fill the 16 bit data to whole 32bit data aria */
    for (uint16_t i = 0; i < trans.play_length * 2; i++) {
        trans.sweep_chirp_play[0][i] = (int32_t)anc_generate_chirp((uint16_t)i, sine_float_flag);
    }

    //spk data through eq
    for (uint8_t i = 0; i < SWEEP_CHIRP_IDLE_BUF_CNT; i++) {
        audio_eq_buf_process((uint8_t *)trans.sweep_chirp_play[1], SWEEP_CHIRP_PER_IDLE_LEN,
                             sweep_chirp_to_memory_idle_cb, true);
    }

    audio_eq_buf_process((uint8_t *)trans.sweep_chirp_play[0], trans.play_length * 8,
                         sweep_chirp_to_memory_idle_cb, true);

    audio_eq_coeff_deinit();

    //move 32bit to 16bit
    for (uint32_t i = 0; i < trans.play_length * 2; i++) {
        int16_t* data_rst = (int16_t *)trans.sweep_chirp_play[0]; //lint !e740 !e826
        data_rst[i] = (int16_t)((uint32_t)trans.sweep_chirp_play[0][i]
                                            >> SWEEP_CHIRP_AUDIO_EQ_DEAL_BUF_MOVE_DATA_OFFSET);
    }

    uint32_t stop_time = iot_rtc_get_global_time_ms();

    DBGLOG_MIC_DUMP_INFO("play_length:%d record_length:%d use_cycle:%d mic_id:%d\n", trans.play_length,
                    trans.record_length[0], start_time - stop_time, trans.mic_id[0]);

    //init sweep dump simulate power
    //iot_audio_adc_config_t adc_cfg = {0};
    //adc_cfg.dfe.mode = IOT_RX_DFE_ADC;
    //adc_cfg.dfe.freq = IOT_RX_DFE_FS_16K;

    //iot_audio_adc_open(trans.mic_id, trans.chan_id, &adc_cfg);

    //iot_audio_rx_fifo_half_word(true);
    //iot_audio_rx_fifo_open(trans.fifo_id);
}

void audio_sweep_spk_deinit(uint8_t asrc_id, uint8_t dac_chn)
{
    UNUSED(asrc_id);
    DBGLOG_MIC_DUMP_INFO("[DUMP] audio_sweep_spk_deinit\n");

    iot_audio_dac_stop((IOT_AUDIO_DAC_CHANNEL)dac_chn);
    //iot_asrc_stop((uint8_t)BIT(asrc_id));
    iot_audio_dac_close((IOT_AUDIO_DAC_CHANNEL)dac_chn);
    //iot_asrc_close((IOT_ASRC_CHANNEL_ID)asrc_id);
    iot_audio_dac_multipath_sync();
}

void audio_sweep_mic_deinit(uint8_t fifo_id, uint8_t mic_id, uint8_t chan_id, uint8_t asrc_id)
{
    DBGLOG_MIC_DUMP_INFO("[DUMP] audio_sweep_mic_deinit\n");

    iot_audio_rx_fifo_close((uint16_t)BIT(fifo_id));
    iot_audio_adc_stop((IOT_AUDIO_ADC_PORT_ID)mic_id, (IOT_AUDIO_CHN_ID)chan_id);

    iot_asrc_close((IOT_ASRC_CHANNEL_ID)asrc_id);

    //mic0~2's micbias is put in the micbias map bit0~2,
    iot_audio_adc_close((uint8_t)BIT(mic_id));
    iot_audio_adc_multipath_sync();

    iot_audio_rx_fifo_dislink_asrc_multipath();
}

void audio_anc_sweep_chirp_stop(void)
{

}

static void audio_sweep_anc_tdd_noise_transmission(const sweep_chirp_trans_t *trans_cfg)
{
    audio_recv_done_handle_cb cb = sweep_chirp_to_memory_idle_cb;

    for (uint8_t i = 0; i < trans.mic_dump_cnt; i++) {
        iot_audio_rx_fifo_to_mem_mount_dma(trans_cfg->fifo_id[i], (char *)trans_cfg->sweep_chirp_play[0],
                RING_ORIGIN_LENGTH, cb);

        if (i + 1 == trans.mic_dump_cnt) {
            cb = rx_fifo_to_mem_idle_cb;
        }

        iot_audio_rx_fifo_to_mem_mount_dma(trans_cfg->fifo_id[i], (char *)trans_cfg->sweep_chirp_record[i],
                trans_cfg->record_length[i] * 2, cb);
    }
}

void audio_anc_tdd_noise_dump_start(uint8_t mic_bitmap, uint8_t bit16_mode)
{
    bool bit16_mode_flag = bit16_mode & 0x1? true : false;
    trans.mic_gain = ((int16_t)((bit16_mode >> 1) & 0xf)) * 32;

    DBGLOG_MIC_DUMP_INFO("[DUMP] audio_anc_tdd_noise_dump_start mic_bitmap:%x mode:%d mic_gain:%d\n",
            mic_bitmap, bit16_mode, trans.mic_gain);

    if (trans.dump_start_flag == 1) {
        return;
    }

    if (trans.sweep_first_init == 0) {
        audio_anc_sweep_dump_first_init();
    }

    trans.only_mic_dump_flag = 1;
    trans.dump_start_flag = 1;

    trans.mic_dump_cnt = 0;

    for (uint8_t mic_idx = 0, mic_cnt = 0;
            mic_cnt < MIC_STORE_DUMP_MAX && mic_idx < RESOURCE_AUDIO_MAX; mic_idx++) {
        if (BIT(mic_idx) & mic_bitmap) {
            trans.mic_dump_cnt++;
            trans.mic_id[mic_cnt] = iot_resource_lookup_adc((RESOURCE_AUDIO_PATH_ID)mic_idx);
            trans.mic_power_id[mic_cnt] = iot_resource_lookup_bias(trans.mic_id[mic_cnt]);
            trans.sweep_chirp_record[mic_cnt] = (uint16_t *)(RING_ORIGIN_START);
            trans.play_length = 0;
            trans.record_length[mic_cnt] = RING_ORIGIN_LENGTH / 2;

            if (mic_cnt == 1) {
                trans.sweep_chirp_record[1] = (uint16_t *)(RING_ORIGIN_START + RING_ORIGIN_LENGTH / 2);
                trans.record_length[0] = RING_ORIGIN_LENGTH / 4;
                trans.record_length[1] = RING_ORIGIN_LENGTH / 4;
                if (trans.only_two_mic_init_flag == 0) {
                    trans.only_two_mic_init_flag = 1;
                    trans.fifo_id[1] = dump_get_fifo_resource();
                    trans.chan_id[1] = dump_get_chan_resource();
                    trans.record_asrc_id[1] = dump_get_asrc_resource();
                }
            }
            mic_cnt++;
        }
    }

    audio_sweep_mic_config(bit16_mode_flag);

    audio_sweep_anc_tdd_noise_transmission(&trans);
}
