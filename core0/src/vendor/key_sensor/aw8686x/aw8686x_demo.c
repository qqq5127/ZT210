/*
* aw8686x.c
*
* Copyright (c) 2021 AWINIC Technology CO., LTD
*
* Author: Alex <zhangpengbiao@awinic.com>
*/

#include "aw8686x_demo.h"

#if KEY_DRIVER_SELECTION == KEY_DRIVER_AW8686X

#include "string.h"
#include "types.h"
#include "modules.h"
#include "os_utils.h"
#include "os_task.h"
#include "os_event.h"
#include "os_timer.h"
#include "iot_i2c.h"
#include "iot_resource.h"
#include "iot_gpio.h"
#include "iot_debounce.h"
#include "lib_dbglog.h"
#include "aw8686x.h"


//#include "cmsis_os.h"
//#include "stdio.h"
//#include "stdbool.h"
//#include "hal_i2c.h"
//#include "hal_timer.h"
//#include "app_utils.h"
//#include "hwtimer_list.h"
//#include "norflash_api.h"
//#include "link_sym_armclang.h"
//#include "hal_norflash.h"
//#include "cmsis.h"
//#include "hal_gpio.h"

#ifdef AW_SPP_USED
#include "spp_api.h"
#include "sdp_api.h"
#include "app_spp.h"
#endif

#define AW8686X_SLAVE_ADDR			(0x6A)
#define INT_GPIO_DEBOUNCE

#define I2C_PORT	IOT_I2C_PORT_0
#define SCL_GPIO 	GPIO_CUSTOMIZE_1  // GPIO3
#define SDA_GPIO 	GPIO_CUSTOMIZE_2  // GPIO63
#define INT_GPIO 	GPIO_CUSTOMIZE_3  // GPIO70
#define VCC_GPIO	GPIO_CUSTOMIZE_4  // GPIO1

static uint8_t gpio_scl = 0xff;
static uint8_t gpio_sda = 0xff;
static uint8_t gpio_int = 0xff;
static uint8_t gpio_vcc = 0xff;
static key_callback_t ptr_aw8686x_key_callback = NULL;

#ifdef AW_SPP_USED
static struct spp_device *aw_spp_deivce = NULL;
static uint8_t rxBuff[AW_SPP_RECV_MAX_LEN];
osMutexDef(spp_mutex_1);
#endif

#ifdef AW_FLASH_USED
extern uint32_t __userdata_start;
extern uint32_t __userdata_end;
extern uint32_t __userdata_start;
extern uint32_t __userdata_end;
const uint32_t userdata_addr_start = 0x283ec000;
static uint32_t sector_size = 0;
static uint32_t block_size = 0;
static uint32_t page_size = 0;

static void aw8686x_userdata_callback(void* param)
{
	NORFLASH_API_OPERA_RESULT *opera_result;

	opera_result = (NORFLASH_API_OPERA_RESULT*)param;

	TRACE(0,"%s:type = %d, addr = 0x%x,len = 0x%x,result = %d.", __func__,
					opera_result->type, opera_result->addr,
					opera_result->len, opera_result->result);
}

static int32_t aw8686x_flash_init(void)
{
	enum NORFLASH_API_RET_T status;

	hal_norflash_get_size(HAL_FLASH_ID_0, NULL, &block_size, &sector_size, &page_size);
	status = norflash_api_register(NORFLASH_API_MODULE_ID_USERDATA,
					HAL_FLASH_ID_0,
					__userdata_start,
					__userdata_end - __userdata_start,
					block_size,
					sector_size,
					page_size,
					0x1000 * 2,
					aw8686x_userdata_callback);
	TRACE(0,"aw8686x status = %d, block_size = 0x%x block_size = 0x%x page_size = 0x%x", status, block_size, sector_size, page_size);
	return (int32_t)status;
}

