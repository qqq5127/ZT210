#include "types.h"
#include "string.h"
#include "os_mem.h"
#include "modules.h"
#include "cli.h"
#include "ro_cfg.h"
#include "app_evt.h"
#include "app_main.h"
#include "app_bt.h"
#include "app_cli.h"
#include "app_spp.h"
#include "app_gatts.h"
#include "app_adv.h"
#include "app_wws.h"
#include "app_econn.h"
#include "app_charger.h"
#include "app_bat.h"
#include "battery.h"
#include "generic_transmission_api.h"
#include "generic_transmission_config.h"
#include "key_sensor.h"
#include "led_manager.h"
#include "oem.h"
#include "app_tone.h"
#include "app_btn.h"
#include "app_user_spp.h"
#include "charger_box.h"
#include "app_user_parse.h"
#include "app_user_flash.h"

static uint8_t user_spp_connected = false;
static BD_ADDR_T remote_addr = {0};
const uint8_t user_uuid_table[16] = {0x12,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11};

static void spp_data_user_callback(BD_ADDR_T *addr, const uint8_t uuid128[16], uint8_t *data, uint16_t len)
{
    UNUSED(uuid128);
    UNUSED(addr);

    if (app_wws_is_master()) {
        DBGLOG_USER_SPP_DBG("user spp data len:%d\n", len);
        app_user_parse_data(data, len, APP_PARSE_TYPE_SPP);
    } else {
        DBGLOG_USER_SPP_DBG("user spp data len:%d, ignore for slave\n", len);
    }
    os_mem_free(data);
}

void app_user_spp_send_data(uint8_t *data,uint16_t len)
{
    if (user_spp_connected) {
        DBGLOG_USER_SPP_DBG("app spp send_spp len:%d\n", len);
        app_spp_send_data_ext(&remote_addr, user_uuid_table, data, len);
    }
}

static void spp_connection_user_callback(BD_ADDR_T *addr, const uint8_t uuid128[16], bool_t connected)
{
    UNUSED(uuid128);
    UNUSED(addr);

    if (connected) {
        DBGLOG_USER_SPP_DBG("user spp connected\n");
        user_spp_connected = true;		
        remote_addr = *addr;
				//app_user_read_data();				
    } else {
        DBGLOG_USER_SPP_DBG("user spp disconnected\n");
        user_spp_connected = false;
    }
}


void app_user_spp_init(void)
{
	app_spp_register_service_ext(user_uuid_table, spp_connection_user_callback, spp_data_user_callback);
}

