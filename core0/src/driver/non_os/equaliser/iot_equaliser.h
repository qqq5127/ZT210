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

#ifndef _IOT_EQUALISER_H
#define _IOT_EQUALISER_H
/**
 * @addtogroup HAL
 * @{
 * @addtogroup EQUALISER
 * @{
 * This section introduces the EQUALISER module's enum, structure, functions and how to use this driver.
 */

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define IOT_EQ_ES_MAX 12
#define IOT_EQ_EM_MAX 20
#define IOT_EQ_FILTER_MAX (IOT_EQ_ES_MAX + IOT_EQ_EM_MAX)
#define IOT_EQ_BIQUE_MAX 56

/** @defgroup hal_equaliser_enum Enum
  * @{
  */

/** @brief EQUALISER flt mode.*/
typedef enum {
    IOT_EQ_FIX_POINT = 0,
    IOT_EQ_FLOATING_POINT,
} IOT_EQ_FLT_MODE;

/** @brief EQUALISER data mode.*/
typedef enum {
    IOT_EQ_32_BIT = 0,
    IOT_EQ_16_BIT,
} IOT_EQ_DATA_MODE;

typedef enum {
    IOT_EQ_TRANS_POLLING,
    IOT_EQ_TRANS_INTERRUPT,
    IOT_EQ_TRANS_MAX,
}IOT_EQ_TRANS_MODE;

typedef enum {
    IOT_EQ_SAMPLERATE_16K,
    IOT_EQ_SAMPLERATE_32K,
    IOT_EQ_SAMPLERATE_44K1,
    IOT_EQ_SAMPLERATE_48K,
    IOT_EQ_SAMPLERATE_MAX,
} IOT_EQ_SAMPLERATE_ID;

typedef enum {
    IOT_EQ_SPEAKER,
    IOT_EQ_MUSIC,
    IOT_EQ_MODE_MAX,
} IOT_EQ_MODE;

typedef enum {
    IOT_EQ_PEQ,
    IOT_EQ_LOW_SHELF,
    IOT_EQ_HIGH_SHELF,
    IOT_EQ_TYPE_MAX,
}IOT_EQ_TYPE;
/**
  * @}
  */

typedef void (*iot_equaliser_done_callback)(void);

typedef struct iot_equaliser_music_param {
    IOT_EQ_FLT_MODE flt_mode;
    IOT_EQ_DATA_MODE data_mode;
    IOT_EQ_TRANS_MODE trans_mode;
    int32_t sample_rate;
    uint32_t freq[IOT_EQ_EM_MAX];
    float gain[IOT_EQ_EM_MAX];
    float q[IOT_EQ_EM_MAX];
    uint8_t type[IOT_EQ_EM_MAX];
    iot_equaliser_done_callback cb;
} iot_equaliser_music_param_t;

typedef struct iot_equaliser_speaker_param {
    int32_t sample_rate;
    float overall_gain;
    float gain[IOT_EQ_ES_MAX];
    uint32_t freq[IOT_EQ_ES_MAX];
    float q[IOT_EQ_ES_MAX];
    uint8_t type[IOT_EQ_ES_MAX];
} iot_equaliser_speaker_param_t;

typedef struct iot_equaliser_process {
    uint8_t *data_in;
    uint8_t *data_out;
    uint32_t data_length;
    iot_equaliser_done_callback cb;
    IOT_EQ_TRANS_MODE trans_mode;
} iot_equaliser_process_t;

/**
 * @brief This function is to init equaliser.
 *
 */
void iot_equaliser_init(void);

/**
 * @brief This function is to deinit equaliser.
 *
 */
void iot_equaliser_deinit(void);

/**
 * @brief design the eq speaker coeff
 *
 * @param param eq speaker parameter
 * @return uint8_t RET_OK for success else error.
 */
uint8_t iot_equaliser_speaker_open(iot_equaliser_speaker_param_t *param);

/**
 * @brief design the eq coeff and configuration
 *
 * @param param eq music parameter
 * @return int8_t be equal of greater than 0 is using eq band number else error
 */
int8_t iot_equaliser_music_open(iot_equaliser_music_param_t *param);

/**
 * @brief This function is to release equaliser.
 *
 */
void iot_equaliser_close(void);

/**
 * @brief
 *
 * @param process  process eq
 * @return uint8_t RET_OK for success else error.
 */
uint8_t iot_equaliser_process(const iot_equaliser_process_t *process);

/**
 * @brief retry configure equaliser
 *
 * @param data_mode 16 bit mode or 32 bit mode
 * @return uint8_t RET_OK for success else error.
 */
uint8_t iot_equaliser_retry_config_poll(IOT_EQ_DATA_MODE data_mode);

/**
 * @brief This function is eq read coef state
 *
 * @param eq_coef_state is coef state address
 */
void iot_equaliser_get_coeff_state(uint8_t *eq_coef_state);

/**
 * @brief the ahb burst trans data bytes number is 2^powers
 *
 * @param powers range is 0~6
 * @return uint8_t RET_OK for success else error.
 */
uint8_t iot_equaliser_set_burst_length(uint8_t powers);

/**
 * @brief set equaliser data mode
 *
 * @param mode 16bit or 32bit
 */
void iot_eq_set_data_mode(IOT_EQ_DATA_MODE mode);

#ifdef __cplusplus
}
#endif
/**
* @}
* @}
*/
#endif /* _IOT_EQUALISER_H */
