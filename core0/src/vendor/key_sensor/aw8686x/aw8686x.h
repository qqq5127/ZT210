#ifndef __AW8686X_H
#define __AW8686X_H

#include "key_sensor.h"

#if KEY_DRIVER_SELECTION == KEY_DRIVER_AW8686X

#include <stdint.h>
#include "aw8686x_demo.h"
#include "aw_algo_touch_data.h"
#include "aw_touch_algo_lib.h"
#include "aw_type.h"

#define AW8686X_SLAVE_ADDR		(0x6A)

#define AW8686X_MAX_CHANNEl_MODE

#define TOUCH_HARD_SILL			(0x25)
#define TOUCH_CABLIB_DIV2		100
#define EXP_FILTER_EXCEED		(16)

#define AW8686X				(0x6)
#define ALGO_YIMR_OUT			(0)
#define AW8686X_TEST_CHIPID		(0x60)
#define AW8686XA			(0x0A)
#define AW8686XB			(0x0B)
#define AW_RAW_DATA_NUM			(100)
#define AW_NOISE_DATA_NUM		(200)
#define AW_ONE_REG			(1)
#define FIRST_LOAD			(0)
#define NEXT_LOAD			(1)
#define AW8686X_I2C_RETRIES		(3)
#define AW_POLLING_RETRIES		(10)
#define AW8686X_INIT_DELAY_TIME	 (10)// (100)	//	(10) 
#define GIVE_ALGO_DATA_TIME		(20)
#define AW_TIMER_DELAY		(10)	//(10) read data must between 10 ms
#define STOP_PRESS_UPDATE_COUNT		(50)
#define AW_ADC_CLOSED			(0)
#define REGADDR_REGVAL			(2)
#define AW_READ_CHIPID_RETRIES		(3)
#define AW_OFFSET_CALI_TIME		(5)
#define AW8686X_EAR_TIME		(80)
#define AW_AFE_IRQ_POLLING		(12)
#define ONE_BIT				(0x1)
#define TWO_BIT				(0x3)
#define THREE_BIT			(0x7)
#define HALF_WORD			(0xf)
#define ONE_WORD			(0xff)
#define CHANNEL_MAX_NUM			(1)
#define AW_LOOP_START			(0)
#define AW_WRITE_UNLOCK			(0xA5)
#define AW_WRITE_LOCK			(0x0)
#define CHANNEL_OFFSET			(0x10)
#define ADCH0CR_OFFSET			(0x10)
#define AW8686X_BIT1_EN			(1 << 1)
#define AW8686X_BIT2_EN			(1 << 2)
#define AW8686X_BIT3_EN			(1 << 3)
#define AW8686X_BIT5_EN			(1 << 5)
#define AW8686X_BIT6_EN			(1 << 6)
#define AW8686X_BIT7_EN			(1 << 7)
#define AW8686X_STANDBY_MODE		(0x05)
#define AW8686X_RST			(0x10)
#define AW8686X_PGA_OPEN		(0x08)
#define AW8686X_CFG_ADM			(0x08)
#define ADC_TURN_ON			(0x08)
#define AW8686X_ADST			(0x08)
#define AW8686X_DAC_VIN			(0x19)
#define AW8686X_DAC_CAL			(0x39)
#define AW8686X_DAC_CODE_CLEAR		(0x00)
#define AW8686X_POLLING_MODE1		(0x4B)
#define AW8686X_OFFSET_VALTAGE		(0x0200)
#define AW8686X_CALI_POLL_END		(11)
#define ADC_DATA_BASE			(0x2000)
#define SYSTEM_CLOCK_REDUCE_2M		(0x03)
#define SYSTEM_CLOCK_DEFAULT		(0x00)
#define SCAN_TIME_16			(0x0B)
#define EN_IRQ_1_2			(0x06)
#define DYNAMIC_CALI_VALUE		(1000)
#define AW_ALGO_NO_PRESS_TIME		(67)
#define AW_ALGO_PRESS_TIME		(25)
#define AW8686X_SIGNAL_IRQ		(100)
#define AW8686X_SIGNAL_TIMER		(101)
#define AW8686X_ERR_DATA		(8192)
#define AW8686X_IMPEDANCE_MAG		(1000)
#define ALGO_CALIB_FACTOR		(0x64)
#define AW_BLE_RAW_LOCA0		(13)
#define AW_BLE_RAW_LOCA1		(14)
#define AW_BLE_STATUS_LOCA		(5)
#define ADCH0CR0_1_CONFIG		(0x30)
#define TEMP_VS_SEL			(0x11)
#define AW_TEMP_DATA4			(0x27)
#define AW_TEMP_DATA5			(0x37)
#define AW_OFFSET0			(0x3fff)
#define AW_NORFLASH_MAX			(128)
#define AW_CALI				(0x5A5A)
#define AW_NORFLASH_INIT_ADD		0
#define PACKS_IN_ONE_COMM		5
#define AW_APP_CURVE_CNT		2
#define APP_REG_MAX			(10)
#define SPP_HEADER_LEN			(10)

