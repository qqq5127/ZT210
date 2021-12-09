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
#include "key_base.h"
	
#if KEY_DRIVER_SELECTION == KEY_DRIVER_NDT_PT135

#include "pt135.h"
#include "os_timer.h"
#include "lib_dbglog.h"
#include "iot_adc.h"
#include "iot_gpio.h"
#include "os_utils.h"
#include "string.h"
#include "iot_adc.h"
#include "iot_uart.h"
#include "vendor_msg.h"
#include "iot_resource.h"
#include "cli.h"

#define DBGLOG_PT135_INFO(fmt, arg...)  DBGLOG_STREAM_INFO(IOT_SENSOR_HUB_MANAGER_MID, fmt, ##arg)
#define DBGLOG_PT135_ERROR(fmt, arg...) DBGLOG_STREAM_ERROR(IOT_SENSOR_HUB_MANAGER_MID, fmt, ##arg)

#define DBG_PT135_GPIO					0

#define APP_CLI_MSGID_GET_FORCETOUCH_VOLT 19

#define PT135_ADC_CHN 					 IOT_ADC_EXT_DIFF_CH01
#define PT135_FORCE_SENSITIVITY			 1			
#define PT135_PRESS_FIFO_LEN 			 8

#define PT135_TIMER_IDLE_INTERVAL      	 10  	// ms
#define PT135_TIMER_MULTI_TAP_INTERVAL   50  	// ms

#define PT135_LONG_PRESS_TIME         	 1500   //ms
#define PT135_LONG_PRESS_RELEASE_TIME 	 100    //ms
#define PT135_CLICK_TIMEOUT           	 1000   //ms

typedef enum {
	FORCESENSOR_STATE_IDLE = 0,
	FORCESENSOR_STATE_DEBOUNCE,	
	FORCESENSOR_STATE_PRESS_START,
	FORCESENSOR_STATE_MAX,
}FORCESENSOR_STATE;

typedef struct {
    uint16_t volt;
} __attribute__((packed)) app_cli_get_pt135_volt_rsp_t;

typedef struct {
	uint32_t time;
    bool_t pressed;
}PT135_PRESS_FIFO;

typedef struct {
	FORCESENSOR_STATE state;
	bool_t inited;
	bool_t press_fifo_lock;
	bool_t press_check_inprocess;
	bool_t press_long_inprocess;
	bool_t last_pressed;
	uint16_t base_cnt;
	float base_volt;
	uint16_t press_fifo_pos;
	PT135_PRESS_FIFO press_fifo[PT135_PRESS_FIFO_LEN];
	timer_id_t check_timer_id;
	timer_id_t smpl_timer_id;
	key_callback_t key_callback;
}PT135_DEV;

PT135_DEV pt135_dev;

//************************************************************************************************************
// Function Define
// 
//************************************************************************************************************
static float pt135_smpl_volt(void)
{
	int32_t adc_val = 0;
	float smpl_voltage = 0.0f;
	
	//gain:3'h0=0dB, 3'h1=6dB, 3'h2=12dB,3'h3=18dB,3'h4=24dB,3'h5=30dB,3'h6=36dB
	adc_val = iot_adc_poll_data(PT135_ADC_CHN, 0, 1);
	smpl_voltage = iot_adc_2_mv(1, adc_val);
	//DBGLOG_PT135_INFO("adc_val=%d, pt135_volt=%d.%dmV\n", adc_val, FLOAT(smpl_voltage));
	
	return smpl_voltage;
}

static bool_t pt135_force_is_pressed(void)
{
	bool_t pressed = false;
	
#if DBG_PT135_GPIO
	if(!iot_gpio_read(IOT_GPIO_64)){
		pressed = true;
	}else{
		pressed = false;
	}
#else
	float smpl_voltage = pt135_smpl_volt();
	if(ABS(smpl_voltage - pt135_dev.base_volt) > PT135_FORCE_SENSITIVITY){
		pressed = true;
	}else{
		pressed = false;
	}
#endif
	return pressed;
}

