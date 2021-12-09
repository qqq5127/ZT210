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

#ifndef _DRIVER_NON_OS_ASRC_H
#define _DRIVER_NON_OS_ASRC_H
/**
 * @addtogroup HAL
 * @{
 * @addtogroup ASRC
 * @{
 * This section introduces the ASRC module's enum, structure, functions and how to use this driver.
 */
#include "types.h"
#include "iot_dma.h"

/** @brief ASRC channel id.*/
typedef enum {
    IOT_ASRC_CHANNEL_0,
    IOT_ASRC_CHANNEL_1,
    IOT_ASRC_CHANNEL_TX_MAX,
    IOT_ASRC_CHANNEL_2 = IOT_ASRC_CHANNEL_TX_MAX,
    IOT_ASRC_CHANNEL_3,
    IOT_ASRC_CHANNEL_MAX,
    IOT_ASRC_CHANNEL_NONE = IOT_ASRC_CHANNEL_MAX
} IOT_ASRC_CHANNEL_ID;

/** @brief ASRC start src.*/
typedef enum {
    IOT_ASRC_SELF_START,
    IOT_ASRC_BT_TRIGGER,
    IOT_ASRC_TIMER_TRIGGER,
    IOT_ASRC_START_SRC_MAX
} IOT_ASRC_START_SRC;

/** @brief ASRC latch src.*/
typedef enum {
    IOT_ASRC_BT_LATCH,
    IOT_ASRC_TIMER_LATCH,
    IOT_ASRC_TIMER_LATCH_NONE,
    IOT_ASRC_LATCH_SRC_MAX
} IOT_ASRC_LATCH_SRC;

typedef enum {
    IOT_ASRC_INT_UNDER_FLOW,
    IOT_ASRC_INT_REACH_SAMPLE_CNT,
    IOT_ASRC_INT_TYPE_MAX,
} IOT_ASRC_INT_TYPE;

/** @brief ASRC half word mode.*/
typedef enum {
    IOT_ASRC_HALF_WORD_DISABLED,
    IOT_ASRC_HALF_WORD_LEAST_16BIT,
    IOT_ASRC_HALF_WORD_16BIT_PAIR,
    IOT_ASRC_HALF_WORD_MODE_MAX
} IOT_ASRC_HALF_WORD_MODE;

/** @brief ASRC coeff param id.*/
typedef enum {
    IOT_ASRC_COEFF_BIQUAD0_COEFF0,
    IOT_ASRC_COEFF_BIQUAD0_COEFF1,
    IOT_ASRC_COEFF_BIQUAD0_COEFF2,
    IOT_ASRC_COEFF_BIQUAD0_COEFF3,
    IOT_ASRC_COEFF_BIQUAD0_COEFF4,
    IOT_ASRC_COEFF_BIQUAD1_COEFF0,
    IOT_ASRC_COEFF_BIQUAD1_COEFF1,
    IOT_ASRC_COEFF_BIQUAD1_COEFF2,
    IOT_ASRC_COEFF_BIQUAD1_COEFF3,
    IOT_ASRC_COEFF_BIQUAD1_COEFF4,
    IOT_ASRC_COEFF_PARAM_ID_MAX
} IOT_ASRC_COEFF_PARAM_ID;

/** @brief ASRC mode.*/
typedef enum {
    IOT_ASRC_TX_MODE,
    IOT_ASRC_RX_MODE,
    IOT_ASRC_MODE_MAX,
}IOT_ASRC_MODE;

typedef struct iot_asrc_config {
    uint32_t freq_in;
    uint32_t freq_out;
    int16_t ppm;
    bool_t sync;
    IOT_ASRC_MODE mode;
    IOT_ASRC_LATCH_SRC latch;
    IOT_ASRC_START_SRC trigger;
    IOT_ASRC_HALF_WORD_MODE half_word;
}iot_asrc_config_t;

typedef void (*iot_asrc_int_hook)(uint32_t id);

/**
 * @brief This function is to init asrc.
 *
 */
void iot_asrc_init(void);

/**
 * @brief This function is to deinit asrc.
 */
void iot_asrc_deinit(void);

/**
 * @brief  This function is to enable asrc.
 *
 * @param id is the asrc channel id.
 * @param sync If sync is true, just temporarily save the enable bits
 * and set them enable together when all of moduler configuration is complete
 */
void iot_asrc_enable(IOT_ASRC_CHANNEL_ID id, bool_t sync);

