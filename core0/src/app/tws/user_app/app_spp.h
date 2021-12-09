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
 * @addtogroup APP_SPP
 * @{
 * This section introduces the APP SPP module's enum, structure, functions and how to use this module.
 */

#ifndef _APP_SPP_H_
#define _APP_SPP_H_
#include "types.h"
#include "userapp_dbglog.h"
#include "app_bt.h"

#define DBGLOG_SPP_DBG(fmt, ...) DBGLOG_USER_APP_INFO("[spp] " fmt, ##__VA_ARGS__)
#define DBGLOG_SPP_ERR(fmt, ...) DBGLOG_USER_APP_ERROR("[spp] " fmt, ##__VA_ARGS__)

/**
 * @brief callback to handle connection stated changed event
 *
 * @param addr bluetooth address of the event
 * @param uuid service uuid
 * @param connected true if connected, false if disconnected
 */
typedef void (*spp_connection_callback_t)(BD_ADDR_T *addr, uint16_t uuid, bool_t connected);

/**
 * @brief callback to handle data from spp client
 *
 * @param addr bluetooth address of the client
 * @param uuid service uuid
 * @param data malloced, need to be freed!
 * @param len lenght of the data
 */
typedef void (*spp_data_callback_t)(BD_ADDR_T *addr, uint16_t uuid, uint8_t *data, uint16_t len);

/**
 * @brief callback to handle connection stated changed event
 *
 * @param addr bluetooth address of the event
 * @param uuid128 128 bit service uuid
 * @param connected true if connected, false if disconnected
 */
typedef void (*spp_connection_callback_ext_t)(BD_ADDR_T *addr, const uint8_t uuid128[16],
                                              bool_t connected);

/**
 * @brief callback to handle data from spp client
 *
 * @param addr bluetooth address of the client
 * @param uuid128 128 bit service uuid
 * @param data malloced, need to be freed!
 * @param len lenght of the data
 */
typedef void (*spp_data_callback_ext_t)(BD_ADDR_T *addr, const uint8_t uuid128[16], uint8_t *data,
                                        uint16_t len);

/**
 * @brief callback to handle connection stated changed event
 *
 * @param addr bluetooth address of the event
 * @param channel rfcomm channel
 * @param connected true if connected, false if disconnected
 */
typedef void (*spp_client_connection_callback_t)(BD_ADDR_T *addr, uint8_t channel,
                                                 bool_t connected);

/**
 * @brief callback to handle data from spp client
 *
 * @param addr bluetooth address of the client
 * @param channel rfcomm channel
 * @param data malloced, need to be freed!
 * @param len lenght of the data
 */
typedef void (*spp_client_data_callback_t)(BD_ADDR_T *addr, uint8_t channel, uint8_t *data,
                                           uint16_t len);

/**
 * @brief register a spp service specified by the uuid
 *
 * @param uuid service uuid, 0x1101 for normal spp service
 * @param conn_cbk callback to handle connection state
 * @param data_cbk callback to handle data from spp client
 */
void app_spp_register_service(uint16_t uuid, spp_connection_callback_t conn_cbk,
                              spp_data_callback_t data_cbk);

/**
 * @brief send data to spp client
 *
 * @param addr bluetooth address of the client
 * @param uuid service uuid
 * @param data data to send
 * @param len length of the data
 *
 * @return 0 for succeed, else for the error reason
 */
int app_spp_send_data(const BD_ADDR_T *addr, uint16_t uuid, const uint8_t *data, uint16_t len);

/**
 * @brief register a spp service specified by the uuid
 *
 * @param uuid128 128 bit service uuid
 * @param conn_cbk_ext callback to handle connection state
 * @param data_cbk_ext callback to handle data from spp client
 */
void app_spp_register_service_ext(const uint8_t uuid128[16],
                                  spp_connection_callback_ext_t conn_cbk_ext,
                                  spp_data_callback_ext_t data_cbk_ext);

/**
 * @brief send data to spp client
 *
 * @param addr bluetooth address of the client
 * @param uuid128 128 bit service uuid
 * @param data data to send
 * @param len length of the data
 *
 * @return 0 for succeed, else for the error reason
 */
int app_spp_send_data_ext(const BD_ADDR_T *addr, const uint8_t uuid128[16], const uint8_t *data,
                          uint16_t len);

/**
 * @brief send disconnect to spp client
 *
 * @param addr bluetooth address of the client
 * @param uuid service uuid
 *
 * @return 0 for succeed, else for the error reason
 */
int app_spp_disconnect(const BD_ADDR_T *addr, uint16_t uuid);

/**
 * @brief send disconnect to spp client
 *
 * @param addr bluetooth address of the client
 * @param uuid128 128 bit service uuid
 *
 * @return 0 for succeed, else for the error reason
 */
int app_spp_disconnect_ext(const BD_ADDR_T *addr, const uint8_t uuid128[16]);

/**
 * @brief connect to remote spp server
 *
 * @param addr bluetooth address of the server
 * @param channel rfcomm channel
 * @param conn_cbk callback to handle connection state
 * @param data_cbk callback to handle remote data
 *
 * @return 0 for succeed, else for the error reason
 */
int app_spp_client_connect(const BD_ADDR_T *addr, uint8_t channel,
                           spp_client_connection_callback_t conn_cbk,
                           spp_client_data_callback_t data_cbk);

/**
 * @brief disconnect to remote spp server
 *
 * @param addr bluetooth address of the server
 * @param channel rfcomm channel
 *
 * @return 0 for succeed, else for the error reason
 */
int app_spp_client_disconnect(const BD_ADDR_T *addr, uint8_t channel);

/**
 * @brief send data to remote spp server
 *
 * @param addr bluetooth address of the server
 * @param channel rfcomm channel
 * @param data data to send
 * @param len length of the data
 *
 * @return 0 for succeed, else for the error reason
 */
int app_spp_client_send_data(const BD_ADDR_T *addr, uint8_t channel, const uint8_t *data,
                             uint16_t len);

/**
 * @brief private function to handle bt spp events
 * @param evt the event
 * @param param param of the event
 */
void app_spp_handle_bt_evt(uint16_t evt, void *param);

/**
 * @brief private function to handle bt power on event
 */
void app_spp_handle_bt_power_on(void);

/**
 * @}
 * addtogroup APP_SPP
 */

/**
 * @}
 * addtogroup APP
 */

#endif   //_APP_SPP_H_
