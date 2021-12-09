
#include "types.h"
#include "string.h"

#include "iot_memory_config.h"
#include "iot_gpio.h"
#include "iot_share_task.h"
#include "iot_audio.h"
#include "iot_audio_adc.h"
#include "iot_asrc.h"
#include "iot_resource.h"
//#include "iot_rtc.h"

#include "lib_dbglog.h"
#include "dtop_app.h"
#include "audio_record.h"
#include "player_internal.h"
#include "audio_cfg.h"
#include "audio_anc_sweep_chirp.h"
#include "audio_anc.h"
#include "generic_transmission_api.h"
#include "generic_transmission_config.h"
#include "mic_dump.h"
#include "os_utils.h"
#include "m_dtop_ringmap.h"
#include "dump_resource.h"

#define SPP_PKT_SIZE_MAX   128
#define AUDIO_ASRC_SEL_REG 0x030a0008

#define MIC_DUMP_USE_NUM     3u
#define MIC_DUMP_BUF_MAX_NUM 3u

#define DUMP_RECORD_BLK_MS             60u   //60ms
#define DUMP_RECORD_PCM_LEN_BYTE       2u
#define DUMP_RECORD_SAMPLING_FREQUENCY 16u
#define DUMP_RECORD_ONE_BUF_LEN \
    (DUMP_RECORD_PCM_LEN_BYTE * DUMP_RECORD_BLK_MS * DUMP_RECORD_SAMPLING_FREQUENCY)

typedef struct _dump_record_msg_t {
    uint8_t *addr;
    uint16_t len;
} dump_record_msg_t;

typedef struct _dump_one_record_buf {
    uint8_t mic_array[MIC_DUMP_USE_NUM][DUMP_RECORD_ONE_BUF_LEN];
} dump_one_record_buf_t;

typedef struct _dump_record_buf {
    dump_one_record_buf_t buf[MIC_DUMP_BUF_MAX_NUM];
} dump_record_buf_t;

typedef struct _dump_mic_env {
    /* audio mic start record flag */
    dump_record_buf_t *mic_rec_buf;
    dump_record_msg_t share_msg[MIC_DUMP_BUF_MAX_NUM];
    record_mic_config mic_cfg[MIC_DUMP_USE_NUM];
    uint32_t msg_id;
    uint16_t asrc_sel_cfg;
    uint16_t record_sample_cnt;
    uint16_t current_sample_cnt;
    uint16_t cur_buf_idx : 5;
    uint16_t record_buf_idx : 5;
    uint16_t start_mic_count : 3;
    uint16_t dump_pause : 1;
    uint16_t timer_first_start : 1;
    uint16_t record_flag : 1;
    uint8_t start_mic_bitmap : 6;
    uint8_t mic_stop_flag : 1;
    uint8_t adj_gtp_prio_flag : 1;
    volatile uint8_t link_buf_cnt[2];
} dump_mic_env;

static void audio_dump_mic_asrc_config(uint8_t mic_id);
static void audio_dump_mic_adc_start(uint8_t mic_cnt);
static void audio_dump_record_run(void);
static uint8_t audio_dump_rx_fifo_config(uint8_t mic_id);
static void audio_dump_mic_set_defaul_gain(void);

static dump_mic_env dump_env;
static uint8_t dump_mode = GENERIC_TRANSMISSION_IO_UART0;
static spp_dump_param spp_audio_dump_param = {.dump_delay_ms = 30,
                                                .need_ack = 0,
                                                .pkt_size = SPP_PKT_SIZE_MAX,
};

static void audio_dump_adc_timer_done_callback(void)
{
    IOT_AUDIO_RXFIFO_LINK_ASRC_ID rx_fifo_matrix[8] = {0};   //lint !e64 type mismatch
    for (uint8_t mic_idx = 0, mic_cnt = 0; mic_idx < MIC_DUMP_USE_NUM; mic_idx++) {
        if (BIT(mic_idx) & dump_env.start_mic_bitmap) {
            rx_fifo_matrix[dump_env.mic_cfg[mic_cnt].fifo_id] = (IOT_AUDIO_RXFIFO_LINK_ASRC_ID)(
                dump_env.mic_cfg[mic_cnt].asrc_id + IOT_AUDIO_LINK_ASRC_BASE);
            mic_cnt++;
        }
    }

    iot_audio_adc_multipath_sync();
    iot_audio_rx_fifo_link_asrc_multipath(rx_fifo_matrix);
    //iot_asrc_timer_start_all();
}