/**
 * @brief This function is to disable asrc.
 *
 * @param id is the asrc channel id.
 * @param sync If sync is true, just temporarily save the disable bits
 * and set them disable together when all of moduler configuration is complete
 */
void iot_asrc_disable(IOT_ASRC_CHANNEL_ID id, bool_t sync);

/**
 * @brief This function is to reset module status,it will not change config register.
 *
 * @param id is the asrc channel id.
 */
void iot_asrc_reset(IOT_ASRC_CHANNEL_ID id);

/**
 * @brief This function is to start asrc.
 *
 * @param id is the asrc channel id.
 */
void iot_asrc_start(IOT_ASRC_CHANNEL_ID id);

/**
 * @brief This function is to stop asrc.
 *
 * @param bitmap_id is the bitmap of asrc channel id.
 */
void iot_asrc_stop(uint8_t bitmap_id);

/**
 * @brief This function is to set asrc coefficient configuration.
 *
 * @param id is the asrc channel id.
 * @param coeff is the coefficient.
 */
void iot_asrc_coeff_config(IOT_ASRC_CHANNEL_ID id, const uint32_t *coeff);

/**
 * @brief This function is set asrc tx coefficient configuration.
 *
 * @param id is the asrc channel id.
 * @param num is the asrc coefficient parameter id.
 * @param value is the value to set.
 */
void iot_asrc_tx_coeff_config(IOT_ASRC_CHANNEL_ID id,
                              IOT_ASRC_COEFF_PARAM_ID num, uint32_t value);

/**
 * @brief This function is to open asrc.
 *
 * @param id is the asrc channel id.
 * @param cfg is the configuration.
 * @return uint8_t RET_OK for success else error.
 */
uint8_t iot_asrc_open(IOT_ASRC_CHANNEL_ID id, const iot_asrc_config_t *cfg);

/**
 * @brief This function is to close asrc.
 *
 * @param id is the asrc channel id.
 * @return uint8_t RET_OK for success else error.
 */
uint8_t iot_asrc_close(IOT_ASRC_CHANNEL_ID id);

/**
 * @brief This function is to set asrc tx mode.
 *
 * @param id is the asrc channel id.
 * @return uint8_t RET_OK for success else error.
 */
uint8_t iot_asrc_set_tx_mode(IOT_ASRC_CHANNEL_ID id);

/**
 * @brief This function is to set asrc rx mode.
 *
 * @param id is the asrc channel id.
 * @return uint8_t RET_OK for success else error.
 */
uint8_t iot_asrc_set_rx_mode(IOT_ASRC_CHANNEL_ID id);

/**
 * @brief This function is to link rx dfe.
 *
 * @param id is the asrc channel id.
 * @param chan is the channel.
 * @return uint8_t RET_OK for success else error.
 */
uint8_t iot_asrc_link_rx_dfe(IOT_ASRC_CHANNEL_ID id, uint8_t chan);

/**
 * @brief This function is to link rx i2c.
 *
 * @param id is the asrc channel id.
 * @param chan is the channel.
 * @return uint8_t RET_OK for success else error.
 */
uint8_t iot_asrc_link_rx_i2s(IOT_ASRC_CHANNEL_ID id, uint8_t chan);

/**
 * @brief This function is to clear transmit mode.
 *
 * @param id is the asrc channel id.
 * @return uint8_t RET_OK for success else error.
 */
uint8_t iot_asrc_clear_trans_mode(IOT_ASRC_CHANNEL_ID id);

/**
 * @brief This function is to config tx frequence.
 *
 * @param id is the asrc channel id.
 * @param freq_in is the frequence input.
 * @param freq_out is the frequence output.
 * @param ppm is the num of asrc_ppm_unit.
 * @return uint8_t RET_OK for success else error.
 */
uint8_t iot_asrc_tx_config_frequence(IOT_ASRC_CHANNEL_ID id, uint32_t freq_in,
                                     uint32_t freq_out, int32_t ppm);

/**
 * @brief This function is to config rx frequence.
 *
 * @param id is the asrc channel id.
 * @param freq_in is the frequence input.
 * @param freq_out is the frequence output.
 * @param ppm is the num of asrc_ppm_unit.
 * @return uint8_t RET_OK for success else error.
 */
uint8_t iot_asrc_rx_config_frequence(IOT_ASRC_CHANNEL_ID id, uint32_t freq_in,
                                     uint32_t freq_out, int32_t ppm);

/**
 * @brief This function is to set tx ppm.
 *
 * @param id is the asrc channel id.
 * @param freq_in is the frequence input.
 * @param ppm is the num of asrc_ppm_unit.
 * @return uint8_t RET_OK for success else error.
 */
