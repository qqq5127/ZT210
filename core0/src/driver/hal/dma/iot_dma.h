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

#ifndef _DRIVER_HAL_DMA_H
#define _DRIVER_HAL_DMA_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup HAL
 * @{
 * @addtogroup DMA
 * @{
 * This section introduces the DMA module's enum, structure, functions and how to use this driver.
 */

typedef enum {
    IOT_DMA_CONTROLLER0,
    IOT_DMA_CONTROLLER1,
    IOT_DMA_CONTROLLER2,
    IOT_DMA_CONTROLLER_MAX,
} IOT_DMA_CONTROLLER;

typedef enum {
    IOT_DMA_CH_PRIORITY_LOW,
    IOT_DMA_CH_PRIORITY_HIGH,
} IOT_DMA_CH_PRIORITY;

typedef struct iot_dma_buf_entry {
    uint8_t *addr;
    uint32_t len;
} iot_dma_buf_entry_t;
typedef enum {
    IOT_DMA_CHANNEL_0,
    IOT_DMA_CHANNEL_1,
    IOT_DMA_CHANNEL_2,
    IOT_DMA_CHANNEL_3,
    IOT_DMA_CHANNEL_4,
    IOT_DMA_CHANNEL_5,
    IOT_DMA_CHANNEL_6,
    IOT_DMA_CHANNEL_7,
    IOT_DMA_CHANNEL_8,
    IOT_DMA_CHANNEL_9,
    IOT_DMA_CHANNEL_10,
    IOT_DMA_CHANNEL_11,
    IOT_DMA_CHANNEL_12,
    IOT_DMA_CHANNEL_13,
    IOT_DMA_CHANNEL_14,
    IOT_DMA_CHANNEL_15,
    IOT_DMA_CHANNEL_16,
    IOT_DMA_CHANNEL_17,
    IOT_DMA_CHANNEL_18,
    IOT_DMA_CHANNEL_19,
    IOT_DMA_CHANNEL_20,
    IOT_DMA_CHANNEL_21,
    IOT_DMA_CHANNEL_22,
    IOT_DMA_CHANNEL_MAX,
    IOT_DMA_CHANNEL_NONE = IOT_DMA_CHANNEL_MAX,
} IOT_DMA_CHANNEL_ID;

typedef enum {
    IOT_DMA_TRANS_MEM_TO_MEM,
    IOT_DMA_TRANS_MEM_TO_PERI,
    IOT_DMA_TRANS_PERI_TO_MEM,
    IOT_DMA_TRANS_PERI_TO_PERI,
} IOT_DMA_TRANS_TYPE;

typedef enum {
    IOT_DMA_DATA_WIDTH_WORD,
    IOT_DMA_DATA_WIDTH_BYTE,
} IOT_DMA_DATA_WIDTH;

typedef enum {
    // for dma0
    IOT_DMA_PERI_SPI_M0_RX,
    IOT_DMA_PERI_SPI_M0_TX,
    IOT_DMA_PERI_ASRC0,
    IOT_DMA_PERI_ASRC1,
    IOT_DMA_PERI_UART0_RX,
    IOT_DMA_PERI_UART0_TX,
    IOT_DMA_PERI_UART1_RX,
    IOT_DMA_PERI_UART1_TX,
    IOT_DMA_PERI_AUDIF_RX0,
    IOT_DMA_PERI_AUDIF_RX1,
    IOT_DMA_PERI_AUDIF_RX2,
    IOT_DMA_PERI_AUDIF_RX3,
    IOT_DMA_PERI_AUDIF_RX4,
    IOT_DMA_PERI_AUDIF_RX5,
    IOT_DMA_PERI_AUDIF_RX6,
    IOT_DMA_PERI_AUDIF_RX7,
    IOT_DMA_PERI_AUDIF_TX0,
    IOT_DMA_PERI_AUDIF_TX1,
    IOT_DMA_PERI_AUDIF_TX2,
    IOT_DMA_PERI_AUDIF_TX3,
    IOT_DMA_PERI_AUDIF_TX4,
    IOT_DMA_PERI_AUDIF_TX5,
    IOT_DMA_PERI_AUDIF_TX6,
    IOT_DMA_PERI_AUDIF_TX7,
    IOT_DMA_PERI_ADA,
    IOT_DMA_PERI_MADC,
    IOT_DMA_PERI_TK,
    IOT_DMA_PERI_SPI_M1_TX,
    IOT_DMA_PERI_SPI_M1_RX,
    IOT_DMA_WIC_ACK_DTOP2AUD,
    IOT_DMA_WIC_ACK_DTOP2BT,
    IOT_DMA_PERI_MAX,
    IOT_DMA_PERI_REQ_NONE = IOT_DMA_PERI_MAX,

    // for dma1
    IOT_DMA_WIC_ACK_BT2DTOP = 0,
    IOT_DMA_WIC_ACK_BT2AUD,

    // for dma2
    IOT_DMA_WIC_ACK_AUD2DTOP = 0,
    IOT_DMA_WIC_ACK_AUD2BT,

} IOT_DMA_PERI_REQ;

typedef void (*dma_mem_mem_done_callback)(void *dst, void *src, uint32_t length, void *param);
typedef void (*dma_peri_mem_done_callback)(void *buf, uint32_t length);
typedef void (*dma_mem_peri_done_callback)(void *buf, uint32_t length);
typedef void (*dma_peri_peri_done_callback)(void);
typedef void (*dma_mem_peri_tx_ack_callback)(IOT_DMA_CHANNEL_ID ch);

uint8_t iot_dma_claim_channel(IOT_DMA_CHANNEL_ID *ch);
void iot_dma_free_channel(IOT_DMA_CHANNEL_ID ch);
void iot_dma_channel_suspend(IOT_DMA_CHANNEL_ID ch);
void iot_dma_channel_reset(IOT_DMA_CHANNEL_ID ch);
void iot_dma_mark_work_type(IOT_DMA_CHANNEL_ID ch, IOT_DMA_TRANS_TYPE type);
void iot_dma_init(void);
void iot_dma_deinit(void);

uint32_t iot_dma_get_act_ch_vect(void);

uint8_t iot_dma_ch_peri_config(IOT_DMA_CHANNEL_ID ch, IOT_DMA_TRANS_TYPE type,
                               IOT_DMA_DATA_WIDTH width, IOT_DMA_CH_PRIORITY pri,
                               IOT_DMA_PERI_REQ src_req, IOT_DMA_PERI_REQ dst_req);
int8_t iot_dma_memcpy(void *dst, void *src, uint32_t length, dma_mem_mem_done_callback cb,
                      void *param);
uint32_t iot_dma_restore(uint32_t data);

/**
 * @brief Set a group memory to memory dma transmission
 *
 * @param[in] src_buf_list The source buffer list, with (addr=NULL) ending
 * @param[in] dst_buf_list The destination buffer list, with (addr=NULL) ending
 * @param[in] cb Please make sure the cb is not NULL, or else the sleep vote would have issue
 * @param[in] param the pointer of callback param
 *
 * @return 0 - OK, others - error
 */
uint32_t iot_dma_memcpy_group(const iot_dma_buf_entry_t src_buf_list[],
                              const iot_dma_buf_entry_t dst_buf_list[],
                              dma_mem_mem_done_callback cb, void *param);

uint8_t iot_dma_mem_to_peri(IOT_DMA_CHANNEL_ID ch, void *dst, const void *src, uint32_t length,
                            dma_mem_peri_done_callback cb);

uint8_t iot_dma_peri_to_mem(IOT_DMA_CHANNEL_ID ch, void *dst, void *src, uint32_t length,
                            dma_peri_mem_done_callback cb);

uint8_t iot_dma_mem_to_peri_with_tx_ack(IOT_DMA_CHANNEL_ID ch, void *dst, const void *src,
                                        uint32_t length, dma_mem_peri_tx_ack_callback cb);

uint8_t iot_dma_ch_flush(IOT_DMA_CHANNEL_ID ch);

uint8_t iot_dma_chs_flush(uint32_t ch_bmp);

#ifdef __cplusplus
}
#endif

/**
 * @}
 * addtogroup DMA
 * @}
 * addtogroup HAL
 */

#endif /* _DRIVER_HAL_DMA_H */