#define EN_LDO_LOSC			(0x83)
#define OPEN_SWDT_DELAY_COUNT		(0x84)
#define CLOSE_SWDT_DELAY_COUNT		(0x80)
#define GO_STDBY_RD_DONE		(0x06)
#define CLEAR_IRQ			(0x06)
#define GO_STDBY_RD_DONE_AUTO_CLEAR	(0x05)
#define ADST_TRRIGER			(0x6B)
#define EN_I2C_WAKE			(0xC4)
#define CLOSE_LOSC			(0x81)
#define SENSOR_TO_GND			(0x02)
#define SENSOR_TO_DIFFERENT		(0x10)
#define AW_SPP_RECV_MAX_LEN		(1024)
//go standby time
#define GO_STDBY_DELAY_1MS		(1000) //1000us
#define GO_STDBY_DELAY_0MS		(0)
#define HOSC_FREQUENCY			(6) //6M HZ
#define GO_STDBY_DELAY_1MS_LOW		((GO_STDBY_DELAY_1MS * HOSC_FREQUENCY) & ONE_WORD)
#define GO_STDBY_DELAY_1MS_MIDDLE	(((GO_STDBY_DELAY_1MS * HOSC_FREQUENCY) >> AW_BIT8) & ONE_WORD)
#define GO_STDBY_DELAY_1MS_HIGH		(((GO_STDBY_DELAY_1MS * HOSC_FREQUENCY) >> AW_BIT16) & ONE_WORD)

//swdt sleep time
#define LOSC_FREQUENCY			(32) //32K HZ
#define SWDT_SLEEP_10MS			(10) //10ms
#define SWDT_SLEEP_TIME_10MS_HIGH	(((LOSC_FREQUENCY * SWDT_SLEEP_10MS) >> AW_BIT8) & ONE_WORD)
#define SWDT_SLEEP_TIME_10MS_LOW	((LOSC_FREQUENCY * SWDT_SLEEP_10MS) & ONE_WORD)

#define AW8686X_IRQ_CFG
#define AW8686X_POLL_DATA_TIME_IN_MS	(10)
/***********************************************
* voltage
************************************************/
#define AW_2_4V				(24)
#define AW_2_8V				(28)
#define AW_3_0V				(30)
#define AW_3_1V				(31)

#define AW_RING_COEF		(10)
#define AW_CALL_COEF		(10)
#define AW_NORAMAL_COEF		(10)
#define AW_EAR_COEF			(80)
/***********************************************
* N or P terminal function
***********************************************/
#define SN_MUX_VS			(0x4)
#define SN_MUX_INPUT			(0x10)
#define SP_MUX_VS			(0x4)
#define SP_MUX_INPUT			(0x10)

/***********************************************
* define register name
***********************************************/
#define REG_CHIPID			0x00
#define REG_WR_UNLOCK			0x05
#define REG_TEMP_TRIM			0x0A
#define REG_TEMP			0x0E
#define REG_CLK_DIV0			0x10
#define REG_LD_SWDT_CNT1		0x14
#define REG_LD_SWDT_CNT0		0x15
#define REG_LD_SWDT_CFG			0x16
#define REG_STDBY_CFG			0x17
#define REG_GO_STDBY_DLY2		0x18
#define REG_GO_STDBY_DLY1		0x19
#define REG_GO_STDBY_DLY0		0x1A
#define REG_GO_STDBY_CFG		0x1B
#define REG_INTR_MASK			0x1F
#define REG_INTR_STAT			0x20
#define REG_ANA_ADJ_CFG			0x21
#define REG_TEMP_VS_SEL			0x22
#define REG_RST_CFG				0x2F
#define REG_ADCH0CR0_1			0x31
#define REG_ADCH0CR0_2			0x32
#define REG_ADCH0CR0_3			0x33
#define REG_ADCH0CR1_0			0x34
#define REG_ADCH0CR1_1			0x35
#define REG_ADCH0CR1_2			0x36
#define REG_DACH0CR_1			0x39
#define REG_DACH0DR_0			0x3C
#define REG_DACH0DR_1			0x3D
#define REG_DACH0DR_2			0x3E
#define REG_DACH0DR_3			0x3F
#define REG_ADCH5CR0_1			0x81
#define REG_ADCH5CR0_2			0x82
#define REG_ADCH5CR0_3			0x83
#define REG_ADCH5CR1_0			0x84
#define REG_ADCH5CR1_1			0x85
#define REG_ADCH5CR1_2			0x86
#define REG_DACH5CR_1			0x89
#define REG_ADCH0DR_0			0xB0
#define REG_ADCH0DR_1			0xB1
#define REG_ADCH5DR_0			0xBA
#define REG_ADCH5DR_1			0xBB
#define REG_ADMCR_0			0xC0
#define REG_ADMCR_1			0xC1
#define REG_ADMCR_2			0xC2
#define REG_ADSR_0			0xC8
#define REG_ADCMPCR0_2			0xCE
#define REG_ADCMPCR0_3			0xCF
#define REG_ADCMPCR1_2			0xD2
#define REG_ADCMPCR1_3			0xD3
#define REG_ADCHEN_0			0xD4
#define REG_DAOSDR_2			0XDA

