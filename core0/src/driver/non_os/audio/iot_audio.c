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
/* common includes */
#include "types.h"
#include "string.h"

/* hw includes */
#include "apb.h"
#include "aud_if.h"
#include "aud_glb.h"
#include "dma.h"
#include "aud_intf_pwr.h"
#include "iot_audio.h"
#include "driver_dbglog.h"
#include "iot_timer.h"
/*
 * TODO: as this is non os drv, so we can't use
 * mutex to sync, it's better to use mutex
 */
#include "critical_sec.h"

typedef struct audio_fifo_info {
    uint8_t fifo_id;
    IOT_DMA_CHANNEL_ID dma_channel;
    bool_t in_use;
    bool_t linked;
} audio_fifo_info_t;

static uint8_t audio_rx_fifo_counter;
static uint8_t audio_tx_fifo_counter;
static audio_fifo_info_t audio_rx_fifos[IOT_AUDIO_RX_FIFO_NUM];
static audio_fifo_info_t audio_tx_fifos[IOT_AUDIO_TX_FIFO_NUM];

void iot_audio_fifo_init(void)
{
    uint8_t i;
    for (i = 0; i < IOT_AUDIO_RX_FIFO_NUM; i++) {
        audio_rx_fifos[i].fifo_id = i;
        audio_rx_fifos[i].in_use = false;
        audio_rx_fifos[i].linked = false;
        audio_rx_fifos[i].dma_channel = IOT_DMA_CHANNEL_NONE;
    }
    for (i = 0; i < IOT_AUDIO_TX_FIFO_NUM; i++) {
        audio_tx_fifos[i].fifo_id = i;
        audio_tx_fifos[i].in_use = false;
        audio_tx_fifos[i].linked = false;
        audio_tx_fifos[i].dma_channel = IOT_DMA_CHANNEL_NONE;
    }

    audio_rx_fifo_counter = 0;
    audio_tx_fifo_counter = 0;
    audio_fifo_init();
}

void iot_audio_fifo_deinit(void)
{
    memset(audio_tx_fifos, 0x00, sizeof(audio_fifo_info_t) * IOT_AUDIO_TX_FIFO_NUM);
    memset(audio_rx_fifos, 0x00, sizeof(audio_fifo_info_t) * IOT_AUDIO_RX_FIFO_NUM);
    audio_fifo_deinit();
}

uint32_t iot_audio_get_global_clk_status(void)
{
    return audio_get_global_clk_status();
}

void iot_audio_rx_dma_reset(void)
{
    audio_reset_module(AUDIO_MODULE_DMA_RX);
}

void iot_audio_rx_fifo_half_word(bool_t enable)
{
    audio_enable_rx_fifo_half_word_mode(enable);
}

static void iot_audio_rx_fifo_dma_config(IOT_DMA_CHANNEL_ID ch, DMA_PERI_REQ src_req)
{
    DMA_CONTROLLER controller = DMA_CONTROLLER0;

    dma_ch_config_t cfg;

    cfg.trans_type = DMA_TRANS_PERI_TO_MEM;
    cfg.src_data_width = DMA_DATA_WIDTH_WORD;
    cfg.dst_data_width = DMA_DATA_WIDTH_WORD;
    cfg.src_bit_order = DMA_MSB_LSB;
    cfg.dst_bit_order = DMA_MSB_LSB;
    cfg.src_word_order = DMA_LITTLE_ENDIAN;
    cfg.dst_word_order = DMA_LITTLE_ENDIAN;

    cfg.src_burst_length = 3;
    cfg.dst_burst_length = 3;
    cfg.src_addr_increase = true;
    cfg.dst_addr_increase = true;
    dma_set_wrap_type(controller, (DMA_CHANNEL_ID)ch, DMA_WRAP_NORMAL,
                      DMA_WRAP_8_BEAT_BURST);

    dma_set_channel_config(controller, (DMA_CHANNEL_ID)ch, &cfg);
    dma_set_channel_priority(controller, (DMA_CHANNEL_ID)ch, DMA_CH_PRIORITY_HIGH);
    dma_set_channel_request(controller, (DMA_CHANNEL_ID)ch, src_req,
                            DMA_PERI_REQ_NONE);

    dma_channel_int_enable(controller, (DMA_CHANNEL_ID)ch, DMA_INT_TX_CURR_DECR);

    iot_dma_mark_work_type(ch, IOT_DMA_TRANS_PERI_TO_MEM);
}

