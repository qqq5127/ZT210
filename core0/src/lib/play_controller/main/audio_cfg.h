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

#ifndef _SRC_LIB_PLAY_AUDIO_CFG_H_
#define _SRC_LIB_PLAY_AUDIO_CFG_H_

/**
 * @addtogroup LIB
 * @{
 */

/**
 * @addtogroup PLAY_CONTROLLER
 * @{
 * This section introduces the LIB PLAY_CONTROLLER module's enum, structure, functions and how to use this module.
 */

/**
 * @addtogroup AUDIO_CFG
 * @{
 * This section introduces the LIB PLAY_CONTROLLER AUDIO_CFG module's enum, structure, functions and how to use this module.
 * @brief Bluetooth audio_cfg  API
 */

/*
 * INCLUDE FILES
 ****************************************************************************
 */
#include "play_controller.h"
/*
 * MACROS
 ****************************************************************************
 */

/** @defgroup lib_play_controller_audio_cfg_enum Enum
 * @{
 */
/*
 * ENUMERATIONS
 ****************************************************************************
 */
/** @brief AUDIO ADC port id.*/
typedef enum {
    AUDIO_ADC_PORT_0,
    AUDIO_ADC_PORT_1,
    AUDIO_ADC_PORT_2,
    AUDIO_ADC_PORT_MAX,
}adc_port_id;

/* @brief audio interface defines */
typedef enum {
    AUDIO_AUDIO_RX_FIFO_0,
    AUDIO_AUDIO_RX_FIFO_1,
    AUDIO_AUDIO_RX_FIFO_2,
    AUDIO_AUDIO_RX_FIFO_3,
    AUDIO_AUDIO_RX_FIFO_4,
    AUDIO_AUDIO_RX_FIFO_5,
    AUDIO_AUDIO_RX_FIFO_6,
    AUDIO_AUDIO_RX_FIFO_7,
    AUDIO_AUDIO_RX_FIFO_MAX,
} AUDIO_AUDIO_RX_FIFO_ID;

/** @brief AUDIO rx def mode.*/
typedef enum {
    AUDIO_RX_DFE_ADC,
    AUDIO_RX_DFE_PDM,
    AUDIO_RX_DFE_ANC,
    AUDIO_RX_DFE_MAX,
} AUDIO_RX_DFE_MODE;

/** @brief AUDIO rx def freq.*/
typedef enum {
    AUDIO_RX_DFE_FREQ_8K,
    AUDIO_RX_DFE_FREQ_16K,
    AUDIO_RX_DFE_FREQ_32K,
    AUDIO_RX_DFE_FREQ_48K,
    AUDIO_RX_DFE_FREQ_62K5,
    AUDIO_RX_DFE_FREQ_96K,
    AUDIO_RX_DFE_FREQ_192K,
    AUDIO_RX_DFE_FREQ_MAX,
} AUDIO_RX_DFE_FREQ;

/** @brief AUDIO rx def.*/
typedef enum {
    AUDIO_AUDIO_RX_DFE_0,
    AUDIO_AUDIO_RX_DFE_1,
    AUDIO_AUDIO_RX_DFE_2,
    AUDIO_AUDIO_RX_DFE_3,
    AUDIO_AUDIO_RX_DFE_4,
    AUDIO_AUDIO_RX_DFE_5,
    AUDIO_AUDIO_RX_DFE_MAX,
} AUDIO_AUDIO_RX_DFE_ID;

/** @brief AUDIO DAC src id.*/
typedef enum {
    AUDIO_AUDIO_DAC_SRC_ADC_0,
    AUDIO_AUDIO_DAC_SRC_ADC_1,
    AUDIO_AUDIO_DAC_SRC_ADC_2,
    AUDIO_AUDIO_DAC_SRC_ADC_3,
    AUDIO_AUDIO_DAC_SRC_ADC_4,
    AUDIO_AUDIO_DAC_SRC_ADC_5,
    AUDIO_AUDIO_DAC_SRC_ASRC,
    AUDIO_AUDIO_DAC_SRC_ANC,
    AUDIO_AUDIO_DAC_SRC_MAX,
} AUDIO_AUDIO_DAC_SRC_ID;

/** @brief AUDIO tx def freq.*/
typedef enum {
    AUDIO_TX_DFE_FREQ_62K5,
    AUDIO_TX_DFE_FREQ_100K,
    AUDIO_TX_DFE_FREQ_125K,
    AUDIO_TX_DFE_FREQ_200K,
    AUDIO_TX_DFE_FREQ_400K,
    AUDIO_TX_DFE_FREQ_500K,
    AUDIO_TX_DFE_FREQ_800K,
    AUDIO_TX_DFE_FREQ_MAX,
} AUDIO_TX_DFE_FREQ_ID;

/** @brief AUDIO DAC channel.*/
typedef enum {
    AUDIO_AUDIO_DAC_CHN_MONO_L, /* (ASRC0) -> (L) */
    AUDIO_AUDIO_DAC_CHN_MONO_R, /* (ASRC1) -> (R) */
    AUDIO_AUDIO_DAC_CHN_STEREO, /* (ASRC0, ASRC1) -> (L, R) */
    AUDIO_AUDIO_DAC_CHN_MAX,
} AUDIO_AUDIO_DAC_CHANNEL;
/**
 * @}
 */

/*
 * DEFINES
 ****************************************************************************
 */
// speaker ppm value
#define AUDIO_DEFAULT_SPK_PPM           0
// speaker freq in  unit hz
#define AUDIO_DEFAULT_SPK_FREQ_IN       44100
// speaker freq in  unit hz
#define AUDIO_TONE_DEFAULT_SPK_FREQ_IN  16000
// speaker freq out  unit hz
#define AUDIO_DEFAULT_SPK_FREQ_OUT      800000
// speaker out channel
#define AUDIO_SPK_OUT_CH                AUDIO_AUDIO_DAC_CHN_MONO_R
#define AUDIO_SPK_DAC_FREQ              AUDIO_TX_DFE_FREQ_800K

// mic freq in  unit hz
#define AUDIO_DEFAULT_MIC_FREQ_IN       16000
// mic freq out  unit hz
#define AUDIO_DEFAULT_MIC_FREQ_OUT      16000

//voice mic config default gain
#define AUDIO_RECORD_GAIN_VALUE 101
//voice mic config ASRC channel
#define AUDIO_VOICE_MIC_CHANNEL_ID AUDIO_MIC_ASRC_CHANNEL_0
//ANC mic config ASRC channel
#define AUDIO_ANC_MIC_CHANNEL_ID AUDIO_MIC_ASRC_CHANNEL_1
//voice mic config default adc id
#define AUDIO_VOICE_ADC_PORT_ID AUDIO_ADC_PORT_1
//anc mic config default adc id
#define AUDIO_ANC_ADC_PORT_FF AUDIO_ADC_PORT_0
#define AUDIO_ANC_ADC_PORT_FB AUDIO_ADC_PORT_2

/*
 * TYPE DEFINITIONS
 ****************************************************************************
 */


/*
 * GLOBAL VARIABLE DECLARATIONS
 ****************************************************************************
 */


/*
 * FUNCTION DECLARATIONS
 ****************************************************************************
 */


/**
 * @}
 * addtogroup AUDIO_CFG
 */

/**
 * @}
 * addtogroup PLAY_CONTROLLER
 */

/**
 * @}
 * addtogroup LIB
 */

#endif /* _SRC_LIB_PLAY_AUDIO_CFG_H_ */
