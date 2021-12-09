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


#ifndef _SRC_LIB_PLAY_CONTROLLER_PLAYER_API_H_
#define _SRC_LIB_PLAY_CONTROLLER_PLAYER_API_H_

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
 * @addtogroup PLAYER_API
 * @{
 * This section introduces the LIB PLAY_CONTROLLER PLAYER_API module's enum, structure, functions and how to use this module.
 * @brief Bluetooth player_api  API
 */


/*
 * INCLUDE FILES
 ****************************************************************************
 */
#include "types.h"
#include "modules.h"
#include "nplayer_api.h"
#include "nplayer_music_api.h"
#include "nplayer_voice_api.h"
#include "nplayer_tone_api.h"
#include "nplayer_volume_api.h"
/*
 * MACROS
 ****************************************************************************
 */

/*
 * DEFINES
 ****************************************************************************
 */
#define DBGLOG_LIB_PLAYER_RAW(fmt, arg...)      DBGLOG_LOG(PLAYER_MID, DBGLOG_LEVEL_VERBOSE, fmt, ##arg)
#define DBGLOG_LIB_PLAYER_INFO(fmt, arg...)     DBGLOG_STREAM_INFO(PLAYER_MID, fmt, ##arg)
#define DBGLOG_LIB_PLAYER_WARNING(fmt, arg...)  DBGLOG_STREAM_WARNING(PLAYER_MID, fmt, ##arg)
#define DBGLOG_LIB_PLAYER_ERROR(fmt, arg...)    DBGLOG_STREAM_ERROR(PLAYER_MID, fmt, ##arg)

/*
 * GLOBAL VARIABLE DECLARATIONS
 ****************************************************************************
 */

/*
 * FUNCTION DECLARATIONS
 ****************************************************************************
 */
/**
 * @brief  player_initialization
 *    config player config parameters (role/channel/...),
 *      and register the event callback handler.
 *
 * @param[in] p_cfg The pointer of the player config param.
 * @param[in] func The user event callback.
 *
 * @return  RET_OK when it's success.
 *          RET_INVAL when parameters invalid.
 */
uint8_t player_initialization(player_config_t *p_cfg,
                              player_event_callback func);

/**
 * @brief  player_adj_vol adjust the volume of the palyer.
 *
 * @param[in] id The player stream id of the player volume.
 * @param[in] db The decibels of the player volume.
 *
 * @return  RET_OK when it's success.
 *          RET_INVAL when parameters invalid.
 */
uint8_t player_adj_vol(uint8_t id, int8_t db);

/**
 * @brief  player_update_app_event
 *       user app update event to the player.
 *
 * @param[in] app_evt_id The event id of the user app.
 * @param[in] p_param The pointer of the user app event id param.
 *
 * @return  RET_OK when it's success.
 *          RET_INVAL when parameters invalid.
 */
uint8_t player_update_app_event(player_app_event_t app_evt_id, void *p_param);

/**
 * @brief This function is to mute speaker voice.
 *
 * @return uint8_t RET_OK for success else error.
 */
uint8_t player_vol_mute(void);

/**
 * @brief This function is to unmute speaker voice.
 *
 * @return uint8_t RET_OK for success else error.
 */
uint8_t player_vol_unmute(void);

/**
 * @brief This function is to change anc mode.
 * @param mode change anc to this.
 * @return uint8_t RET_OK for success else error.
 */
uint8_t player_anc_switch_mode(uint8_t mode);

/**
 * @brief This function is to start bone.
 *
 * @return uint8_t RET_OK for success else error.
 */
uint8_t player_bone_start(void);

/**
 * @brief This function is to stop bone.
 *
 * @return uint8_t RET_OK for success else error.
 */
uint8_t player_bone_stop(void);

/**
 * @brief player_tone_set_stream_vol_coeff
 * @param[in] tone_coeff   config playing tone vol coeff
 * @param[in] other_coeff  config playing music/call vol coeff
 * @return  RET_OK when it's success, else is fail
 *
 */
uint8_t player_tone_set_stream_vol_coeff(uint8_t tone_coeff, uint8_t other_coeff);

/**
 * @brief player_delay_mode_set.
 *
 * @param[in] mode      The player delay mode.
 *
 * @return  RET_OK when it's success, else is fail
 */
uint8_t player_delay_mode_set(player_delay_mode_t mode);

/**
 * @brief player_delay_mode_config.
 *
 * @param[in] p_param   The pointer of player delay mode param.
 *
 * @return  RET_OK when it's success, else is fail
 */
uint8_t player_delay_mode_config(player_delay_mode_param_t *p_param);

/**
 * @}
 * addtogroup PLAYER_API
 */

/**
 * @}
 * addtogroup PLAY_CONTROLLER
 */

/**
 * @}
 * addtogroup LIB
 */
#endif /* _SRC_LIB_PLAY_CONTROLLER_PLAYER_API_H_ */
