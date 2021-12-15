#ifndef _APP_ECONN_WQDEMO_H__
#define _APP_ECONN_WQDEMO_H__

#include "storage_controller.h"

#if KEY_DRIVER_SELECTION == KEY_DRIVER_AW8686X
void app_econn_aw8686x_send_msg(uint32_t msg);
#endif

#endif

