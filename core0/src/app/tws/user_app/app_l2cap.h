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
 * @addtogroup APP_L2CAP
 * @{
 * This section introduces the APP L2CAP module's enum, structure, functions and how to use this module.
 */

#ifndef _APP_L2CAP_H_
#define _APP_L2CAP_H_
#include "types.h"
#include "userapp_dbglog.h"
#include "app_bt.h"

#define DBGLOG_L2CAP_DBG(fmt, ...) DBGLOG_USER_APP_INFO("[l2cap] " fmt, ##__VA_ARGS__)
#define DBGLOG_L2CAP_ERR(fmt, ...) DBGLOG_USER_APP_ERROR("[l2cap] " fmt, ##__VA_ARGS__)

/**
 * @brief callback to handle connection stated changed event
 *
 * @param addr bluetooth address of the event
 * @param psm l2cap psm
 * @param connected true if connected, false if disconnected
 */
typedef void (*l2cap_connection_callback_t)(BD_ADDR_T *addr, uint16_t psm, bool_t connected);

/**
 * @brief callback to handle data from spp client
 *
 * @param addr bluetooth address of the client
 * @param psm l2cap uuid
 * @param data malloced, need to be freed!
 * @param len lenght of the data
 */
typedef void (*l2cap_data_callback_t)(BD_ADDR_T *addr, uint16_t psm, uint8_t *data, uint16_t len);

/**
 * @brief register a spp service specified by the psm
 *
 * @param psm l2cap psm
 * @param conn_cbk callback to handle connection state
 * @param data_cbk callback to handle data from spp client
 */
void app_l2cap_register_service(uint16_t psm, l2cap_connection_callback_t conn_cbk,
                                l2cap_data_callback_t data_cbk);

/**
 * @brief send data to spp client
 *
 * @param addr bluetooth address of the client
 * @param psm l2cap psm
 * @param data data to send
 * @param len length of the data
 */
int app_l2cap_send_data(BD_ADDR_T *addr, uint16_t psm, const uint8_t *data, uint16_t len);

/**
 * @brief private function to handle bt spp events
 * @param evt the event
 * @param param param of the event
 */
void app_l2cap_handle_bt_evt(uint16_t evt, void *param);

/**
 * @brief private function to handle bt power on event
 */
void app_l2cap_handle_bt_power_on(void);

/**
 * @}
 * addtogroup APP_L2CAP
 */

/**
 * @}
 * addtogroup APP
 */

#endif   //_APP_L2CAP_H_