static void cli_get_pt135_volt(uint8_t *buffer, uint32_t length)
{
    app_cli_get_pt135_volt_rsp_t rsp;

    UNUSED(buffer);
    UNUSED(length);

    if (!pt135_dev.inited) {
        cli_interface_msg_response(CLI_MODULEID_APPLICATION, APP_CLI_MSGID_GET_FORCETOUCH_VOLT, NULL, 0, 0,
                                   RET_FAIL);
        return;
    }

    rsp.volt = pt135_smpl_volt();
    cli_interface_msg_response(CLI_MODULEID_APPLICATION, APP_CLI_MSGID_GET_FORCETOUCH_VOLT,
                               (uint8_t *)&rsp, sizeof(rsp), 0, RET_OK);
}

static void send_key_event(uint8_t key_type)
{
    key_pressed_info_t key_press_info;

    if (!pt135_dev.key_callback) {
        DBGLOG_PT135_INFO("pt135 send key error, key_callback==NULL\n");
        return;
    }

    key_press_info.num = 1;
    key_press_info.id[0] = 0;
    key_press_info.src[0] = KEY_SRC_EXTERNAL;
    key_press_info.type[0] = key_type;

    pt135_dev.key_callback(&key_press_info);
}

static void pt135_check_timer_handle(timer_id_t timer_id, void *arg)
{
    UNUSED(timer_id);
    UNUSED(arg);

    uint32_t pos;
    uint32_t prev_pos;
    uint32_t tm;
    bool_t pressed;
    uint32_t click_count = 0;
    uint32_t last_tm = 0;
    bool_t last_pressed = false;
	
    pt135_dev.press_fifo_lock = true;

    prev_pos = (pt135_dev.press_fifo_pos + PT135_PRESS_FIFO_LEN - 1) % PT135_PRESS_FIFO_LEN;
    if(pt135_dev.press_fifo[prev_pos].pressed){   //long press timer occured
        if(pt135_force_is_pressed()){
            send_key_event(BTN_TYPE_LONG);
            DBGLOG_PT135_INFO("[pt135_check_timer_handle]:long press\n");
            pt135_dev.press_long_inprocess = true;
			pt135_dev.press_fifo_lock = false;
			return;
        }
    }else if(!pt135_dev.press_long_inprocess){   //click timeout occured
        pos = pt135_dev.press_fifo_pos;
        do{
            tm = pt135_dev.press_fifo[pos].time;
            pressed = pt135_dev.press_fifo[pos].pressed;

            if(tm == 0) {
                pos = (pos + 1) % PT135_PRESS_FIFO_LEN;
                continue;
            }

            if((last_tm != 0) && (tm - last_tm >= PT135_TIMER_MULTI_TAP_INTERVAL)){
                if(last_pressed && (!pressed)) {   //button up, click count +1
                    click_count += 1;
                }
            }else if (last_tm != 0) {
                DBGLOG_PT135_INFO("[pt135_check_timer_handle]:%d too short %d->%d diff:%d\n", pos, last_pressed, pressed,
                                       tm - last_tm);
            }

            last_tm = tm;
            last_pressed = pressed;
            pos = (pos + 1) % PT135_PRESS_FIFO_LEN;
        } while (pos != pt135_dev.press_fifo_pos);
    } else{
        pt135_dev.press_long_inprocess = false;
    }

    memset(pt135_dev.press_fifo, 0x00, sizeof(pt135_dev.press_fifo));
    pt135_dev.press_fifo_pos = 0;
    pt135_dev.press_fifo_lock = false;
	
	pt135_dev.state = FORCESENSOR_STATE_IDLE;
	os_start_timer(pt135_dev.smpl_timer_id, PT135_TIMER_IDLE_INTERVAL);

    if (click_count >= 1) {
        DBGLOG_PT135_INFO("[pt135_check_timer_handle]:click count:%d\n", click_count);
        switch (click_count) {
            case 1:
                send_key_event(BTN_TYPE_SINGLE);
                break;
            case 2:
                send_key_event(BTN_TYPE_DOUBLE);
                break;
            case 3:
                send_key_event(BTN_TYPE_TRIPLE);
                break;
            case 4:
                send_key_event(BTN_TYPE_QUADRUPLE);
                break;
            default:
                break;
        }
    }
}

