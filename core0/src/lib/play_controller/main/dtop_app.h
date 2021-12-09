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

#ifndef _SRC_APP_TWS_DTOP_MAIN_DTOP_APP_H_
#define _SRC_APP_TWS_DTOP_MAIN_DTOP_APP_H_

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
 * @addtogroup DTOP_APP
 * @{
 * This section introduces the LIB PLAY_CONTROLLER DTOP_APP module's enum, structure, functions and how to use this module.
- * @brief Bluetooth dtop_app  API
 */
/*
 * INCLUDE FILES
 ****************************************************************************
 */
#include "types.h"

/*
 * MACROS
 ****************************************************************************
 */

/*
 * DEFINES
 ****************************************************************************
 */

/*
 * ENUMERATIONS
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
 * @brief Main app entry.
 */
void dtop_app_main(void *arg);

/**
 * @brief This function is to init asrc.
 *
 * @param id is to init the channel of asrc.
 * @param freq_in is the asrc sample frequence.
 */
void app_asrc_init(uint8_t id, uint32_t freq_in);

/**
 * @brief This function is to init audio play path.
 */
void audio_spk_init(void);

/**
 * @brief This function is start audio play.
 * @param id is using the channel of speaker.
 * @return uint8_t RET_OK for success else error.
 */
uint8_t audio_spk_start(uint8_t id);

/**
 * @brief This function is stop audio play.
 * @param id is using the channel of speaker.
 * @return uint8_t RET_OK for success else error.
 */
uint8_t audio_spk_stop(uint8_t id);

/**
 * @brief audio_spk_adjust_gain_target.
 *
 * @param db is the decibels of the player volume.
 * @return uint8_t RET_OK for success else error.
 */
uint8_t audio_spk_adjust_gain_target(int8_t db);

/**
 * @brief audio_spk_get_gain_target.
 * @return current gain val, unit db.
 */
int8_t audio_spk_get_gain_target(void);

/**
 * @brief audio_spk_set_full_scale_limit.
 * @param full_scale set spk full scale limit.
 */
void audio_spk_set_full_scale_limit(int16_t full_scale);

/**
 * @}
 * addtogroup DTOP_APP
 */

/**
 * @}
 * addtogroup PLAY_CONTROLLER
 */

/**
 * @}
 * addtogroup LIB
 */

#endif /* _SRC_APP_TWS_DTOP_MAIN_DTOP_APP_H_ */
