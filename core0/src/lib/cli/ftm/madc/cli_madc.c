/****************************************************************************

Copyright(c) 2016 by WuQi Technologies. ALL RIGHTS RESERVED.

This Information is proprietary to WuQi Technologies and MAY NOT
be copied by any method or incorporated into another program without
the express written consent of WuQi. This Information or any portion
thereof remains the property of WuQi. The Information contained herein
is believed to be accurate and WuQi assumes no responsibility or
liability for its use in any way and conveys no license or title under
any patent or copyright and makes no representation or warranty that this
Information is free from patent or copyright infringement.

****************************************************************************/
#include "types.h"
#include "cli_command.h"
#include "cli_ftm_definition.h"
#include "os_mem.h"
#include "cli.h"
#include "hw_reg_api.h"
#include "lib_dbglog.h"
#include "iot_adc.h"
#include "cli_madc.h"
#include "iot_audio_dac.h"
#include "iot_audio_adc.h"

#include "iot_dma.h"
#include "iot_asrc.h"
#include "iot_audio.h"
#include "os_utils.h"

#include "os_task.h"
#include "sdm_dac.h"

#define MADC_DUMP_LEN (10)  //(5 * 1024)
#define TX_BUF_LEN 32*4     //320 * 4
#define FREQUENCE  32
#define THREE_MIC_DEFAULT_MICBAIS_MAP 4

uint32_t mdata[MADC_DUMP_LEN];
uint32_t tx_ll_buffer[32];   //[320]

#pragma pack(push) /* save the pack status */
#pragma pack(1)    /* 1 byte align */
typedef struct _cli_audio_spk_ana_gain_ack {
    uint8_t result;
} cli_audio_spk_ana_gain_ack;

typedef struct _cli_audio_spk_ana_gain_param {
    uint8_t spk_chn;
    uint32_t gain_step;
    uint8_t up;
} cli_audio_spk_ana_gain_param;

/* set data ack */
typedef struct madc_start_ack_result {
    uint8_t result;
} madc_start_ack_result_t;

typedef struct madc_stop_ack_result {
    uint8_t result;
} madc_stop_ack_result_t;

typedef struct madc_init_ack_result {
    uint8_t result;
} madc_init_ack_result_t;

typedef struct madc_set_chn_ack_result {
    uint8_t result;
} madc_set_chn_ack_result_t;

typedef struct madc_set_gain_ack_result {
    uint8_t result;
} madc_set_gain_ack_result_t;

typedef struct madc_poll_data_ack_result {
    uint8_t result;
    uint32_t madc_data;
} madc_poll_data_ack_result_t;

typedef struct madc_dump_ack_result {
    uint32_t dump_len;
    uint32_t dump_data;
} madc_dump_ack_result_t;

typedef struct madc_ftm_stop_cmd {
    uint8_t phase_no;
} madc_ftm_stop_cmd_t;

typedef struct madc_ftm_init_cmd {
    uint8_t phase_no;
} madc_ftm_init_cmd_t;

typedef struct madc_ftm_chn_sel_cmd {
    uint8_t phase_no;
    uint32_t singe_type;
} madc_ftm_chn_sel_cmd_t;

typedef struct madc_ftm_gain_sel_cmd {
    uint32_t gain_val;
} madc_ftm_gain_sel_cmd_t;

typedef struct madc_ftm_dump_cmd {
    uint32_t buf_len;
} madc_ftm_dump_cmd_t;
#pragma pack(pop)

void cli_madc_start_handler(uint8_t *buffer, uint32_t bufferlen)
{
    madc_start_ack_result_t rx_result;
    rx_result.result = RET_OK;
    UNUSED(buffer);
    UNUSED(bufferlen);

    iot_adc_start();

    cli_interface_msg_response(CLI_MODULEID_FTM, CLI_MSGID_FTM_MADC_START, (uint8_t *)&rx_result,
                               sizeof(madc_start_ack_result_t), 0, RET_OK);
}

void cli_madc_stop_handler(uint8_t *buffer, uint32_t bufferlen)
{
    madc_stop_ack_result_t rx_result;
    rx_result.result = RET_OK;

    (void)bufferlen;
    (void)buffer;

    iot_adc_stop();

    DBGLOG_LIB_CLI_INFO("cli_madc_stop_handler\n");
    cli_interface_msg_response(CLI_MODULEID_FTM, CLI_MSGID_FTM_MADC_STOP, (uint8_t *)&rx_result,
                               sizeof(madc_stop_ack_result_t), 0, RET_OK);
}

static int sine_table[FREQUENCE] = {
#if 0
    0x000000, 0x17b915, 0x2e88c8, 0x438ead, 0x55fbf3, 0x651b50, 0x7057ff, 0x774373,
    0x799999, 0x774373, 0x7057ff, 0x651b50, 0x55fbf3, 0x438ead, 0x2e88c8, 0x17b915,
    0x000000, 0xe846eb, 0xd17738, 0xbc7153, 0xaa040d, 0x9ae4b0, 0x8fa801, 0x88bc8d,
    0x866667, 0x88bc8d, 0x8fa801, 0x9ae4b0, 0xaa040d, 0xbc7153, 0xd17738, 0xe846eb,

#else
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
#endif
};