uint8_t iot_asrc_tx_ppm_set(IOT_ASRC_CHANNEL_ID id, uint32_t freq_in,
                            int32_t ppm);

/**
 * @brief This function is to set tx asrc filter num and ppm.
 *
 * @param id is the asrc channel id.
 * @param freq_in is the frequence input.
 * @param ppm is the num of asrc_ppm_unit.
 * @param filter_num is the filter num of asrc.
 * @return uint8_t RET_OK for success else error.
 */
uint8_t iot_asrc_tx_filter_ppm_set(IOT_ASRC_CHANNEL_ID id, uint32_t freq_in, int32_t ppm,
                                   uint8_t filter_num);

/**
 * @brief This function is to get asrc over sample factor.
 *
 * @param freq_in is the frequence input.
 * @param freq_out is the frequence output.
 * @return uint8_t the over sample factor.
 */
uint8_t iot_asrc_over_sample_factor_get(uint32_t freq_in, uint32_t freq_out);

/**
 * @brief This function is to get asrc filter num.
 *
 * @param freq_in is the frequence input.
 * @param freq_out is the frequence output.
 * @return uint8_t the filter num of asrc.
 */
uint8_t iot_asrc_over_filter_num_get(uint32_t freq_in, uint32_t freq_out);

/**
 * @brief This function is to set rx ppm.
 *
 * @param id is the asrc channel id.
 * @param freq_in is the frequence input.
 * @param freq_out is the frequence output.
 * @param ppm is the num of asrc_ppm_unit.
 * @return uint8_t RET_OK for success else error.
 */
uint8_t iot_asrc_rx_ppm_set(IOT_ASRC_CHANNEL_ID id, uint32_t freq_in,
                            uint32_t freq_out, int32_t ppm);

/**
 * @brief get the counter corresponding to the upsample
 *
 * @param id is the asrc channel id.
 * @return uint8_t return upsample
 */
uint8_t iot_asrc_get_upsample(IOT_ASRC_CHANNEL_ID id);

/**
 * @brief This function is to push memory data to asrc by mount dma.
 *
 * @param id is the asrc channel id.
 * @param src is the source address.
 * @param size is the size of tx data.
 * @param cb is the dma memory period done callback function.
 * @return uint8_t RET_OK for success else error.
 */
uint8_t iot_tx_asrc_from_mem_mount_dma(IOT_ASRC_CHANNEL_ID id, const char *src, uint32_t size,
                              dma_mem_peri_done_callback cb);

/**
 * @brief This function is to set asrc half_word mode.
 *
 * @param id is the asrc channel id.
 * @param mode is the half_word mode parameter.
 * @return uint8_t RET_OK for success else error.
 */
uint8_t iot_asrc_set_half_word_mode(IOT_ASRC_CHANNEL_ID id,
                                    IOT_ASRC_HALF_WORD_MODE mode);

/**
 * @brief This function is to config the starting source.
 *
 * @param id is the asrc channel id.
 * @param src is the start of source.
 * @return uint8_t RET_OK for success else error.
 */
uint8_t iot_asrc_config_start_src(IOT_ASRC_CHANNEL_ID id,
                                  IOT_ASRC_START_SRC src);

/**
 * @brief This function is to cinfig latch source.
 *
 * @param id is the asrc channel id.
 * @param src is the latch source.
 * @return uint8_t RET_OK for success else error.
 */
uint8_t iot_asrc_config_latch_src(IOT_ASRC_CHANNEL_ID id,
                                  IOT_ASRC_LATCH_SRC src);

/**
 * @brief This function is to overwrite start position.
 *
 * @param start is the start signal.
 * @return uint8_t RET_OK for success else error.
 */
uint8_t iot_asrc_overwrite_start(uint8_t start);

/**
 * @brief This function is to overwrite latch position.
 *
 * @param latch is the latch signal.
 * @return uint8_t RET_OK for success else error.
 */
uint8_t iot_asrc_overwrite_latch(uint8_t latch);

/**
 * @brief This function is to enable bluetooth tgt(target tracker).
 *
 * @param enable is the enable signal.
 */
void iot_asrc_enable_bt_tgt(bool_t enable);

/**
 * @brief This function is to set target sample count.
 *
 * @param id is the asrc channel id.
 * @param cnt is the count.
 */
void iot_asrc_set_target_sample_cnt(IOT_ASRC_CHANNEL_ID id, uint32_t cnt);

