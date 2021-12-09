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

/**
 * @addtogroup APP
 * @{
 */

/**
 * @addtogroup APP_WWS
 * @{
 * This section introduces the APP WWS module's enum, structure, functions and how to use this module.
 */

#ifndef _APP_WWS_H__
#define _APP_WWS_H__
#include "types.h"
#include "userapp_dbglog.h"
#include "app_bt.h"
#include "app_audio.h"
#include "app_main.h"
#include "app_charger.h"

#define DBGLOG_WWS_DBG(fmt, ...) DBGLOG_USER_APP_INFO("[wws] " fmt, ##__VA_ARGS__)
#define DBGLOG_WWS_ERR(fmt, ...) DBGLOG_USER_APP_ERROR("[wws] " fmt, ##__VA_ARGS__)

#define WWS_DEFAULT_PAIR_MAGIC 0xFF

#define MAX_REMOTE_MSG_PARAM_LEN 520

// clang-format off
#ifndef FTM_PEER_BDADDR
#define FTM_PEER_BDADDR {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF}
#endif

#ifndef AUTO_TWS_PAIR_PEER_BDADDR
#define AUTO_TWS_PAIR_PEER_BDADDR {0x00,0x00,0x00,0x00,0x00,0x00}
#endif
// clang-format on

static const BD_ADDR_T ftm_peer_bdaddr = {.addr = FTM_PEER_BDADDR};
static const BD_ADDR_T auto_tws_pair_peer_bdaddr = {.addr = AUTO_TWS_PAIR_PEER_BDADDR};

/**
 * @brief init the app wws module
 */
void app_wws_init(void);

/**
 * @brief deinit the app wws module
 */
void app_wws_deinit(void);

/**
 * @brief start wws pair
 *
 * @param vid vendor id
 * @param pid product id
 * @param magic magic number
 */
void app_wws_start_pair(uint16_t vid, uint16_t pid, uint8_t magic);

/**
 * @brief trigger a role switch for wws
 */
void app_wws_role_switch(void);

/**
 * @brief check if wws is connected
 *
 * @return true if wws is connected, false if not
 */
bool_t app_wws_is_connected(void);

/**
 * @brief check if current wws role is slave
 *
 * @return true if current role is slave, false if not
 */
bool_t app_wws_is_slave(void);

/**
 * @brief check if current wws role is master
 *
 * @return true if current role is master, false if not
 */
bool_t app_wws_is_master(void);

/**
 * @brief check if the earbud is the left one
 *
 * @return true if left, false if stereo or right.
 */
bool_t app_wws_is_left(void);

/**
 * @brief check if the earbud is the right one
 *
 * @return true if right, false if stereo or left.
 */
bool_t app_wws_is_right(void);

/**
 * @brief check if wws is enabled.
 *
 * @return true if enabled, false if not.
 */
bool_t app_wws_is_enabled(void);

/**
 * @brief check if current wws role is master and wws is connected
 *
 * @return true if current is connected master, false if not
 */
bool_t app_wws_is_connected_slave(void);

/**
 * @brief check if current wws role is slave and wws is connected
 *
 * @return true if current is connected slave, false if not
 */
bool_t app_wws_is_connected_master(void);

/**
 * @brief check if current wws is pairing
 *
 * @return true if current is pairing, false if not
 */
bool_t app_wws_is_pairing(void);

/**
 * @brief check if current wws is connecting
 *
 * @return true if current is connecting, false if not
 */
bool_t app_wws_is_connecting(void);

/**
 * @brief check if peer is inear
 *
 * @return true if peer is inear, false if not
 */
bool_t app_wws_peer_is_inear(void);

/**
 * @brief check if peer is charging
 *
 * @return true if peer is charging, false if not
 */
bool_t app_wws_peer_is_charging(void);

/**
 * @brief get the box state of peer
 *
 * @return box state of peer (unknown, opened, closed)
 */
box_state_t app_wws_peer_get_box_state(void);

/**
 * @brief get the battery level of peer
 *
 * @return battery level of peer
 */
uint8_t app_wws_peer_get_battery_level(void);

/**
 * @brief get peer bt address
 *
 * @return peer bt address
 */
const BD_ADDR_T *app_wws_get_peer_addr(void);

/**
 * @brief get the firmware version build of peer
 *
 * @return firmware version build of peer
 */
uint16_t app_wws_peer_get_fw_ver_build(void);

/**
 * @brief get the firmware version configed in KV
 *
 * @return firmware version in KV
 */
uint32_t app_wws_peer_get_fw_in_kv(void);

/**
 * @brief check if peer is discoverable
 *
 * @return true if peer is discoverable, false if not
 */
bool_t app_wws_peer_is_discoverable(void);

/**
 * @brief check if peer is connectable
 *
 * @return true if peer is connectable, false if not
 */
bool_t app_wws_peer_is_connectable(void);

/**
 * @brief slave send user event to master
 *
 * @param evt the event triggered by slave UI
 */
void app_wws_send_usr_evt(uint16_t evt);

/**
 * @brief send in ear state to peer
 *
 * @param enabled in ear detection enabled or not, master will ignore
 *                 this field when received the message.
 * @param in_ear the earbud is in ear or not
 */
void app_wws_send_in_ear(bool_t enabled, bool_t in_ear);

/**
 * @brief slave send this message to request master enable/disable inear detection
 *
 * @param enable true:enable inear detection  false:disable inear detection
 */
void app_wws_send_in_ear_enable_request(bool_t enable);

/**
 * @brief send charger state to peer
 *
 * @param charging current charging or not
 */
void app_wws_send_charger(bool_t charging);

/**
 * @brief send battery level to peer
 *
 * @param level battery level, 0-100
 */
void app_wws_send_battery(uint8_t level);