static void tone_fill_16(char *buf, int len)
{
    int i = 0;
    int *p;

    assert(len > 0);
    assert((len & 3) == 0);

    p = (int *)buf;
    while (len > 0) {
        *p = sine_table[i];
        p++;

        i = (i + 1) % FREQUENCE;
        len -= 4;
    }
}

static void demo_data_buffer_init(void)
{
    tone_fill_16((char *)tx_ll_buffer, TX_BUF_LEN);
}

static void cli_audio_spk_task(void *arg)
{
    UNUSED(arg);

    while (1) {
        iot_tx_asrc_from_mem_mount_dma(0, (char *)tx_ll_buffer, TX_BUF_LEN, NULL);
        os_delay(5);
        iot_tx_asrc_from_mem_mount_dma(1, (char *)tx_ll_buffer, TX_BUF_LEN, NULL);
        os_delay(5);
        DBGLOG_LIB_CLI_INFO("##\n");
    }
}

static void cli_audio_spk_init(void)
{
    iot_asrc_config_t asrc_cfg = {0};
    iot_audio_dac_config_t audio_cfg = {0};

    audio_cfg.src = IOT_AUDIO_DAC_SRC_ASRC;
    audio_cfg.fs = IOT_AUDIO_DAC_FS_800K;
    audio_cfg.full_scale_limit = 0;
    audio_cfg.dc_offset_dig_calibration = 0;

    asrc_cfg.ppm = 0;
    asrc_cfg.freq_in = 32000;
    asrc_cfg.freq_out = 32000;
    asrc_cfg.sync = false;
    asrc_cfg.half_word = IOT_ASRC_HALF_WORD_DISABLED;
    asrc_cfg.trigger = IOT_ASRC_SELF_START;
    asrc_cfg.latch = IOT_ASRC_BT_LATCH;
    asrc_cfg.mode = IOT_ASRC_TX_MODE;

    iot_audio_dac_init();

    iot_asrc_open(IOT_ASRC_CHANNEL_0, &asrc_cfg);
    iot_asrc_open(IOT_ASRC_CHANNEL_1, &asrc_cfg);
    iot_audio_dac_open(IOT_AUDIO_DAC_CHN_STEREO, &audio_cfg);
    iot_tx_dfe_hpf_enable(IOT_TX_DFE_CHN_0, IOT_ENABLE);
    iot_tx_dfe_hpf_enable(IOT_TX_DFE_CHN_1, IOT_ENABLE);
    iot_asrc_start(IOT_ASRC_CHANNEL_0);
    iot_asrc_start(IOT_ASRC_CHANNEL_1);
    iot_audio_dac_multipath_sync();
    iot_audio_dac_start(IOT_AUDIO_DAC_CHN_STEREO);
    iot_audio_dac_unmute(IOT_AUDIO_DAC_CHN_STEREO, IOT_AUDIO_DAC_GAIN_DIRECTLY);

    demo_data_buffer_init();

    os_create_task_ext(cli_audio_spk_task, NULL, 5, 256, "spk_app");
}

static void cli_madc_timer_done_callback(void)
{
}

void cli_madc_init_handler(uint8_t *buffer, uint32_t bufferlen)
{
    uint8_t mic_bitmap = BIT(IOT_AUDIO_ADC_PORT_0) | BIT(IOT_AUDIO_ADC_PORT_1) | BIT(IOT_AUDIO_ADC_PORT_2);
    uint8_t power_bitmap = THREE_MIC_DEFAULT_MICBAIS_MAP; //binary:100,mic 0 and 1 use micbias0,bit0 and bit1 fill 0, mic2 use micbias1,bit2 fill 1.
    madc_init_ack_result_t rx_result;
    rx_result.result = RET_OK;
    (void)bufferlen;
    (void)buffer;

    iot_asrc_init();
    iot_audio_fifo_init();
    //iot_mic_init();

    iot_audio_adc_init();
    iot_audio_adc_open(mic_bitmap, power_bitmap, cli_madc_timer_done_callback);
    cli_audio_spk_init();

    iot_adc_open( 0, IOT_ADC_SAMPLE_RATE_16K, 0, IOT_ADC_WORK_MODE_SINGLE);

    cli_interface_msg_response(CLI_MODULEID_FTM, CLI_MSGID_FTM_MADC_INIT, (uint8_t *)&rx_result,
                               sizeof(madc_init_ack_result_t), 0, RET_OK);
}

