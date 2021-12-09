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

#ifndef _DRIVER_NON_OS_AUDIO_H
#define _DRIVER_NON_OS_AUDIO_H
/**
 * @addtogroup HAL
 * @{
 * @addtogroup AUDIO
 * @{
 * This section introduces the AUDIO module's enum, structure, functions and how to use this driver.
 */

#include "types.h"
#include "iot_dma.h"

#ifdef __cplusplus
extern "C" {
#endif

#define IOT_AUDIO_RX_FIFO_NUM 8
#define IOT_AUDIO_TX_FIFO_NUM 8

/** @defgroup hal_audio_enum Enum
  * @{
  */

/** @brief AUDIO out from id.*/
typedef enum {
    IOT_AUDIO_OUT_FROM_DFE_0,
    IOT_AUDIO_OUT_FROM_DFE_1,
    IOT_AUDIO_OUT_FROM_DFE_2,
    IOT_AUDIO_OUT_FROM_DFE_3,
    IOT_AUDIO_OUT_FROM_DFE_4,
    IOT_AUDIO_OUT_FROM_DFE_5,
    IOT_AUDIO_OUT_FROM_I2S_0,
    IOT_AUDIO_OUT_FROM_I2S_1,
    IOT_AUDIO_OUT_FROM_I2S_2,
    IOT_AUDIO_OUT_FROM_I2S_3,
    IOT_AUDIO_OUT_FROM_I2S_4,
    IOT_AUDIO_OUT_FROM_I2S_5,
    IOT_AUDIO_OUT_FROM_I2S_6,
    IOT_AUDIO_OUT_FROM_I2S_7,
    IOT_AUDIO_OUT_FROM_MAX,
} IOT_AUDIO_OUT_FROM_ID;

/** @brief AUDIO timer target id.*/
typedef enum {
    IOT_AUDIO_TIMER_TARGET_0,
    IOT_AUDIO_TIMER_TARGET_1,
    IOT_AUDIO_TIMER_TARGET_2,
    IOT_AUDIO_TIMER_TARGET_3,
    IOT_AUDIO_TIMER_TARGET_MAX,
} IOT_AUDIO_TIMER_TARGET_ID;

typedef enum {
    IOT_AUDIO_LINK_ASRC_BASE = 8,
    IOT_AUDIO_LINK_ASRC_0 = 8,
    IOT_AUDIO_LINK_ASRC_1 = 9,
    IOT_AUDIO_LINK_ASRC_2 = 10,
    IOT_AUDIO_LINK_ASRC_3 = 11,
    IOT_AUDIO_LINK_ASRC_MAX = 12,
}IOT_AUDIO_RXFIFO_LINK_ASRC_ID;
/**
 * @}
 */

/**
 * @brief This function is to initailize audio fifo driver.
 */
void iot_audio_fifo_init(void);

/**
 * @brief This function is to deinitailize audio fifo driver.
 *
 */
void iot_audio_fifo_deinit(void);

/**
 * @brief return global clk register enable status
 *
 * @return uitn32_t global clk register enable status
 */
uint32_t iot_audio_get_global_clk_status(void);

/**
 * @brief This function is to reset audio rx dma.
 *
 */
void iot_audio_rx_dma_reset(void);

/**
 * @brief This function is to configure if audio rx fifo works in half word mode.
 *
 * @param enable judge audio rx fifo if in half word mode.
 */
void iot_audio_rx_fifo_half_word(bool_t enable);

/**
 * @brief This function is to configure dma channel for audio tx fifo and enable.
 *
 * @param id is audio rx fifo id.
 * @return uint8_t RET_OK for success else error.
 */
uint8_t iot_audio_rx_fifo_open(uint8_t id);

/**
 * @brief This function is to disable audio rx fifo and free dma channel.
 *
 * @param bitmap is audio rx fifo id Bit.
 * @return uint8_t RTE_OK for success else error.
 */
uint8_t iot_audio_rx_fifo_close(uint16_t bitmap);

/**
 * @brief This function is to mount dma from audio rx fifo to mem buffer.
 *
 * @param id is audio rx fifo id.
 * @param dst is pointer of data ready for pull.
 * @param size is size of data need to pull.
 * @param cb is callback function called when data transfer done.
 * @return uint8_t RET_OK for success else error.
 */
uint8_t iot_audio_rx_fifo_to_mem_mount_dma(uint8_t id, char *dst, uint32_t size,
                                           dma_peri_mem_done_callback cb);

/**
 * @brief This function is to configure rx fifo data source from i2s rx channel.
 *
 * @param id is audio rx fifo id.
 * @param i2s_chan is audio i2s rx channel.
 * @return uint8_t RTE_OK for success else error.
 */
uint8_t iot_audio_rx_fifo_link_i2s(uint8_t id, uint8_t i2s_chan);

/**
 * @brief This function is to configure rx fifo data source from asrc channel.
 *
 * @param id is audio rx fifo id.
 * @param asrc_chan is audio asrc channel.
 * @return uint8_t RTE_OK for success else error.
 */
uint8_t iot_audio_rx_fifo_link_asrc(uint8_t id, uint8_t asrc_chan);

/**
 * @brief multiway rx fifo link asrc
 *
 * @param rx_fifo_matrix binary representation, each bit represents a rx fifo id
 * @return uint8_t RTE_OK for success else error.
 */
uint8_t iot_audio_rx_fifo_link_asrc_multipath(const IOT_AUDIO_RXFIFO_LINK_ASRC_ID *rx_fifo_matrix);

/**
 * @brief  multiway rx fifo dislink asrc
 */
void iot_audio_rx_fifo_dislink_asrc_multipath(void);

/**
 * @brief This function is to configure rx fifo data source from vad.
 *
 * @param id is audio rx fifo id.
 * @return uint8_t RTE_OK for success else error.
 */
uint8_t iot_audio_rx_fifo_link_vad(uint8_t id);

/**
 * @brief This function is to configure rx fifo data source from adc.
 *
 * @param id is audio rx fifo id.
 * @return uint8_t RTE_OK for success else error.
 */
uint8_t iot_audio_rx_fifo_link_adc(uint8_t id);

/**
 * @brief  multiway rx fifo link adc
 *
 */
void iot_audio_rx_fifo_link_adc_multipath(void);

/**
 * @brief  multiway rx fifo dislink adc
 *
 */
void iot_audio_rx_fifo_dislink_adc_multipath(void);

/**
 * @brief This function is to reset audio tx dma.
 *
 */
void iot_audio_tx_dma_reset(void);

/**
 * @brief This function is to configure dma channel for audio tx fifo and enable.
 *
 * @param id is audio rx fifo id.
 * @return uint8_t RTE_OK for success else error.
 */
uint8_t iot_audio_tx_fifo_prepare(uint8_t id);

/**
 * @brief This function is to push data to audio tx fifo.
 *
 * @param id is audio rx fifo id.
 * @param src is pointer of data need to push.
 * @param size is size of data need to push.
 * @param cb is callback function called when data transfer done.
 * @return uint8_t RTE_OK for success else error.
 */
uint8_t iot_audio_tx_fifo_push_data(uint8_t id, const char *src, uint32_t size,
                                    dma_mem_peri_done_callback cb);

/**
 * @brief This function is to disable audio tx fifo and free dma channel.
 *
 * @param id is audio rx fifo id.
 * @return uint8_t RTE_OK for success else error.
 */
uint8_t iot_audio_tx_fifo_release(uint8_t id);

/**
 * @brief This function is to configure tx fifo data source from i2s tx channel.
 *
 * @param id is audio rx fifo id.
 * @param i2s_chan is audio i2s tx channel
 * @return uint8_t RTE_OK for success else error.
 */
uint8_t iot_audio_tx_fifo_link_i2s(uint8_t id, uint8_t i2s_chan);

/**
 * @brief This function is to configure tx fifo data source from tx dfe channel.
 *
 * @param id is audio rx fifo id.
 * @param dfe_chan is audio tx dfe channel.
 * @return uint8_t RTE_OK for success else error.
 */
uint8_t iot_audio_tx_fifo_link_dfe(uint8_t id, uint8_t dfe_chan);

/**
 * @brief This function is to configure audio out from dfe or i2s.
 *
 * @param set is audio out from id,range 0~5: from Rx dfe 0~5; 6~13: from I2S 0~7; otherwise zero output.
 * @return uint8_t RTE_OK for success else error.
 */
uint8_t iot_audio_set_out_from(IOT_AUDIO_OUT_FROM_ID set);

/**
 * @brief This function is to enable audio timer.
 *
 */
void iot_audio_timer_enable(void);

/**
 * @brief This function is to disable audio timer.
 *
 */
void iot_audio_timer_disable(void);

/**
 * @brief This function is to enable audio timer target.
 *
 * @param id is audio timer target id.
 * @return uint8_t RTE_OK for success else error.
 */
uint8_t iot_audio_timer_enable_target(IOT_AUDIO_TIMER_TARGET_ID id);

/**
 * @brief This function is to disable audio timer target.
 *
 * @param id is audio timer target id.
 * @return uint8_t RTE_OK for success else error.
 */
uint8_t iot_audio_timer_disable_target(IOT_AUDIO_TIMER_TARGET_ID id);

/**
 * @brief This function is to set audio timer target count.
 *
 * @param id is audio timer target id.
 * @param cnt tick count; trigger happen when the timer tick equal this count
 * @return uint8_t RTE_OK for success else error.
 */
uint8_t iot_audio_timer_set_target_cnt(IOT_AUDIO_TIMER_TARGET_ID id, uint32_t cnt);

/**
 * @brief This function is to get audio timer freerun tick. The audio timer is always running if enabled.
 *
 * @return uint32_t audio timer freerun tick.
 */
uint32_t iot_audio_timer_get_freerun_tick(void);

/**
 * @brief This function is to get audio timer tick when BT latch happened.
 *
 * @return uint32_t audio timer freerun tick.
 */
uint32_t iot_audio_timer_get_latched_tick(void);

#ifdef __cplusplus
}
#endif

/**
* @}
* @}
*/
#endif /* _DRIVER_NON_OS_AUDIO_H */
