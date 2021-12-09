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

#ifndef _APP_AUDIO_H_
#define _APP_AUDIO_H_

/**
 * @addtogroup APP
 * @{
 */

/**
 * @addtogroup APP_AUDIO
 * @{
 * This section introduces the APP AUDIO module's enum, structure, functions and how to use this module.
 */

#include "types.h"
#include "userapp_dbglog.h"
#include "player_api.h"

#define DBGLOG_AUDIO_DBG(fmt, ...) DBGLOG_USER_APP_INFO("[audio] " fmt, ##__VA_ARGS__)
#define DBGLOG_AUDIO_ERR(fmt, ...) DBGLOG_USER_APP_ERROR("[audio] " fmt, ##__VA_ARGS__)

#define CALL_VOLUME_LEVEL_MAX  15
#define MUSIC_VOLUME_LEVEL_MAX 127

#ifndef DEFAULT_ANC_LEVEL
#define DEFAULT_ANC_LEVEL ANC_LEVEL_FLIGHT
#endif

#ifndef DEFAULT_TRANSPARENCY_LEVEL
#define DEFAULT_TRANSPARENCY_LEVEL TRANSPARENCY_LEVEL_FULL
#endif

/** @defgroup app_audio_enum Enum
 * @{
 */

typedef enum {
    AUDIO_VOLUME_MUSIC,
    AUDIO_VOLUME_CALL,
    AUDIO_VOLUME_TONE,
    AUDIO_VOLUME_MIC,
} audio_volume_type_t;

typedef enum {
    LISTEN_MODE_UNKNOWN = 0,
    LISTEN_MODE_NORMAL = 1,
    LISTEN_MODE_ANC = 2,
    LISTEN_MODE_TRANSPARENCY = 3
} listen_mode_t;

typedef enum {
    ANC_LEVEL_FLIGHT,
    ANC_LEVEL_OUT_DOOR,
    ANC_LEVEL_IN_DOOR,
} anc_level_t;

typedef enum {
    TRANSPARENCY_LEVEL_FULL,
    TRANSPARENCY_LEVEL_VOICE,
} transparency_level_t;

typedef enum {
    LISTEN_MODE_TOGGLE_NONE = 0,
    LISTEN_MODE_TOGGLE_NORMAL = 1,
    LISTEN_MODE_TOGGLE_ANC = 2,
    LISTEN_MODE_TOGGLE_NORMAL_ANC = 3,
    LISTEN_MODE_TOGGLE_TRANSPARENCY = 4,
    LISTEN_MODE_TOGGLE_NORMAL_TRANSPARENCY = 5,
    LISTEN_MODE_TOGGLE_ANC_TRANSPARENCY = 6,
    LISTEN_MODE_TOGGLE_NORMAL_ANC_TRANSPARENCY = 7,
    LISTEN_MODE_TOGGLE_NORMAL_TRANSPARENCY_ANC = 8,
} listen_mode_toggle_cfg_t;

/**
 * @}
 */

/**
 * @brief init app audio module
 */
void app_audio_init(void);

/**
 * @brief deinit app audio module
 *
 */
void app_audio_deinit(void);

/**
 * @brief increase volume one step
 */
void app_audio_volume_up(void);

/**
 * @brief decrease volume one step
 */
void app_audio_volume_down(void);

/**
 * @brief set music volume
 * @param volume_level  current new  volume level for music,0-127
 */
void app_audio_music_volume_set(uint8_t volume_level);

/**
 * @brief set calling volume
 * @param volume_level  current new  volume level for calling,0-15
 */
void app_audio_call_volume_set(uint8_t volume_level);

/**
 * @brief start music play
 * @param sample sample rate
 * @param codec_type auido encode type
 * @return 0 for success, else for the error reason
 */
int app_audio_music_start(uint32_t sample, player_codec_type_t codec_type);

/**
 * @brief stop music playing
 *
 * @return 0 for success, else for the error reason
 */
int app_audio_music_stop(void);

/**
 * @brief start calling
 * @param sample sample rate
 * @param codec_type voice encode type
 * @return 0 for success, else for the error reason
 */
int app_audio_call_start(uint32_t sample, player_codec_type_t codec_type);

/**
 * @brief stop stop calling
 *
 * @return 0 for success, else for the error reason
 */
int app_audio_call_stop(void);

/**
 * @brief get the curren listen mode
 *
 * @return current listen mode
 */
listen_mode_t app_audio_listen_mode_get(void);

/**
 * @brief change listen mode
 *
 * @param mode new listen mode
 */
void app_audio_listen_mode_set(listen_mode_t mode);

/**
 * @brief change listen mode, no tone and evtsys will be generated
 *
 * @param mode new listen mode
 */
void app_audio_listen_mode_set_silently(listen_mode_t mode);

/**
 * @brief switch listen mode specified by app_audio_set_listen_mode_toggle_cfg
 */
void app_audio_listen_mode_toggle(void);

/**
 * @brief set the current listen mode toggle config
 *
 * @param cfg the config to be set
 */
void app_audio_listen_mode_set_toggle_cfg(listen_mode_toggle_cfg_t cfg);

/**
 * @brief get current listen mode toggle configuration
 *
 * @return the current listen mode toggle configuration
 */
listen_mode_toggle_cfg_t app_audio_listen_mode_get_toggle_cfg(void);

/**
 * @brief enter normal mode, ignore current listen mode
 *
 * @param force_normal true if force normal mode, false if not
 */
void app_audio_listen_mode_force_normal(bool_t force_normal);

/**
 * @brief change anc level
 *
 * @param level new level
 */
void app_audio_anc_level_set(anc_level_t level);

/**
 * @brief change transparency level
 *
 * @param level new level
 */
void app_audio_transparency_level_set(transparency_level_t level);

/**
 * @brief switch listen mode by different charging status
 *
 * @param charging true if charging, false if not
 */
void app_audio_handle_charging_changed(bool_t charging);

/**
 * @brief app_audio_game_mode_set_enabled
 *
 * @param enabled default false
 *
 * @return  true if success, false if not
 */
bool_t app_audio_game_mode_set_enabled(bool_t enabled);

/**
 * @brief app_audio_game_mode_toggle
 *
 * @return  true if success, false if not
 */
bool_t app_audio_game_mode_toggle(void);

/**
 * @brief app_audio_game_mode_get
 *
 * @return  true if enabled, false if not
 */
bool_t app_audio_game_mode_get(void);

/**
 * @brief private function to handle bt power on event
 */
void app_audio_handle_bt_power_on(void);

/**
 * @brief private function to handle bt power off event
 */
void app_audio_handle_bt_power_off(void);

/**
 * @brief private function to notify player of the new state about AG
 *
 * @param new_state new state
 */
void app_audio_handle_sys_state(uint16_t new_state);

/**
 * @brief private function to notify player of the new stage about wws
 *
 * @param new_event new event
 */
void app_audio_handle_wws_event(uint16_t new_event);

/**
 * @}
 * addtogroup APP_AUDIO
 */

/**
 * @}
 * addtogroup APP
 */

#endif