static void audio_dump_mic_env_start(void)
{
    uint8_t mic_bitmap = 0;
    uint8_t power_bitmap = 0;
    dump_env.record_flag = 1;
    dump_env.record_buf_idx = 0;
    dump_env.timer_first_start = 1;
    dump_env.dump_pause = 0;
    dump_env.cur_buf_idx = 0;
    dump_env.link_buf_cnt[0] = 0;
    dump_env.link_buf_cnt[1] = 0;
    dump_env.asrc_sel_cfg = 0;
    for (uint8_t mic_idx = 0, mic_cnt = 0; mic_idx < MIC_DUMP_USE_NUM; mic_idx++) {
        if (BIT(mic_idx) & dump_env.start_mic_bitmap) {
            audio_dump_rx_fifo_config(mic_cnt);
            audio_dump_mic_asrc_config(mic_cnt);
            mic_bitmap |= (uint8_t)BIT(dump_env.mic_cfg[mic_cnt].adc_id);
            //mic0~2's micbias is put in the bit0~2,
            power_bitmap |= (uint8_t)((unsigned)dump_env.mic_cfg[mic_cnt].mic_power_id
                                        << dump_env.mic_cfg[mic_cnt].adc_id);
            mic_cnt++;
        }
    }

    DBGLOG_MIC_DUMP_INFO("[DUMP] audio_dump_mic_env_start mic_bitmap:%x power_bitmap:%x\n",
                                mic_bitmap, power_bitmap);

    iot_audio_adc_open(mic_bitmap, power_bitmap, audio_dump_adc_timer_done_callback);

    for (uint8_t mic_idx = 0, mic_cnt = 0; mic_idx < MIC_DUMP_USE_NUM; mic_idx++) {
        if (BIT(mic_idx) & dump_env.start_mic_bitmap) {
            iot_asrc_start((IOT_ASRC_CHANNEL_ID)dump_env.mic_cfg[mic_cnt].asrc_id);
            audio_dump_mic_adc_start(mic_cnt);
            dump_env.asrc_sel_cfg |= (uint16_t)(0xFU << (dump_env.mic_cfg[mic_cnt].asrc_id * 4));
            mic_cnt++;
        }
    }

    iot_audio_rx_fifo_half_word(dump_env.start_mic_count != 1);

    for (uint8_t i = 0; i < MIC_DUMP_BUF_MAX_NUM; i++) {
        audio_dump_record_run();
    }
}

static void audio_dump_mic_set_defaul_gain(void)
{
    for (uint8_t mic_idx = 0, mic_cnt = 0; mic_idx < MIC_DUMP_USE_NUM; mic_idx++) {
        if (BIT(mic_idx) & dump_env.start_mic_bitmap) {
            iot_audio_adc_gain_set((IOT_AUDIO_CHN_ID)dump_env.mic_cfg[mic_cnt].chan_id,
                                   dump_env.mic_cfg[mic_cnt].adj_gain);
            mic_cnt++;
        }
    }
}

static void audio_dump_mic_env_stop(void)
{
    uint8_t mic_bitmap = 0;
    uint8_t fifo_bitmap = 0;

    dump_env.record_flag = 0;

    for (uint8_t mic_idx = 0, mic_cnt = 0; mic_idx < MIC_DUMP_USE_NUM; mic_idx++) {
        if (BIT(mic_idx) & dump_env.start_mic_bitmap) {
            fifo_bitmap |= (uint8_t)BIT(dump_env.mic_cfg[mic_cnt].fifo_id);
            mic_cnt++;
        }
    }

    iot_audio_rx_fifo_close(fifo_bitmap);

    for (uint8_t mic_idx = 0, mic_cnt = 0; mic_idx < MIC_DUMP_USE_NUM; mic_idx++) {
        if (BIT(mic_idx) & dump_env.start_mic_bitmap) {
            iot_audio_adc_stop((IOT_AUDIO_ADC_PORT_ID)dump_env.mic_cfg[mic_cnt].adc_id,
                               (IOT_AUDIO_CHN_ID)dump_env.mic_cfg[mic_cnt].chan_id);
            iot_asrc_stop((uint8_t)BIT(dump_env.mic_cfg[mic_cnt].asrc_id));
            mic_bitmap |= (uint8_t)BIT(dump_env.mic_cfg[mic_cnt].adc_id);
            iot_asrc_close((IOT_ASRC_CHANNEL_ID)dump_env.mic_cfg[mic_cnt].asrc_id);
            //iot_asrc_timer_close((IOT_ASRC_CHANNEL_ID)dump_env.mic_cfg[mic_cnt].asrc_id);
            mic_cnt++;
        }
    }

    iot_audio_adc_close(mic_bitmap);

    iot_audio_adc_multipath_sync();
}

