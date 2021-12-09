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
#ifndef _DRIVER_HW_DMA_H
#define _DRIVER_HW_DMA_H

#include "gpio_mtx_signal.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    DMA_CONTROLLER0,
    DMA_CONTROLLER1,
    DMA_CONTROLLER2,
    DMA_CONTROLLER_MAX,
} DMA_CONTROLLER;

typedef enum {
    DMA_CHANNEL_0,
    DMA_CHANNEL_1,
    DMA_CHANNEL_2,
    DMA_CHANNEL_3,
    DMA_CHANNEL_4,
    DMA_CHANNEL_5,
    DMA_CHANNEL_6,
    DMA_CHANNEL_7,
    DMA_CHANNEL_8,
    DMA_CHANNEL_9,
    DMA_CHANNEL_10,
    DMA_CHANNEL_11,
    DMA_CHANNEL_12,
    DMA_CHANNEL_13,
    DMA_CHANNEL_14,
    DMA_CHANNEL_15,
    DMA_CHANNEL_16,
    DMA_CHANNEL_17,
    DMA_CHANNEL_18,
    DMA_CHANNEL_19,
    DMA_CHANNEL_20,
    DMA_CHANNEL_21,
    DMA_CHANNEL_22,
    DMA_CHANNEL_MAX,
    DMA_CHANNEL_NONE = DMA_CHANNEL_MAX,
} DMA_CHANNEL_ID;

typedef enum {
    DMA_INT_0,
    DMA_INT_1,
    DMA_INT_2,
    DMA_INT_3,
    DMA_INT_MAX,
} DMA_INT_ID;

typedef enum {
    DMA_BURST_LEN_0,
    DMA_BURST_LEN_1,
    DMA_BURST_LEN_2,
    DMA_BURST_LEN_3,
    DMA_DEFAULT_MAX_BURST_LEN = DMA_BURST_LEN_3,
    DMA_BURST_LEN_4,
    DMA_BURST_LEN_5,
    DMA_BURST_LEN_6,
    DMA_BURST_LEN_7,
    DMA_BURST_LEN_8,
    DMA_BURST_LEN_9,
    DMA_BURST_LEN_10,
    DMA_BURST_LEN_11,
    DMA_BURST_LEN_12,
    DMA_BURST_LEN_13,
    DMA_BURST_LEN_14,
    DMA_BURST_LEN_15,
    DMA_CHANNEL0_MAX_BURST_LEN = DMA_BURST_LEN_15,
} DMA_BURST_LEN;

typedef enum {
    // for dma0
    DMA_PERI_SPI_M0_RX,
    DMA_PERI_SPI_M0_TX,
    DMA_PERI_ASRC0,
    DMA_PERI_ASRC1,
    DMA_PERI_UART0_RX,
    DMA_PERI_UART0_TX,
    DMA_PERI_UART1_RX,
    DMA_PERI_UART1_TX,
    DMA_PERI_AUDIF_RX0,
    DMA_PERI_AUDIF_RX1,
    DMA_PERI_AUDIF_RX2,
    DMA_PERI_AUDIF_RX3,
    DMA_PERI_AUDIF_RX4,
    DMA_PERI_AUDIF_RX5,
    DMA_PERI_AUDIF_RX6,
    DMA_PERI_AUDIF_RX7,
    DMA_PERI_AUDIF_TX0,
    DMA_PERI_AUDIF_TX1,
    DMA_PERI_AUDIF_TX2,
    DMA_PERI_AUDIF_TX3,
    DMA_PERI_AUDIF_TX4,
    DMA_PERI_AUDIF_TX5,
    DMA_PERI_AUDIF_TX6,
    DMA_PERI_AUDIF_TX7,
    DMA_PERI_ADA,
    DMA_PERI_MADC,
    DMA_PERI_TK,
    DMA_PERI_SPI_M1_TX,
    DMA_PERI_SPI_M1_RX,
    DMA_WIC_ACK_DTOP2AUD,
    DMA_WIC_ACK_DTOP2BT,
    DMA_PERI_MAX,
    DMA_PERI_REQ_NONE = DMA_PERI_MAX,

    // for dma1
    DMA_WIC_ACK_BT2DTOP = 0,
    DMA_WIC_ACK_BT2AUD,

    // for dma2
    DMA_WIC_ACK_AUD2DTOP = 0,
    DMA_WIC_ACK_AUD2BT,

} DMA_PERI_REQ;

typedef enum {
    DMA_DATA_WIDTH_WORD,
    DMA_DATA_WIDTH_BYTE,
} DMA_DATA_WIDTH;

typedef enum {
    DMA_TRANS_MEM_TO_MEM,
    DMA_TRANS_MEM_TO_PERI,
    DMA_TRANS_PERI_TO_MEM,
    DMA_TRANS_PERI_TO_PERI,
} DMA_TRANS_TYPE;

typedef enum {
    DMA_LITTLE_ENDIAN,
    DMA_BIG_ENDIAN,
} DMA_WORD_ORDER;

typedef enum {
    DMA_MSB_LSB,
    DMA_LSB_MSB,
} DMA_BIT_ORDER;

typedef enum {
    DMA_CH_PRIORITY_LOW,
    DMA_CH_PRIORITY_HIGH,
} DMA_CH_PRIORITY;