static int32_t aw8686x_flash_read(uint32_t addr_offset, uint8_t *buf, uint32_t len)
{
	uint32_t lock;

	lock = int_lock_global();
	memcpy(buf, (&__userdata_start) + addr_offset, len);
	int_unlock_global(lock);

	return AW_OK;
}

static int32_t aw8686x_flash_write(uint32_t addr_offset, uint8_t *buf, uint32_t len)
{
	uint32_t lock;
	enum NORFLASH_API_RET_T status;

	lock = int_lock_global();
	status = norflash_api_erase(NORFLASH_API_MODULE_ID_USERDATA, (uint32_t)(userdata_addr_start + addr_offset), 0x1000 , false);
	if (status) {
		int_unlock_global(lock);
		TRACE(0, "erase failed, status = %d", status);
		return status;
	}

	status = norflash_api_write(NORFLASH_API_MODULE_ID_USERDATA, userdata_addr_start + addr_offset, buf, len, false);
	if (status) {
		int_unlock_global(lock);
		TRACE(0, "write failed, status = %d", status);
		return status;
	}
	int_unlock_global(lock);
	return status;
}
#endif

#ifdef AW_SPP_USED
static const U8 AwSppClassId[] = {
	SDP_ATTRIB_HEADER_8BIT(3), /* Data Element Sequence, 6 bytes */
	SDP_UUID_16BIT(SC_SERIAL_PORT), /* Hands-Free UUID in Big Endian */
};

static const U8 AwSppProtoDescList[] = {
	SDP_ATTRIB_HEADER_8BIT(12),  /* Data element sequence, 12 bytes */

	/* Each element of the list is a Protocol descriptor which is a
	* data element sequence. The first element is L2CAP which only
	* has a UUID element.
	*/
	SDP_ATTRIB_HEADER_8BIT(3),   /* Data element sequence for L2CAP, 3 bytes */

	SDP_UUID_16BIT(PROT_L2CAP),  /* Uuid16 L2CAP */

	/* Next protocol descriptor in the list is RFCOMM. It contains two
	* elements which are the UUID and the channel. Ultimately this
	* channel will need to filled in with value returned by RFCOMM.
	*/

	/* Data element sequence for RFCOMM, 5 bytes */
	SDP_ATTRIB_HEADER_8BIT(5),

	SDP_UUID_16BIT(PROT_RFCOMM), /* Uuid16 RFCOMM */

	/* Uint8 RFCOMM channel number - value can vary */
	SDP_UINT_8BIT(RFCOMM_CHANNEL_CUSTOM_4)
};

static const U8 AwSppProfileDescList[] = {
	SDP_ATTRIB_HEADER_8BIT(8), /* Data element sequence, 8 bytes */
	/* Data element sequence for ProfileDescriptor, 6 bytes */
	SDP_ATTRIB_HEADER_8BIT(6),
	SDP_UUID_16BIT(SC_SERIAL_PORT), /* Uuid16 SPP */
	SDP_UINT_16BIT(0x0102) /* As per errata 2239 */
};

/*
 * * OPTIONAL *  ServiceName
 */
static const U8 AwSppServiceName1[] = {
	SDP_TEXT_8BIT(5),          /* Null terminated text string */
	'S', 'P', 'P', '2', '\0'
};

static sdp_attribute_t AwSppSdpAttributes1[] = {

	SDP_ATTRIBUTE(AID_SERVICE_CLASS_ID_LIST, AwSppClassId),

	SDP_ATTRIBUTE(AID_PROTOCOL_DESC_LIST, AwSppProtoDescList),

	SDP_ATTRIBUTE(AID_BT_PROFILE_DESC_LIST, AwSppProfileDescList),

	/* SPP service name*/
	SDP_ATTRIBUTE((AID_SERVICE_NAME + 0x0100), AwSppServiceName1),
};