uint8_t iot_audio_rx_fifo_open(uint8_t id)
{
    DMA_PERI_REQ req;
    uint8_t ret;

    if (id >= IOT_AUDIO_RX_FIFO_NUM) {
        return RET_INVAL;
    }

    if (audio_rx_fifos[id].in_use) {
        return RET_BUSY;
    }

    //audio intf power on vote
    aud_intf_pwr_on(AUDIO_MODULE_DMA_RX);

    if(audio_rx_fifo_counter == 0) {
        audio_enable_module(AUDIO_MODULE_DMA_RX);
    }

    assert(audio_rx_fifos[id].dma_channel == IOT_DMA_CHANNEL_NONE);
    ret = iot_dma_claim_channel(&audio_rx_fifos[id].dma_channel);
    if(ret != RET_OK) {
        return ret;
    }

    audio_reset_rx_fifo((AUDIO_RX_FIFO_ID)id);
    audio_enable_rx_fifo((AUDIO_RX_FIFO_ID)id);

    DBGLOG_DRIVER_INFO("fifo:%u open @%u ch:%d\n",
            id, iot_timer_get_time(), audio_rx_fifos[id].dma_channel);

    req = DMA_PERI_AUDIF_RX0 + (DMA_PERI_REQ)id;
    iot_audio_rx_fifo_dma_config(audio_rx_fifos[id].dma_channel, req);

    cpu_critical_enter();
    audio_rx_fifos[id].in_use = true;
    audio_rx_fifo_counter++;
    cpu_critical_exit();

    return RET_OK;
}

uint8_t iot_audio_rx_fifo_close(uint16_t bitmap)
{
    uint32_t ch_bitmap = 0;
    uint32_t fifo_bitmap = 0;
    for (uint32_t id = 0; id < IOT_AUDIO_RX_FIFO_NUM; id++) {
        if (BIT(id) & bitmap) {
            if (!audio_rx_fifos[id].in_use) {
                DBGLOG_DRIVER_WARNING("iot_audio_rx_fifo_close id:%d bitmap:%x\n", id, bitmap);
                assert(0);
            } else {
                ch_bitmap |= BIT(audio_rx_fifos[id].dma_channel);
                fifo_bitmap |= BIT(id);
            }
        }
    }

    DBGLOG_DRIVER_INFO("bitmap:%x fifo:%x ch:%x close @%u\n",
                        bitmap, fifo_bitmap, ch_bitmap, iot_timer_get_time());

    iot_dma_chs_flush(ch_bitmap);

    for (uint32_t id = 0; id < IOT_AUDIO_RX_FIFO_NUM; id++) {
        if (BIT(id) & fifo_bitmap) {
            audio_disable_rx_fifo((AUDIO_RX_FIFO_ID)id);
            iot_dma_channel_suspend(audio_rx_fifos[id].dma_channel);
            iot_dma_channel_reset(audio_rx_fifos[id].dma_channel);
            //clear rx wrap type
            dma_set_wrap_type(DMA_CONTROLLER0, (DMA_CHANNEL_ID)audio_rx_fifos[id].dma_channel,
                    DMA_WRAP_NORMAL, DMA_WRAP_NORMAL);
            dma_release_channel(DMA_CONTROLLER0, (DMA_CHANNEL_ID)audio_rx_fifos[id].dma_channel);

            cpu_critical_enter();
            audio_rx_fifos[id].in_use = false;
            audio_rx_fifos[id].linked = false;
            audio_rx_fifos[id].dma_channel = IOT_DMA_CHANNEL_NONE;
            audio_rx_fifo_counter--;
            cpu_critical_exit();
        }
    }

    if(audio_rx_fifo_counter == 0) {
        audio_disable_module(AUDIO_MODULE_DMA_RX);
        //audio intf power off vote
        aud_intf_pwr_off(AUDIO_MODULE_DMA_RX);
    }

    return RET_OK;
}

uint8_t iot_audio_rx_fifo_to_mem_mount_dma(uint8_t id, char *dst, uint32_t size,
                                           dma_peri_mem_done_callback cb)
{
    uint8_t ret;

    ret = iot_dma_peri_to_mem(audio_rx_fifos[id].dma_channel, dst,
                        (void *)(audio_get_rx_fifo_dma_addr((AUDIO_RX_FIFO_ID)id)), size, cb);
    if (ret != RET_OK) {
        DBGLOG_DRIVER_WARNING("rx fifo dma mount fail: ret %d, id %d, cb %p\n", ret, id, cb);
    }

    return ret;
}

uint8_t iot_audio_rx_fifo_link_i2s(uint8_t id, uint8_t i2s_chan)
{
    if (id >= AUDIO_RX_FIFO_MAX) {
        return RET_INVAL;
    }
    if (i2s_chan >= AUDIO_RX_FIFO_MAX) {
        return RET_INVAL;
    }
    if (audio_rx_fifos[id].linked) {
        return RET_INVAL;
    }

    audio_set_rx_fifo_src((AUDIO_RX_FIFO_ID)id,
                          AUDIO_RX_FIFO_SRC_I2S_RX_0 + (AUDIO_RX_FIFO_SRC_ID)i2s_chan);
    audio_rx_fifos[id].linked = true;

    return RET_OK;
}