/**
 * @brief send box state to peer
 *
 * @param box_state new box state
 */
void app_wws_send_box_state(uint8_t box_state);

/**
 * @brief send reset pdl to peer
 */
void app_wws_send_reset_pdl(void);

/**
 * @brief master send current volume to slave
 *
 * @param call call volume
 * @param music music volume
 */
void app_wws_send_volume(uint8_t call, uint8_t music);

/**
 * @brief master send current listen mode to slave
 *
 * @param mode current listen mode
 */
void app_wws_send_listen_mode(uint8_t mode);

/**
 * @brief master send game mode enabled or disable to slave
 *
 * @param enabled or disable game mode
 */
void app_wws_send_game_mode_set_enabled(bool_t enabled);

/**
 * @brief master send current anc level to slave
 *
 * @param level current anc level
 */
void app_wws_send_anc_level(uint8_t level);

/**
 * @brief master send current transparency level to slave
 *
 * @param level current transparency level
 */
void app_wws_send_transparency_level(uint8_t level);

/**
 * @brief send set custom key config to remote
 *
 * @param type key type
 * @param state_mask mask of system state
 * @param event the event to trigger
 */
void app_wws_send_set_cus_key(uint8_t type, uint16_t state_mask, uint16_t event);

/**
 * @brief get remote custom key config
 *
 * @param count count of the custom key to get
 * @param type key type array
 * @param state_mask state mask array
 * @param callback to handle the result
 */
void app_wws_send_get_cus_key(uint8_t count, const uint8_t *type, const uint16_t *state_mask,
                              void (*callback)(uint8_t count, const uint16_t *events));

/**
 * @brief send reset custom key config to remote
 *
 */
void app_wws_send_reset_cus_key(void);

/**
 * @brief send current visibility to peer
 *
 * @param discoverable true if discoverable, else if not;
 * @param connectable true if connectable, else if not;
 */
void app_wws_send_visibility(bool_t discoverable, bool_t connectable);

/**
 * @brief send setting volume request to peer
 *
 * @param call call volume level
 * @param music music volume level
 */
void app_wws_send_vol_set_request(uint8_t call, uint8_t music);

/**
 * @brief send setting listen mode request to peer
 *
 * @param mode listen mode;
 */
void app_wws_send_listen_mode_set_request(listen_mode_t mode);

/**
 * @brief send clearing pdl request to peer
 *
 */
void app_wws_send_pdl_clear_request(void);

/**
 * @brief send message to remote module
 * @param type @see app_msg_type_t for detial
 * @param id remote message id
 * @param param_len length of message parameter, <=MAX_REMOTE_MSG_PARAM_LEN
 * @param param message parameter
 */
void app_wws_send_remote_msg(app_msg_type_t type, uint16_t id, uint16_t param_len,
                             const uint8_t *param);

/**
 * @brief same as app_wws_send_remote_msg, but will not request exit sniff mode
 * @param type @see app_msg_type_t for detial
 * @param id remote message id
 * @param param_len length of message parameter, <=MAX_REMOTE_MSG_PARAM_LEN
 * @param param message parameter
 */
void app_wws_send_remote_msg_no_activate(app_msg_type_t type, uint16_t id, uint16_t param_len,
                                         const uint8_t *param);
/**
 * @brief send message to remote module
 * @param type @see app_msg_type_t for detial
 * @param id remote message id
 * @param param message parameter
 * @param param_data_len length of message parameter, <=MAX_REMOTE_MSG_PARAM_LEN
 * @param param_data_offset offset of the message parameter,>= 8
 */
void app_wws_send_remote_msg_ext(app_msg_type_t type, uint16_t id, uint8_t *param,
                                 uint16_t param_data_len, uint8_t param_data_offset);

/**
 * @brief wws handler for volume changed event
 */
void app_wws_handle_volume_changed(void);

/**
 * @brief private function to handle wws state from bt
 *
 * @param state the new state
 * @param reason the reason for disconnect state
 * @param peer_addr peer device address.
 */
void app_wws_handle_state_changed(uint8_t state, uint8_t reason, BD_ADDR_T *peer_addr);

/**
 * @brief private function to handle wws role changed event from bt
 *
 * @param new_role new role of current earbud
 * @param peer_addr peer device address.
 * @param reason role changed reason
 */
void app_wws_handle_role_changed(uint8_t new_role, BD_ADDR_T *peer_addr,
                                 tws_role_changed_reason_t reason);

/**
 * @brief private function to handle peer data from bt
 *
 * @param data data received from peer
 * @param len length of the data
 */
void app_wws_handle_recv_data(uint8_t *data, uint16_t len);

/**
 * @brief private function to handle tds data from bt
 *
 * @param remote_addr remote bluetooth address
 * @param data data from tds
 * @param len length of the data
 */
void app_wws_handle_tds_data(BD_ADDR_T *remote_addr, uint8_t *data, uint16_t len);

/**
 * @brief private function to handle bt core init event
 */
void app_wws_handle_bt_inited(void);

/**
 * @brief private function to handle bt power on event
 */
void app_wws_handle_bt_power_on(void);

/**
 * @brief private function to handle bt power off event
 */
void app_wws_handle_bt_power_off(void);

/**
 * @brief private function to handle system state
 *
 * @param state current system state
 */
void app_wws_handle_sys_state(uint32_t state);

/**
 * @brief private function to handle bt connected event
 */
void app_wws_handle_bt_connected(void);

/**
 * @brief private function to handle tws mode changed event
 *
 * @param single_mode true if single mode, false if tws mode
 */
void app_wws_handle_mode_changed(bool_t single_mode);
/**
 * @}
 * addtogroup APP_WWS
 */

/**
 * @}
 * addtogroup APP
 */

#endif   //_APP_WWS_H__