static void spp_Aw_callback(uint8_t device_id, struct spp_device *locDev, struct spp_callback_parms *Info)
{
	switch (Info->event) {
	case BTIF_SPP_EVENT_REMDEV_CONNECTED:
		AWLOGD("::BTIF_SPP_EVENT_REMDEV_CONNECTED");
		break;
	case BTIF_SPP_EVENT_REMDEV_DISCONNECTED:
		AWLOGD("::BTIF_SPP_EVENT_REMDEV_DISCONNECTED");
		break;
	case BTIF_SPP_EVENT_DATA_SENT:
		AWLOGD("::BTIF_SPP_EVENT_DATA_SENT");
		break;
	default:
		AWLOGD("::unknown event 0x%x", Info->event);
		break;
	}
}

static int32_t aw8686x_spp_handle_data_event_func(uint8_t device_id, void *pDev, uint8_t process, uint8_t *pData, uint16_t dataLen)
{
	aw8686x_spp_cmd_handler(pData, dataLen);
	return AW_OK;
}

void aw8686x_spp_write(uint8_t *buf, uint16_t length)
{
	btif_spp_write(aw_spp_deivce, (char *)buf, &length);
}

void aw8686x_spp_device_create(void)
{
	osMutexId mid;
	btif_sdp_record_param_t param;
	btif_sdp_record_t *sdp_rcd = NULL;
	struct spp_service *spp_service = NULL;

	aw_spp_deivce = btif_create_spp_device();
	btif_spp_init_rx_buf(aw_spp_deivce, rxBuff, AW_SPP_RECV_MAX_LEN);
	aw_spp_deivce->rx_buffer = rxBuff;

	mid = osMutexCreate(osMutex(spp_mutex_1));
	sdp_rcd = btif_sdp_create_record();
	AWLOGD("%s spp_open : sdp_record=0x%p", __func__, sdp_rcd);
	param.attrs = &AwSppSdpAttributes1[0],
	param.attr_count = ARRAY_SIZE(AwSppSdpAttributes1);
	param.COD = BTIF_COD_MAJOR_PERIPHERAL;
	btif_sdp_record_setup(sdp_rcd, &param);
	spp_service = btif_create_spp_service();
	spp_service->rf_service.serviceId = RFCOMM_CHANNEL_CUSTOM_4;
	spp_service->numPorts = 0;
	btif_spp_service_setup(aw_spp_deivce, spp_service, sdp_rcd);

	aw_spp_deivce->portType = BTIF_SPP_SERVER_PORT;
	aw_spp_deivce->app_id = app_spp_alloc_server_id();
	aw_spp_deivce->spp_handle_data_event_func = aw8686x_spp_handle_data_event_func;
	btif_spp_init_device(aw_spp_deivce, 5, mid);

	btif_spp_open(aw_spp_deivce, NULL, spp_Aw_callback);
}
#endif

#ifdef INT_GPIO_DEBOUNCE
//void aw8686x_demo_force_irq_handler(enum HAL_GPIO_PIN_T pin)
static void aw8686x_demo_force_irq_handler(uint16_t gpio, IOT_DEBOUNCE_INT int_type)
{
	UNUSED(gpio);
	UNUSED(int_type);
	
	aw8686x_irq_handler();
}
#else

static void aw8686x_demo_int_isr(void) IRAM_TEXT(gpio_int_isr);
static void aw8686x_demo_int_isr(void)
{
    aw8686x_irq_handler();
}
#endif

//static osThreadId aw8686x_thread_id0;
static os_task_h aw8686x_thread_id0 = NULL;
static os_event_h p_aw8686x_event;

static void (*aw8686x_demo_thread0_cb)(void) = NULL;

//void aw8686x_demo_thread0_handler(void const *argument)
static void aw8686x_demo_thread0_handler(void* arg)
{
	UNUSED(arg);
	
	aw8686x_demo_thread0_cb();
}