uint8_t iot_audio_rx_fifo_link_asrc(uint8_t id, uint8_t asrc_chan)
{
    if (id >= AUDIO_RX_FIFO_MAX) {
        return RET_INVAL;
    }
    if (asrc_chan >= AUDIO_RX_ASRC_MAX) {
        return RET_INVAL;
    }
    if (audio_rx_fifos[id].linked) {
        return RET_INVAL;
    }

    audio_set_rx_fifo_src((AUDIO_RX_FIFO_ID)id,
                          AUDIO_RX_FIFO_SRC_ASRC_0 + (AUDIO_RX_FIFO_SRC_ID)asrc_chan);
    audio_rx_fifos[id].linked = true;

    return RET_OK;
}

uint8_t iot_audio_rx_fifo_link_asrc_multipath(const IOT_AUDIO_RXFIFO_LINK_ASRC_ID *rx_fifo_matrix)
{
    for(uint8_t i = 0; i < 8; i++) {
        if((rx_fifo_matrix[i] >= IOT_AUDIO_LINK_ASRC_0)
                && (rx_fifo_matrix[i] < IOT_AUDIO_LINK_ASRC_MAX)) {
            if (audio_rx_fifos[i].in_use == false) {
                assert(0);
            }
        }
    }
    audio_set_rx_fifo_link_asrc_multipath((AUDIO_RXFIFO_LINK_ASRC_ID*)rx_fifo_matrix);
    return RET_OK;
}

void iot_audio_rx_fifo_dislink_asrc_multipath(void)
{
    uint8_t mask = 0;
    for (uint8_t i = 0; i < IOT_AUDIO_RX_FIFO_NUM; i++) {
        if (audio_rx_fifos[i].in_use == false) {
            mask |= (uint8_t)BIT(i);
        }
    }
    audio_set_rx_fifo_dislink_multipath(mask);
}

uint8_t iot_audio_rx_fifo_link_vad(uint8_t id)
{
    if (id >= AUDIO_RX_FIFO_MAX) {
        return RET_INVAL;
    }
    if (audio_rx_fifos[id].linked) {
        return RET_INVAL;
    }

    audio_set_rx_fifo_src((AUDIO_RX_FIFO_ID)id, AUDIO_RX_FIFO_SRC_VAD);
    audio_rx_fifos[id].linked = true;

    return RET_OK;
}

uint8_t iot_audio_rx_fifo_link_adc(uint8_t id)
{
    if (id >= AUDIO_RX_FIFO_MAX) {
        return RET_INVAL;
    }
    if (audio_rx_fifos[id].linked) {
        return RET_INVAL;
    }

    audio_set_rx_fifo_src((AUDIO_RX_FIFO_ID)id, AUDIO_RX_FIFO_SRC_ADC);
    audio_rx_fifos[id].linked = true;

    return RET_OK;
}

void iot_audio_rx_fifo_link_adc_multipath(void)
{
    uint8_t mask = 0;
    for (uint8_t i = 0; i < IOT_AUDIO_RX_FIFO_NUM; i++) {
        if (audio_rx_fifos[i].in_use) {
            mask |= (uint8_t)BIT(i);
        }
    }
    audio_set_rx_fifo_link_dfe_multipath(mask);
}

void iot_audio_rx_fifo_dislink_adc_multipath(void)
{
    uint8_t mask = 0;
    for (uint8_t i = 0; i < IOT_AUDIO_RX_FIFO_NUM; i++) {
        if (audio_rx_fifos[i].in_use == false) {
            mask |= (uint8_t)BIT(i);
        }
    }
    audio_set_rx_fifo_dislink_multipath(mask);
}

void iot_audio_tx_dma_reset(void)
{
    audio_reset_module(AUDIO_MODULE_DMA_TX);
}