void audio_dump_mic_start(uint16_t sample_cnt)
{
    if (dump_env.dump_pause == 0) {

        audio_dump_mic_set_defaul_gain();
        audio_dump_mic_env_start();
    } else {
        dump_env.dump_pause = 0;
    }

    DBGLOG_MIC_DUMP_INFO("audio_dump_mic_start pause:%d\n", dump_env.dump_pause);

    dump_env.mic_stop_flag = 0;
    dump_env.record_sample_cnt = sample_cnt;
    dump_env.current_sample_cnt = 0;
}

void audio_dump_mic_stop(uint8_t pause)
{
    dump_env.dump_pause = pause;

    if (dump_env.dump_pause == 0) {
        dump_env.mic_stop_flag = 1;
        while (dump_env.link_buf_cnt[1] || dump_env.link_buf_cnt[0]) {
            os_delay(5);
        }
        audio_dump_mic_env_stop();
    }

    DBGLOG_MIC_DUMP_INFO("[DUMP] audio_dump_mic_stop dump_pause:%d\n", dump_env.dump_pause);
}

void audio_dump_anc_start(uint8_t anc_mode)
{
    dump_set_anc_resource();
    audio_anc_switch_mode((AUDIO_DENOISE_MODE)anc_mode);
}

void audio_dump_anc_stop(void)
{
    audio_anc_switch_mode(AUDIO_DENOISE_PHYSICAL);
}

static uint8_t audio_dump_rx_fifo_config(uint8_t mic_id)
{
    return iot_audio_rx_fifo_open(dump_env.mic_cfg[mic_id].fifo_id);
}

static void audio_dump_mic_adc_start(uint8_t mic_id)
{
    iot_audio_adc_config_t mic_adc_cfg;

    mic_adc_cfg.dfe.mode = (IOT_RX_DFE_MODE)dump_env.mic_cfg[mic_id].dfe_mode;
    mic_adc_cfg.dfe.fs = (IOT_RX_DFE_FREQ_SAMPLING)dump_env.mic_cfg[mic_id].dfe_freq;
    iot_audio_adc_start((IOT_AUDIO_ADC_PORT_ID)dump_env.mic_cfg[mic_id].adc_id,
                        (IOT_AUDIO_CHN_ID)dump_env.mic_cfg[mic_id].chan_id, &mic_adc_cfg);
}

static void audio_dump_mic_asrc_config(uint8_t mic_id)
{
    iot_asrc_config_t mic_asrc_cfg = {0};

    mic_asrc_cfg.ppm = 0;
    mic_asrc_cfg.freq_in = dump_env.mic_cfg[mic_id].freq_in;
    mic_asrc_cfg.freq_out = dump_env.mic_cfg[mic_id].freq_out;
    mic_asrc_cfg.sync = true;
    mic_asrc_cfg.mode = IOT_ASRC_RX_MODE;
    mic_asrc_cfg.latch = IOT_ASRC_TIMER_LATCH_NONE;
    mic_asrc_cfg.trigger = IOT_ASRC_SELF_START;
    mic_asrc_cfg.half_word = IOT_ASRC_HALF_WORD_16BIT_PAIR;

    iot_asrc_open((IOT_ASRC_CHANNEL_ID)dump_env.mic_cfg[mic_id].asrc_id, &mic_asrc_cfg);

    //audio_dump_mic_adc_config(mic_id);

    iot_asrc_link_rx_dfe((IOT_ASRC_CHANNEL_ID)dump_env.mic_cfg[mic_id].asrc_id,
                         dump_env.mic_cfg[mic_id].chan_id);

    //iot_asrc_timer_open(dump_env.mic_cfg[mic_id].asrc_id, AUDIO_MIC_ASRC_DELAY_TIME);
}

