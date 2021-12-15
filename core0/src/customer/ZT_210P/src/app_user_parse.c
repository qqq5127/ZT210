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
#include "app_pm.h"
#include "aw8686x/aw8686x.h"


static uint8_t CRC16withBaseDate(const uint8_t *buffer, uint32_t size, uint16_t crc)
{
    if (NULL != buffer && size > 0)
    {
        while (size--)
        {
            crc = (crc >> 8) | (crc << 8);
            crc ^= *buffer++;
            crc ^= ((unsigned char) crc) >> 4;
            crc ^= crc << 12;
            crc ^= (crc & 0xFF) << 5;
        }
    }

    return crc;
}

static uint8_t app_user_get_earside(void)
{
#if 0	// TODO :parse earside
	if(app_get_earside() == 0)
	{// left
		return EARPHONE_LEFT;
	}
	else if(app_get_earside() == 1)
	{//right
		return EARPHONE_RIGHT;
	}
#endif
	//unknow
	return EARPHONE_UNKNOWN;
}


void app_user_parse_data(uint8_t *data,uint8_t len,app_parse_type_e type)
{
	uint8_t crc;
	uint8_t response_data[20],response_len;

	
	UNUSED(type);

	memset(response_data,0,sizeof(response_data));
	response_len = 0;

	response_data[0] = app_user_get_earside();

	if(len < data[2] + 4)
	{
		return;
	}

	crc = CRC16withBaseDate(data,data[2] + 3,0xffff);
	
	DBGLOG_USER_PARSE_DBG("user spp parse crc %x  %x\n",crc,data[data[2]+3]);
	
	if(crc != (data[data[2]+3]))
	{
		return;	//error crc
	}

	DBGLOG_USER_PARSE_DBG("user spp parse case %x\n",data[1]);
	response_data[1] = data[1];
	response_len = 0;	

	switch(data[1])
	{
		case APP_PARSE_ENTRY_PAIR:
			if(app_wws_is_master())
			{
				app_charger_evt_send(CHARGER_EVT_AG_PAIR);
			}
		break;
			
		case APP_PARSE_GET_PAIR_STATUS:
			response_len = 1;
			if(app_bt_get_a2dp_state() < A2DP_STATE_CONNECTED)
			{
				response_data[3] = 0;
			}
			else
			{
				response_data[3] = 1;
			}
			break;

		case APP_PARSE_ENTRY_OTA:
			app_charger_evt_send(CHARGER_EVT_ENTER_OTA);
			break;

		case APP_PARSE_FACTORY_RESET:
			app_charger_evt_send(CHARGER_EVT_CLEAR_PDL);
			app_charger_evt_send(CHARGER_EVT_REBOOT);
			break;
			
		case APP_PARSE_GET_BATTERY:
			response_len = app_bat_get_level();
			response_data[3] = data[3];
			break;

		case APP_PARSE_SOFT_RESET:
			app_evt_send(EVTUSR_FACTORY_RESET);
			break;
		
		case APP_PARSE_POWER_OFF:			
			app_evt_send(EVTUSR_POWER_OFF);
			break;

		case APP_PARSE_ENTRY_FREEMAN:
			app_charger_evt_send(CHARGER_EVT_ENTER_MONO_MODE);
			break;

//		case APP_PARSE_HARDWARE_RESET:
//			app_charger_evt_send(CHARGER_EVT_CLEAR_PDL);
//			break;

		case APP_PARSE_OPEN_BOX:
	    if (app_pm_is_shuting_down()) {
				// reset when shuting
	        app_pm_reboot(PM_REBOOT_REASON_USER);
	    }			
			app_charger_evt_send(CHARGER_EVT_BOX_OPEN);
			break;

		case APP_PARSE_CLOSE_BOX:
			app_charger_evt_send(CHARGER_EVT_BOX_CLOSE);
			break;

		case APP_PARSE_ENTRY_DUT:
			app_charger_evt_send(CHARGER_EVT_ENTER_DUT);
			break;

		case APP_PARSE_SET_EARSIDE:
			DBGLOG_USER_PARSE_DBG("set earside %d\n",data[3]);				
			app_flash_data.earside = data[3];
			app_user_write_data(app_flash_data);
			response_len = 1;
			response_data[3] = data[3];
			break;

		case APP_PARSE_ENTRY_FTM:
#if KEY_DRIVER_SELECTION == KEY_DRIVER_AW8686X
			aw8686x_deinit(false);
#endif
			DBGLOG_USER_PARSE_DBG("APP_PARSE_ENTRY_FTM \n"); 			
		break;

		case APP_PARSE_FORCE_TOUCH_READ_ID:
			{
				char* response_char = NULL;
				
				response_len = 8;
				//response_char = aw_8686x_check_chip_id();
				DBGLOG_USER_PARSE_DBG("read chip id %d\n",response_char);				
				for(uint8_t i=0;i<8;i++)
				{
					response_data[3+i] = response_char[i];
				}
			}
		break;

		case APP_PARSE_FORCE_TOUCH_CURVE_DATA:
			{								
				response_len = 10;
				//aw8686x_get_current_data(&response_data[3]);
				DBGLOG_USER_PARSE_DBG("read curve data %x %x %x\n",response_data[3],response_data[4],response_data[5]); 			
			}

		break;

		default:

		break;
	}
	response_data[2] = response_len;
	response_data[response_len +3] = CRC16withBaseDate(response_data,response_len +3,0xffff);;

	if(type == APP_PARSE_TYPE_SPP)
	{
		app_user_spp_send_data(response_data,response_len+4);
	}
	else if(type == APP_PARSE_TYPE_UART)
	{
		// TODO : response uart
	}
	else if(type == APP_PARSE_TYPE_BLE)
	{
		// TODO : response ble
	}
	
}