typedef enum
{
	AW_TOUCH_EVENT_NONE = 0x00,
	AW_TOUCH_EVENT_SINGLE = 0x01,
	AW_TOUCH_EVENT_DOUBLE = 0x02,
	AW_TOUCH_EVENT_TRIPLE = 0x03,
	AW_TOUCH_EVENT_LONG = 0x04,
} AW_TOUCH_EVENT_E;

typedef enum
{
	AW_VALID_DATA_LEN = 60,
	AW_CHIP_NAME = 24,
	AW_VALID_DATA_FIRST_ADDR = 64,
} AW_PARA_HEADER_E;

typedef enum
{
	AW_TEMP_INIT = 20,
	AW_TEMP_UMV = 2800,
	AW_TEMP_COEF = 3672,
	AW_TEMP_MAP_10 = 10,
	AW_TEMP_MAP_100 = 100,
} AW_TEMP_T;

/******************************************
* error handle
******************************************/
enum aw8686x_bit {
	AW_BIT0,
	AW_BIT1,
	AW_BIT2,
	AW_BIT3,
	AW_BIT4,
	AW_BIT5,
	AW_BIT6,
	AW_BIT7,
	AW_BIT8,
	AW_BIT16 = 16,
	AW_BIT24 = 24,
	AW_BIT32 = 32,
};

enum system_clock {
	REDUCE_2M,
	DEFAULT_HZ,
};

enum dac_inform {
	EN_DAC_VIN,
	EN_DAC_CAL,
};

enum residual_infor {
	OPEN_RESIDUAL,
	CLOSE_RESIDUAL,
};

/******************************************
* Voltage mapping relationship
******************************************/
enum aw8686x_voltage {
	MAPPING_2_4V,
	MAPPING_2_8V,
	MAPPING_3_0V,
	MAPPING_3_1V,
};

typedef enum
{
	ADC_BITS_NUM = 12,
	DAC_BITS_NUM = 11,
	AW_BIT_NUM_MAX = 32,
	BASE_NUM = 2,
} AW_BIT_NUM_T;

typedef enum
{
	PGA1_MAG_1,
	PGA1_MAG_16,
	PGA1_MAG_32,
	PGA1_MAG_64,
	PGA1_MAG_128,
	PGA1_MAG_256,
} AW_PGA1_MAG_T;

typedef enum
{
	PGA1_MAG_1_MAP = 1,
	PGA1_MAG_16_MAP = 16,
	PGA1_MAG_32_MAP = 32,
	PGA1_MAG_64_MAP = 64,
	PGA1_MAG_128_MAP = 128,
	PGA1_MAG_256_MAP = 256,
} AW_PGA1_T;

typedef enum {
	APK_VALID_DATA_HEASER = 3,
	APK_HEADER = 0x3A,
	OFFSET = 0x82,
	NOISE,
	SIGNAL_RAW_0,
	SIGNAL_RAW_1,
	VERIFY_0,
	VERIFY_1,
	DYNAMIC_CALI = 0x8C,
	APK_END0 = 0x0D,
	APK_END1 = 0x0A,
} CALI_FLAG_T;

typedef enum {
	NO_PRESS,
	SIGGLE_PRESS,
	DOUBLE_PRESS,
	TRIPLE_PRESS,
	LONG_PRESS = 255,
} AW_PRESS_T;

enum aw8686x_func_en {
	DIS,
	EN,
};

typedef enum {
	WEIGHT_G = 200,
	W_MAG = 100,
	RAW_DATA_DELAY = 4000,
} SIGNAL_PARA_VAL_E;

typedef enum {
	V_TRANS_MV = 1000,
	V_TRANS_UV = 1000000,
} V__TRANS_E;

