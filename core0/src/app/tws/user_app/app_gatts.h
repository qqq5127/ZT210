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

#ifndef _APP_GATTS_H__
#define _APP_GATTS_H__

/**
 * @addtogroup APP
 * @{
 */

/**
 * @addtogroup APP_GATTS
 * @{
 * This section introduces APP GATT Server module's functions and how to use this module.
 */

#include "types.h"
#include "app_bt.h"
#include "userapp_dbglog.h"

#define DBGLOG_GATTS_DBG(fmt, ...) DBGLOG_USER_APP_INFO("[gatts] " fmt, ##__VA_ARGS__)
#define DBGLOG_GATTS_ERR(fmt, ...) DBGLOG_USER_APP_ERROR("[gatts] " fmt, ##__VA_ARGS__)

#define GATT_SERVER_PROP_BROADCAST         0x01
#define GATT_SERVER_PROP_READ              0x02
#define GATT_SERVER_PROP_WRITE_WITHOUT_RSP 0x04
#define GATT_SERVER_PROP_WRITE             0x08
#define GATT_SERVER_PROP_NOTIFY            0x10

typedef struct gatts_character gatts_character_t;
typedef struct gatts_service gatts_service_t;

/**
 * @brief callback to handle connection stated changed event
 *
 * @param addr bluetooth address of the event
 * @param is_ble is ble connection or not
 * @param connected true if connected, false if disconnected
 */
typedef void (*gatts_connection_callback_t)(BD_ADDR_T *addr, bool_t is_ble, bool_t connected);

/**
 * @brief callback to handle read request from client
 *
 * @param addr bluetooth address of the client
 * @param character character of the read request
 * @param data buffer to store read response
 * @param max_len max length of read response
 *
 * @return the length of read response
 */
typedef uint16_t (*gatts_read_callback_t)(BD_ADDR_T *addr, gatts_character_t *character,
                                          uint8_t *data, uint16_t max_len);

/**
 * @brief callback to handle write request from client
 *
 * @param addr bluetooth address of the client
 * @param character character of the write request
 * @param data data of the write request, malloced, need to be freed!!
 * @param len lenght of the data
 */
typedef void (*gatts_write_callback_t)(BD_ADDR_T *addr, gatts_character_t *character, uint8_t *data,
                                       uint16_t len);

/**
 * @brief callback to handle notification enabled/disabled event
 *
 * @param addr bluetooth address of the client
 * @param character character of the event
 * @param enabled notification enabled or not
 */
typedef void (*gatts_notify_enable_callback_t)(BD_ADDR_T *addr, gatts_character_t *character,
                                               bool_t enabled);

/** @defgroup app_gatts_struct Struct
 * @{
 */
struct gatts_character {
    gatts_character_t *next;
    uint8_t uuid128[16];
    uint16_t uuid;
    uint16_t handle;
    uint16_t ccd_handle;
    uint16_t ccd_value;
    uint8_t props;
    gatts_read_callback_t read_callback;
    gatts_write_callback_t write_callback;
    gatts_notify_enable_callback_t notify_enable_callback;
};

struct gatts_service {
    gatts_service_t *next;
    uint8_t uuid128[16];
    uint16_t uuid;
    uint16_t handle;
    gatts_character_t *characters;
};
/**
 * @}
 */

/**
 * @brief init the app_gatts module
 */
void app_gatts_init(void);

/**
 * @brief deinit the app_gatts module
 */
void app_gatts_deinit(void);

/**
 * @brief send a notification to remote client
 *
 * @param addr bluetooth address of remote client
 * @param character the character of the notification
 * @param data the data of the notification
 * @param len lenght of the notification data
 *
 * @return true if send succeed, false if send error
 */
bool_t app_gatts_send_notify(BD_ADDR_T *addr, gatts_character_t *character, const uint8_t *data,
                             uint16_t len);

/**
 * @brief send a indicate to remote client
 *
 * @param addr bluetooth address of remote client
 * @param character the character of the indicate
 * @param data the data of the indicate
 * @param len lenght of the indicate data
 *
 * @return true if send succeed, false if send error
 */
bool_t app_gatts_send_indicate(BD_ADDR_T *addr, gatts_character_t *character, const uint8_t *data,
                               uint16_t len);

/**
 * @brief register callback to handle connected/disconnected events
 *
 * @param callback the callback to handle connected/disconnected events
 */
void app_gatts_register_connection_callback(gatts_connection_callback_t callback);

/**
 * @brief connect to device.
 *
 * @param addr bluetooth address
 * @param addr_type @see BLE_ADDR_TYPE_XXX
 * @param link_type @see GATT_LINK_TYPE_XXX
 *
 * @return 0 for succeed, else for error code
 */
int app_gatts_connect(const BD_ADDR_T *addr, uint8_t addr_type, uint8_t link_type);

/**
 * @brief disconnect gatt service
 *
 * @param addr bluetooth address
 *
 * @return 0 for succeed, else for error code
 */
int app_gatts_disconnect(const BD_ADDR_T *addr);

/**
 * @brief register a service, duplicate service of different module is allowed.
 * @param uuid 16 bit uuid of the service
 *
 * @return pointer to the service
 */
gatts_service_t *app_gatts_register_service(uint16_t uuid);

/**
 * @brief register a character, duplicate character of different module will cause error.
 *
 * @param service service the service it belong to
 * @param uuid 16 bit uuid of the character
 * @param props
 * @param read_callback callback to handle read request, should be null if read not supported
 * @param write_callback callback to handle write request, should be null if write not supported
 * @param notify_enable_callback callback to handle notify enable request, should be null if notify not supported
 *
 * @return pointer to the character
 */
gatts_character_t *app_gatts_register_character(
    gatts_service_t *service, uint16_t uuid, uint8_t props, gatts_read_callback_t read_callback,
    gatts_write_callback_t write_callback, gatts_notify_enable_callback_t notify_enable_callback);

/**
 * @brief register a service, duplicate service of different module is allowed.
 * @param uuid128 128 bit uuid of the service
 *
 * @return pointer to the service
 */
gatts_service_t *app_gatts_register_service_ext(const uint8_t uuid128[16]);

/**
 * @brief register a character, duplicate character of different module will cause error.
 *
 * @param service service the service it belong to
 * @param uuid128 128 bit uuid of the character
 * @param props
 * @param read_callback callback to handle read request, should be null if read not supported
 * @param write_callback callback to handle write request, should be null if write not supported
 * @param notify_enable_callback callback to handle notify enable request, should be null if notify not supported
 *
 * @return pointer to the character
 */
gatts_character_t *
app_gatts_register_character_ext(gatts_service_t *service, const uint8_t uuid128[16], uint8_t props,
                                 gatts_read_callback_t read_callback,
                                 gatts_write_callback_t write_callback,
                                 gatts_notify_enable_callback_t notify_enable_callback);

/**
 * @brief private function to handle bt gatt server events
 * @param evt the event
 * @param param param of the event
 */
void app_gatts_handle_bt_evt(uint16_t evt, void *param);

/**
 * @brief private function to handle bt power on event
 */
void app_gatts_handle_bt_power_on(void);

/**
 * @}
 * addtogroup APP_GATTS
 */

/**
 * @}
 * addtogroup APP
 */

#endif   //_APP_GATTS_H__