static uint32_t aw8686x_demo_signal_wait(void)
{
#if 0
	osEvent event;
	event = osSignalWait(0, osWaitForever);
	if (event.status == osEventSignal) {
		return (uint32_t)event.value.signals;
	}
#endif

	bool_t ret;
	uint32_t events;
	
	ret = os_wait_event(p_aw8686x_event, MAX_TIME, &events);
	if (ret) {
		return events;
	}
	
	return AW_OK;
}

static void aw8686x_demo_clear_signal(uint32_t signal)
{
	//osSignalClear(aw8686x_thread_id0, (int32_t)signal);
	os_clear_event(p_aw8686x_event, signal);
}

static void aw8686x_demo_set_signal0(uint32_t signal)
{
	//osSignalSet(aw8686x_thread_id0, (int32_t)signal);
	os_set_event(p_aw8686x_event, signal);
}
// osThreadDef(aw8686x_demo_thread0_handler, osPriorityNormal, 1, 2048, "aw_thread0");

//osThreadDef(aw8686x_demo_thread0_handler, osPriorityHigh7, 1, 2048, "aw_thread0");
static void aw8686x_demo_create_thread0(void (*thread_cb) (void))
{
#if 0
	aw8686x_demo_thread0_cb = thread_cb;
	aw8686x_thread_id0 = osThreadCreate(osThread(aw8686x_demo_thread0_handler), NULL);
#endif
	p_aw8686x_event = os_create_event(IOT_DRIVER_MID);
    if(!p_aw8686x_event) {
		AWLOGD("aw8686x create event failed!!!\n");
        return;
    }
	
	aw8686x_demo_thread0_cb = thread_cb;
	aw8686x_thread_id0 = os_create_task_ext(aw8686x_demo_thread0_handler, NULL, 7, 2048, "aw_thread0");
    if(!aw8686x_thread_id0) {
        os_delete_task(aw8686x_thread_id0);
    }
}

//static HWTIMER_ID aw8686x_timer_id0;
static timer_id_t aw8686x_timer_id0 = 0;

//static void aw8686x_demo_timer0_handler()
static void aw8686x_demo_timer0_handler(timer_id_t timer_id, void *arg)
{
	UNUSED(timer_id);
    UNUSED(arg);
	
	aw8686x_timer0_handler();
}

static void aw8686x_demo_timer0_start(uint32_t ms)
{
	// hwtimer_start(aw8686x_timer_id0, MS_TO_TICKS(ms));
	//hwtimer_start(aw8686x_timer_id0, (MS_TO_TICKS(ms)*3/4));
	os_start_timer(aw8686x_timer_id0, ms);
}

static void aw8686x_demo_timer0_stop(void)
{
	//hwtimer_stop(aw8686x_timer_id0);
	os_stop_timer(aw8686x_timer_id0);
}

static void aw8686x_demo_timer0_init(void)
{
	//aw8686x_timer_id0 = hwtimer_alloc((HWTIMER_CALLBACK_T)aw8686x_demo_timer0_handler, NULL);
	aw8686x_timer_id0 = os_create_timer(LIB_KEYMGMT_MID, 0, aw8686x_demo_timer0_handler, NULL);
}