typedef enum {
	DAC_V = 614,
	RES_SEG = 3,
	IMPEDENT1 = 2,
	IMPEDENT2 = 4,
} V__DAC_E;

enum aw8686x_channel {
	IC_ERR,
	AW86861,
	AW86862,
	AW86864 = 4,
};

enum {
	AW_APP_CMD_GET_DEVICE_INFO = 0x00,
	AW_APP_CMD_GET_CURVE_DATA = 0x01,
	AW_APP_CMD_READ_REG = 0x02,
	AW_APP_CMD_WRITE_REG = 0x03,
	AW_APP_CMD_GET_ALGO_PARA = 0x04,
	AW_APP_CMD_SET_ALGO_PARA = 0x05,
};

enum aw_err_code {
	AW_OK,
	AW_ERR,
	AW_ERR_CHIPID,
	AW_ERR_IRQ_INIT,
	AW_PROT_UPDATE_ERR,
};

enum {
	CNT_LONG_SPP = 0,
	CNT_DOUBLE_SPP = 4,
	DATA_SMOOTH_SPP = 8,
	K_SMOOTH_SPP = 12,
	BESE_UP_SPP = 16,
	START_SILL_SPP = 20,
	STOP_SILL_SPP = 24,
	K_SILL_SPP = 28,
	HEAVY_SILL_SPP = 32,
	REPLY_CNT_SPP = 36,
	TIMEOUT_SPP = 40,
	ST_LEN_SPP = 44,
	FTC_COEF_SPP = 48,
	NOISE_PT_SPP = 52,
	ALG_PARA_TOTAL = 56,
};

#define AW_SPP_PACK_LEN (AW_APP_CURVE_CNT * SPP_CURVE_DATA_LEN + SPP_REG_DATA_LEN)

enum {
	SPP_CURVE_DATA_LEN = 16,
	SPP_REG_DATA_LEN = 8,
	SPP_CURVE_NUM = 4,
	SPP_SIGGLE_CURVE = 32,
	SPP_DOUBLE_CURVE = 33,
	SPP_TRIPLE_CURVE = 34,
	SPP_LONG_CURVE = 35,
};

typedef struct aw8686x_data {
	int16_t diff_data[AW_NOISE_DATA_NUM];
	int16_t adc_data[AW_NOISE_DATA_NUM + 1];
	int32_t offset_uv;
	int32_t signal;
	int32_t raw_data0;
	int32_t raw_data1;
	int16_t adc_peak;
} AW8686X_DATA_T;

typedef struct aw_FTC_para_struct {
	uint32_t check_sum;
	uint16_t cali_coef[CHANNEL_MAX_NUM];
	uint16_t para_len;
	uint16_t cali_flag;
} AW_FTC_PARA_T;

#define DATA_MAX_LEN 512
typedef struct aw_app_prf
{
	uint8_t cmd;
	uint8_t dat[DATA_MAX_LEN];
	uint8_t len;
} aw_app_prf_t;

struct aw8686x_spp_struct {
	uint8_t send_total_buf[DATA_MAX_LEN];
	uint8_t valid_buf[(2 * 16 + 8) * 5];
	uint8_t last_status[CHANNEL_MAX_NUM];
	uint8_t count_curve;
	uint8_t curve_mode_flag;
};

struct aw8686x_chara_struct {
	uint8_t ch_status;
	uint8_t version;
	char chip_type[8];
	uint8_t t_vref;
	uint16_t temp_para_total;
	uint8_t temp_para_l;
	uint16_t g_ch_calib_factor_lib[TOUCH_CHANNEL_MAX];
};

extern int32_t aw8686x_sensor_init(struct aw8686x_api_interface *api);
extern void aw8686x_irq_handler(void);
extern void aw8686x_timer0_handler(void);
extern uint32_t aw8686x_data_get(uint8_t *adc_data, int16_t *adc_data_cur, uint8_t ch_num);
void aw8686x_temperature_get(uint16_t temper_data, int32_t *temperature);
void aw8686x_factory_mode(void);
void aw8686x_close_irq(void);
void aw8686x_open_irq(void);
void aw8686x_i2c_low_power(void);
void aw8686x_swdt_low_power(void);
void aw8686x_i2c_wakeup(void);
void aw8686x_sensor_free_memory(void);
#ifdef AW8686X_FTC
extern void aw8686x_FTC(CALI_FLAG_T flag);
#endif
void aw8686x_touch_alg_no_press(uint8_t chx, uint32_t set_flag);
#ifdef AW_SPP_USED
extern void aw8686x_spp_cmd_handler(uint8_t *dat, uint16_t len);
#endif

#endif
void aw8686x_thread0_cb(uint8_t msg);

#endif