static void pt135_smpl_timer_handle(timer_id_t timer_id, void *arg)
{
    UNUSED(timer_id);
    UNUSED(arg);

    vendor_send_msg(VENDOR_MSG_TYPE_FORCE_TOUCH, 0, 0);
}

static void pt135_press_fifo_in(bool_t pressed)
{
    if (pt135_dev.press_fifo_lock) {
        return;
    }

    pt135_dev.press_fifo[pt135_dev.press_fifo_pos].time = os_boot_time32();
    pt135_dev.press_fifo[pt135_dev.press_fifo_pos].pressed = pressed;
    pt135_dev.press_fifo_pos = (pt135_dev.press_fifo_pos + 1) % PT135_PRESS_FIFO_LEN;
}

static void pt135_msg_handler(uint8_t msg_id, uint16_t msg_value)
{
	bool_t pressed = false;
	
	UNUSED(msg_id);
    UNUSED(msg_value);
	
    if (!pt135_dev.inited) {
        return;
    }

	pressed = pt135_force_is_pressed();
	switch(pt135_dev.state){
	case FORCESENSOR_STATE_IDLE:
		 if(pressed){
			 pt135_dev.base_cnt = 0;
			 pt135_dev.state = FORCESENSOR_STATE_DEBOUNCE;
		 }else{
			 pt135_dev.base_cnt++;
			 if(pt135_dev.base_cnt >= 3000){ // 30s
			 	pt135_dev.base_cnt = 0;	
				pt135_dev.base_volt = pt135_smpl_volt();
			 	DBGLOG_PT135_INFO("[pt135_msg_handler]:base_volt=%d.%dmV\n", FLOAT(pt135_dev.base_volt));
			 }
		 }
		 break;
	case FORCESENSOR_STATE_DEBOUNCE:
		 if(pressed){
			pt135_dev.state = FORCESENSOR_STATE_PRESS_START;
		 }else{
			pt135_dev.state = FORCESENSOR_STATE_IDLE;
			DBGLOG_PT135_INFO("[pt135_msg_handler]->DEBOUNCE:diff=%d.%dmV\n", FLOAT(ABS(pt135_smpl_volt() - pt135_dev.base_volt)));
		 }
		 break;
	case FORCESENSOR_STATE_PRESS_START:
		 os_start_timer(pt135_dev.smpl_timer_id, PT135_TIMER_MULTI_TAP_INTERVAL);
		 
		 if(!pt135_dev.press_check_inprocess){
		 	pt135_dev.last_pressed = false;
		 }

		 if(!pt135_dev.press_check_inprocess && !pressed) {
	        pt135_dev.state = FORCESENSOR_STATE_IDLE;
			DBGLOG_PT135_INFO("[pt135_msg_handler]->PRESS_START:diff=%d.%dmV\n", FLOAT(ABS(pt135_smpl_volt() - pt135_dev.base_volt)));
		    os_start_timer(pt135_dev.smpl_timer_id, PT135_TIMER_IDLE_INTERVAL);
	        return;
	     }

	     if(pt135_dev.last_pressed != pressed) {
	        pt135_press_fifo_in(pressed);
	     }

 		 if(pressed){
 	        if(pt135_dev.last_pressed != pressed){
 	            DBGLOG_PT135_INFO("[pt135_msg_handler]:Pressed edge..........\n");
 	            send_key_event(BTN_TYPE_PRESS);
 	            os_stop_timer(pt135_dev.check_timer_id);
 	            pt135_dev.press_check_inprocess = true;
 	            if(!pt135_dev.press_long_inprocess) {
 	                os_start_timer(pt135_dev.check_timer_id, PT135_LONG_PRESS_TIME);
 	            }
 	        }
 	     }else{
	        if(pt135_dev.last_pressed != pressed){
	            DBGLOG_PT135_INFO("[pt135_msg_handler]:Release edge..........\n");
	            pt135_dev.press_check_inprocess = false;
	            os_stop_timer(pt135_dev.check_timer_id);
	            if(pt135_dev.press_long_inprocess){
	                os_start_timer(pt135_dev.check_timer_id, PT135_LONG_PRESS_RELEASE_TIME);
	            }else{
	                os_start_timer(pt135_dev.check_timer_id, PT135_CLICK_TIMEOUT);
	            }
	        }
	     }
	     pt135_dev.last_pressed = pressed;
		 break; 
	default:
		 break;
	}
}