static void audio_dump_system_mic_config(uint8_t mic_id, const record_mic_config *cfg)
{
    memcpy(&dump_env.mic_cfg[mic_id], cfg, sizeof(record_mic_config));

    DBGLOG_MIC_DUMP_INFO("audio_dump_system_mic_config mic_id:%d adc_id:%d asrc_id:%d chin_id:%d\n",
                    mic_id, dump_env.mic_cfg[mic_id].adc_id, dump_env.mic_cfg[mic_id].asrc_id,
                    dump_env.mic_cfg[mic_id].chan_id);

    //audio_dump_rx_fifo_config(mic_id);
    //audio_dump_mic_asrc_config(mic_id);
}

static void audio_dump_idle_done_cb(void *arg, uint32_t length) IRAM_TEXT(audio_dump_idle_done_cb);
static void audio_dump_idle_done_cb(void *arg, uint32_t length)
{
    UNUSED(arg);
    UNUSED(length);
    dump_env.link_buf_cnt[0]--;

    if (dump_env.mic_stop_flag) {
        if (dump_env.link_buf_cnt[0] == 0 && dump_env.link_buf_cnt[1] == 0) {
            uint32_t asrc_sel_reg = *(volatile uint32_t *)AUDIO_ASRC_SEL_REG;
            asrc_sel_reg |= dump_env.asrc_sel_cfg;
            (*(volatile uint32_t *)AUDIO_ASRC_SEL_REG) = (asrc_sel_reg);
        }
        return;
    }
}

static void audio_dump_recv_done_cb(void *arg, uint32_t length) IRAM_TEXT(audio_dump_recv_done_cb);
static void audio_dump_recv_done_cb(void *arg, uint32_t length)
{
    UNUSED(arg);
    UNUSED(length);
    dump_env.link_buf_cnt[1]--;

    if (dump_env.mic_stop_flag) {
        if (dump_env.link_buf_cnt[1] == 0 && dump_env.link_buf_cnt[0] == 0) {
            uint32_t asrc_sel_reg = *(volatile uint32_t *)AUDIO_ASRC_SEL_REG;
            asrc_sel_reg |= dump_env.asrc_sel_cfg;
            (*(volatile uint32_t *)AUDIO_ASRC_SEL_REG) = (asrc_sel_reg);
        }
        return;
    }

    dump_record_msg_t *msg = &dump_env.share_msg[dump_env.record_buf_idx];
    uint8_t *buf = &(dump_env.mic_rec_buf->buf[dump_env.record_buf_idx].mic_array[0][0]);

    msg->addr = (uint8_t *)buf;
    msg->len = 0;

    for (uint8_t i = 0; i < dump_env.start_mic_count; i++) {
        msg->len += DUMP_RECORD_ONE_BUF_LEN;
    }

    dump_env.record_buf_idx++;
    dump_env.record_buf_idx %= MIC_DUMP_BUF_MAX_NUM;

    if (dump_env.timer_first_start == 1) {
        dump_env.timer_first_start = 0;
        //iot_asrc_timer_stop_all();
    }

    if (dump_env.record_flag) {
        iot_share_task_post_msg_from_isr(IOT_SHARE_TASK_QUEUE_LP, (int32_t)dump_env.msg_id, (void *)msg);
    }
}

