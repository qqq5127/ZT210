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
#ifndef _APP_ECONN_H_
#define _APP_ECONN_H_
/**
 * @brief easy connection module
 */

#include "types.h"
#include "userapp_dbglog.h"
#include "app_charger.h"
#include "key_sensor.h"
#include "app_bat.h"
#include "charger_box.h"
#include "ro_cfg.h"
#include "app_wws.h"

#define DBGLOG_ECONN_DBG(fmt, ...) DBGLOG_USER_APP_INFO("[econn] " fmt, ##__VA_ARGS__)
#define DBGLOG_ECONN_ERR(fmt, ...) DBGLOG_USER_APP_ERROR("[econn] " fmt, ##__VA_ARGS__)

#ifdef ECONN
/**
 * @brief init the easy connection module
 */
void app_econn_init(void);

/**
 * @brief deinit the easy connection module
 */
void app_econn_deinit(void);

/**
 * @brief check if the easy connection module exists
 *
 * @return true if exist else not exists
 */
bool_t app_econn_exists(void);

/**
 * @brief enter ag pairing mode
 */
void app_econn_enter_ag_pairing(void);

/**
 * @brief handler for sys state changed event
 *
 * @param sys_state new sys state
 */
void app_econn_handle_sys_state(uint32_t sys_state);

/**
 * @brief handler for listen mode changed event
 *
 * @param mode new listen mode
 */
void app_econn_handle_listen_mode_changed(uint8_t mode);

/**
 * @brief handler for in ear changed event
 *
 * @param in_ear new in ear state
 */
void app_econn_handle_in_ear_changed(bool_t in_ear);

/**
 * @brief handler for peer in ear changed event
 *
 * @param in_ear new in ear state
 */
void app_econn_handle_peer_in_ear_changed(bool_t in_ear);

/**
 * @brief handle for charging changed event
 *
 * @param charging true if charging, false if not
 */
void app_econn_handle_charging_changed(bool_t charging);

/**
 * @brief handle for peer charging changed event
 *
 * @param charging true if charging, false if not
 */
void app_econn_handle_peer_charging_changed(bool_t charging);

/**
 * @brief handle for box state changed event
 *
 * @param state new box state
 */
void app_econn_handle_box_state_changed(box_state_t state);

/**
 * @brief handle for peer box state changed event
 *
 * @param state new box state
 */
void app_econn_handle_peer_box_state_changed(box_state_t state);

/**
 * @brief handler for battery level changed event
 *
 * @param level battery level, 0-100
 */
void app_econn_handle_battery_level_changed(uint8_t level);

/**
 * @brief handler for peer battery level changed event
 *
 * @param level battery level, 0-100
 */
void app_econn_handle_peer_battery_level_changed(uint8_t level);

/**
 * @brief handler for tws state changed event
 *
 * @param connected true if connected, false if disconnected
 */
void app_econn_handle_tws_state_changed(bool_t connected);

/**
 * @brief handler for tws role changed event
 *
 * @param is_master true if is master, false if not
 */
void app_econn_handle_tws_role_changed(bool_t is_master);

/**
 * @brief handle for user event
 *
 * @param event the user event
 * @param from_peer true if this is a peer event, false if not
 *
 * @return true if econn handled the event and app will not handle it, false if not
 */
bool_t app_econn_handle_usr_evt(uint16_t event, bool_t from_peer);

/**
 * @brief handle for system event
 *
 * @param event the system event
 * @param param event parameter
 *
 * @return true if econn handled the event and app will not handle it, false if not
 */
bool_t app_econn_handle_sys_evt(uint16_t event, void *param);

/**
 * @brief handler for ntc temperature
 *
 * @param value temperature in degrees centigrade
 */
void app_econn_handle_ntc_value(int8_t value);

/**
 * @brief handle event led
 *
 * @brief event EVTUSR_XXX or EVTSYS_XXX
 *
 * @return true if econn handled the event and app will not handle it, false if not

 */
bool_t app_econn_handle_event_led(uint32_t event);

/**
 * @brief handle state led
 *
 * @param state current system state
 *
 * @return true if econn handled the event and app will not handle it, false if not
 */
bool_t app_econn_handle_state_led(uint32_t state);

/**
 * @brief handle state led
 *
 * @param info information fo the key pressed event
 *
 * @return true if econn handled the event and app will not handle it, false if not
 */
