#ifndef _APP_ECONN_ZT210_H__
#define _APP_ECONN_ZT210_H__

#include "storage_controller.h"

#define ECONN_MSG_ID_FACTORY_RESET                1
#define ECONN_REMOTE_MSG_ID_UPDATE_EQ             2
#define ECONN_REMOTE_MSG_ID_UPDATE_TOUCH_TONE     3
#define ECONN_REMOTE_MSG_ID_DELIVER_CMD           4
#define ECONN_MSG_ID_FORCE_DISCOVERABLE           5
#define ECONN_MSG_ID_CLAER_LAST_BATTERY_LOW       6
#define ECONN_MSG_ID_REPORT_TWS_STATE             7
#define ECONN_MSG_ID_CHECK_SPP_STATE              8

void app_at_test_msg_handler(uint16_t msg_id, void *param);
/**
 * @brief handle box battery low
 *
 */
void econn_handle_box_battery_low(bool_t battery_low);

#endif