typedef enum {
    DMA_WRAP_NORMAL,
    DMA_WRAP_4_BEAT_BURST,
    DMA_WRAP_8_BEAT_BURST,
    DMA_WRAP_16_BEAT_BURST,
    DMA_WRAP_MODE_MAX,
} DMA_WRAP_MODE;

typedef enum {
    DMA_INT_RX_DECR_IS_NOT_HW,
    DMA_INT_RX_ALL_DECR,
    DMA_INT_RX_CURR_DECR,
    DMA_INT_RX_ACK,
    DMA_INT_RX_BUFF_ADDR_NULL,
    DMA_INT_TX_BUFF_ADDR_NULL,
    DMA_INT_TX_DECR_IS_NOT_HW,
    DMA_INT_TX_ALL_DECR,
    DMA_INT_TX_CURR_DECR,
    DMA_INT_TX_ACK,
    DMA_INT_TX_BUFF_FULL,
    DMA_INT_TX_RX_EXCEED_TX,
    DMA_INT_TYPE_MAX,
} DMA_INT_TYPE;

typedef struct dma_descriptor {
    uint32_t buf_size : 24;
    uint32_t int_en : 1;
    uint32_t dummy_end : 1; // dummy bit, because when equals true, dma need cpu to retrigger engine
    uint32_t dummy_start : 1;
    uint32_t end : 1;       // occupy reserved bit for sw using
    uint32_t start : 1;
    uint32_t reserved : 2;
    uint32_t owner : 1;
    uint32_t buf_addr;
    struct dma_descriptor *next;
} dma_descriptor_t;

typedef struct dma_ch_config {
    DMA_TRANS_TYPE trans_type;
    DMA_DATA_WIDTH src_data_width;
    DMA_DATA_WIDTH dst_data_width;
    DMA_BIT_ORDER src_bit_order;
    DMA_BIT_ORDER dst_bit_order;
    DMA_WORD_ORDER src_word_order;
    DMA_WORD_ORDER dst_word_order;
    uint8_t src_burst_length;
    uint8_t dst_burst_length;
    bool_t src_addr_increase;
    bool_t dst_addr_increase;
} dma_ch_config_t;

bool_t dma_is_channel_idle(DMA_CONTROLLER dma, DMA_CHANNEL_ID ch);
void dma_release_channel(DMA_CONTROLLER dma, DMA_CHANNEL_ID ch);
DMA_CHANNEL_ID dma_claim_channel(DMA_CONTROLLER dma);

void dma_init(DMA_CONTROLLER dma);
void dma_deinit(DMA_CONTROLLER dma);
void dma_channel_start(DMA_CONTROLLER dma, DMA_CHANNEL_ID ch);
void dma_channel_stop(DMA_CONTROLLER dma, DMA_CHANNEL_ID ch);
void dma_channel_reset(DMA_CONTROLLER dma, DMA_CHANNEL_ID ch);
void dma_channel_suspend(DMA_CONTROLLER dma, DMA_CHANNEL_ID ch);
void dma_channel_int_select(DMA_CONTROLLER dma, DMA_CHANNEL_ID ch, DMA_INT_ID n);
uint8_t dma_get_ch_max_burst_length(DMA_CONTROLLER dma, DMA_CHANNEL_ID ch);
void dma_set_channel_rx_descriptor(DMA_CONTROLLER dma, DMA_CHANNEL_ID ch,
                                   dma_descriptor_t *desc);
void dma_set_channel_tx_descriptor(DMA_CONTROLLER dma, DMA_CHANNEL_ID ch,
                                   dma_descriptor_t *desc);
void dma_set_channel_config(DMA_CONTROLLER dma, DMA_CHANNEL_ID ch,
                            const dma_ch_config_t *ch_cfg);
void dma_set_channel_priority(DMA_CONTROLLER dma, DMA_CHANNEL_ID ch,
                              DMA_CH_PRIORITY priority);
void dma_set_channel_request(DMA_CONTROLLER dma, DMA_CHANNEL_ID ch,
                             DMA_PERI_REQ src, DMA_PERI_REQ dst);
void dma_set_wic_trigger(DMA_CONTROLLER dma, DMA_CHANNEL_ID ch);
void dma_set_wrap_type(DMA_CONTROLLER dma, DMA_CHANNEL_ID ch,
                        DMA_WRAP_MODE tx_wrap, DMA_WRAP_MODE rx_wrap);
uint32_t dma_get_ch_int_status(DMA_CONTROLLER dma, DMA_CHANNEL_ID ch);
void dma_channel_int_enable(DMA_CONTROLLER dma, DMA_CHANNEL_ID ch,
                            DMA_INT_TYPE id);
void dma_channel_int_disable(DMA_CONTROLLER dma, DMA_CHANNEL_ID ch,
                             DMA_INT_TYPE id);
void dma_channel_int_clear(DMA_CONTROLLER dma, DMA_CHANNEL_ID ch, DMA_INT_TYPE id);
void dma_channel_int_clear_all(DMA_CONTROLLER dma, DMA_CHANNEL_ID ch);
void dma_channel_int_clear_bits(DMA_CONTROLLER dma, DMA_CHANNEL_ID ch,
                           uint32_t clear_bits);
void dma_owner_eb_ignore(DMA_CONTROLLER dma, DMA_CHANNEL_ID ch);

#ifdef __cplusplus
}
#endif

#endif /* _DRIVER_HW_DMA_H */