bool_t app_econn_handle_key_pressed(key_pressed_info_t *info);

/**
 * @brief get battery level from econn
 *
 * @return BAT_LEVEL_INVALID for default, 0-100 for new battery level
 */
uint8_t app_econn_get_bat_level(void);

/**
 * @brief handle charger event from charger module
 *
 * @param evt the charger event
 * @param param parameter of the event
 *
 * @return true if econn handled the event and app will not handle it, false if not
 */
bool_t app_econn_handle_charger_evt(charger_evt_t evt, void *param);

/**
 * @brief get vendor id from econn
 *
 * @return vendor id
 */
uint16_t app_econn_get_vid(void);

/**
 * @brief get product id from econn
 *
 * @return product id
 */
uint16_t app_econn_get_pid(void);

/**
 * @brief get tws pair magic number from econn
 *
 * @return tws pair magic number
 */
uint8_t app_econn_get_tws_pair_magic(void);

/**
 * @brief handle volume change from econn
 *
 * @param type volume type
 * @param level volume level
 *
 * @return new volume level
 */
uint8_t app_econn_handle_volume(audio_volume_type_t type, uint8_t level);

#else
static inline void app_econn_init(void)
{
}
static inline void app_econn_deinit(void)
{
}
static inline bool_t app_econn_exists(void)
{
    return false;
}
static inline void app_econn_enter_ag_pairing(void)
{
}
static inline void app_econn_handle_sys_state(uint32_t sys_state)
{
    UNUSED(sys_state);
}
static inline void app_econn_handle_listen_mode_changed(uint8_t mode)
{
    UNUSED(mode);
}
static inline void app_econn_handle_in_ear_changed(bool_t in_ear)
{
    UNUSED(in_ear);
}
static inline void app_econn_handle_peer_in_ear_changed(bool_t in_ear)
{
    UNUSED(in_ear);
}
static inline void app_econn_handle_battery_level_changed(uint8_t level)
{
    UNUSED(level);
}
static inline void app_econn_handle_peer_battery_level_changed(uint8_t level)
{
    UNUSED(level);
}
static inline void app_econn_handle_tws_state_changed(bool_t connected)
{
    UNUSED(connected);
}
static inline void app_econn_handle_tws_role_changed(bool_t is_master)
{
    UNUSED(is_master);
}
static inline bool_t app_econn_handle_usr_evt(uint16_t event, bool_t from_peer)
{
    UNUSED(event);
    UNUSED(from_peer);

    return false;
}
static inline bool_t app_econn_handle_sys_evt(uint16_t event, void *param)
{
    UNUSED(event);
    UNUSED(param);

    return false;
}
static inline void app_econn_handle_ntc_value(int8_t value)
{
    UNUSED(value);
}
static inline void app_econn_handle_charging_changed(bool_t charging)
{
    UNUSED(charging);
}
static inline void app_econn_handle_peer_charging_changed(bool_t charging)
{
    UNUSED(charging);
}
static inline void app_econn_handle_box_state_changed(box_state_t state)
{
    UNUSED(state);
}
static inline void app_econn_handle_peer_box_state_changed(box_state_t state)
{
    UNUSED(state);
}
static inline bool_t app_econn_handle_event_led(uint32_t event)
{
    UNUSED(event);
    return false;
}
static inline bool_t app_econn_handle_state_led(uint32_t state)
{
    UNUSED(state);
    return false;
}
static inline bool_t app_econn_handle_key_pressed(key_pressed_info_t *info)
{
    UNUSED(info);
    return false;
}
static inline uint8_t app_econn_get_bat_level(void)
{
    return BAT_LEVEL_INVALID;
}
static inline bool_t app_econn_handle_charger_evt(charger_evt_t evt, void *param)
{
    UNUSED(evt);
    UNUSED(param);
    return false;
}
static inline uint16_t app_econn_get_vid(void)
{
    return ro_gen_cfg()->vid;
}
static inline uint16_t app_econn_get_pid(void)
{
    return ro_gen_cfg()->pid;
}
static inline uint8_t app_econn_get_tws_pair_magic(void)
{
    return WWS_DEFAULT_PAIR_MAGIC;
}
static inline uint8_t app_econn_handle_volume(audio_volume_type_t type, uint8_t level)
{
    UNUSED(type);
    return level;
}
#endif

#endif   //_APP_ECONN_H_