static void audio_dump_record_run(void)
{
    if (dump_env.record_flag) {
        audio_recv_done_handle_cb cb = audio_dump_idle_done_cb;
        for (uint8_t mic_id = 0, mic_cnt = 0; mic_id < MIC_DUMP_USE_NUM; mic_id++) {
            if (BIT(mic_id) & dump_env.start_mic_bitmap) {
                if (mic_cnt + 1 == dump_env.start_mic_count) {
                    dump_env.link_buf_cnt[1]++;
                    cb = audio_dump_recv_done_cb;
                } else {
                    dump_env.link_buf_cnt[0]++;
                }

                iot_audio_rx_fifo_to_mem_mount_dma(
                    dump_env.mic_cfg[mic_cnt].fifo_id,
                    (char *)(&(
                        dump_env.mic_rec_buf->buf[dump_env.cur_buf_idx].mic_array[mic_cnt][0])),
                    DUMP_RECORD_ONE_BUF_LEN, cb);
                //DBGLOG_MIC_DUMP_INFO("audio_dump_record_run %d %d %d\n",
                //        mic_id, mic_cnt, dump_env.mic_cfg[mic_cnt].adc_id);
                mic_cnt++;
            }
        }
        dump_env.cur_buf_idx++;
        dump_env.cur_buf_idx %= MIC_DUMP_BUF_MAX_NUM;
    }
}

uint8_t audio_dump_mode_set(uint8_t mode)
{
    uint8_t ret = RET_INVAL;

    if (mode <= GENERIC_TRANSMISSION_IO_NUM) {
        dump_mode = mode;
        ret = RET_OK;
    }

    return ret;
}

uint8_t spp_audio_dump_parameter_set(const spp_dump_param *param)
{
    uint8_t ret = RET_INVAL;

    if (param->dump_delay_ms < 1) {
        return ret;
    }
    if (param->pkt_size > SPP_PKT_SIZE_MAX) {
        return ret;
    }
    memcpy(&spp_audio_dump_param, param, sizeof(spp_audio_dump_param));
    ret = RET_OK;
    return ret;
}

void audio_dump_2_uart(const uint8_t *buf, uint32_t buf_len)
{
    int32_t remain_len = (int32_t)buf_len;
    int32_t ret;
    int32_t send_len = remain_len;
    bool need_ack = true;
    uint32_t delay_time = 1;
    generic_transmission_tx_mode_t dump_tx_mode = GENERIC_TRANSMISSION_TX_MODE_ASAP;

    if (dump_env.adj_gtp_prio_flag == 0) {
        dump_env.adj_gtp_prio_flag = 1;
        generic_transmission_set_priority(CONFIG_GENERIC_TRANSMISSION_CONSUMER_TASK_PRIO_HIGH);
    }

    //uint32_t start_time = iot_rtc_get_global_time_ms();
    do {
        /* Attention!!!!
         * This test code is used for "throughput" testing, there's no schedule out operations,
         * such as semaphore take, queue wait, os_delay and etc.
         * In actual scenario, should not call the API in a forever loop without any delay.
         * Especially, If the retern value is a negative value, it means there's error, such
         * as -RET_NOMEM, please do delay, semaphore take or other method which can cause yield.
         */

        if (dump_mode == GENERIC_TRANSMISSION_IO_SPP) {
            if (remain_len > SPP_PKT_SIZE_MAX) {
                send_len = spp_audio_dump_param.pkt_size;
            } else {
                send_len = remain_len;
            }
            dump_tx_mode = GENERIC_TRANSMISSION_TX_MODE_LAZY;
            need_ack = (bool)spp_audio_dump_param.need_ack;
            delay_time = spp_audio_dump_param.dump_delay_ms;
        } else {
            send_len = remain_len;
        }

        ret = generic_transmission_data_tx(
            dump_tx_mode, GENERIC_TRANSMISSION_DATA_TYPE_AUDIO_DUMP, DUMP_TID,
            (generic_transmission_io_t)dump_mode, buf + buf_len - remain_len, (uint32_t)send_len,
            /* need ack */ need_ack);
        if (ret >= 0) {
            remain_len -= ret;
            os_delay(delay_time);
        } else {
            os_delay(delay_time);
        }
    } while (ret < 0 || remain_len > 0);
   //uint32_t stop_time = iot_rtc_get_global_time_ms();
   //DBGLOG_MIC_DUMP_INFO("audio_dump_2_uart dump_time:%d addr:%x %d\n", stop_time - start_time, buf, buf_len);
}

