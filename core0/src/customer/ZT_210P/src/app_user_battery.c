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
#include "app_user_battery.h"
#include "usr_cfg.h"

void app_user_battery_init(void)
{
	//iot_gpio_open(BATTERY_CTRL_GPIO, IOT_GPIO_DIRECTION_OUTPUT);
	//iot_gpio_set_pull_mode(BATTERY_CTRL_GPIO, IOT_GPIO_PULL_DOWN);
	//iot_gpio_write(BATTERY_CTRL_GPIO, 0);
}

void app_user_battery_off(void)
{
	//iot_gpio_write(BATTERY_CTRL_GPIO, 1);
}

void app_user_battery_ntc_check(int8_t value)
{
	uint16_t current_ma = 0;
	static bool_t temprature_abnamal_protect = false;
	static uint16_t ntc_limit_current = 0xff;

	temprature_abnamal_protect = false;
	if(value < 0)
	{
		temprature_abnamal_protect = true;
	}
	else if(value >= 45)
	{
		temprature_abnamal_protect = true;
	}

	if(temprature_abnamal_protect)
	{
		current_ma = 0;
	}
	else
	{
		current_ma = 40;
	}

	if(ntc_limit_current != current_ma)
	{
		DBGLOG_USER_BATTERY_DBG("change charge current %d",current_ma);
		ntc_limit_current = current_ma;
		battery_charger_set_cur_limit(current_ma);
	}
}