void cli_madc_set_channel_handler(uint8_t *buffer, uint32_t bufferlen)
{
    madc_set_chn_ack_result_t rx_result;
    rx_result.result = RET_OK;
    (void)bufferlen;
    madc_ftm_chn_sel_cmd_t *chn_sel = (madc_ftm_chn_sel_cmd_t *)buffer;
    DBGLOG_LIB_CLI_INFO("chn_sel->phase_no,%d chn_sel->singe_type%d\n", chn_sel->singe_type,
                   chn_sel->singe_type);

    iot_adc_ana_chn_sel(chn_sel->phase_no, chn_sel->singe_type);

    DBGLOG_LIB_CLI_INFO("chn_sel->phase_no,%d chn_sel->singe_type%d\n", chn_sel->phase_no,
                   chn_sel->singe_type);
    cli_interface_msg_response(CLI_MODULEID_FTM, CLI_MSGID_FTM_MADC_CHANNEL_SELECT,
                               (uint8_t *)&rx_result, sizeof(madc_set_chn_ack_result_t), 0, RET_OK);
}

void cli_madc_poll_data_handler(uint8_t *buffer, uint32_t bufferlen)
{
    UNUSED(buffer);
    UNUSED(bufferlen);

    madc_poll_data_ack_result_t rx_result;
    rx_result.result = RET_OK;
    rx_result.madc_data = iot_adc_dfe_poll();

    cli_interface_msg_response(CLI_MODULEID_FTM, CLI_MSGID_FTM_MADC_POLL_DATA,
                               (uint8_t *)&rx_result, sizeof(madc_poll_data_ack_result_t), 0,
                               RET_OK);
}

void cli_madc_dump_handler(uint8_t *buffer, uint32_t bufferlen)
{
    madc_dump_ack_result_t rx_result;
    uint32_t mdac_num;
    (void)bufferlen;
    madc_ftm_dump_cmd_t *dump_data = (madc_ftm_dump_cmd_t *)buffer;

    for (mdac_num = 0; mdac_num < dump_data->buf_len; mdac_num++) {
        mdata[mdac_num] = iot_adc_dfe_poll();
    }

    rx_result.dump_len = dump_data->buf_len;
    rx_result.dump_data = (uint32_t)&mdata[0];
    DBGLOG_LIB_CLI_INFO("rx_result.dump_len, %d rx_result.dump_data%d\n", rx_result.dump_len,
                   rx_result.dump_data);

    cli_interface_msg_response(CLI_MODULEID_FTM, CLI_MSGID_FTM_MADC_DUMP, (uint8_t *)&rx_result,
                               sizeof(madc_dump_ack_result_t), 0, RET_OK);
}

void cli_madc_set_audio_gain_handler(uint8_t *buffer, uint32_t bufferlen)
{
    (void)bufferlen;
    cli_audio_spk_ana_gain_param *spk_ana_gain_param = (cli_audio_spk_ana_gain_param *)buffer;
    cli_audio_spk_ana_gain_ack spk_ana_gain_ack = {0};

    DBGLOG_LIB_CLI_INFO("SPK ANA GAIN START\n");

    iot_tx_dfe_ana_gain_step_set(spk_ana_gain_param->spk_chn, spk_ana_gain_param->gain_step);
    iot_tx_dfe_adjust_analog_gain(spk_ana_gain_param->spk_chn, DAC_GAIN_CONTROL_DIRECTLY,
                                  spk_ana_gain_param->up);
    os_delay(10);
    DBGLOG_LIB_CLI_INFO("spk_ana_gain_param->spk_chn,%d spk_ana_gain_param->gain_step%d\n",
                   spk_ana_gain_param->spk_chn, spk_ana_gain_param->gain_step);

    cli_interface_msg_response(CLI_MODULEID_FTM, CLI_MSGID_FTM_SET_AUDIO_GAIN,
                               (uint8_t *)&spk_ana_gain_ack, sizeof(madc_dump_ack_result_t), 0,
                               RET_OK);
}

CLI_ADD_COMMAND(CLI_MODULEID_FTM, CLI_MSGID_FTM_MADC_START, cli_madc_start_handler);
CLI_ADD_COMMAND(CLI_MODULEID_FTM, CLI_MSGID_FTM_MADC_STOP, cli_madc_stop_handler);
CLI_ADD_COMMAND(CLI_MODULEID_FTM, CLI_MSGID_FTM_MADC_INIT, cli_madc_init_handler);
CLI_ADD_COMMAND(CLI_MODULEID_FTM, CLI_MSGID_FTM_MADC_CHANNEL_SELECT, cli_madc_set_channel_handler);
CLI_ADD_COMMAND(CLI_MODULEID_FTM, CLI_MSGID_FTM_MADC_POLL_DATA, cli_madc_poll_data_handler);
CLI_ADD_COMMAND(CLI_MODULEID_FTM, CLI_MSGID_FTM_MADC_DUMP, cli_madc_dump_handler);
CLI_ADD_COMMAND(CLI_MODULEID_FTM, CLI_MSGID_FTM_SET_AUDIO_GAIN, cli_madc_set_audio_gain_handler);