static void audio_dump_share_msg_func(void *param)
{
    dump_record_msg_t *msg = (dump_record_msg_t *)param;

    if ((!dump_env.dump_pause) && dump_env.current_sample_cnt < dump_env.record_sample_cnt) {
        audio_dump_2_uart(msg->addr, msg->len);
        dump_env.current_sample_cnt++;
    }

    //DBGLOG_MIC_DUMP_WARNING("audio_dump_share_msg_func dump_pause:%d\n", dump_env.dump_pause);

    audio_dump_record_run();
}

void audio_dump_init(uint8_t mic_bitmap, int16_t gain)
{
    static uint8_t first_dump_init = 0;
    DBGLOG_MIC_DUMP_INFO("[DUMP] audio_dump_init mic_bitmap:%x init:%d dump_pause:%d\n",
            mic_bitmap, first_dump_init, dump_env.dump_pause);

    if (first_dump_init == 0) {
        first_dump_init = 1;
        dump_env.msg_id = (uint32_t)iot_share_task_msg_register(audio_dump_share_msg_func);

        dump_env.mic_rec_buf =
            (dump_record_buf_t *)(RING_ORIGIN_START + (SWEEP_CHIRP_SAMPLING_TOTAL_NUMBER * 2));

        for (uint8_t mic_id = 0, mic_cnt = 0;
             mic_cnt < MIC_DUMP_USE_NUM && mic_id < RESOURCE_AUDIO_MAX; mic_id++) {
            /* config mic */
            if (BIT(mic_id) & mic_bitmap) {
                dump_env.start_mic_count++;
                record_mic_config cfg = {0};
                cfg.adc_id = iot_resource_lookup_adc((RESOURCE_AUDIO_PATH_ID)mic_id);
                cfg.asrc_id = dump_get_asrc_resource();
                cfg.chan_id = dump_get_chan_resource();
                cfg.fifo_id = dump_get_fifo_resource();
                cfg.dfe_mode = AUDIO_RX_DFE_ADC;
                cfg.adj_gain = gain;
                cfg.dfe_freq = AUDIO_RX_DFE_FREQ_16K;
                cfg.freq_in = AUDIO_DEFAULT_MIC_FREQ_IN;
                cfg.freq_out = AUDIO_DEFAULT_MIC_FREQ_OUT;
                cfg.mic_power_id = iot_resource_lookup_bias(cfg.adc_id);

                if (cfg.adc_id < AUDIO_ADC_PORT_MAX && cfg.asrc_id < AUDIO_MIC_ASRC_CHANNEL_MAX
                    && cfg.chan_id < AUDIO_AUDIO_RX_DFE_MAX) {
                    audio_dump_system_mic_config(mic_cnt, &cfg);
                } else {
                    DBGLOG_MIC_DUMP_WARNING(
                        " mic_dump_init mic_id:%d mic_cnt:%d adc_id:%d asrc_id:%d chin_id:%d\n",
                        mic_id, mic_cnt, cfg.adc_id, cfg.asrc_id, cfg.chan_id);
                }

                mic_cnt++;
            }
        }
    } else {
        dump_env.start_mic_count = 0;
        for (uint8_t mic_id = 0, mic_cnt = 0;
             mic_cnt < MIC_DUMP_USE_NUM && mic_id < RESOURCE_AUDIO_MAX; mic_id++) {
            /* config mic */
            if (BIT(mic_id) & mic_bitmap) {
                dump_env.start_mic_count++;
                dump_env.mic_cfg[mic_cnt].adc_id = iot_resource_lookup_adc((RESOURCE_AUDIO_PATH_ID)mic_id);
                dump_env.mic_cfg[mic_cnt].mic_power_id = iot_resource_lookup_bias(dump_env.mic_cfg[mic_cnt].adc_id);
                DBGLOG_MIC_DUMP_INFO("[DUMP] mic_count:%d adc_id:%d mic_power_id:%d\n",
                        dump_env.start_mic_count, dump_env.mic_cfg[mic_cnt].adc_id, dump_env.mic_cfg[mic_cnt].mic_power_id);
                mic_cnt++;
            }
        }
    }

    dump_env.dump_pause = 0;
    dump_env.start_mic_bitmap = mic_bitmap;
    assert(dump_env.start_mic_count && dump_env.start_mic_bitmap);
}