static void aw8686x_demo_interrupt_init(void)
{
#if 0
	static const struct HAL_IOMUX_PIN_FUNCTION_MAP g_force_interrupt_config =
	{
		HAL_GPIO_PIN_P2_5, HAL_IOMUX_FUNC_AS_GPIO, HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_PULLUP_ENABLE
	};
	struct HAL_GPIO_IRQ_CFG_T gpiocfg;

	hal_iomux_init((struct HAL_IOMUX_PIN_FUNCTION_MAP *)&g_force_interrupt_config, 1);

	gpiocfg.irq_enable = true;
	gpiocfg.irq_debounce = true;
	gpiocfg.irq_polarity = HAL_GPIO_IRQ_POLARITY_LOW_FALLING;
	gpiocfg.irq_handler = aw8686x_demo_force_irq_handler;
	gpiocfg.irq_type = HAL_GPIO_IRQ_TYPE_EDGE_SENSITIVE;
	hal_gpio_pin_set_dir((enum HAL_GPIO_PIN_T)(g_force_interrupt_config.pin), HAL_GPIO_DIR_IN, 1);
	hal_gpio_setup_irq((enum HAL_GPIO_PIN_T)(g_force_interrupt_config.pin), &gpiocfg);
#endif
	uint8_t pull_mode;

#ifdef INT_GPIO_DEBOUNCE
    gpio_int = iot_resource_lookup_gpio(INT_GPIO);
    if (gpio_int != 0xff) {
        if (iot_gpio_open(gpio_int, IOT_GPIO_DIRECTION_INPUT)) {
            AWLOGD("aw8686x open gpio %d error\n", gpio_int);
            return;
        }
        pull_mode = iot_resource_lookup_pull_mode(gpio_int);
        AWLOGD("aw8686x gpio_int pull_mode = %d\n", pull_mode);
        //iot_gpio_set_pull_mode(gpio_int, pull_mode);
        iot_gpio_set_pull_mode(gpio_int, IOT_GPIO_PULL_NONE);
    } else {
        AWLOGD("int io get fail!\n");
        return;
    }

    iot_debounce_int_cfg_t int_cfg;
    int_cfg.cb = aw8686x_demo_force_irq_handler;
    int_cfg.int_io_en = false;
    int_cfg.int_press_en = true;
    int_cfg.int_press_mid_en = true;

    iot_debounce_gpio(gpio_int, 50, IOT_DEBOUNCE_EDGE_FALLING, &int_cfg);
#else
    UNUSED(isr_state_changed);
    int ret = 0;
    uint8_t gpio_int = iot_resource_lookup_gpio(INT_GPIO);

    iot_gpio_close(gpio_int);
    ret = iot_gpio_open_as_interrupt(gpio_int, IOT_GPIO_INT_EDGE_FALLING, aw8686x_demo_int_isr);
    assert(ret == RET_OK);
    pull_mode = iot_resource_lookup_pull_mode(gpio_int);
    iot_gpio_set_pull_mode(gpio_int, pull_mode);
    iot_gpio_int_enable(gpio_int);

    if (ret != 0) {
        AWLOGD("open interrupt fail\n");
        return;
    }
#endif
}

static void aw8686x_demo_delay_ms(uint32_t time)
{
	//osDelay(time);
	os_delay(time);
}

