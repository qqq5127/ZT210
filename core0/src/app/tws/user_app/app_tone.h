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
 * @addtogroup APP_TONE
 * @{
 * This section introduces the APP TONE module's enum, structure, functions and how to use this module.
 */

#ifndef _APP_TONE_H_
#define _APP_TONE_H_
#include "types.h"
#include "userapp_dbglog.h"

#define DBGLOG_TONE_DBG(fmt, ...) DBGLOG_USER_APP_INFO("[tone] " fmt, ##__VA_ARGS__)
#define DBGLOG_TONE_ERR(fmt, ...) DBGLOG_USER_APP_ERROR("[tone] " fmt, ##__VA_ARGS__)

/**
 * @brief init app tone module
 */
void app_tone_init(void);

/**
  * @brief deinit app tone module
 */
void app_tone_deinit(void);

/**
 * @brief  tone prompt indicate the event
 *
 * @param event_id new event need be indicated
 */
void app_tone_indicate_event(uint16_t event_id);

/**
 * @brief  check if tone is playing
 * @return true if tones is on playing.
 */
bool app_tone_is_playing(void);

/**
 * @brief  get playing event tone
 * @return playing event tone
 */
uint32_t app_tone_get_playing_event(void);

/**
 * @brief  stop a indicated event tone
 *
 * @param event_id  event should be stopped
 * @param stop_playing stop current playing if current playing is event_id
 */
void app_tone_cancel(uint16_t event_id, bool_t stop_playing);

/**
 * @brief  stop all tones that on working or on waiting
 *
 * @param stop_playing stop current playing
 */
void app_tone_cancel_all(bool_t stop_playing);

/**
 * @brief  handle the callback of tone playing finished
 *
 */
void app_tone_action_end(void);

/**
 * @brief process sync tone play request from BT.
 * @param tone_id: id of tone to be played.
 * @param sync_by_sn: 1 - send by sn, 0 - snyc by bt clock.
 * @param start_point: sn if sync by sn, bt clock if snyc by bt clock.
 * @param start_rtc_ms: rtc time to play tone,
 *      only vaild in bt clock trigger case.
 * @return set RET_XXX for detail.
 */
uint32_t app_tone_send_play_msg(uint32_t tone_id, uint32_t sync_by_sn, uint64_t start_point,
                                uint32_t start_rtc_ms);

/**
 * @brief for SLAVE to process sync tone cancel cmd from MASTER.
 * @param tone_evt_id: id of sync tone to cancel.
 * @param stop_playing: whether stop playing current tone if it's already playing.
 * @return set RET_XXX for detail.
 */
uint32_t app_tone_send_cancel_msg(uint32_t tone_evt_id, uint32_t stop_playing);

/**
 * @brief set bt clock for trigger ASRC.
 * @param bts: bt virtual clock to trigger ASRC.
 * @return RET_OK if succeed, other value if failed.
 */
uint8_t app_set_trigger_bt_clk(uint64_t bts);

/**
 * @brief check if specified tone is playing on SEC dev.
 * @param tone_id: id of tone to check.
 * @return 1 if it's SEC dev and a sync tone with specified id is playing.
 *          0 if not.
 */
uint32_t is_sec_playing_sync_tone_id(uint32_t tone_id);

/**
 * @brief  enable tone or not. for test.
 * @param enable: enable or not.
 */
void app_tone_enable(bool_t enable);

/**
 * @}
 * addtogroup APP_TONE
 */

/**
 * @}
 * addtogroup APP
 */

#endif