void pt135_init(key_callback_t callback)
{
    if (pt135_dev.inited) {
        DBGLOG_PT135_INFO("[pt135_init]:pt135 already init!\n");
        return;
    }

#if DBG_PT135_GPIO
	uint8_t ret = iot_gpio_open(IOT_GPIO_64, IOT_GPIO_DIRECTION_INPUT);
	if (ret != RET_OK) {
        DBGLOG_PT135_INFO("[pt135_init]:open gpio failed, ret = %d\n", ret);
        assert(ret == RET_OK);
        return;
    }

    iot_gpio_set_pull_mode(IOT_GPIO_64, IOT_GPIO_PULL_UP);
#endif

	pt135_dev.state = FORCESENSOR_STATE_IDLE;
	pt135_dev.press_fifo_lock = false;
	pt135_dev.press_check_inprocess = false;
	pt135_dev.press_long_inprocess = false;
	pt135_dev.last_pressed = false;
	pt135_dev.base_cnt = 0;
	pt135_dev.base_volt = pt135_smpl_volt();
	pt135_dev.press_fifo_pos = 0;
	memset(pt135_dev.press_fifo, 0x00, sizeof(pt135_dev.press_fifo));
    pt135_dev.key_callback = callback;
    vendor_register_msg_handler(VENDOR_MSG_TYPE_FORCE_TOUCH, pt135_msg_handler);

	pt135_dev.check_timer_id = os_create_timer(LIB_KEYMGMT_MID, 0, pt135_check_timer_handle, NULL);
    if (!pt135_dev.check_timer_id) {
        DBGLOG_PT135_ERROR("[pt135_init]:check_timer_id create failed!\n");
    }
	
    pt135_dev.smpl_timer_id = os_create_timer(LIB_KEYMGMT_MID, 1, pt135_smpl_timer_handle, NULL);
    if (!pt135_dev.smpl_timer_id) {
        DBGLOG_PT135_ERROR("[pt135_init]:smpl_timer_id create failed!\n");
    }

    os_start_timer(pt135_dev.smpl_timer_id, PT135_TIMER_IDLE_INTERVAL);

    pt135_dev.inited = true;
	DBGLOG_PT135_INFO("[pt135_init]:******************************\n");
}

void pt135_deinit(bool_t wakeup_enable)
{
	UNUSED(wakeup_enable);
	
    if (!pt135_dev.inited) {
        return;
    }
	
	os_stop_timer(pt135_dev.smpl_timer_id);
	os_stop_timer(pt135_dev.check_timer_id);
	os_delete_timer(pt135_dev.check_timer_id);
	os_delete_timer(pt135_dev.smpl_timer_id);

#if DBG_PT135_GPIO
	iot_gpio_close(IOT_GPIO_64);	
#endif

    pt135_dev.inited = 0;
}

CLI_ADD_COMMAND(CLI_MODULEID_APPLICATION, APP_CLI_MSGID_GET_FORCETOUCH_VOLT, cli_get_pt135_volt);

#endif /* KEY_DRIVER_SELECTION == KEY_DRIVER_NDT_PT135 */