static void aw8686x_demo_i2c_init_func(void)
{
#if 0
	struct HAL_I2C_CONFIG_T touchI2CMode;
	touchI2CMode.mode = HAL_I2C_API_MODE_SIMPLE;
	touchI2CMode.use_sync = true;
	touchI2CMode.use_dma = true;
	touchI2CMode.as_master = true;
	touchI2CMode.speed = HIGH_SPEED;

	hal_iomux_set_i2c0();
	hal_i2c_open(HAL_I2C_ID_0, &touchI2CMode);
#endif
	uint8_t pull_mode;

	AWLOGD("[%s,%d]:********************************\n", __func__, __LINE__);
	gpio_scl = iot_resource_lookup_gpio(SCL_GPIO);
    AWLOGD("aw8686x gpio_scl = %d\n", gpio_scl);

    if (gpio_scl != 0xFF) {
        pull_mode = iot_resource_lookup_pull_mode(gpio_scl);
        AWLOGD("aw8686x gpio_scl pull_mode = %d\n", pull_mode);
        if (gpio_scl == IOT_AONGPIO_00) {
            gpio_scl = IOT_GPIO_63;
        } else if (gpio_scl == IOT_AONGPIO_01) {
            gpio_scl = IOT_GPIO_64;
        }
        iot_gpio_open(gpio_scl, IOT_GPIO_DIRECTION_OUTPUT);
        //iot_gpio_set_pull_mode(gpio_scl, pull_mode);
        iot_gpio_set_pull_mode(gpio_scl, IOT_GPIO_PULL_NONE);
        iot_gpio_write(gpio_scl, 0);
    } else {
        AWLOGD("i2c scl io get fail!\n");
        return;
    }

    gpio_sda = iot_resource_lookup_gpio(SDA_GPIO);
    AWLOGD("aw8686x gpio_sda = %d\n", gpio_sda);

    if (gpio_sda != 0xFF) {
        pull_mode = iot_resource_lookup_pull_mode(gpio_sda);
        AWLOGD("aw8686x gpio_sda pull_mode = %d\n", pull_mode);
        if (gpio_sda == IOT_AONGPIO_00) {
            gpio_sda = IOT_GPIO_63;
        } else if (gpio_sda == IOT_AONGPIO_01) {
            gpio_sda = IOT_GPIO_64;
        }
        iot_gpio_open(gpio_sda, IOT_GPIO_DIRECTION_OUTPUT);
        //iot_gpio_set_pull_mode(gpio_sda, pull_mode);
        iot_gpio_set_pull_mode(gpio_sda, IOT_GPIO_PULL_NONE);
        iot_gpio_write(gpio_sda, 0);
    } else {
        AWLOGD("i2c sda io get fail!\n");
        return;
    }

    os_delay(20);
    iot_gpio_close(gpio_scl);
    iot_gpio_close(gpio_sda);
	os_delay(20);

	iot_i2c_config_t cfg;
	
	cfg.i2c_busrt_mode = 1;
	cfg.baudrate = 200000;
	cfg.wait_nack_max_time = 100;

	if (iot_i2c_init(I2C_PORT, &cfg)) {
		AWLOGD("iot_i2c_init failed\n");
		return;
	}

	iot_i2c_gpio_cfg_t gpio_cfg;
    gpio_cfg.scl = gpio_scl;
    gpio_cfg.sda = gpio_sda;

    if (iot_i2c_open(I2C_PORT, &gpio_cfg)) {
        AWLOGD("iot_i2c_open failed\n");
        return;
    }
}

static uint32_t aw8686x_demo_i2c_read_func(uint8_t addr, uint8_t *p_data, uint16_t data_len)
{
	uint32_t ret = 0;

#if 0	
	uint32_t active_len = 0;

	ret = hal_i2c_mst_write(HAL_I2C_ID_0, AW8686X_SLAVE_ADDR,
								&addr, 1, &active_len, 0, 1, 0);
	ret |= hal_i2c_mst_read(HAL_I2C_ID_0, AW8686X_SLAVE_ADDR,
								p_data, data_len, &active_len, 1, 1, 0);
#endif
    ret = iot_i2c_master_receive_from_memory_poll(I2C_PORT, AW8686X_SLAVE_ADDR, addr,
                                                  IOT_I2C_MEMORY_ADDR_8BIT, p_data, data_len, 1000);
    if (ret != 0) {
        AWLOGD("[%s,%d]:i2c read error %d!\n", __func__, __LINE__, ret);
    }
	
	return ret;
}

static uint32_t aw8686x_demo_i2c_write_func(uint8_t *p_msg, uint8_t addr, uint8_t *p_data, uint16_t data_len)
{
	uint32_t ret = 0;
#if 0	
	uint32_t active_len = 0;

	p_msg[0] = addr;
	memcpy(&p_msg[1], p_data, data_len);
	ret = hal_i2c_mst_write(HAL_I2C_ID_0, AW8686X_SLAVE_ADDR, p_msg, data_len + 1, &active_len, 1, 1, 0);
#endif
	UNUSED(p_msg);
	ret = iot_i2c_master_transmit_to_memory_poll(I2C_PORT, AW8686X_SLAVE_ADDR, addr,
                                                 IOT_I2C_MEMORY_ADDR_8BIT, p_data, data_len, 1000);
    if (ret != 0) {
        AWLOGD("[%s,%d]:write i2c error %d!\n", __func__, __LINE__, ret);
    }

	return ret;
}