uint8_t iot_audio_tx_fifo_prepare(uint8_t id)
{
    IOT_DMA_PERI_REQ req;
    uint8_t ret;
    IOT_DMA_CHANNEL_ID dma_ch;

    if (id >= IOT_AUDIO_TX_FIFO_NUM) {
        return RET_INVAL;
    }

    if (audio_tx_fifos[id].in_use) {
        return RET_BUSY;
    }

    //audio intf power on vote
    aud_intf_pwr_on(AUDIO_MODULE_DMA_TX);

    if(audio_tx_fifo_counter == 0) {
        audio_enable_module(AUDIO_MODULE_DMA_TX);
    }

    ret = iot_dma_claim_channel(&dma_ch);
    if(ret != RET_OK) {
        return ret;
    }

    req = IOT_DMA_PERI_AUDIF_TX0 + (IOT_DMA_PERI_REQ)id;
    ret = iot_dma_ch_peri_config(dma_ch, IOT_DMA_TRANS_MEM_TO_PERI, IOT_DMA_DATA_WIDTH_WORD,
                                 IOT_DMA_CH_PRIORITY_HIGH,
                                 IOT_DMA_PERI_AUDIF_TX0 + (IOT_DMA_PERI_REQ)id, req);
    if (ret != RET_OK) {
        iot_dma_free_channel(dma_ch);
        return ret;
    }

    cpu_critical_enter();
    audio_tx_fifos[id].dma_channel = dma_ch;
    audio_tx_fifos[id].in_use = true;
    audio_tx_fifo_counter++;
    cpu_critical_exit();

    audio_reset_tx_fifo((AUDIO_TX_FIFO_ID)id);
    audio_enable_tx_fifo((AUDIO_TX_FIFO_ID)id);

    return RET_OK;
}

uint8_t iot_audio_tx_fifo_push_data(uint8_t id, const char *src, uint32_t size,
                                    dma_mem_peri_done_callback cb)
{
    if(id >= IOT_AUDIO_TX_FIFO_NUM) {
        return RET_INVAL;
    }
    iot_dma_mem_to_peri(audio_tx_fifos[id].dma_channel,
                        (void *)(audio_get_tx_fifo_dma_addr((AUDIO_TX_FIFO_ID)id)), src, size, cb);
    return RET_OK;
}

uint8_t iot_audio_tx_fifo_release(uint8_t id)
{
    if (id >= IOT_AUDIO_TX_FIFO_NUM) {
        return RET_INVAL;
    }

    if (!audio_tx_fifos[id].in_use) {
        return RET_OK;
    }

    iot_dma_free_channel(audio_tx_fifos[id].dma_channel);
    audio_disable_tx_fifo((AUDIO_TX_FIFO_ID)id);

    cpu_critical_enter();
    audio_tx_fifos[id].dma_channel = IOT_DMA_CHANNEL_NONE;
    audio_tx_fifos[id].in_use = false;
    audio_tx_fifo_counter--;
    cpu_critical_exit();
    if(audio_tx_fifo_counter == 0) {
        audio_disable_module(AUDIO_MODULE_DMA_TX);
        //audio intf power off vote
        aud_intf_pwr_off(AUDIO_MODULE_DMA_TX);
    }
    return RET_OK;
}

uint8_t iot_audio_tx_fifo_link_i2s(uint8_t id, uint8_t i2s_chan)
{
    if (id >= AUDIO_TX_FIFO_MAX) {
        return RET_INVAL;
    }
    if (i2s_chan >= 8) {
        return RET_INVAL;
    }
    if (audio_tx_fifos[id].linked) {
        return RET_INVAL;
    }

    audio_set_tx_fifo_src((AUDIO_TX_FIFO_ID)id,
                          AUDIO_TX_FIFO_SRC_I2S_TX_0 + (AUDIO_TX_FIFO_SRC_ID)i2s_chan);
    audio_tx_fifos[id].linked = true;

    return RET_OK;
}

uint8_t iot_audio_tx_fifo_link_dfe(uint8_t id, uint8_t dfe_chan)
{
    if (id >= AUDIO_TX_FIFO_MAX) {
        return RET_INVAL;
    }
    if (dfe_chan >= AUDIO_TX_DFE_MAX) {
        return RET_INVAL;
    }
    if (audio_tx_fifos[id].linked) {
        return RET_INVAL;
    }

    audio_set_tx_fifo_src((AUDIO_TX_FIFO_ID)id,
                          AUDIO_TX_FIFO_SRC_DFE_TX_0 + (AUDIO_TX_FIFO_SRC_ID)dfe_chan);
    audio_tx_fifos[id].linked = true;

    return RET_OK;
}

uint8_t iot_audio_set_out_from(IOT_AUDIO_OUT_FROM_ID set)
{
    audio_set_out_sel(set);
    return RET_OK;
}

uint8_t iot_audio_timer_set_target_cnt(IOT_AUDIO_TIMER_TARGET_ID id, uint32_t cnt)
{
    if (id >= IOT_AUDIO_TIMER_TARGET_MAX) {
        return RET_INVAL;
    }
    audio_timer_set_target((AUDIO_TIMER_TARGET_ID)id, cnt);
    return RET_OK;
}

uint32_t iot_audio_timer_get_freerun_tick(void)
{
    return audio_timer_get_freerun_tick();
}

uint32_t iot_audio_timer_get_latched_tick(void)
{
    return audio_timer_get_latched_tick();
}
