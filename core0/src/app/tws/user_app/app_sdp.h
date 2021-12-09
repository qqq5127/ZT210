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
 * @addtogroup APP_SDP
 * @{
 * This section introduces the APP SDP module's enum, structure, functions and how to use this module.
 */

#ifndef _APP_SDP_H_
#define _APP_SDP_H_
#include "types.h"
#include "userapp_dbglog.h"
#include "app_bt.h"

#define DBGLOG_SDP_DBG(fmt, ...) DBGLOG_USER_APP_INFO("[sdp] " fmt, ##__VA_ARGS__)
#define DBGLOG_SDP_ERR(fmt, ...) DBGLOG_USER_APP_ERROR("[sdp] " fmt, ##__VA_ARGS__)

/**
 * @brief function to handle remote rfcomm channel found event
 *
 * @param uuid128 128 bit uuid
 * @param channel rfcomm channel
 */
typedef void (*app_sdp_rfcomm_found_handler_t)(const uint8_t uuid128[16], uint8_t channel);

/**
 * @brief add a uuid to sdp, only available after bt inited
 *
 * @param uuid 128 bit uuid
 *
 * @return profile index to the uuid
 */
uint8_t app_sdp_add_uuid(const uint8_t uuid[16]);

/**
 * @brief get index of a uuid, only available after bt inited
 *
 * @param uuid 128 bit uuid
 *
 * @return profile index to the uuid
 */
uint8_t app_sdp_get_uuid_index(const uint8_t uuid[16]);

/**
 * @brief register a sdp record, only available after bt inited
 *
 * @param profile_index_num length of profile index list
 * @param profile_index_list list of profile index
 * @param attribute_num count of attribute list
 * @param attribute_list list of attributes
 *
 * @return 0 for success else for the error code
 */
int app_sdp_register_record(uint8_t profile_index_num, uint8_t *profile_index_list,
                            uint8_t attribute_num, const bt_service_attribute_t *attribute_list);

/**
 * @brief register a function to handler remote rfcomm channel found event
 *
 * @param handler function to handle the rfcomm channel found event
 *
 * @return true if succeed, false if not
 */
bool_t app_sdp_client_register_rfcomm_found_handler(app_sdp_rfcomm_found_handler_t handler);

/**
 * @brief private function to handle bt rpc event
 *
 * @param evt the rpc event
 * @param param parameter of the rpc event
 */
void app_sdp_handle_bt_evt(uint16_t evt, void *param);
/**
 * @}
 * addtogroup APP_SDP
 */

/**
 * @}
 * addtogroup APP
 */

#endif   //_APP_SDP_H_