static void aw8686x_demo_send_key_event(uint8_t key_type)
{
    key_pressed_info_t key_press_info;

    if (!ptr_aw8686x_key_callback) {
        AWLOGD("aw8686x send key error, ptr_aw8686x_key_callback==NULL\n");
        return;
    }

    key_press_info.num = 1;
    key_press_info.id[0] = 0;
    key_press_info.src[0] = KEY_SRC_EXTERNAL;
    key_press_info.type[0] = key_type;

    ptr_aw8686x_key_callback(&key_press_info);
}

static void aw8686x_power_cfg(void)
{
	uint8_t pull_mode = IOT_GPIO_PULL_NONE;
	
	gpio_vcc = iot_resource_lookup_gpio(VCC_GPIO);
    AWLOGD("aw8686x gpio_vcc = %d\n", gpio_vcc);
    if (gpio_vcc != 0xFF) {
        iot_gpio_open(gpio_vcc, IOT_GPIO_DIRECTION_OUTPUT);
        pull_mode = iot_resource_lookup_pull_mode(gpio_vcc);
        AWLOGD("aw8686x gpio_vcc pull_mode = %d\n", pull_mode);
        //iot_gpio_set_pull_mode(gpio_vcc, pull_mode);
        iot_gpio_set_pull_mode(gpio_vcc, IOT_GPIO_PULL_NONE);    
        iot_gpio_write(gpio_vcc, 0);
    }	
}

void aw8686x_init(key_callback_t callback)
{
	UNUSED(callback);
	
	struct aw8686x_api_interface api = {
		.aw_delay_ms_func = aw8686x_demo_delay_ms,
		.p_snd_key_event = aw8686x_demo_send_key_event,
		.p_i2c_tx = aw8686x_demo_i2c_write_func,
		.p_i2c_rx = aw8686x_demo_i2c_read_func,
		.p_timer0_start = aw8686x_demo_timer0_start,
		.p_timer0_stop = aw8686x_demo_timer0_stop,
		.p_set_signal0 = aw8686x_demo_set_signal0,
		.p_wait_signal = aw8686x_demo_signal_wait,
		.p_clear_signal = aw8686x_demo_clear_signal,
		.p_thread0_create = aw8686x_demo_create_thread0,
#ifdef AW_SPP_USED
		.p_aw_spp_write = aw8686x_spp_write,
#endif
#ifdef AW_FLASH_USED
		.p_aw_flash_write = aw8686x_flash_write,
		.p_aw_flash_read = aw8686x_flash_read,
#endif
	};
#ifdef AW_FLASH_USED
	aw8686x_flash_init();
#endif
	aw8686x_power_cfg();
	ptr_aw8686x_key_callback = callback;
	aw8686x_demo_i2c_init_func();
	aw8686x_demo_timer0_init();
#ifdef AW_SPP_USED
	aw8686x_spp_device_create();
#endif
	aw8686x_demo_interrupt_init();
	aw8686x_sensor_init(&api);
	AWLOGD("[aw8686x_init]:***************\n");
}

void aw8686x_deinit(bool_t wakeup_enable)
{	
	if (!wakeup_enable) {
	   iot_gpio_int_disable(gpio_int);
	   iot_gpio_close(gpio_int);
	}

	os_stop_timer(aw8686x_timer_id0);
	
	os_delete_event(p_aw8686x_event);
    p_aw8686x_event = NULL;
    os_delete_task(aw8686x_thread_id0);
	
	iot_i2c_close(I2C_PORT);
	
	iot_gpio_write(gpio_vcc, 1);
	AWLOGD("[aw8686x_deinit]:***************\n");
}

#endif

