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
 * @addtogroup APP_USR_CFG
 * @{
 * This section introduces the APP USR CFG module's enum, structure, functions and how to use this module.
 */

#ifndef _USR_CFG_H__
#define _USR_CFG_H__
#include "types.h"
#include "userapp_dbglog.h"
#include "app_bt.h"
#include "storage_controller.h"

#define DBGLOG_CFG_DBG(fmt, ...) DBGLOG_USER_APP_INFO("[cfg] " fmt, ##__VA_ARGS__)
#define DBGLOG_CFG_ERR(fmt, ...) DBGLOG_USER_APP_ERROR("[cfg] " fmt, ##__VA_ARGS__)

#ifndef USR_CFG_MAX_PAIR_LIST
#define USR_CFG_MAX_PAIR_LIST 8
#endif

/** @defgroup usr_cfg_enum Enum
 * @{
 */

typedef enum {
    APP_USR_CFG_KEY_ID = APP_BASE_ID + 0x80,
    APP_CUS_EVT_KEY_ID,
    APP_PDL_KEY_ID,
    APP_CUSTOMIZED_KEY_ID_START = APP_BASE_ID + 0xD0,
    APP_CUSTOMIZED_KEY_ID_END = APP_END_ID
} app_cfg_id_e;

/**
 * @}
 */

/**
 * @brief init the user config module
 */
void usr_cfg_init(void);

/**
 * @brief deinit the user config module
 */
void usr_cfg_deinit(void);

/**
 * @brief clear all user configurations
 */
void usr_cfg_reset(void);

/**
 * @brief clear all paired list
 */
void usr_cfg_reset_pdl(void);

/**
 * @brief get the saved volume of music
 *
 * @return the saved volume of music
 */
uint8_t usr_cfg_get_music_vol(void);

/**
 * @brief get the save volume of call
 *
 * @return the saved volume of call
 */
uint8_t usr_cfg_get_call_vol(void);

/**
 * @brief save current music volume
 *
 * @param vol current music volume
 */
void usr_cfg_set_music_vol(uint8_t vol);

/**
 * @brief save current call volume
 *
 * @param vol current call volume
 */
void usr_cfg_set_call_vol(uint8_t vol);

/**
 * @brief change local name
 *
 * @param name the local name
 */
void usr_cfg_set_local_name(const char *name);

/**
 * @brief get current local name
 *
 * @param name the buffer to save the name
 * @param max_name_len length of the name buffer
 */
void usr_cfg_get_local_name(char *name, int max_name_len);

/**
 * @brief get the current language id
 *
 * @return current language id
 */
uint8_t usr_cfg_get_cur_language(void);

/**
 * @brief change new language
 *
 * @param language_id - the new language
 */
void usr_cfg_set_cur_language(uint8_t language_id);

/**
 * @brief get current listen mode
 *
 * @return current listen mode
 */
uint8_t usr_cfg_get_listen_mode(void);

/**
 * @brief get saved anc level
 *
 * @return current anc level
 */
uint8_t usr_cfg_get_anc_level(void);

/**
 * @brief save new anc level
 *
 * @param level new anc level
 */
void usr_cfg_set_anc_level(uint8_t level);

/**
 * @brief get saved transparency level
 *
 * @return current transparency level
 */
uint8_t usr_cfg_get_transparency_level(void);

/**
 * @brief save new transparency level
 *
 * @param level new transparency level
 */
void usr_cfg_set_transparency_level(uint8_t level);

/**
 * @brief set new listen mode
 *
 * @param mode new listen mode
 */
void usr_cfg_set_listen_mode(uint8_t mode);

/**
 * @brief get current listen mode configuration
 *
 * @return current listen mode configuration
 */
uint8_t usr_cfg_get_listen_mode_cfg(void);

/**
 * @brief set new listen mode configuration
 *
 * @param cfg new listen mode configuration
 */
void usr_cfg_set_listen_mode_cfg(uint8_t cfg);

/**
 * @brief get current adv cnt
 *
 * @return current adv cnt
 */
uint8_t usr_cfg_get_adv_cnt(void);

/**
 * @brief set current adv cnt
 *
 * @param adv_cnt adv cnt
 */
void usr_cfg_set_adv_cnt(uint8_t adv_cnt);

/**
 * @brief get inear enabled
 *
 * @return true if enabled, false if not.
 */
bool_t usr_cfg_get_inear_enabled(void);

/**
 * @brief set battery low prompt interval
 *
 * @param interval_s battery low prompt interval in seconds
 */
void usr_cfg_set_battery_low_prompt_interval(uint16_t interval_s);

/**
 * @brief get battery low prompt interval
 *
 * @return battery low prompt interval in seconds
 */
uint16_t usr_cfg_get_battery_low_prompt_interval(void);

/**
 * @brief set inear enabled
 *
 * @param enabled true if enabled, false if not.
 */
void usr_cfg_set_inear_enabled(bool_t enabled);

/**
 * @brief add a remote device to the pair list,
 *        the device will be moved to first
 *
 * @param addr the address of the device
 */
void usr_cfg_pdl_add(BD_ADDR_T *addr);

/**
 * @brief remove a remote device from the pair list
 *
 * @param addr the address of the device
 */
void usr_cfg_pdl_remove(BD_ADDR_T *addr);

/**
 * @brief check if a device is in the device list or not
 *
 * @param addr the address of the device
 *
 * @return true if exists, false if not
 */
bool_t usr_cfg_pdl_exists(const BD_ADDR_T *addr);

/**
 * @brief get first paired remote device
 *
 * @param addr the buffer to save the device address
 */
void usr_cfg_pdl_get_first(BD_ADDR_T *addr);

/**
 * @brief get next paired remote device
 *
 * @param addr in: the current remote device address, all 0 means to get the first
 *             out when success: the buffer to save the returned device address
 *             out when fail: all bytes of the parameter will be set to 0
 */
void usr_cfg_pdl_get_next(BD_ADDR_T *addr);

/**
 * @brief check if pired list is empty
 *
 * @return true if pdl is empty, false if not
 */
bool_t usr_cfg_pdl_is_empty(void);

/**
 * @brief save current a2dp codec
 *
 * @param addr bluetooth address
 * @param codec current a2dp codec
 */
void usr_cfg_set_a2dp_codec(const BD_ADDR_T *addr, bt_a2dp_codec_t codec);

/**
 * @brief get a2dp codec of target device
 *
 * @param addr bluetooth address
 * @return a2dp codec of target device
 */
bt_a2dp_codec_t usr_cfg_get_a2dp_codec(const BD_ADDR_T *addr);

/**
 * @brief get current phone type
 *
 * @return current phone type
 */
bt_phone_type_t usr_cfg_get_phone_type(void);

/**
 * @brief save current phone type
 *
 * @param type current phone type
 */
void usr_cfg_set_phone_type(bt_phone_type_t type);
/**
 * @}
 * addtogroup APP_USR_CFG
 */

/**
 * @}
 * addtogroup APP
 */

#endif