/**
 * @brief This function is to get latched sample count.
 *
 * @param id is the asrc channel id.
 * @return uint32_t is the latched sample.
 */
uint32_t iot_asrc_get_latched_sample_cnt(IOT_ASRC_CHANNEL_ID id);

/**
 * @brief This function is to get freerun sample count.
 *
 * @param id is the asrc channel id.
 * @return uint32_t is the freerun sample.
 */
uint32_t iot_asrc_get_freerun_sample_cnt(IOT_ASRC_CHANNEL_ID id);

/**
 * @brief This function is to register underrun irq hook.
 *
 * @param id is the asrc channel id.
 * @param hook is the int hook.
 */
void iot_asrc_register_underrun_irq_hook(IOT_ASRC_CHANNEL_ID id,
                                         iot_asrc_int_hook hook);

/**
 * @brief This function isvto register reachcnt irq hook.
 *
 * @param id is the asrc channel id.
 * @param hook is the int hook.
 */
void iot_asrc_register_reachcnt_irq_hook(IOT_ASRC_CHANNEL_ID id,
                                         iot_asrc_int_hook hook);
/**
 * @brief this function is to enable asrc interrupt
 *
 * @param id is the asrc channel id.
 * @param type interrupt type
 */
void iot_asrc_int_enable(IOT_ASRC_CHANNEL_ID id, IOT_ASRC_INT_TYPE type);

/**
 * @brief this function is to disable asrc interrupt
 *
 * @param id is the asrc channel id.
 * @param type interrupt type
 */
void iot_asrc_int_disable(IOT_ASRC_CHANNEL_ID id, IOT_ASRC_INT_TYPE type);

/**
 * @brief This function is to start asrc autamatically.
 *
 * @param id is the asrc channel id.
 * @param enable is the enable signal.
 */
void iot_asrc_auto_start(IOT_ASRC_CHANNEL_ID id, bool_t enable);

/**
 * @brief This function is to stop asrc automatically.
 *
 * @param id is the asrc channel id.
 * @param enable is the enable signal.
 */
void iot_asrc_auto_stop(IOT_ASRC_CHANNEL_ID id, bool_t enable);

/**
 * @brief This function is to let asrc continue run.
 *
 * @param id is the asrc channel id.
 * @param enable is the enable signal.
 */
void iot_asrc_continue(IOT_ASRC_CHANNEL_ID id, bool_t enable);

/**
 * @brief This function is threshold to let asrc auto run.
 *
 * @param chn is the asrc channel id.
 * @param threshold is threshold value
 */
void iot_asrc_auto_start_threshold(IOT_ASRC_CHANNEL_ID chn, uint8_t threshold);

/**
 * @brief asrc timer config
 *
 * @param id is the asrc channel id.
 * @param delay_tick tick to delay before trigger timer.
 * @return uint8_t
 */
uint8_t iot_asrc_timer_open(IOT_ASRC_CHANNEL_ID id, uint32_t delay_tick);

/**
 * @brief asrc timer config release
 *
 * @param id is the asrc channel id.
 * @return uint8_t
 */
uint8_t iot_asrc_timer_close(IOT_ASRC_CHANNEL_ID id);

/**
 * @brief start timer
 *
 * @return uint8_t
 */
uint8_t iot_asrc_timer_start_all(void);

/**
 * @brief stop timer
 *
 * @return uint8_t
 */
uint8_t iot_asrc_timer_stop_all(void);

/**
 * @brief access timer use
 *
 * @return bool_t
 */
bool_t iot_asrc_get_timer_start_all(void);

/**
 * @brief all of asrc timer start
 *
 * @param asrc_matrix binary representation, each bit represents a id
 */
void iot_asrc_timer_matrix_start(uint8_t asrc_matrix);

/**
 * @brief asrc_bt_tgt to bt select id
 *
 * @param id is the asrc channel id.
 */
void iot_asrc_bt_select(IOT_ASRC_CHANNEL_ID id);

/**
 * @brief This function is to set rx asrc filter num and ppm.
 *
 * @param id is the asrc channel id.
 * @param freq_out is the frequence output.
 * @param ppm is the num of asrc_ppm_unit.
 * @param filter_num is the filter num of asrc.
 * @return uint8_t RET_OK for success else error.
 */
uint8_t iot_asrc_rx_filter_ppm_set(IOT_ASRC_CHANNEL_ID id, uint32_t freq_out, int32_t ppm,
                                        uint8_t filter_num);
/**
 * @}
 * addtogroup ASRC
 * @}
 * addtogroup HAL
 */

#endif
