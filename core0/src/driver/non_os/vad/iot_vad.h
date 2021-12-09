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

#ifndef _DRIVER_NON_OS_VAD_H
#define _DRIVER_NON_OS_VAD_H

/**
 * @addtogroup HAL
 * @{
 * @addtogroup VAD
 * @{
 * This section introduces the VAD module's enum, structure, functions and how to use this driver.
 */

#include "types.h"
#include "vad.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup hal_vad_enum Enum
 * @{
 */

/** @brief VAD mode. */
typedef enum {
    IOT_VAD_MEM,
    IOT_VAD_GMM,
    IOT_VAD_TVAD,
    IOT_VAD_DBG,
    IOT_VAD_MAX,
} IOT_VAD_MODE;

/** @brief VAD debug mode. */
typedef enum {
    IOT_VAD_DBG_LARGE_BKGE,
    IOT_VAD_DBG_DEBUG,
    IOT_VAD_DBG_MAX,
} IOT_VAD_DBG_MODE;

/** @brief TVAD mode. */
typedef enum {
    IOT_TVAD_SPEECH_ONLY_EDGE,
    IOT_TVAD_SPEECH_DOUBLE_EDGE,
    IOT_TVAD_MODE_MAX,
} IOT_TVAD_MODE;

typedef enum {
    IOT_WEBRTC_MODE_SLEEP,
    IOT_WEBRTC_MODE_ALL,
    IOT_WEBRTC_MODE_MAX,
} IOT_WEBRTC_MODE;

/** @brief VAD source id. */
typedef enum {
    IOT_VAD_SRC_AUDIO,
    IOT_VAD_SRC_MADC,
    IOT_VAD_SRC_MAX,
} IOT_VAD_SRC_ID;

/** @brief VAD frequence. */
typedef enum {
    IOT_VAD_FRQ_4K,
    IOT_VAD_FRQ_8K,
    IOT_VAD_FRQ_16K,
    IOT_VAD_FRQ_32K,
    IOT_VAD_FRQ_MAX,
} IOT_VAD_FREQUENCE;

/** @brief VAD debug id. */
typedef enum {
    IOT_VAD_DBG_EF,
    IOT_VAD_DBG_ZF,
    IOT_VAD_DBG_ZCR_STATUS,
    IOT_VAD_DBG_ESEL_NE,
    IOT_VAD_DBG_ESEL_VAD,
    IOT_VAD_DBG_NOISE_NE,
    IOT_VAD_DBG_SIGMA_NE,
    IOT_VAD_DBG_NOISE_REF,
    IOT_VAD_DBG_SIGMA_REFF,
    IOT_VAD_DBG_VAD_ST,
    IOT_VAD_SPEECH_STATE,
    IOT_VAD_DBG_BIT_MAX,
} IOT_VAD_DBG_ID;

/**
 * @}
 */

/**
 * @defgroup hal_vad_struct Struct
 * @{
 */
typedef struct iot_vad_iir_coeff {
    int iir_a0;
    int iir_a1;
    int iir_b0;
    int iir_b1;
    int iir_b2;
} iot_vad_iir_coeff_t;

typedef struct iot_vad_config {
    uint8_t shift_bit;
    vad_gmm_ctrl_config_t gmm_ctrl;
    vad_decision_config_t vad_decision;
    vad_noise_control_config_t noise_nontrol;
    vad_zcr_config_t zcr_cfg;
    vad_cal_config_t energy_calculat;
} iot_vad_config_t;

/**
 * @}
 */

/**
 * @defgroup hal_vad_typedef Typedef
 * @{
 */
typedef void (*iot_vad_callback)(uint32_t *buffer, uint32_t length);
/**
 * @}
 */

/**
 * @brief This function is to init the gvad module.
 *
 */
void iot_gvad_init(void);

/**
 * @brief This function is to init the vad memory module.
 *
 */
void iot_vad_mem_init(void);

/**
 * @brief This function is is to init the tvad module.
 *
 * @param mode is tvad mode.
 */
void iot_tvad_init(IOT_TVAD_MODE mode);

/**
 * @brief This function is to init the vad debug module.
 *
 * @param mode is vad debug(dbg) mode.
 */
void iot_vad_dbg_init(IOT_VAD_DBG_MODE mode);

/**
 * @brief This function is to open the vad debug module.
 *
 * @param buffer is the pointer to interrupt service routine prototype.
 * @param length is the length of the buffer.
 * @param cb is the vad callback variable quantity.
 * @param mode
 * @return uint8_t RET_INVAL or RET_AGAIN or RET_OK.
 */
uint8_t iot_vad_dbg_open(uint32_t *buffer, uint32_t length, iot_vad_callback cb,
                         IOT_WEBRTC_MODE mode);

/**
 * @brief This function is to open the vad memory module.
 *
 * @param buffer is the pointer to interrupt service routine prototype.
 * @param length is the length of the buffer.
 * @param cb is the vad callback variable quantity.
 * @param mode
 * @return uint8_t RET_INVAL or RET_AGAIN or RET_OK.
 */
uint8_t iot_vad_mem_open(uint32_t *buffer, uint32_t length, iot_vad_callback cb,
                         IOT_WEBRTC_MODE mode);

/**
 * @brief This function is to open the tvad module.
 *
 * @param buffer is the pointer to interrupt service routine prototype.
 * @param length is the length of the buffer.
 * @param cb is the vad callback variable quantity.
 * @return uint8_t RET_INVAL or RET_AGAIN or RET_OK.
 */
uint8_t iot_tvad_open(uint32_t *buffer, uint32_t length, iot_vad_callback cb);

/**
 * @brief This function is to open the gvad module.
 *
 * @param buffer is the pointer to interrupt service routine prototype.
 * @param length is the length of the buffer.
 * @param cb is the vad callback variable quantity.
 * @param mode
 * @return uint8_t RET_INVAL or RET_AGAIN or RET_OK.
 */
uint8_t iot_gvad_open(uint32_t *buffer, uint32_t length, iot_vad_callback cb, IOT_WEBRTC_MODE mode);

/**
 * @brief This function is to reset the vad module.
 *
 */
void iot_vad_reset(void);

/**
 * @brief This function is to enable the vad clock module.
 *
 */
void iot_vad_clk_enable(void);

/**
 * @brief This function is to reset the vad soft module.
 *
 */
void iot_vad_soft_reset(void);

/**
 * @brief This function is to stop the vad td.
 *
 */
void iot_vad_td_stop(void);

/**
 * @brief This function is to start the vad td.
 *
 */
void iot_vad_td_start(void);

/**
 * @brief This function is to stop the vad gmm.
 *
 */
void iot_vad_gmm_stop(void);

/**
 * @brief This function is to start the vad gmm.
 *
 */
void iot_vad_gmm_start(void);

/**
 * @brief This function is to enable ram remapping.
 *
 * @param en is the signal of enable.
 */
void iot_vad_ram_remap_enable(bool_t en);

/**
 * @brief This function is to config vad frequence.
 *
 * @param frq is the vad frequence.
 */
void iot_vad_config(IOT_VAD_FREQUENCE frq);

/**
 * @brief This function is to set bitmap.
 *
 * @param mode is the vad mode.
 * @param bitmap is the bitmap.
 */
void iot_vad_set_bitmap(IOT_VAD_MODE mode, uint16_t bitmap);

/**
 * @brief This function is to link source.
 *
 * @param src is the vad source id.
 */
void iot_vad_src_link(IOT_VAD_SRC_ID src);

/**
 * @brief This function is to config iir coefficient.
 *
 * @param coeff is the iir coefficient.
 */
void iot_vad_iir_coefficient_config(const iot_vad_iir_coeff_t *coeff);

/**
 * @brief This function is to config vad generally.
 *
 * @param cfg is the vad configuration.
 */
void iot_vad_general_config(const iot_vad_config_t *cfg);

/**
 * @brief This function is to config strong background noise.
 *
 */
void iot_vad_strong_backgroud_noise_config(void);

/**
 * @brief This function is to get buffer it raw data.
 *
 * @return bool_t true for success else fail.
 */
bool_t iot_vad_get_buf_it_raw(void);

/**
 * @brief This function is to get buffer it status.
 *
 * @return bool_t is the buffer it's status.
 */
bool_t iot_vad_get_buf_it_status(void);

/**
 * @brief This function is to get buffer speeck statement.
 *
 * @return bool_t is the buffer speeck's statement.
 */
bool_t iot_vad_get_buf_speeck_state(void);

/**
 * @brief This function is to get dbg it status.
 *
 * @return bool_t is dbg it's status.
 */
bool_t iot_vad_vad_get_dbg_it_status(void);

/**
 * @brief This function is to get the pointer of buffer.
 *
 * @return uint16_t is the pointer of buffer.
 */
uint16_t iot_vad_get_buf_pointer(void);

uint8_t iot_vad_read_memory_poll(uint32_t *mem_pointer, bool_t state);

/**
 * @brief This function is to get the frame index of vad.
 *
 * @return uint16_t is the frame index of vad.
 */
uint16_t iot_vad_get_dbg_frame_index(void);

/**
 * @brief This function is to get the debug mode of vad.
 *
 * @return uint16_t is the debug mode of vad.
 */
uint8_t iot_vad_get_debug_mode(void);

/**
 * @brief This function is to get the bkg flag of vad.
 *
 * @return uint16_t is the bkg flag of vad.
 */
uint8_t iot_vad_get_long_large_bkg_flag(void);

/**
 * @brief This function is to clear bkg flag of vad.
 */
void iot_vad_long_large_bkg_flag_clr(void);

/**
 * @brief This function is to get the noise value of vad.
 *
 * @return uint32_t is the noise value of vad.
 */
uint32_t iot_vad_get_noise_ne(void);

/**
 * @brief This function is to read the data of first wake.
 *
 * @return uint32_t is the read index of buffer.
 */
uint32_t iot_vad_read_memory_first(void);

/**
 * @brief This function is to get the length of frame.
 *
 * @return uint16_t is the length of frame.
 */
uint16_t iot_vad_get_frame_length(void);

/**
 * @brief This function is to close tvad.
 *
 * @return uint8_t is RET_OK.
 */
uint8_t iot_tvad_close(void);

/**
 * @brief This function is to close gvad.
 *
 * @return uint8_t is RET_OK.
 */
uint8_t iot_gvad_close(void);

/**
 * @brief This function is to close mem.
 *
 * @return uint8_t is RET_OK.
 */
uint8_t iot_vad_mem_close(void);

/**
 * @brief This function is to close dbg.
 *
 * @return uint8_t is RET_OK.
 */
uint8_t iot_vad_dbg_close(void);

#ifdef __cplusplus
}
#endif

/**
 * @}
 * @}
 */
#endif
