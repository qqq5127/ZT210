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
#include "app_user_flash.h"
#include "usr_cfg.h"


app_flash_type_t	app_flash_data;

static uint8_t app_user_get_earside(void)
{
	return app_flash_data.earside;
}

static void app_user_write_default_data(void)
{
	memset(&app_flash_data,0,sizeof(app_flash_data));
	
	app_user_write_data(app_flash_data);
}

void app_user_read_data(void)
{
	uint32_t len = sizeof(app_flash_data);
	uint32_t result;

	result = storage_read(APP_BASE_ID,APP_CUSTOMIZED_KEY_ID_START,&app_flash_data,&len);
	if((result != RET_OK) || (len != sizeof(app_flash_data)))
		{
			app_user_write_default_data();
			DBGLOG_USER_FLASH_DBG("app_user_read_data error len %d result %d",len,result);
		}
		DBGLOG_USER_FLASH_DBG("app_user_get_earside %d ",app_user_get_earside());
}

void app_user_write_data(app_flash_type_t flash_data)
{
	if(storage_write(APP_BASE_ID,APP_CUSTOMIZED_KEY_ID_START,&flash_data,sizeof(flash_data)) != RET_OK)
		{
			DBGLOG_USER_FLASH_DBG("app_user_write_data error");
			
		}

}

