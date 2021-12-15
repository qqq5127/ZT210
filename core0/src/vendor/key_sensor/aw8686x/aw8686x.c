/*
* aw8686x.c
*
* Copyright (c) 2021 AWINIC Technology CO., LTD
*
*/
#include "aw8686x.h"

#if KEY_DRIVER_SELECTION == KEY_DRIVER_AW8686X

#include "string.h"
#include "stdio.h"
#include "stdbool.h"
#include "math.h"
#include "aw8686x_demo.h"
#include "aw8686x_reg.h"

#ifdef __3NOD_APP_TOUCH__
#include "app_touch.h"
#endif

#include "os_mem.h"
#include "lib_dbglog.h"
#include "iot_resource.h"

//#include "cmsis_os.h"
//#include "app_thread.h"
//#include "cmsis.h"
//#include "hal_trace.h"
//#include "app_utils.h"
//#include "hal_timer.h"
//#include "app_bt_stream.h"
//#include "apps.h"

#define AW8686X_DRIVER_VERSION "v0.1.0.9"

static aw_delay_ms_t aw_delay_ms = NULL;
static aw_send_key_event_t aw_send_key_event = NULL;
static aw_i2c_transfer_tx_t i2c_transfer_tx = NULL;
static aw_i2c_transfer_rx_t i2c_transfer_rx = NULL;
static aw_timer_start_t aw_timer0_start = NULL;
static aw_timer_stop_t aw_timer0_stop = NULL;
static aw_signal_set_t aw_signal0_set = NULL;
static aw_signal_wait_t aw_signal_wait = NULL;
static aw_signal_clear_t aw_signal_clear = NULL;
static aw_thread_create_t thread0_create = NULL;
#ifdef AW_FLASH_USED
static aw_flash_handle_t aw_flash_read = NULL;
static aw_flash_handle_t aw_flash_write = NULL;
#endif
struct aw8686x_chara_struct g_aw8686x_s;
int16_t g_adc_data[CHANNEL_MAX_NUM];
uint8_t g_cali_flag;
static AW_ALGO_PATAM_T g_touch_param_alg_lib;
uint8_t *p_touch_alg_out = NULL;
AW8686X_DATA_T aw8686x_FTC_data[CHANNEL_MAX_NUM];
AW_FTC_PARA_T aw8686x_FTC_para;
#ifdef AW_SPP_USED
static aw_spp_write_t aw_spp_write = NULL;
#endif

void aw8686x_cali(uint8_t ch_status, uint8_t version);
void aw8686x_dynamic_cali(uint8_t chx);
void aw8686x_FTC(CALI_FLAG_T flag);

static uint32_t aw8686x_i2c_write_seq(uint8_t *p_msg, uint8_t addr_data, uint8_t *p_data, uint16_t data_len)
{
	uint8_t cnt = 0;
	uint32_t status = 0;

	while (cnt < AW8686X_I2C_RETRIES) {
		status = i2c_transfer_tx(p_msg, addr_data, p_data, data_len);
		if (status == AW_OK) {
			return AW_OK;
		} else {
			AWLOGD("[%s,%d]:i2c write error = %x, cnt = %d", __func__, __LINE__, status, cnt);
			aw_delay_ms(1);
			cnt++;
		}
	}

	return AW_ERR;
}

static uint32_t aw8686x_i2c_read_seq(uint8_t addr_data, uint8_t *p_data, int16_t data_len)
{
	uint8_t cnt = 0;
	uint32_t status = 0;

	while (cnt < AW8686X_I2C_RETRIES) {
		status = i2c_transfer_rx(addr_data, p_data, data_len);
		if (status == AW_OK) {
			return AW_OK;
		} else {
			AWLOGD("[%s,%d]:i2c read error = %x, cnt = %d", __func__, __LINE__, status, cnt);
			aw_delay_ms(1);
			cnt++;
		}
	}

	return AW_ERR;
}

static uint32_t aw8686x_i2c_read(uint8_t reg_addr, uint8_t *reg_data)
{
	return aw8686x_i2c_read_seq(reg_addr, reg_data, AW_ONE_REG);
}

static uint32_t aw8686x_i2c_write(uint8_t reg_addr, uint8_t reg_data)
{
	uint8_t msg[2] = { 0 };

	return aw8686x_i2c_write_seq(msg, reg_addr, &reg_data, AW_ONE_REG);
}

static uint32_t aw8686x_i2c_write_bits(uint8_t slavereg, uint16_t mark, uint8_t endata)
{
	uint8_t slavedata = 0;
	uint32_t ret = 0;

	ret = aw8686x_i2c_read(slavereg, &slavedata);
	if (ret != AW_OK) {
		return AW_ERR;
	}
	slavedata &= mark;
	slavedata |= endata;
	ret = aw8686x_i2c_write(slavereg, slavedata);
	if (ret != AW_OK) {
		return AW_ERR;
	}
	return AW_OK;
}

/*******************************************************************
*
* function : waking up aw8686x by reading or writing every regsiter.
*
********************************************************************/
void aw8686x_i2c_wakeup(void)
{
	uint8_t reg_data = 0;

	aw8686x_i2c_read(REG_CHIPID, &reg_data);
}

void aw8686x_swdt_low_power(void)
{
	aw8686x_i2c_write(REG_WR_UNLOCK, AW_WRITE_UNLOCK);
	//go standby 1ms
	aw8686x_i2c_write(REG_GO_STDBY_DLY2, GO_STDBY_DELAY_1MS_HIGH);
	aw8686x_i2c_write(REG_GO_STDBY_DLY1, GO_STDBY_DELAY_1MS_MIDDLE);
	aw8686x_i2c_write(REG_GO_STDBY_DLY0, GO_STDBY_DELAY_1MS_LOW);

	//swdt counter
	aw8686x_i2c_write(REG_LD_SWDT_CNT1, SWDT_SLEEP_TIME_10MS_HIGH);
	aw8686x_i2c_write(REG_LD_SWDT_CNT0, SWDT_SLEEP_TIME_10MS_LOW);

	aw8686x_i2c_write(REG_STDBY_CFG, EN_LDO_LOSC);
	aw8686x_i2c_write(REG_LD_SWDT_CFG, OPEN_SWDT_DELAY_COUNT);
	aw8686x_i2c_write(REG_LD_SWDT_CFG, CLOSE_SWDT_DELAY_COUNT);

	aw8686x_i2c_write(REG_GO_STDBY_CFG, GO_STDBY_RD_DONE);
	aw8686x_i2c_write(REG_ADMCR_0, ADST_TRRIGER);
}

void aw8686x_i2c_low_power(void)
{
	aw8686x_i2c_write(REG_WR_UNLOCK, AW_WRITE_UNLOCK);

	aw8686x_i2c_write(REG_GO_STDBY_DLY2, GO_STDBY_DELAY_0MS);
	aw8686x_i2c_write(REG_GO_STDBY_DLY1, GO_STDBY_DELAY_0MS);
	aw8686x_i2c_write(REG_GO_STDBY_DLY0, GO_STDBY_DELAY_0MS);

	aw8686x_i2c_write(REG_STDBY_CFG, CLOSE_LOSC);

	aw8686x_i2c_write(REG_LD_SWDT_CFG, EN_I2C_WAKE);
	aw8686x_i2c_write(REG_GO_STDBY_CFG, GO_STDBY_RD_DONE_AUTO_CLEAR);
	aw8686x_i2c_write(REG_ADMCR_0, ADST_TRRIGER);
}

static void aw8686x_close_low_power(void)
{
	aw8686x_i2c_write(REG_WR_UNLOCK, AW_WRITE_UNLOCK);
	aw8686x_i2c_write(REG_GO_STDBY_CFG, DIS);
}

uint32_t aw8686x_data_get(uint8_t *adc_data, int16_t *adc_data_cur, uint8_t ch_num)
{
	int32_t ret = 0;

	ret = aw8686x_i2c_read_seq(REG_ADCH0DR_0 + ch_num * REGADDR_REGVAL, &adc_data[REGADDR_REGVAL * ch_num], REGADDR_REGVAL);
	if (ret != AW_OK) {
		AWLOGD("get data err");
		return AW_ERR;
	}
	adc_data_cur[ch_num] = (int16_t)(((uint16_t)adc_data[ch_num * REGADDR_REGVAL] |
			((uint16_t)adc_data[ch_num * REGADDR_REGVAL + 1] << AW_BIT8)) - ADC_DATA_BASE);


	AWLOGD("[%s,%d]:adc data[%d] = %d", __func__, __LINE__, ch_num, adc_data_cur[ch_num]);

	return AW_OK;
}

static uint32_t aw8686x_adc_temp_data_get(int32_t *temperature, int16_t *adc_data_cur)
{
	int32_t ret = 0;
	uint8_t adc_data[REGADDR_REGVAL * REGADDR_REGVAL] = { 0 };
	uint16_t temper_adc_data = 0;

	ret = aw8686x_i2c_read_seq(REG_ADCH0DR_0, adc_data, REGADDR_REGVAL * REGADDR_REGVAL);
	if (ret != AW_OK) {
		AWLOGD("get data err");
		return AW_ERR;
	}
	*adc_data_cur = (int16_t)(((uint16_t)adc_data[0] | ((uint16_t)adc_data[1] << AW_BIT8)) - ADC_DATA_BASE);
	temper_adc_data = (int16_t)(((uint16_t)adc_data[2] | ((uint16_t)adc_data[3] << AW_BIT8))) & AW_OFFSET0;
	aw8686x_temperature_get(temper_adc_data, temperature);

	AWLOGD("adc data = %d", *adc_data_cur);
	return AW_OK;
}

static void aw8686x_afe_data_handle(int16_t *adc_data, uint8_t chx)
{
	AW_ALGO_CH_EXP_T p_exp_data;
	AW_S16 new_raw_buff[TOUCH_CHANNEL_MAX] = { 0 };

	/*** Add algo main code ***/
	memcpy(new_raw_buff + chx, adc_data, sizeof(AW_S16));

	p_exp_data.filter_flag = SM_ALP_FILTER;
	p_exp_data.channel_max = TOUCH_CHANNEL_MAX;
	p_exp_data.exp_data = EXP_FILTER_EXCEED;

	aw_touch_alg((void*)p_touch_alg_out, new_raw_buff, &p_exp_data);
}

static void aw8686x_alg_process(struct aw8686x_spp_struct *p_spp_s, uint8_t *key_status, uint8_t *key_event, uint8_t chx)
{
	int16_t adc_data[CHANNEL_MAX_NUM] = { 0 };
	int32_t ret = 0;
	int32_t temperature = 0;
	
	UNUSED(p_spp_s);
#ifdef AW_SPP_USED
	static uint8_t last_status[CHANNEL_MAX_NUM] = { 0 };
	uint16_t spp_curve_len = 0;
#endif
	AW_TOUCH_STATUS_S *touch_status_s = NULL;

	ret = aw8686x_adc_temp_data_get(&temperature, adc_data);
	if (ret < 0) {
		AWLOGD("alg err! ret = %d", ret);
		return;
	}

#ifndef AW_DYNAMIC_CALI_CLEAR
	if (g_cali_flag == AW_TRUE) {
		adc_data[chx] += g_adc_data[chx];
	}
#endif
	aw8686x_afe_data_handle(&adc_data[chx], chx);
	aw_get_key_status(p_touch_alg_out, &touch_status_s);
	if (touch_status_s == NULL) {
		AWLOGD("[%s,%d]:touch status struct is err", __func__, __LINE__);
		return;
	}
	key_event[chx] = touch_status_s->key_event[chx];
	key_status[chx] = touch_status_s->key_status[chx];
	if (key_event[chx] != 0) {
		touch_status_s->key_event[chx] = 0;
	}
#ifdef AW_SPP_USED
	spp_curve_len = chx * SPP_CURVE_NUM + p_spp_s->count_curve * AW_SPP_PACK_LEN;
	p_spp_s->valid_buf[spp_curve_len] = (uint8_t)(temperature & ONE_WORD);
	p_spp_s->valid_buf[spp_curve_len + 1] = (uint8_t)(temperature >> AW_BIT8 & ONE_WORD);
	p_spp_s->valid_buf[spp_curve_len + 2] = (uint8_t)((adc_data[chx]) & ONE_WORD);
	p_spp_s->valid_buf[spp_curve_len + 3] = (uint8_t)((adc_data[chx] >> AW_BIT8) & ONE_WORD);
	AWLOGD("%s : key_status = %d, last_status= %d", __func__, key_status[chx], last_status[chx]);
	if (key_status[chx] <= DOUBLE_PRESS) {
		if ((key_status[chx] == 0) && (last_status[chx] != 0)) {
			AWLOGD("%s : event enter", __func__);
			if (last_status[chx] == SIGGLE_PRESS) {
				p_spp_s->valid_buf[SPP_SIGGLE_CURVE + spp_curve_len] = 1;
			} else if (last_status[chx] == DOUBLE_PRESS) {
				p_spp_s->valid_buf[SPP_DOUBLE_CURVE + spp_curve_len] = 1;
			}
		} else if ((key_status[chx] == NO_PRESS) && (last_status[chx] == NO_PRESS)) {
			p_spp_s->valid_buf[SPP_SIGGLE_CURVE + spp_curve_len] = 0;
			p_spp_s->valid_buf[SPP_DOUBLE_CURVE + spp_curve_len] = 0;
		}
	} else if ((key_status[chx] > DOUBLE_PRESS) && (key_status[chx] < LONG_PRESS)) {
		if (last_status[chx] == DOUBLE_PRESS) {
			p_spp_s->valid_buf[SPP_TRIPLE_CURVE + spp_curve_len] = 1;
		} else {
			p_spp_s->valid_buf[SPP_TRIPLE_CURVE + spp_curve_len] = 0;
		}
	} else if (key_status[chx] == LONG_PRESS) {
		if (last_status[chx] != LONG_PRESS) {
			p_spp_s->valid_buf[SPP_LONG_CURVE + spp_curve_len] = 1;
		} else {
			p_spp_s->valid_buf[SPP_LONG_CURVE + spp_curve_len] = 0;
		}
	}
	last_status[chx] = key_status[chx];
#endif
}

void aw8686x_open_irq(void)
{
	aw8686x_i2c_write(REG_ADSR_0, CLEAR_IRQ);
	aw8686x_i2c_write(REG_INTR_MASK, EN_IRQ_1_2);
}

void aw8686x_close_irq(void)
{
	aw8686x_i2c_write(REG_INTR_MASK, DIS);
	aw8686x_i2c_write(REG_ADSR_0, CLEAR_IRQ);
	aw8686x_i2c_write(REG_ADSR_0, EN);
}

void aw8686x_factory_mode(void)
{
	aw_timer0_stop();
	aw8686x_close_irq();
	aw8686x_i2c_write(REG_GO_STDBY_CFG, DIS);
	aw8686x_i2c_write(REG_ADMCR_1, AW8686X_ADST);
	aw8686x_i2c_write(REG_GO_STDBY_CFG, DIS);
}

static void aw8686x_vs_voltage_get(uint8_t *vs_voltage)
{
	uint8_t vs_voltage_mapping = 0;
	uint8_t reg_data = 0;

	aw8686x_i2c_read(REG_ANA_ADJ_CFG, &reg_data);
	vs_voltage_mapping = (reg_data >> AW_BIT4) & TWO_BIT;
	switch (vs_voltage_mapping) {
	case MAPPING_2_4V:
		*vs_voltage = AW_2_4V;
		break;
	case MAPPING_2_8V:
		*vs_voltage = AW_2_8V;
		break;
	case MAPPING_3_0V:
		*vs_voltage = AW_3_0V;
		break;
	case MAPPING_3_1V:
		*vs_voltage = AW_3_1V;
		break;
	default:
		*vs_voltage = AW_3_0V;
		break;
	}
}

static void aw8686x_vref_voltage_get(uint8_t *vref_voltage, uint8_t chx)
{
	uint8_t vref_voltage_mapping = 0;
	uint8_t reg_data = 0;

	aw8686x_i2c_read(REG_ADCH0CR1_2 + chx * ADCH0CR_OFFSET, &reg_data);
	vref_voltage_mapping = (reg_data >> AW_BIT6) & TWO_BIT;
	switch (vref_voltage_mapping) {
	case MAPPING_2_4V:
		*vref_voltage = AW_2_4V;
		break;
	case MAPPING_2_8V:
		*vref_voltage = AW_2_8V;
		break;
	case MAPPING_3_0V:
		*vref_voltage = AW_3_0V;
		break;
	case MAPPING_3_1V:
		*vref_voltage = AW_3_1V;
		break;
	default:
		*vref_voltage = AW_3_0V;
		break;
	}
}

#ifdef AW_FLASH_USED
static int32_t aw8686x_flash_read_val(uint32_t offset)
{
	uint8_t i = 0;
	uint8_t rbuf[AW_NORFLASH_MAX] = { 0 };
	uint16_t check_data_len = 0;
	uint16_t cali_flag = 0;
	uint32_t check_sum_flash = 0;
	uint32_t check_sum_data = 0;

	aw8686x_FTC_para.para_len = sizeof(AW_ALGO_PATAM_T) + sizeof(AW_FTC_PARA_T);

	AWLOGD("%s enter", __func__);
	aw_flash_read(offset, rbuf, aw8686x_FTC_para.para_len);
	memcpy(&check_data_len, rbuf + aw8686x_FTC_para.para_len - sizeof(uint16_t) * 2, sizeof(aw8686x_FTC_para.para_len));
	AWLOGD("%s : check_data_len = %d, para_len = %d", __func__, check_data_len, aw8686x_FTC_para.para_len);
	if (check_data_len != aw8686x_FTC_para.para_len) {
		AWLOGD("%s : check_data_len is not equal para_len", __func__);
		return AW_ERR;
	}
	memcpy(&cali_flag, rbuf + check_data_len - sizeof(aw8686x_FTC_para.cali_flag), sizeof(aw8686x_FTC_para.cali_flag));
	if (cali_flag != AW_CALI) {
		AWLOGD("%s : flash not cali data", __func__);
		return AW_ERR;
	}
	memcpy(&check_sum_flash, rbuf, sizeof(int32_t));
	for (i = 0; i < (sizeof(AW_ALGO_PATAM_T) + sizeof(uint16_t)); i ++) {
		check_sum_data += rbuf[i + sizeof(uint32_t)];
	}
	AWLOGD("%s : check sum data in flash = %d, check_sum data= %d", __func__, check_sum_flash, check_sum_data);
	if (check_sum_data != check_sum_flash) {
		AWLOGD("%s : check sum err", __func__);
		return AW_ERR;
	}
	memcpy(&g_touch_param_alg_lib, rbuf + sizeof(aw8686x_FTC_para.cali_coef), sizeof(AW_ALGO_PATAM_T));
	memcpy(g_aw8686x_s.g_ch_calib_factor_lib, rbuf, sizeof(aw8686x_FTC_para.cali_coef));

	return AW_OK;
}
#endif

static AW_ALGO_PATAM_T *aw_touch_get_param_alg_lib(void)
{
	return &g_touch_param_alg_lib;
}

#ifdef AW_SPP_USED
static int32_t aw_app_data_unpack(uint8_t *in, uint8_t in_len, aw_app_prf_t *out)
{
	uint32_t valid_dat_len = 0;
	uint64_t sum_r = 0;
	uint64_t sum_v = 0;

	if (in[0] != APK_HEADER) {
		AWLOGD("%s : aw frame header error.", __func__);
		return AW_ERR;
	}

	if (in[in_len - 2] != APK_END0 || in[in_len - 1] != APK_END1) {
		AWLOGD("%s : awinic frame end error.", __func__);
		return AW_ERR;
	}

	valid_dat_len = in[2];
	AWLOGD("%s : aw valid_dat_len = %d.", __func__, valid_dat_len);

	for (int i = 0; i < valid_dat_len; i++) {
		sum_v += (long)in[i + 3];
	}
	sum_r = (uint64_t)(((uint64_t)in[APK_VALID_DATA_HEASER + valid_dat_len]) |
			((uint64_t)in[APK_VALID_DATA_HEASER + valid_dat_len + 1]) << AW_BIT8 |
			((uint64_t)in[APK_VALID_DATA_HEASER + valid_dat_len + 2]) << AW_BIT16 |
			((uint64_t)in[APK_VALID_DATA_HEASER + valid_dat_len + 3]) << AW_BIT24 |
			((uint64_t)in[APK_VALID_DATA_HEASER + valid_dat_len + 4]) << AW_BIT32 );
	if (sum_r != sum_v) {
		AWLOGD("%s : aw sum_r = 0x%lld  sum_v = 0x%lld.", __func__, sum_r, sum_v);
		return AW_ERR;
	}
	out->cmd = in[1];
	out->len = valid_dat_len;
	memcpy((void *)out->dat, (void *)&in[APK_VALID_DATA_HEASER], valid_dat_len);

	return AW_OK;
}

static void aw_app_data_send(uint8_t cmd, uint8_t *buf, uint32_t length)
{
	uint64_t sum = 0;
	uint8_t send_total_buf[DATA_MAX_LEN] = { 0 };

	send_total_buf[0] = APK_HEADER;
	send_total_buf[1] = cmd;
	send_total_buf[2] = length;

	for (int i = 0; i < length; i++) {
		sum += buf[i];
		send_total_buf[APK_VALID_DATA_HEASER + i] = buf[i];
	}
	send_total_buf[APK_VALID_DATA_HEASER + length] = (byte)(sum & ONE_WORD);
	send_total_buf[APK_VALID_DATA_HEASER + length + 1] = (byte)((sum >> AW_BIT8) & ONE_WORD);
	send_total_buf[APK_VALID_DATA_HEASER + length + 2] = (byte)((sum >> AW_BIT16) & ONE_WORD);
	send_total_buf[APK_VALID_DATA_HEASER + length + 3] = (byte)((sum >> AW_BIT24) & ONE_WORD);
	send_total_buf[APK_VALID_DATA_HEASER + length + 4] = (byte)((sum >> AW_BIT32) & ONE_WORD);
	send_total_buf[APK_VALID_DATA_HEASER + length + 5] = APK_END0;
	send_total_buf[APK_VALID_DATA_HEASER + length + 6] = APK_END1;

	length += SPP_HEADER_LEN;
	aw_spp_write(send_total_buf, (uint16_t)length);
}

static void aw_app_set_reg(aw_app_prf_t app_data)
{
	uint8_t ret = 0;

	AWLOGD("%s : addr = 0x%x, data = 0x%x", __func__, app_data.dat[0], app_data.dat[1]);
	aw8686x_i2c_write(app_data.dat[0], app_data.dat[1]);

	aw_app_data_send(AW_APP_CMD_WRITE_REG, &ret, sizeof(ret));
}

static void aw_app_get_reg(aw_app_prf_t app_data)
{
	uint8_t valid_buf[APP_REG_MAX] = { 0 };

	if (app_data.dat[1] > APP_REG_MAX) {
		app_data.dat[1] = APP_REG_MAX;
	}
	aw8686x_i2c_read_seq(app_data.dat[0], valid_buf, app_data.dat[1]);
	aw_app_data_send(AW_APP_CMD_READ_REG, valid_buf, app_data.dat[1]);
}

static void aw_app_send_params(void)
{
	uint8_t valid_buf[ALG_PARA_TOTAL] = { 0 };
	AW_ALGO_PATAM_T *p_touch_param_alg = aw_touch_get_param_alg_lib();

	if (p_touch_param_alg != AW_NULL) {
		AWLOGD("%s : count_long = %d", __func__, p_touch_param_alg->count_long);
		valid_buf[CNT_LONG_SPP] = (uint8_t)(p_touch_param_alg->count_long & ONE_WORD);
		valid_buf[CNT_LONG_SPP + 1] = (uint8_t)((p_touch_param_alg->count_long >> AW_BIT8)& ONE_WORD);

		valid_buf[CNT_DOUBLE_SPP] = (uint8_t)(p_touch_param_alg->count_double & ONE_WORD);
		valid_buf[CNT_DOUBLE_SPP + 1] = (uint8_t)((p_touch_param_alg->count_double >> AW_BIT8)& ONE_WORD);

		valid_buf[DATA_SMOOTH_SPP] = (uint8_t)(p_touch_param_alg->data_smooth_range & ONE_WORD);
		valid_buf[DATA_SMOOTH_SPP + 1] = (uint8_t)((p_touch_param_alg->data_smooth_range >> AW_BIT8)& ONE_WORD);

		valid_buf[K_SMOOTH_SPP] = (uint8_t)(p_touch_param_alg->k_smooth_range & ONE_WORD);
		valid_buf[K_SMOOTH_SPP + 1] = (uint8_t)((p_touch_param_alg->k_smooth_range >> AW_BIT8)& ONE_WORD);

		valid_buf[BESE_UP_SPP] = (uint8_t)(p_touch_param_alg->baseline_up & ONE_WORD);
		valid_buf[BESE_UP_SPP + 1] = (uint8_t)((p_touch_param_alg->baseline_up >> AW_BIT8)& ONE_WORD);

		valid_buf[START_SILL_SPP] = (uint8_t)(p_touch_param_alg->start_sill & ONE_WORD);
		valid_buf[START_SILL_SPP + 1] = (uint8_t)((p_touch_param_alg->start_sill >> AW_BIT8)& ONE_WORD);

		valid_buf[STOP_SILL_SPP] = (uint8_t)(p_touch_param_alg->stop_sill & ONE_WORD);
		valid_buf[STOP_SILL_SPP + 1] = (uint8_t)((p_touch_param_alg->stop_sill >> AW_BIT8)& ONE_WORD);

		valid_buf[K_SILL_SPP] = (uint8_t)(p_touch_param_alg->k_sill & ONE_WORD);
		valid_buf[K_SILL_SPP + 1] = (uint8_t)((p_touch_param_alg->k_sill >> AW_BIT8)& ONE_WORD);

		valid_buf[HEAVY_SILL_SPP] = (uint8_t)(p_touch_param_alg->touch_heavy_sill & ONE_WORD);
		valid_buf[HEAVY_SILL_SPP + 1] = (uint8_t)((p_touch_param_alg->touch_heavy_sill >> AW_BIT8)& ONE_WORD);

		valid_buf[REPLY_CNT_SPP] = (uint8_t)(p_touch_param_alg->touch_heavy_reply_count & ONE_WORD);
		valid_buf[REPLY_CNT_SPP + 1] = (uint8_t)((p_touch_param_alg->touch_heavy_reply_count >> AW_BIT8)& ONE_WORD);

		valid_buf[TIMEOUT_SPP] = (uint8_t)(p_touch_param_alg->timeout_updata_baseline & ONE_WORD);
		valid_buf[TIMEOUT_SPP + 1] = (uint8_t)((p_touch_param_alg->timeout_updata_baseline >> AW_BIT8)& ONE_WORD);
		valid_buf[TIMEOUT_SPP + 2] = (uint8_t)((p_touch_param_alg->timeout_updata_baseline >> AW_BIT16)& ONE_WORD);
		valid_buf[TIMEOUT_SPP + 3] = (uint8_t)((p_touch_param_alg->timeout_updata_baseline >> AW_BIT24)& ONE_WORD);

		valid_buf[ST_LEN_SPP] = (uint8_t)(p_touch_param_alg->touch_st_cnt_len & ONE_WORD);
		valid_buf[ST_LEN_SPP + 1] = (uint8_t)((p_touch_param_alg->touch_st_cnt_len >> AW_BIT8)& ONE_WORD);

		valid_buf[FTC_COEF_SPP] = (uint8_t)(g_aw8686x_s.g_ch_calib_factor_lib[0] & ONE_WORD);
		valid_buf[FTC_COEF_SPP + 1] = (uint8_t)((g_aw8686x_s.g_ch_calib_factor_lib[0] >> AW_BIT8)& ONE_WORD);

		valid_buf[NOISE_PT_SPP] = (uint8_t)(p_touch_param_alg->noise_pt & ONE_WORD);
		valid_buf[NOISE_PT_SPP + 1] = (uint8_t)((p_touch_param_alg->noise_pt >> AW_BIT8)& ONE_WORD);
	}
	aw_app_data_send(AW_APP_CMD_GET_ALGO_PARA, valid_buf, ALG_PARA_TOTAL);
}

static void aw_app_set_params(aw_app_prf_t app_data)
{
	uint8_t buf = 0;
	AW_ALGO_PATAM_T *p_touch_param_alg = aw_touch_get_param_alg_lib();

	if (p_touch_param_alg != NULL) {
		p_touch_param_alg->count_long =(uint16_t)((((uint16_t)app_data.dat[CNT_LONG_SPP + 1])
							<< AW_BIT8) | app_data.dat[CNT_LONG_SPP]);
		p_touch_param_alg->count_double = (uint16_t)((((uint16_t)app_data.dat[CNT_DOUBLE_SPP + 1])
							<< AW_BIT8) | app_data.dat[CNT_DOUBLE_SPP]);
		p_touch_param_alg->data_smooth_range = (uint16_t)((((uint16_t)app_data.dat[DATA_SMOOTH_SPP + 1])
							<< AW_BIT8) | app_data.dat[DATA_SMOOTH_SPP]);
		p_touch_param_alg->k_smooth_range = (uint16_t)((((uint16_t)app_data.dat[K_SMOOTH_SPP + 1])
							<< AW_BIT8) | app_data.dat[K_SMOOTH_SPP]);
		p_touch_param_alg->baseline_up = (uint16_t)((((uint16_t)app_data.dat[BESE_UP_SPP + 1])
							<< AW_BIT8) | app_data.dat[BESE_UP_SPP]);
		p_touch_param_alg->start_sill = (uint16_t)((((uint16_t)app_data.dat[START_SILL_SPP + 1])
							<< AW_BIT8) | app_data.dat[START_SILL_SPP]);
		p_touch_param_alg->stop_sill = (uint16_t)((((uint16_t)app_data.dat[STOP_SILL_SPP + 1])
							<< AW_BIT8) | app_data.dat[STOP_SILL_SPP]);
		p_touch_param_alg->k_sill = (uint16_t)((((uint16_t)app_data.dat[K_SILL_SPP + 1])
							<< AW_BIT8) | app_data.dat[K_SILL_SPP]);
		p_touch_param_alg->touch_heavy_sill = (uint16_t)((((uint16_t)app_data.dat[HEAVY_SILL_SPP + 1])
							<< AW_BIT8) | app_data.dat[HEAVY_SILL_SPP]);
		p_touch_param_alg->touch_heavy_reply_count = (uint16_t)((((uint16_t)app_data.dat[REPLY_CNT_SPP + 1])
							<< AW_BIT8) | app_data.dat[REPLY_CNT_SPP]);
		p_touch_param_alg->timeout_updata_baseline = (uint32_t)((((uint32_t)app_data.dat[TIMEOUT_SPP + 3])
							<< AW_BIT24) | (((uint32_t)app_data.dat[TIMEOUT_SPP + 2])
							<< AW_BIT16) | (((uint32_t)app_data.dat[TIMEOUT_SPP + 1])
							<< AW_BIT8) | app_data.dat[TIMEOUT_SPP]);
		p_touch_param_alg->touch_st_cnt_len = (uint16_t)((((uint16_t)app_data.dat[ST_LEN_SPP + 1])
							<< AW_BIT8) | app_data.dat[ST_LEN_SPP]);
		g_aw8686x_s.g_ch_calib_factor_lib[0] = (uint16_t)((((uint16_t)app_data.dat[FTC_COEF_SPP + 1])
							<< AW_BIT8) | app_data.dat[FTC_COEF_SPP]);
		p_touch_param_alg->noise_pt = (uint16_t)((((uint16_t)app_data.dat[NOISE_PT_SPP + 1])
							<< AW_BIT8) | app_data.dat[NOISE_PT_SPP]);
		aw_alg_param_init(p_touch_alg_out, aw_touch_get_param_alg_lib());
		aw_alg_ch_calib_coef_init(p_touch_alg_out, g_aw8686x_s.g_ch_calib_factor_lib);
		aw_touch_alg_clear(p_touch_alg_out);
	} else {
		AWLOGD("%s : set params err", __func__);
	}
	aw_app_data_send(AW_APP_CMD_SET_ALGO_PARA, &buf, sizeof(buf));
}

static void aw8686x_app_send_dev_info(void)
{
	uint8_t temp_buf[6] = { 0 };
	temp_buf[0] = 1;
	temp_buf[1] = AW_BIT8;
	temp_buf[2] = AW_BIT8;
	temp_buf[3] = 100;
	temp_buf[4] = PACKS_IN_ONE_COMM;
	temp_buf[5] = AW_BIT16;
	aw_app_data_send(AW_APP_CMD_GET_DEVICE_INFO, temp_buf, sizeof(temp_buf));
}

static void aw_app_get_cc(uint8_t *dat, uint16_t len)
{
	uint8_t ch_num = 0;
	uint8_t channel_en[CHANNEL_MAX_NUM] = { 0 };

	if (dat[1] < OFFSET) {
		AWLOGD("%s : err cmd", __func__);
	} else if ((dat[1] >= OFFSET) && (dat[1] < DYNAMIC_CALI)) {
		aw8686x_FTC(dat[1]);
	} else if (dat[1] == DYNAMIC_CALI) {
		AWLOGD("%s : enter, g_status = %d", __func__, g_aw8686x_s.ch_status);
		for (ch_num = AW_LOOP_START; ch_num < CHANNEL_MAX_NUM; ch_num++) {
			channel_en[ch_num] = (g_aw8686x_s.ch_status >> (AW_BIT0 + ch_num)) & ONE_BIT;
			if (channel_en[ch_num] == EN) {
				AWLOGD("%s : start to cali", __func__);
				aw8686x_dynamic_cali(ch_num);
			}
		}
	}
}

void aw8686x_display_adc_data_always()
{
	uint8_t ch_num = 0;
	uint8_t channel_en[CHANNEL_MAX_NUM] = { 0 };
	uint8_t key_status[CHANNEL_MAX_NUM] = { 0 };
	uint8_t key_event[CHANNEL_MAX_NUM] = { 0 };
	struct aw8686x_spp_struct spp_s;

	memset(&spp_s.valid_buf, AW_ALG_BUFF_CLEAR, sizeof(spp_s.valid_buf));
	aw8686x_factory_mode();
	while (spp_s.count_curve <= 4) {
		for (ch_num = AW_LOOP_START; ch_num < CHANNEL_MAX_NUM; ch_num++) {
			channel_en[ch_num] = (g_aw8686x_s.ch_status >> (AW_BIT0 + ch_num)) & ONE_BIT;
			if (channel_en[ch_num] == EN) {
				aw8686x_alg_process(&spp_s, key_status, key_event, ch_num);
			}
		}
		spp_s.count_curve++;
	}
	spp_s.count_curve = 0;
	aw_app_data_send(1, spp_s.valid_buf, AW_SPP_PACK_LEN * PACKS_IN_ONE_COMM);
}
#endif

static void aw8686x_pga_magnification_get(uint16_t *pga_mag, uint16_t *pga1_mag, uint8_t chx)
{
	uint8_t pga_data = 0;
	uint8_t pga2_data = 0;
	uint8_t pga1_data = 0;

	aw8686x_i2c_read(REG_ADCH0CR1_0 + chx * CHANNEL_OFFSET, &pga_data);
	pga2_data = pga_data & THREE_BIT;
	pga1_data = (pga_data >> AW_BIT4) & THREE_BIT;
	switch(pga1_data) {
	case PGA1_MAG_1:
		pga1_mag[chx] = PGA1_MAG_1_MAP;
		break;
	case PGA1_MAG_16:
		pga1_mag[chx] = PGA1_MAG_16_MAP;
		break;
	case PGA1_MAG_32:
		pga1_mag[chx] = PGA1_MAG_32_MAP;
		break;
	case PGA1_MAG_64:
		pga1_mag[chx] = PGA1_MAG_64_MAP;
		break;
	case PGA1_MAG_128:
		pga1_mag[chx] = PGA1_MAG_128_MAP;
		break;
	case PGA1_MAG_256:
		pga1_mag[chx] = PGA1_MAG_256_MAP;
		break;
	default:
		pga1_mag[chx] = PGA1_MAG_256_MAP;
		break;
	}
	pga_mag[chx] = (pga2_data + 1) * pga1_mag[chx];
	AWLOGD("ch[%d]pga mag = %d", chx, pga_mag[chx]);
}

static uint32_t aw8686x_power_of_2(AW_BIT_NUM_T aw_bit_num_flag)
{
	uint8_t i = 0;
	uint32_t result_power = 1;

	if (aw_bit_num_flag < AW_BIT_NUM_MAX) {
		for (i = 0; i < aw_bit_num_flag; i++) {
			result_power *= BASE_NUM;
		}
		AWLOGD("the result_power = %d", result_power);
	} else {
		AWLOGD("err power");
	}

	return result_power;
}

static void aw8686x_adc_transfer_vol_coef(uint32_t *adc_coef, uint8_t chx)
{
	uint8_t vref = 0;
	uint8_t signal_mode = 0;
	uint32_t result_power = 0;
	uint16_t pga_mag[CHANNEL_MAX_NUM] = { 0 };
	uint16_t pga1_mag[CHANNEL_MAX_NUM] = { 0 };

	aw8686x_vref_voltage_get(&vref, chx);
	AWLOGD("vref = %d", vref);
	aw8686x_pga_magnification_get(pga_mag, pga1_mag, chx);
	AWLOGD("adc pga_mag[%d] = %d", chx, pga_mag[chx]);
	aw8686x_i2c_read(REG_ADCH0CR0_1 + chx * CHANNEL_OFFSET, &signal_mode);
	if (((signal_mode >> AW_BIT4) & ONE_BIT) == EN) {
		result_power = aw8686x_power_of_2(ADC_BITS_NUM - 1);
	} else {
		result_power = aw8686x_power_of_2(ADC_BITS_NUM);
	}
	adc_coef[chx] = (uint32_t)vref * V_TRANS_UV / AW_TEMP_MAP_10 / (result_power * pga_mag[chx]);
	AWLOGD("adc_coef[%d] = %d", chx, adc_coef[chx]);
}

static void aw8686x_dac_transfer_vol_coef(uint32_t *dac_coef, uint8_t chx)
{
	uint8_t vs = 0;
	uint32_t result_power = 0;
	uint16_t pga_mag[CHANNEL_MAX_NUM] = { 0 };
	uint16_t pga1_mag[CHANNEL_MAX_NUM] = { 0 };
	uint32_t v_dac[CHANNEL_MAX_NUM] = { 0 };

	aw8686x_vs_voltage_get(&vs);
	AWLOGD("vs = %d", vs);
	aw8686x_pga_magnification_get(pga_mag, pga1_mag, chx);
	if (pga1_mag[chx] <= PGA1_MAG_64_MAP) {
		v_dac[chx] = DAC_V * vs / RES_SEG;
	} else if (pga1_mag[chx] == PGA1_MAG_128_MAP) {
		v_dac[chx] = (DAC_V * vs / RES_SEG) / IMPEDENT1;
	} else if (pga1_mag[chx] == PGA1_MAG_256_MAP){
		v_dac[chx] = (DAC_V * vs / RES_SEG) / IMPEDENT2;
	} else {
		v_dac[chx] = 0;
		AWLOGD("err pga");
		result_power = AW_ERR;
		return;
	}
	AWLOGD("v_dac[%d] = %d", chx, v_dac[chx]);
	result_power = aw8686x_power_of_2(DAC_BITS_NUM - 1);
	AWLOGD("2^n = %d", result_power);

	dac_coef[chx] = v_dac[chx] * V_TRANS_MV / result_power;
	AWLOGD("dac_coef[%d] = %d", chx, dac_coef[chx]);
}

static void aw8686x_FTC_offset(uint8_t chx)
{
	uint8_t dac_data_raw[CHANNEL_MAX_NUM * REGADDR_REGVAL] = { 0 };
	int16_t dac_data[CHANNEL_MAX_NUM] = { 0 };
	uint32_t dac_coef[CHANNEL_MAX_NUM] = { 0 };
#ifdef AW_SPP_USED
	uint8_t i = 0;
	uint8_t valid_buf[4] = { 0 };
#endif

	aw8686x_dac_transfer_vol_coef(dac_coef, chx);
	aw8686x_i2c_read_seq(REG_DACH0DR_0 + chx * CHANNEL_OFFSET, &dac_data_raw[chx * REGADDR_REGVAL], REGADDR_REGVAL);
	AWLOGD("dac_dat_l[%d] = %d", chx, dac_data_raw[chx * REGADDR_REGVAL]);
	AWLOGD("dac_dat_h[%d] = %d", chx, dac_data_raw[chx * REGADDR_REGVAL + 1]);
	if (((dac_data_raw[chx * REGADDR_REGVAL + 1] >> AW_BIT2) && ONE_BIT) == EN) {
		dac_data_raw[chx * REGADDR_REGVAL + 1] &= (ONE_WORD - AW_BIT5);
		AWLOGD("dac_data_raw[%d] = %d", chx, dac_data_raw[chx * REGADDR_REGVAL + 1]);
		dac_data[chx] = -((int16_t)((uint16_t)dac_data_raw[chx * REGADDR_REGVAL] | ((uint16_t)dac_data_raw[chx * REGADDR_REGVAL + 1] << AW_BIT8)));
	} else {
		dac_data[chx] = ((int16_t)((uint16_t)dac_data_raw[chx * REGADDR_REGVAL] | ((uint16_t)dac_data_raw[chx * REGADDR_REGVAL + 1] << AW_BIT8)));
	}
	AWLOGD("dac_data[%d] = %d", chx, dac_data[chx]);
	aw8686x_FTC_data[chx].offset_uv = dac_data[chx] * dac_coef[chx];
	AWLOGD("offset_uv[%d] = %d", chx, aw8686x_FTC_data[chx].offset_uv);
#ifdef AW_SPP_USED
	for (i = 0; i < 4; i++) {
		valid_buf[i] = (uint8_t)(((uint32_t)aw8686x_FTC_data[chx].offset_uv >> (i * AW_BIT8)) & ONE_WORD);
	}
	aw_app_data_send(OFFSET, valid_buf, sizeof(valid_buf));
#endif
}

/*
[2]aw8686x_FTC_data[chx].adc_peak

*/

static void aw8686x_FTC_noise_sta(uint8_t chx)
{
	int16_t min_adc_data = 0;
	int16_t max_adc_data = 0;
	uint32_t i = 0;
	int16_t max_diff_data[CHANNEL_MAX_NUM] = { 0 };
	int16_t min_diff_data[CHANNEL_MAX_NUM] = { 0 };
	uint8_t adc_code[REGADDR_REGVAL * CHANNEL_MAX_NUM] = { 0 };
	int16_t adc_data[CHANNEL_MAX_NUM] = { 0 };
	int16_t adc_diff[CHANNEL_MAX_NUM] = { 0 };
	uint32_t adc_coef[CHANNEL_MAX_NUM] = { 0 };
	uint32_t sum[CHANNEL_MAX_NUM] = { 0 };
	double sqrt_ret[CHANNEL_MAX_NUM] = { 0 };
#ifdef AW_SPP_USED
	uint8_t valid_buf[15] = { 0 };
#endif

	AWLOGD("[%s,%d]: enter", __func__, __LINE__);

	aw8686x_adc_transfer_vol_coef(adc_coef, chx);

	for (i = 0; i < AW_NOISE_DATA_NUM; i++) {
		aw8686x_data_get(adc_code, adc_data, chx);
		aw8686x_FTC_data[chx].adc_data[i] = adc_data[chx];
		AWLOGD("ch = %d, 8686x-adc-data = %d", chx, aw8686x_FTC_data[chx].adc_data[i]);
		if (i == 0) {
			max_adc_data = aw8686x_FTC_data[chx].adc_data[i];
			min_adc_data = aw8686x_FTC_data[chx].adc_data[i];
		} else {
			aw8686x_FTC_data[chx].diff_data[i - 1] = (aw8686x_FTC_data[chx].adc_data[i] - aw8686x_FTC_data[chx].adc_data[i - 1]);
			AWLOGD("ch = %d, 8686x-1diff-data = %d", chx, aw8686x_FTC_data[chx].diff_data[i-1]);
			if (i == 1) {
				max_diff_data[chx] = aw8686x_FTC_data[chx].diff_data[0];
				min_diff_data[chx] = aw8686x_FTC_data[chx].diff_data[0];
			}
			if (max_adc_data < aw8686x_FTC_data[chx].adc_data[i]) {
				max_adc_data = aw8686x_FTC_data[chx].adc_data[i];
			}
			if (min_adc_data > aw8686x_FTC_data[chx].adc_data[i]) {
				min_adc_data = aw8686x_FTC_data[chx].adc_data[i];
			}
			if (max_diff_data[chx] < aw8686x_FTC_data[chx].diff_data[i - 1]) {
				max_diff_data[chx] = aw8686x_FTC_data[chx].diff_data[i - 1];
			}
			if (min_diff_data[chx] > aw8686x_FTC_data[chx].diff_data[i - 1]) {
				min_diff_data[chx] = aw8686x_FTC_data[chx].diff_data[i - 1];
			}
			sum[chx] += (aw8686x_FTC_data[chx].diff_data[i - 1] * aw8686x_FTC_data[chx].diff_data[i - 1]);
		}
		aw_delay_ms(10);
	}

	aw8686x_FTC_data[chx].adc_peak = adc_diff[chx] = max_adc_data - min_adc_data;
	g_touch_param_alg_lib.noise_pt = (uint16_t)aw8686x_FTC_data[chx].adc_peak;
	AWLOGD("ch = %d, max data = %d, min_data = %d, adc_diff = %d", chx, max_adc_data, min_adc_data, adc_diff[chx] * adc_coef[chx]);
#ifdef AW_SPP_USED
	for (i = 0; i < 4; i++) {
		valid_buf[i] = (uint8_t)(((adc_diff[chx] * adc_coef[chx]) >> (i * AW_BIT8)) & ONE_WORD);
	}
	valid_buf[i] = 0;
#endif
	if (min_diff_data[chx] < 0) {
		if (-min_diff_data[chx] > max_diff_data[chx]) {
			max_diff_data[chx] = -min_diff_data[chx];
		}
	}
	AWLOGD("ch = %d, max diff = %d, min_diff_data = %d", chx, max_diff_data[chx] * adc_coef[chx], min_diff_data[chx]);
#ifdef AW_SPP_USED
	for (i = 5; i < 9; i++) {
		valid_buf[i] = (uint8_t)(((max_diff_data[chx] * adc_coef[chx]) >> ((i - 5) * AW_BIT8)) & ONE_WORD);
	}
	valid_buf[i] = 0;
#endif
	sqrt_ret[chx] = sqrt((((float)sum[chx]) / AW_RAW_DATA_NUM));
	AWLOGD("ch = %d, sum = %d", chx, sum[chx]);
	AWLOGD("ch = %d, sqrt = %d", chx, (uint32_t)(sqrt_ret[chx] * adc_coef[chx]));
#ifdef AW_SPP_USED
	for (i = 10; i < 14; i++) {
		valid_buf[i] = (uint8_t)(((uint32_t)(sqrt_ret[chx] * adc_coef[chx]) >> ((i - 10) * AW_BIT8)) & ONE_WORD);
	}
	valid_buf[i] = 0;
	aw_app_data_send(NOISE, valid_buf, sizeof(valid_buf));
#endif
}

static void aw8686x_raw_data_get(int32_t *raw_data, uint8_t chx)
{
	int32_t sum_data = 0;
	uint32_t i = 0;
	uint8_t adc_code[REGADDR_REGVAL * CHANNEL_MAX_NUM] = { 0 };
	int16_t adc_data[CHANNEL_MAX_NUM] = { 0 };

	for (i = 0; i < AW_RAW_DATA_NUM; i++) {
		aw8686x_data_get(adc_code, adc_data, chx);
		sum_data += adc_data[chx];
		aw_delay_ms(10);
	}
	*raw_data = sum_data / AW_RAW_DATA_NUM;
	AWLOGD("ch = %d, sum_data = %d ", chx, sum_data);
	AWLOGD("ch = %d, raw = %d ", chx, *raw_data);
}

static void aw8686x_FTC_signal(uint8_t chx)
{
	uint32_t adc_coef[CHANNEL_MAX_NUM] = { 0 };
#ifdef AW_FLASH_USED
	uint8_t i = 0;
	uint8_t FTC_data_len = 0;
	uint8_t FTC_data_len0 = 0;
	uint16_t FTC_data_len1 = 0;
	uint8_t wbuf[AW_NORFLASH_MAX] = { 0 };
#endif
#ifdef AW_SPP_USED
	uint8_t valid_buf[4] = { 0 };
#endif

	aw8686x_adc_transfer_vol_coef(adc_coef, chx);

	aw8686x_raw_data_get(&aw8686x_FTC_data[chx].raw_data1, chx);
	aw8686x_FTC_data[chx].signal = (aw8686x_FTC_data[chx].raw_data1 - aw8686x_FTC_data[chx].raw_data0) * adc_coef[chx];
	AWLOGD("ch = %d, signal = %d ", chx, aw8686x_FTC_data[chx].signal);
#ifdef AW_SPP_USED
	memcpy(valid_buf, &(aw8686x_FTC_data[chx].signal), 4);
	aw_app_data_send(SIGNAL_RAW_1, valid_buf, 4);
#endif
	if (aw8686x_FTC_data[chx].signal == 0) {
		AWLOGD("err set FTC_signal");
		return;
	}
	aw8686x_FTC_para.cali_coef[chx] = (WEIGHT_G * W_MAG) / (aw8686x_FTC_data[chx].raw_data1 - aw8686x_FTC_data[chx].raw_data0);
	AWLOGD("ch = %d, cali_coef = %d ", chx, aw8686x_FTC_para.cali_coef[chx]);
	aw8686x_FTC_para.cali_flag = AW_CALI;

#ifdef AW_FLASH_USED
	FTC_data_len = sizeof(aw8686x_FTC_para.check_sum);
	FTC_data_len0 = sizeof(aw8686x_FTC_para.cali_coef);
	AWLOGD("FTC data len0 = %d ", FTC_data_len0);
	memcpy(wbuf + FTC_data_len, &aw8686x_FTC_para.cali_coef, FTC_data_len0);
	FTC_data_len1 = FTC_data_len0 + sizeof(AW_ALGO_PATAM_T);
	AWLOGD("FTC data len1 = %d ", FTC_data_len1);
	memcpy(wbuf + FTC_data_len0 + FTC_data_len, &g_touch_param_alg_lib, sizeof(AW_ALGO_PATAM_T));
	aw8686x_FTC_para.para_len = FTC_data_len1 + sizeof(AW_FTC_PARA_T) - FTC_data_len0;
	memcpy(wbuf + aw8686x_FTC_para.para_len - sizeof(uint16_t) * 2, &aw8686x_FTC_para.para_len, sizeof(uint16_t));
	for (i = 0; i < FTC_data_len1; i++) {
		aw8686x_FTC_para.check_sum += wbuf[i + FTC_data_len];
	}
	memcpy(wbuf, &aw8686x_FTC_para.check_sum, sizeof(uint32_t));
	memcpy(wbuf + aw8686x_FTC_para.para_len - sizeof(uint16_t), &aw8686x_FTC_para.cali_flag, sizeof(uint16_t));
	aw_flash_write(AW_NORFLASH_INIT_ADD, wbuf, aw8686x_FTC_para.para_len);
	aw8686x_flash_read_val(AW_NORFLASH_INIT_ADD);
#endif
}

static void aw8686x_alg_verify_signal(uint8_t chx)
{
	uint8_t adc_code[REGADDR_REGVAL * CHANNEL_MAX_NUM] = { 0 };
	int16_t adc_data[CHANNEL_MAX_NUM] = { 0 };
	uint32_t i = 0;
	int32_t mass_weight[CHANNEL_MAX_NUM] = { 0 };
	int16_t diff_data[CHANNEL_MAX_NUM] = { 0 };
#ifdef AW_SPP_USED
	uint8_t valid_buf[4] = { 0 };
#endif
	AW_TOUCH_STATUS_S *touch_status_s = NULL;

	aw_get_key_status(p_touch_alg_out, &touch_status_s);
	if (touch_status_s == NULL) {
		AWLOGD("%s : touch alg status err", __func__);
		return;
	}
	while (touch_status_s->key_status[chx] == 0) {
		AWLOGD("%s : status = %d\n", __func__, touch_status_s->key_status[chx]);
		aw8686x_data_get(adc_code, adc_data, chx);
		aw8686x_afe_data_handle(&adc_data[chx], chx);
		aw_get_key_status(p_touch_alg_out, &touch_status_s);
		aw_delay_ms(AW_TIMER_DELAY);
	}
	for (i = AW_LOOP_START; i < 100; i++) {
		aw8686x_data_get(adc_code, adc_data, chx);
		aw8686x_afe_data_handle(&adc_data[chx], chx);
		aw_delay_ms(AW_TIMER_DELAY);
	}
	aw_get_diff_chx(p_touch_alg_out, chx, &diff_data[chx]);
	mass_weight[chx] = diff_data[chx] * aw8686x_FTC_para.cali_coef[chx];
	AWLOGD("%s : chx = %d, diff = %d\n", __func__, chx, diff_data[chx]);
	AWLOGD("%s : chx = %d, cali = %d\n", __func__, chx, aw8686x_FTC_para.cali_coef[chx]);
	AWLOGD("%s : chx = %d, mass_weight = %d\n", __func__, chx, mass_weight[chx]);
#ifdef AW_SPP_USED
	memcpy(valid_buf, &mass_weight[chx], sizeof(valid_buf));
	aw_app_data_send(VERIFY_1, valid_buf, sizeof(valid_buf));
#endif
}

static void aw8686x_after_signal_give_data_alg(uint8_t chx)
{
	uint32_t i = 0;
	uint8_t adc_code[REGADDR_REGVAL * CHANNEL_MAX_NUM] = { 0 };
	int16_t adc_data[CHANNEL_MAX_NUM] = { 0 };
	AW_ALGO_PATAM_T *p_touch_param_alg = aw_touch_get_param_alg_lib();

	if (aw8686x_FTC_para.cali_flag != AW_CALI) {
		AWLOGD("%s : not cali, unsupport!", __func__);
		return;
	}
	aw_touch_alg_clear(p_touch_alg_out);
	if (p_touch_param_alg != AW_NULL) {
		p_touch_param_alg->noise_pt = aw8686x_FTC_data[chx].adc_peak;
		g_aw8686x_s.g_ch_calib_factor_lib[chx] = aw8686x_FTC_para.cali_coef[chx];
	}
	for (i = AW_LOOP_START; i < GIVE_ALGO_DATA_TIME; i++) {
		aw8686x_data_get(adc_code, adc_data, chx);
		aw8686x_afe_data_handle(&adc_data[chx], chx);
		aw_delay_ms(AW_TIMER_DELAY);
	}
}

void aw8686x_FTC(CALI_FLAG_T flag)
{
	uint8_t ch_num = 0;
	uint8_t channel_en[CHANNEL_MAX_NUM] = { 0 };

	aw8686x_factory_mode();
	for (ch_num = AW_LOOP_START; ch_num < CHANNEL_MAX_NUM; ch_num++) {
		channel_en[ch_num] = (g_aw8686x_s.ch_status >> (AW_BIT0 + ch_num)) & ONE_BIT;
		if (channel_en[ch_num] == EN) {
			if (flag == OFFSET) {
				aw8686x_FTC_offset(ch_num);
			} else if (flag == NOISE) {
				aw8686x_FTC_noise_sta(ch_num);
			} else if (flag == SIGNAL_RAW_0) {
				aw8686x_raw_data_get(&aw8686x_FTC_data[ch_num].raw_data0, ch_num);
			} else if (flag == SIGNAL_RAW_1) {
				aw8686x_FTC_signal(ch_num);
			} else if (flag == VERIFY_0) {
				aw8686x_after_signal_give_data_alg(ch_num);
			} else if (flag == VERIFY_1) {
				AWLOGD("%s : cali sta", __func__);
				aw8686x_alg_verify_signal(ch_num);
			}
		}
	}
}

#ifdef AW_SPP_USED
void aw8686x_spp_cmd_handler(uint8_t *dat, uint16_t len)
{
	uint32_t i = 0;
	aw_app_prf_t app_data;
	for (i = 0; i < len; i++) {
		AWLOGD("%s : dat[%d] = 0x%02x", __func__, i, dat[i]);
	}

	if (aw_app_data_unpack(dat, len, &app_data)) {
		return;
	}

	switch (app_data.cmd) {
	case AW_APP_CMD_GET_DEVICE_INFO:
		AWLOGD("%s : aw get dev info", __func__);
		aw8686x_app_send_dev_info();
		break;
	case AW_APP_CMD_GET_CURVE_DATA:
		AWLOGD("%s : aw get curve info", __func__);
		aw8686x_display_adc_data_always();
		break;
	case AW_APP_CMD_READ_REG:
		AWLOGD("%s : aw get reg", __func__);
		aw_app_get_reg(app_data);
		break;
	case AW_APP_CMD_WRITE_REG:
		AWLOGD("%s : aw get curve info", __func__);
		aw_app_set_reg(app_data);
		break;
	case AW_APP_CMD_GET_ALGO_PARA:
		AWLOGD("%s : aw get param info", __func__);
		aw_app_send_params();
		break;
	case AW_APP_CMD_SET_ALGO_PARA:
		AWLOGD("%s : aw set param info", __func__);
		aw_app_set_params(app_data);
		break;
	default:
		aw_app_get_cc(dat, len);
		break;
	}
}
#endif

static uint32_t aw8686x_irq_threshold_set(int16_t p_thresh, int16_t n_thresh)
{
	int32_t ret = 0;
	int16_t p_thresh_temp = p_thresh + ADC_DATA_BASE;
	int16_t n_thresh_temp = n_thresh + ADC_DATA_BASE;
	uint8_t p_thresh_h = (uint8_t)((p_thresh_temp >> AW_BIT8) & ONE_WORD);
	uint8_t p_thresh_l = (uint8_t)(p_thresh_temp & ONE_WORD);
	uint8_t n_thresh_h = (uint8_t)((n_thresh_temp >> AW_BIT8) & ONE_WORD);
	uint8_t n_thresh_l = (uint8_t)(n_thresh_temp & ONE_WORD);

	AWLOGD("p thresh temp = 0x%04x", p_thresh_temp);
	AWLOGD("n thresh temp = 0x%04x", n_thresh_temp);
	ret = aw8686x_i2c_write(REG_ADCMPCR0_3, p_thresh_h);
	if (ret != AW_OK) {
		AWLOGD("irq threshold set failed!!!");
		return AW_ERR;
	}
	ret = aw8686x_i2c_write(REG_ADCMPCR0_2, p_thresh_l);
	if (ret != AW_OK) {
		AWLOGD("irq threshold set failed!!!");
		return AW_ERR;
	}
	ret = aw8686x_i2c_write(REG_ADCMPCR1_3, n_thresh_h);
	if (ret != AW_OK) {
		AWLOGD("irq threshold set failed!!!");
		return AW_ERR;
	}
	ret = aw8686x_i2c_write(REG_ADCMPCR1_2, n_thresh_l);
	if (ret != AW_OK) {
		AWLOGD("irq threshold set failed!!!");
		return AW_ERR;
	}

	return AW_OK;
}

static uint32_t aw8686x_adst_trigger_init(void)
{
	uint32_t ret = 0;
	int32_t cnt = 0;

	AWLOGD("%s : enter", __func__);

	while (cnt < AW8686X_I2C_RETRIES) {
		ret = aw8686x_i2c_write_bits(REG_ADMCR_0, ~AW8686X_BIT5_EN, AW8686X_BIT5_EN);
		if (ret == AW_OK) {
			AWLOGD("%s : write adst", __func__);
			aw8686x_i2c_write(REG_ADMCR_1, AW8686X_ADST);
			return AW_OK;
		}
		cnt++;
	}
	return AW_ERR;
}

void aw8686x_touch_alg_no_press(uint8_t chx, uint32_t set_flag)
{
	int16_t hard_sill_max = 0;
	int16_t hard_sill_min = 0;
	int16_t data_add = 0;
	int16_t baseline[CHANNEL_MAX_NUM] = { 0 };
	uint16_t coef[CHANNEL_MAX_NUM] = { 0 };
	uint16_t count[CHANNEL_MAX_NUM] = { 0 };
	AW_TOUCH_STATUS_S *touch_status_s = NULL;

	aw_get_key_status(p_touch_alg_out, &touch_status_s);
	if (touch_status_s == NULL) {
		AWLOGD("%s touch status struct err", __func__);
		return;
	}
	aw_get_stop_press_update_cnt(p_touch_alg_out, chx, &count[chx]);
	if ((touch_status_s->key_flg[chx] == AW_FALSE) && (count[chx] >= STOP_PRESS_UPDATE_COUNT)) {
		set_flag = EN;
	}
	aw_get_baseline_chx(p_touch_alg_out, chx, &baseline[chx]);
	aw_get_coef_chx(p_touch_alg_out, chx, &coef[chx]);
	if (coef[chx] == 0) {
		AWLOGD("%s : coef is err", __func__);
		return;
	}
	data_add = TOUCH_HARD_SILL;
	if (set_flag == EN) {
		AWLOGD("start to set threshold!");
		coef[chx] = coef[chx] / TOUCH_CABLIB_DIV2;
		hard_sill_max = (int16_t)((baseline[chx] + data_add) / coef[chx]);
		hard_sill_min = (int16_t)((baseline[chx] - data_add) / coef[chx]);
#ifndef AW_DYNAMIC_CALI_CLEAR
		hard_sill_max = (int16_t)((baseline[chx] + data_add) / coef[chx]) - g_adc_data[chx];
		hard_sill_min = (int16_t)((baseline[chx] - data_add) / coef[chx]) - g_adc_data[chx];
#endif
		aw8686x_irq_threshold_set(hard_sill_max, hard_sill_min);
	}
	AWLOGD("open irq, close timer!!");
	aw_timer0_stop();
	
#ifndef AW_POLLING_MODE
	//app_sysfreq_req(APP_SYSFREQ_USER_APP_13, APP_SYSFREQ_52M);
	aw8686x_swdt_low_power();
	aw8686x_open_irq();
#endif

#ifdef AW_POLLING_MODE
	aw_signal0_set(AW8686X_SIGNAL_IRQ);
#endif
}

void aw8686x_dynamic_cali(uint8_t chx)
{
	uint8_t adc_code[REGADDR_REGVAL * CHANNEL_MAX_NUM] = { 0 };
	int16_t adc_data[CHANNEL_MAX_NUM] = { 0 };
	uint32_t i = 0;

	AWLOGD("%s : enter", __func__);

	aw_timer0_stop();
	aw8686x_i2c_write(REG_WR_UNLOCK, AW_WRITE_UNLOCK);
	aw8686x_i2c_write(REG_INTR_MASK, DIS);
	aw8686x_i2c_write(REG_ADSR_0, CLEAR_IRQ);
	aw8686x_i2c_write(REG_GO_STDBY_CFG, DIS);
	aw8686x_i2c_write_bits(REG_ADMCR_0, ~AW8686X_BIT5_EN, AW8686X_BIT5_EN);
#ifdef AW_DYNAMIC_CALI_CLEAR
	aw_touch_alg_clear();
#endif
	aw8686x_cali(g_aw8686x_s.ch_status, g_aw8686x_s.version);
	aw8686x_adst_trigger_init();
	for (i = AW_LOOP_START; i < GIVE_ALGO_DATA_TIME; i++) {
		aw_delay_ms(AW_TIMER_DELAY);
		aw8686x_data_get(adc_code, adc_data, chx);
#ifndef AW_DYNAMIC_CALI_CLEAR
		adc_data[chx] += g_adc_data[chx];
#endif
		aw8686x_afe_data_handle(&adc_data[chx], chx);
	}
	aw8686x_touch_alg_no_press(chx, EN);
	AWLOGD("%s : completely", __func__);
}

static void aw8686x_dynamic_cali_judge(uint8_t chx)
{
	AW_BOOL cali_flag;
	uint8_t adc_code[REGADDR_REGVAL * CHANNEL_MAX_NUM] = { 0 };
	int16_t adc_data[CHANNEL_MAX_NUM] = { 0 };
	AW_ALGO_CH_VALUE_T ch_sill_value;

	ch_sill_value.channel = chx;
	ch_sill_value.sill_value = DYNAMIC_CALI_VALUE;
	aw8686x_data_get(adc_code, adc_data, chx);
	aw_get_reverse_updata_offset_flag((void *)p_touch_alg_out, &cali_flag, &ch_sill_value);
	if (cali_flag == AW_TRUE) {
		g_cali_flag = AW_TRUE;
		g_adc_data[chx] = adc_data[chx];
		AWLOGD("g_adc_data[%d] = %d", chx, g_adc_data[chx]);
		cali_flag = AW_FALSE;
		aw8686x_dynamic_cali(chx);
	}
}

static void aw8686x_irq_thread_handler(void)
{
	uint8_t irq_data = 0;
	/*bool ear_flag = AW_FALSE;

	ear_flag = app_local_ear_wearing();
	if (ear_flag == AW_TRUE) {
		AWLOGD("the tws is already earring");
		if (app_bt_stream_isrun(APP_BT_STREAM_A2DP_SBC)) {
			AWLOGD("RING");
			app_sysfreq_req(APP_SYSFREQ_USER_APP_13, APP_SYSFREQ_104M);
		} else if (app_bt_stream_isrun(APP_BT_STREAM_HFP_PCM)) {
			AWLOGD("CALL");
			app_sysfreq_req(APP_SYSFREQ_USER_APP_13, APP_SYSFREQ_208M);
		} else {
			AWLOGD("NORMAL");
			app_sysfreq_req(APP_SYSFREQ_USER_APP_13, APP_SYSFREQ_52M);
		}
	} else {
		AWLOGD("the tws is not ear");
		app_sysfreq_req(APP_SYSFREQ_USER_APP_13, APP_SYSFREQ_52M);
	}*/

	if(aw8686x_timer_is_active())
		{
		return;	
	}
	aw8686x_close_low_power();
	aw8686x_i2c_read(REG_INTR_STAT, &irq_data);
	AWLOGD("irq data = 0x%x", irq_data);
	if (((irq_data >> AW_BIT1 & ONE_BIT) == EN) || ((irq_data >> AW_BIT2 & ONE_BIT) == EN)) {
		aw8686x_i2c_write(REG_INTR_MASK, DIS);
		aw8686x_i2c_write(REG_ADSR_0, CLEAR_IRQ);
		aw_timer0_start(AW_TIMER_DELAY);
		AWLOGD("%s: timer_start!", __func__);
	}
}

static void aw8686x_dect(uint8_t chx)
{
	uint8_t key_event[CHANNEL_MAX_NUM] = { 0 };
	static uint8_t key_status[CHANNEL_MAX_NUM] = { 0 };
	uint8_t last_status[CHANNEL_MAX_NUM] = { 0 };
	static int32_t timer_counter[CHANNEL_MAX_NUM] = { 1 };// counter 4s
	static int32_t timer_counter1[CHANNEL_MAX_NUM] = { 0 };
	static uint8_t flag[CHANNEL_MAX_NUM] = { 0 };

	struct aw8686x_spp_struct spp_s;

	last_status[chx] = key_status[chx];
	aw8686x_alg_process(&spp_s, key_status, key_event, chx);
	AWLOGD("[%s,%d]:laststatus = %d, keystatus = %d, keyevent = %d", __func__, __LINE__, last_status[0], key_status[0], key_event[0]);
	AWLOGD("cunter0 = %d, counter1 = %d", timer_counter[chx], timer_counter1[chx]);
	
	if ((key_status[chx] == 1) && (last_status[chx] != key_status[chx])) {
		AWLOGD("keys Press");
		aw_send_key_event(BTN_TYPE_PRESS);
	}

	if ((timer_counter[chx] < 400) && (timer_counter[chx] > 0)) {
		AWLOGD("ch = %d, timer_counter = %d", chx, timer_counter[chx]);
		if ((key_event[chx] == 1) || (key_event[chx] == 2) || (key_event[chx] == 3) || (key_event[chx] == 4)) {
			AWLOGD("keys Release key_event[%d] = %d", chx, key_event[chx]);
			flag[chx] = 1;
			timer_counter[chx] = 0;
			switch (key_event[chx]) {
	            case 1:
	                aw_send_key_event(BTN_TYPE_SINGLE);
	                break;
	            case 2:
	                aw_send_key_event(BTN_TYPE_DOUBLE);
	                break;
	            case 3:
	                aw_send_key_event(BTN_TYPE_TRIPLE);
	                break;
	            case 4:
	                aw_send_key_event(BTN_TYPE_QUADRUPLE);
	                break;
	            default:
	                break;
        	}
		} else if ((last_status[chx] != key_status[chx]) && (key_status[chx] == 5)) {
			AWLOGD("keys Release key_status[%d] = %d", chx, key_status[chx]);
			flag[chx] = 2;
			timer_counter[chx] = 0;
			aw_send_key_event(BTN_TYPE_QUINTUPLE);
		} else if ((key_status[chx] == 0xfd)) {//Long Press
			if (last_status[chx] != key_status[chx]) {
				AWLOGD("keys LongPress key_status[%d] = %d", chx, key_status[chx]);
				aw_send_key_event(BTN_TYPE_LONG);
			}
			if (key_event[chx] == 0xfd) {
				AWLOGD("keys LongRelease key_status[%d] = %d", chx, key_status[chx]);
				flag[chx] = 3;
				timer_counter[chx] = 0;
				aw_send_key_event(BTN_TYPE_LONG_RELEASE);
			}
		} else if ((key_status[chx] == 0xfe)) {//Long Press
			if (last_status[chx] != key_status[chx]) {
				AWLOGD("keys VLongPress key_status[%d] = %d", chx, key_status[chx]);
				aw_send_key_event(BTN_TYPE_VLONG);
			}
			if (key_event[chx] == 0xfe) {
				AWLOGD("keys VLongRelease key_status[%d] = %d", chx, key_status[chx]);
				flag[chx] = 3;
				timer_counter[chx] = 0;
				aw_send_key_event(BTN_TYPE_VLONG_RELEASE);
			}
		} else if ((key_status[chx] == 0xff)) {//Long Press
			if (last_status[chx] != key_status[chx]) {
				AWLOGD("keys VVLongPress key_status[%d] = %d", chx, key_status[chx]);
				aw_send_key_event(BTN_TYPE_VVLONG);
			}
			if (key_event[chx] == 0xff) {
				AWLOGD("keys VVLongRelease key_status[%d] = %d", chx, key_status[chx]);
				flag[chx] = 3;
				timer_counter[chx] = 0;
				aw_send_key_event(BTN_TYPE_VVLONG_RELEASE);
			}
		} else if (key_status[chx] < 5) {
			if (key_status[chx] == 0) { 
				flag[chx] = 0;
			}
			timer_counter[chx]++;
		}
	} else if (timer_counter[chx] >= 400) {
		if ((key_event[chx] == 1) || (key_event[chx] == 2) || (key_event[chx] == 3) || (key_event[chx] == 4)) {
			timer_counter[chx] = 1;
		  timer_counter1[chx] = 0;
		} else {
			if (flag[chx] == 0) {
				AWLOGD("NO press, stop timer");
				timer_counter[chx] = 1;
				timer_counter1[chx] = 0;
				flag[chx] = 0;
				aw_timer0_stop();
				aw8686x_touch_alg_no_press(chx, DIS);
				aw8686x_dynamic_cali_judge(chx);
				return;
			}
		}
	}
	if (flag[chx] == 1) {
		timer_counter[chx] = 0;
		timer_counter1[chx] = 1;
	} else if ((flag[chx] == 2) || (flag[chx] == 3)) {
		AWLOGD("key_status[%d] = %d", chx, key_status[chx]);
		if ((last_status[chx] != 0) && (key_status[chx] == 0)) {
			flag[chx] = 1;
		}
	}

	if ((timer_counter1[chx] < 150) && (timer_counter1[chx] > 0)) {
		if (key_status[chx] == 0) {
			AWLOGD("keep base_line start");
			timer_counter1[chx]++;
		} else if (key_status[chx] == 1) {
			timer_counter[chx] = 1;
			timer_counter1[chx] = 0;
		}
		flag[chx] = 0;
	} else if (timer_counter1[chx] >= 150) {
		AWLOGD("keep baseline stop");
		timer_counter[chx] = 1;
		timer_counter1[chx] = 0;
		flag[chx] = 0;
		aw8686x_touch_alg_no_press(chx, DIS);
		aw8686x_dynamic_cali_judge(chx);
		return;
	}

	aw_timer0_start(20); //20ms

}

static void aw8686x_get_adc_data_to_alg(void)
{
	uint8_t ch_num = 0;
	uint8_t channel_en[CHANNEL_MAX_NUM] = { 0 };

	for (ch_num = AW_LOOP_START; ch_num < CHANNEL_MAX_NUM; ch_num++) {
		AWLOGD("1 ch_num is %d", ch_num);
		channel_en[ch_num] = (g_aw8686x_s.ch_status >> (AW_BIT0 + ch_num)) & ONE_BIT;
		if (channel_en[ch_num] == EN) {
			aw8686x_dect(ch_num);
		}
	}
}

void aw8686x_thread0_cb(uint8_t msg)
{

	if(msg == AW8686X_SIGNAL_TIMER)
	{
		aw8686x_get_adc_data_to_alg();
	}
	else if(msg == AW8686X_SIGNAL_IRQ)
	{
		aw8686x_irq_thread_handler();
	}
}

void aw8686x_irq_handler(void)
{
	AWLOGD("[%s,%d]:**********", __func__, __LINE__);

	aw_signal0_set(AW8686X_SIGNAL_IRQ);
}

void aw8686x_timer0_handler(void)
{
//	uint32_t slow;
//	uint32_t fast;
	
//	slow = hal_sys_timer_get();
//	fast = _real_fast_sys_timer_get();
//	AWLOGD("%s : slow_1 %d", __func__,slow);
//	AWLOGD("%s : slow %ld", __func__,slow);

	aw_signal0_set(AW8686X_SIGNAL_TIMER);
}

static void aw8686x_temperature_para_init(void)
{
	uint8_t temp_trim[REGADDR_REGVAL] = { 0 };
	uint8_t temp_dac = 0;
	uint8_t data_set = 0;

	aw8686x_i2c_write(REG_WR_UNLOCK, AW_WRITE_UNLOCK);
	aw8686x_i2c_write_bits(REG_ADCHEN_0, ~AW8686X_BIT1_EN, AW8686X_BIT1_EN);
	aw8686x_i2c_read(REG_DACH0CR_1 + 0x10, &temp_dac);
	data_set = temp_dac & (ONE_WORD >> AW_BIT1);
	aw8686x_i2c_write(REG_DACH0CR_1 + 0x10, data_set);
	aw8686x_i2c_write(REG_WR_UNLOCK, AW_WRITE_UNLOCK);
	aw8686x_i2c_write(REG_TEMP_VS_SEL, TEMP_VS_SEL);
	aw8686x_i2c_write(REG_ADCH0CR0_1 + 0x10, AW_TEMP_DATA4);
	aw8686x_i2c_write(REG_ADCH0CR0_2 + 0x10, ADC_TURN_ON);
	aw8686x_i2c_write(REG_ADCH0CR0_3 + 0x10, ADC_TURN_ON);
	aw8686x_i2c_write(REG_ADCH0CR1_0 + 0x10, DIS);
	aw8686x_i2c_write(REG_ADCH0CR1_1 + 0x10, DIS);

	aw8686x_vref_voltage_get(&(g_aw8686x_s.t_vref), 1);
	aw8686x_i2c_read(REG_TEMP_TRIM, temp_trim);
	aw8686x_i2c_read(REG_TEMP, &(temp_trim[1]));
	g_aw8686x_s.temp_para_total = ((uint16_t)((temp_trim[0]) << AW_BIT8) | (uint16_t)(temp_trim[1])) >> AW_BIT4;
	g_aw8686x_s.temp_para_l = temp_trim[1] & HALF_WORD;
}

void aw8686x_temperature_get(uint16_t temper_data, int32_t *temperature)
{
	float temp_b0 = 0;

	temp_b0 = ((float)g_aw8686x_s.t_vref) * AW_TEMP_MAP_100 * AW8686X_IMPEDANCE_MAG / (AW8686X_ERR_DATA
					* REGADDR_REGVAL) * temper_data * AW_TEMP_MAP_10 / AW_TEMP_COEF;
	*temperature = (int32_t)((g_aw8686x_s.temp_para_l + AW_TEMP_INIT) * AW_TEMP_MAP_10
			+(g_aw8686x_s.temp_para_total * AW_TEMP_UMV / AW8686X_ERR_DATA *
			AW8686X_IMPEDANCE_MAG) * AW_TEMP_MAP_10 / AW_TEMP_COEF - temp_b0);
	AWLOGD("tmep = %d.%d", (*temperature / AW_TEMP_MAP_10), (*temperature % AW_TEMP_MAP_10));
}

//static void aw8686x_external_impedance_self_checking(float *p_impedance_ratio, float *n_impedance_ratio, uint8_t chx)
//{
//	uint8_t vs_voltage = 0;
//	uint8_t reg_admcr_1_data_protect = 0;
//	uint8_t irqdata = 0;
//	uint8_t count0 =0;
//	uint8_t count1 = 0;
//	uint8_t reg_adch0cr1_1_data_protect = 0;
//	uint8_t reg_adch0cr0_1_data_protect = 0;
//	uint8_t reg_adch0cr0_2_data_protect = 0;
//	uint8_t reg_adch0cr0_3_data_protect = 0;
//	uint8_t adc[REGADDR_REGVAL] = { 0 };
//	uint8_t vref_voltage = 0;
//	int16_t adc_data[CHANNEL_MAX_NUM] = { 0 };
//	float temp_data = 0;
//
//	AWLOGD("enter");
//
//	aw8686x_vs_voltage_get(&vs_voltage);
//	aw8686x_vref_voltage_get(&vref_voltage, chx);
//	AWLOGD("vs_voltage = %d", vs_voltage);
//	AWLOGD("vref_voltage[%d] = %d", chx, vref_voltage);
//
//	/**********************************
//	*Calculate p-terminal of impedance
//	**********************************/
//	aw8686x_i2c_read(REG_ADCH0CR1_1 + chx * ADCH0CR_OFFSET, &reg_adch0cr1_1_data_protect);
//	aw8686x_i2c_read(REG_ADCH0CR0_1 + chx * ADCH0CR_OFFSET, &reg_adch0cr0_1_data_protect);
//	aw8686x_i2c_read(REG_ADCH0CR0_2 + chx * ADCH0CR_OFFSET, &reg_adch0cr0_2_data_protect);
//	aw8686x_i2c_read(REG_ADCH0CR0_3 + chx * ADCH0CR_OFFSET, &reg_adch0cr0_3_data_protect);
//	aw8686x_i2c_write(REG_ADCH0CR1_1 + chx * ADCH0CR_OFFSET, DIS);//disable PGA
//	aw8686x_i2c_write(REG_ADCH0CR0_1 + chx * ADCH0CR_OFFSET, ADCH0CR0_1_CONFIG);
//	aw8686x_i2c_write(REG_ADCH0CR0_2 + chx * ADCH0CR_OFFSET, SN_MUX_VS);
//	aw8686x_i2c_write(REG_ADCH0CR0_3 + chx * ADCH0CR_OFFSET, SP_MUX_INPUT);
//
//	aw8686x_i2c_read(REG_ADMCR_1, &reg_admcr_1_data_protect);
//	aw8686x_i2c_write(REG_ADSR_0, EN);
//	aw8686x_i2c_write(REG_ADMCR_1, ADC_TURN_ON);
//	irqdata = DIS;
//	while ((irqdata & ONE_BIT) != EN) {
//		aw8686x_i2c_read(REG_INTR_STAT, &irqdata);
//		if ((irqdata & ONE_BIT) == EN) {
//			aw8686x_i2c_write(REG_ADMCR_1, AW_ADC_CLOSED);
//			aw8686x_data_get(adc, adc_data, chx);
//		} else {
//			count0++;
//			aw_delay_ms(1);
//			if (count0 == AW_OFFSET_CALI_TIME) {
//				AWLOGD("%s : external_impedance_self is err", __func__);
//				return;
//			}
//		}
//	}
//	if (adc_data[chx] < 0) {
//		adc_data[chx] = -adc_data[chx];
//	}
//	temp_data = ((float)adc_data[chx] / ADC_DATA_BASE) * (float)vref_voltage;
//	AWLOGD("p temp_data[%d] = %d", chx, (int32_t)(temp_data * AW8686X_IMPEDANCE_MAG));
//	p_impedance_ratio[chx] = temp_data / (float)(vs_voltage - temp_data);
//	AWLOGD("p impedance ratio[%d] = %d", chx, (int32_t)(p_impedance_ratio[chx] *
//							AW8686X_IMPEDANCE_MAG));
//
//	/**********************************
//	*Calculate N-terminal of impedance
//	**********************************/
//	aw8686x_i2c_write(REG_ADCH0CR0_1 + chx * ADCH0CR_OFFSET, ADCH0CR0_1_CONFIG);
//	aw8686x_i2c_write(REG_ADCH0CR0_2 + chx * ADCH0CR_OFFSET, SN_MUX_INPUT);
//	aw8686x_i2c_write(REG_ADCH0CR0_3 + chx * ADCH0CR_OFFSET, SP_MUX_VS);
//	aw8686x_i2c_write(REG_ADSR_0, EN);
//	aw8686x_i2c_write(REG_ADMCR_1, ADC_TURN_ON);
//	irqdata = DIS;
//	while ((irqdata & ONE_BIT) != EN) {
//		aw8686x_i2c_read(REG_INTR_STAT, &irqdata);
//		if ((irqdata & ONE_BIT) == EN) {
//			aw8686x_i2c_write(REG_ADMCR_1, AW_ADC_CLOSED);
//			aw8686x_data_get(adc, adc_data, chx);
//		} else {
//			count1++;
//			aw_delay_ms(1);
//			if (count1 == AW_OFFSET_CALI_TIME) {
//				AWLOGD("%s : external_impedance_self is err", __func__);
//				return;
//			}
//		}
//	}
//	if (adc_data[chx] < 0) {
//		adc_data[chx] = -adc_data[chx];
//	}
//	temp_data = ((float)adc_data[chx] / ADC_DATA_BASE) * (float)vref_voltage;
//	*n_impedance_ratio = temp_data / (float)(vs_voltage - temp_data);
//	AWLOGD("n temp_data[%d] = %d", chx, (int32_t)(temp_data * AW8686X_IMPEDANCE_MAG));
//	AWLOGD("n impedance ratio[%d] = %d", chx, (int32_t)(*n_impedance_ratio * AW8686X_IMPEDANCE_MAG));
//
//	aw8686x_i2c_write(REG_ADCH0CR1_1 + chx * ADCH0CR_OFFSET, reg_adch0cr1_1_data_protect);
//	aw8686x_i2c_write(REG_ADCH0CR0_2 + chx * ADCH0CR_OFFSET, reg_adch0cr0_2_data_protect);
//	aw8686x_i2c_write(REG_ADCH0CR0_3 + chx * ADCH0CR_OFFSET, reg_adch0cr0_3_data_protect);
//	aw8686x_i2c_write(REG_ADMCR_1, reg_admcr_1_data_protect);
//}

//static void aw8686x_external_impedance_en_ch(uint8_t channel_data)
//{
//	uint8_t ch_num = 0;
//	uint8_t channel_en[CHANNEL_MAX_NUM] = { 0 };
//	float p_impedance_ratio[CHANNEL_MAX_NUM] = { 0 };
//	float n_impedance_ratio[CHANNEL_MAX_NUM] = { 0 };
//
//	for (ch_num = AW_LOOP_START; ch_num < CHANNEL_MAX_NUM; ch_num++) {
//		channel_en[ch_num] = (channel_data >> (AW_BIT0 + ch_num)) & ONE_BIT;
//		if (channel_en[ch_num] == EN) {
//			aw8686x_i2c_write(REG_ADCHEN_0, (EN << ch_num));
//			aw8686x_external_impedance_self_checking(&p_impedance_ratio[ch_num], &n_impedance_ratio[ch_num], ch_num);
//		}
//	}
//	aw8686x_i2c_write(REG_ADCHEN_0, channel_data);
//}

static void aw8686x_system_clock_config(uint8_t config_flag)
{
	if (config_flag == REDUCE_2M) {
		aw8686x_i2c_write(REG_CLK_DIV0, SYSTEM_CLOCK_REDUCE_2M);
	} else if(config_flag == DEFAULT_HZ) {
		aw8686x_i2c_write(REG_CLK_DIV0, SYSTEM_CLOCK_DEFAULT);
	}
}

static void aw8686x_close_ADC_config_and_en_ch(uint8_t chx, uint8_t *adc_data_h)
{
	aw8686x_i2c_read(REG_ADCH0DR_1 + chx * REGADDR_REGVAL, adc_data_h);
}

/* the step five of offsetvoltage cali*/
static void aw8686x_adc_config_polling(uint8_t chx, uint8_t config_num)
{
	uint8_t adc_data_h = 0;
	uint8_t adc_flag[CHANNEL_MAX_NUM] = { 0 };
	uint8_t dac_data1[CHANNEL_MAX_NUM] = { 0 };
	uint8_t dac_data0[CHANNEL_MAX_NUM] = { 0 };
	uint8_t adc_data1[CHANNEL_MAX_NUM] = { 0 };
	uint8_t adc_data0[CHANNEL_MAX_NUM] = { 0 };
	uint16_t temp_data = 0 ;
	static uint16_t dac_data_first[CHANNEL_MAX_NUM] = { 0 };
	static uint16_t dac_data_next[CHANNEL_MAX_NUM] = { 0 };
	static uint16_t dac_data_sum[CHANNEL_MAX_NUM] = { 0 };
	static uint16_t dac_data_temp[CHANNEL_MAX_NUM] = { 0 };
	static uint16_t first_flag[CHANNEL_MAX_NUM] = { 0 };

	if (config_num < AW8686X_CALI_POLL_END) {
		aw8686x_close_ADC_config_and_en_ch(chx, &adc_data_h);
		if (config_num == AW_LOOP_START) {
			temp_data = AW8686X_OFFSET_VALTAGE;
			adc_flag[chx] = (adc_data_h >> AW_BIT5) & ONE_BIT;
			if (adc_flag[chx] == EN) {
				dac_data_first[chx] = EN << (AW8686X_CALI_POLL_END - 1);
			} else {
				dac_data_first[chx] = DIS << (AW8686X_CALI_POLL_END - 1);
			}
			dac_data_sum[chx] = dac_data_first[chx] + temp_data;
			dac_data_temp[chx] = dac_data_first[chx];
			first_flag[chx] = (dac_data_sum[chx] >> (AW8686X_CALI_POLL_END - 1)) & AW_BIT1;
			AWLOGD("config num = %d, dac_data_sum[%d] :  = 0x%04x", config_num, chx, dac_data_sum[chx]);
		} else if (config_num > AW_LOOP_START) {
			temp_data = AW8686X_OFFSET_VALTAGE >> config_num;
			adc_flag[chx] = (adc_data_h >> AW_BIT5) & ONE_BIT;
			if (adc_flag[chx] == first_flag[chx]) {
				dac_data_next[chx] = (EN << (AW8686X_CALI_POLL_END - 1 - config_num));
			} else {
				dac_data_next[chx] = (DIS << (AW8686X_CALI_POLL_END - 1 - config_num));
			}
			dac_data_temp[chx] += dac_data_next[chx];
			dac_data_sum[chx] = dac_data_temp[chx] + temp_data;
			AWLOGD("config num = %d, dac_data_sum[%d] :  = 0x%04x", config_num, chx, dac_data_sum[chx]);
		}
		dac_data1[chx] = (uint8_t)((dac_data_sum[chx] >> AW_BIT8) & ONE_WORD);
		dac_data0[chx] = (uint8_t)((dac_data_sum[chx]) & ONE_WORD);
		aw8686x_i2c_write(REG_DACH0DR_1 + chx * CHANNEL_OFFSET, dac_data1[chx]);
		aw8686x_i2c_write(REG_DACH0DR_0 + chx * CHANNEL_OFFSET, dac_data0[chx]);
	} else if (config_num == AW8686X_CALI_POLL_END) {
		aw8686x_i2c_read(REG_ADCH0DR_0 + chx * REGADDR_REGVAL, &adc_data1[chx]);
		aw8686x_i2c_read(REG_ADCH0DR_1 + chx * REGADDR_REGVAL, &adc_data0[chx]);
		aw8686x_i2c_write(REG_DACH0DR_3 + chx * CHANNEL_OFFSET, adc_data0[chx]);
		aw8686x_i2c_write(REG_DACH0DR_2 + chx * CHANNEL_OFFSET, adc_data1[chx]);
		aw8686x_i2c_write_bits(REG_ADCH0CR0_1 + chx * CHANNEL_OFFSET, ~AW8686X_BIT7_EN, AW8686X_BIT7_EN);
	}
}

/*the step four of offsetvoltage cali*/
static void aw8686x_config_ADSR_and_open_AFE(void)
{
	aw8686x_i2c_write(REG_ADSR_0, EN);
	aw8686x_i2c_write(REG_ADMCR_1, AW8686X_CFG_ADM);
}

static void aw8686x_enable_ch_offest_voltage(uint8_t ch_data, uint8_t config_num)
{
	uint8_t ch_num = 0;
	uint8_t channel_en[CHANNEL_MAX_NUM] = { 0 };

	for (ch_num = AW_LOOP_START; ch_num < CHANNEL_MAX_NUM; ch_num++) {
		channel_en[ch_num] = (ch_data >> (AW_BIT0 + ch_num)) & ONE_BIT;
		if (channel_en[ch_num] == EN) {
			AWLOGD("aw8686x_enable_ch_offest_voltage : enter, ch = %d", ch_num);
			aw8686x_adc_config_polling(ch_num, config_num);
		}
	}
}

static void aw8686x_adc_config(uint8_t chx)
{
	uint8_t config_num = 0;
	uint8_t irqdata = 0;
	uint8_t count = 0;

	for (config_num = AW_LOOP_START; config_num < AW_AFE_IRQ_POLLING; config_num++) {
		AWLOGD("aw8686x_adc_config : config_num = %d!", config_num);
		aw8686x_config_ADSR_and_open_AFE();
		irqdata = 0;
		count = 0;
		while (((irqdata & ONE_BIT) != EN) && (count < AW_OFFSET_CALI_TIME)) {
			aw8686x_i2c_read(REG_INTR_STAT, &irqdata);
			if ((irqdata & ONE_BIT) == EN) {
				aw8686x_i2c_write(REG_ADMCR_1, AW_ADC_CLOSED);
				aw8686x_enable_ch_offest_voltage(chx, config_num);
			} else {
				count++;
				aw_delay_ms(1);
			}
		}
		if (count == AW_OFFSET_CALI_TIME) {
			AWLOGD("%s : adc config time is %d, err\n", __func__, config_num);
		}
	}
}

/*the first step of offsetvoltage cali*/
static void aw8686x_config_AFE_mode_and_close_ADST(void)
{
	aw8686x_i2c_write(REG_ADMCR_0, AW8686X_POLLING_MODE1);
	aw8686x_i2c_write(REG_ADMCR_1, DIS);
}

static void aw8686x_set_ADST(void)
{
	uint8_t adf_data = 0;
	uint8_t count = 0;

	while (((adf_data & ONE_BIT) != EN) && (count < 5)) {
		aw8686x_i2c_read(REG_ADSR_0, &adf_data);
		if ((adf_data & ONE_BIT) == EN) {
			aw8686x_i2c_write(REG_ADMCR_1, DIS);
		} else {
			aw_delay_ms(1);
			count++;
		}
	}
}

static void aw8686x_set_work_and_DAO(void)
{
	aw8686x_i2c_write(REG_ADMCR_0, AW8686X_POLLING_MODE1);
	aw8686x_i2c_write(REG_ADMCR_2, SCAN_TIME_16);
	aw8686x_i2c_write(REG_DAOSDR_2, EN);
	aw8686x_i2c_write(REG_ADMCR_1, AW8686X_CFG_ADM);
}

static void aw8686x_config_DAC(uint8_t chx, uint8_t config_flag)
{
	if (config_flag == EN_DAC_VIN) {
		aw8686x_i2c_write(REG_DACH0CR_1 + chx * CHANNEL_OFFSET, AW8686X_DAC_VIN);
	} else if (config_flag == EN_DAC_CAL) {
		aw8686x_i2c_write(REG_DACH0CR_1 + chx * CHANNEL_OFFSET, AW8686X_DAC_CAL);
	}
}

/*the third step of offsetvoltage cali*/
static void aw8686x_config_and_clear_DAC(uint8_t chx)
{
	aw8686x_config_DAC(chx, EN_DAC_VIN);
	aw8686x_i2c_write(REG_DACH0DR_1 + chx * CHANNEL_OFFSET, AW8686X_DAC_CODE_CLEAR);
	aw8686x_i2c_write(REG_DACH0DR_0 + chx * CHANNEL_OFFSET, AW8686X_DAC_CODE_CLEAR);
}

static void aw8686x_config_residual(uint8_t chx, uint8_t config_flag)

{
	if (config_flag == CLOSE_RESIDUAL) {
		aw8686x_i2c_write_bits(REG_ADCH0CR0_1 + chx * CHANNEL_OFFSET,
							~AW8686X_BIT7_EN, DIS);
	} else if (config_flag == OPEN_RESIDUAL) {
		aw8686x_i2c_write_bits(REG_ADCH0CR0_1 + chx * CHANNEL_OFFSET,
					~AW8686X_BIT7_EN, AW8686X_BIT7_EN);
	} else {
		AWLOGD("err config flag!");
	}
}

/*the second step of offsetvoltage cali*/
static void aw8686x_close_residual_and_open_PGA(uint8_t chx)
{
	aw8686x_config_residual(chx, CLOSE_RESIDUAL);
	aw8686x_i2c_write(REG_ADCH0CR1_1 + chx* CHANNEL_OFFSET, AW8686X_PGA_OPEN);
}

static void aw8686x_enforce_cali_accuracy(uint8_t chx)
{
	aw8686x_i2c_write(REG_ADCH0CR0_2 + chx* CHANNEL_OFFSET, SENSOR_TO_GND);
	aw8686x_i2c_write(REG_ADCH0CR0_3 + chx* CHANNEL_OFFSET, SENSOR_TO_GND);
	aw_delay_ms(1);
	aw8686x_i2c_write(REG_ADCH0CR0_2 + chx* CHANNEL_OFFSET, SENSOR_TO_DIFFERENT);
	aw8686x_i2c_write(REG_ADCH0CR0_3 + chx* CHANNEL_OFFSET, SENSOR_TO_DIFFERENT);
}

static void aw8686x_hw_offsetvoltage_cali(uint8_t channel_data)
{
	uint8_t ch_num = 0;
	uint8_t channel_en[CHANNEL_MAX_NUM] = { 0 };

	aw8686x_i2c_write(REG_WR_UNLOCK, AW_WRITE_UNLOCK);
	aw8686x_system_clock_config(REDUCE_2M);
	for (ch_num = AW_LOOP_START; ch_num < CHANNEL_MAX_NUM; ch_num++) {
		channel_en[ch_num] = (channel_data >> (AW_BIT0 + ch_num)) & ONE_BIT;
		if (channel_en[ch_num] == EN) {
			aw8686x_enforce_cali_accuracy(ch_num);
			aw8686x_config_residual(ch_num, CLOSE_RESIDUAL);
			aw8686x_config_DAC(ch_num, EN_DAC_CAL);
		}
	}
	aw8686x_set_work_and_DAO();
	aw8686x_set_ADST();
	for (ch_num = AW_LOOP_START; ch_num < CHANNEL_MAX_NUM; ch_num++) {
		channel_en[ch_num] = (channel_data >> (AW_BIT0 + ch_num)) & ONE_BIT;
		if (channel_en[ch_num] == EN) {
			aw8686x_config_DAC(ch_num, EN_DAC_VIN);
		}
	}
	aw8686x_i2c_write(REG_DAOSDR_2, DIS);
	aw8686x_system_clock_config(DEFAULT_HZ);
	aw8686x_i2c_write(REG_ADMCR_2, 0x0A);
	for (ch_num = AW_LOOP_START; ch_num < CHANNEL_MAX_NUM; ch_num++) {
		channel_en[ch_num] = (channel_data >> (AW_BIT0 + ch_num)) & ONE_BIT;
		if (channel_en[ch_num] == EN) {
			aw8686x_config_residual(ch_num, OPEN_RESIDUAL);
		}
	}
}

static void aw8686x_offsetvoltage_cali_chx(uint8_t channel_data)
{
	uint8_t ch_num = 0;
	uint8_t channel_en[CHANNEL_MAX_NUM] = { 0 };

	aw8686x_i2c_write(REG_WR_UNLOCK, AW_WRITE_UNLOCK);
	aw8686x_config_AFE_mode_and_close_ADST();
	aw8686x_i2c_write(REG_INTR_MASK, DIS);

	for (ch_num = AW_LOOP_START; ch_num < CHANNEL_MAX_NUM; ch_num++) {
		channel_en[ch_num] = (channel_data >> (AW_BIT0 + ch_num)) & ONE_BIT;
		if (channel_en[ch_num] == EN) {
			aw8686x_enforce_cali_accuracy(ch_num);
			aw8686x_close_residual_and_open_PGA(ch_num);
			aw8686x_config_and_clear_DAC(ch_num);
		}
	}
	aw8686x_adc_config(channel_data);
}

void aw8686x_cali(uint8_t ch_status, uint8_t version)
{
	if (version == AW8686XA) {
		AWLOGD("%s : use sw offset voltage cali", __func__);
		aw8686x_offsetvoltage_cali_chx(ch_status);
	} else if (version == AW8686XB) {
		AWLOGD("%s : use hw offset voltage cali", __func__);
		aw8686x_hw_offsetvoltage_cali(ch_status);
	}
}

static void aw8686x_confirm_enable_ch(uint8_t *channel_status, uint8_t *channel_count)
{
	uint8_t ch_num = 0;
	uint8_t channel_en[CHANNEL_MAX_NUM] = { 0 };

	aw8686x_i2c_read(REG_ADCHEN_0, channel_status);
	for (ch_num = AW_LOOP_START; ch_num < CHANNEL_MAX_NUM; ch_num++) {
		channel_en[ch_num] = (*channel_status >> (AW_BIT0 + ch_num)) & ONE_BIT;
		if (channel_en[ch_num] == EN) {
			(*channel_count)++;
		}
	}
	AWLOGD("%s : channel_stastus = %x, config para channel num = %d",
				__func__, *channel_status, *channel_count);
}

static uint8_t aw8686x_parameter_loaded(void)
{
	int32_t ret = 0;
	uint32_t i = 0;
	uint32_t len0 = ((uint32_t)((uint8_t)aw8686x_reg_param[AW_VALID_DATA_LEN]) |
			((uint32_t)((uint8_t)aw8686x_reg_param[AW_VALID_DATA_LEN + 1]) << AW_BIT8));
	char chip_name[8] = { 0 };
	int32_t chip_judge = 0;

	memcpy(chip_name, &aw8686x_reg_param[AW_CHIP_NAME], sizeof(chip_name) / sizeof(chip_name[0]));
	AWLOGD("%s : chipid_name = %s", __func__, chip_name);
	chip_judge = strcmp(chip_name, g_aw8686x_s.chip_type);
	AWLOGD("%s : chip_judge = %d", __func__, chip_judge);
	if (chip_judge != 0) {
		AWLOGD("%s : chip judge err = %d, chipd_name = %s",
					__func__, chip_judge, chip_name);
		AWLOGD("%s : chipid_type = %s", __func__, g_aw8686x_s.chip_type);
		return AW_ERR;
	}
	aw8686x_i2c_write(REG_WR_UNLOCK, AW_WRITE_UNLOCK);

	AWLOGD("%s : len = %d", __func__, len0);
	for (i = AW_LOOP_START; i < len0; i++) {
		ret = aw8686x_i2c_write((uint8_t)aw8686x_reg_param[AW_VALID_DATA_FIRST_ADDR + i * REGADDR_REGVAL],
					(uint8_t)aw8686x_reg_param[AW_VALID_DATA_FIRST_ADDR + i * REGADDR_REGVAL + 1]);
		if (ret != AW_OK) {
			return AW_ERR;
		}
	}

	return AW_OK;
}

static uint8_t aw8686x_ic_confirm(uint8_t chipid)
{
	uint8_t chipid_high_4_bits = 0;

	chipid_high_4_bits = (chipid >> AW_BIT4) & HALF_WORD;
	if (chipid_high_4_bits == AW8686X) {
		return AW_OK;
	}

	return AW_ERR;
}

static void aw8686x_confirm_version(uint8_t chipid, uint8_t *version)
{
	uint8_t chipid_bit3 = 0;

	chipid_bit3 = (chipid >> AW_BIT3) & ONE_BIT;
	if (chipid_bit3 == EN) {
		*version = AW8686XA;
	} else  {
		*version = AW8686XB;
	}
}

static void aw8686x_confirm_ch_num(uint8_t chipid, uint8_t *ch_num)
{
	uint8_t chipid_low_3_bits = 0;

	chipid_low_3_bits = chipid & THREE_BIT;
	if (chipid_low_3_bits == AW86861) {
		*ch_num = AW86861;
		strcpy(g_aw8686x_s.chip_type, "AW86861");
	} else if (chipid_low_3_bits == AW86862) {
		*ch_num = AW86862;
		strcpy(g_aw8686x_s.chip_type, "AW86862");
	} else if (chipid_low_3_bits == AW86864) {
		*ch_num = AW86864;
		strcpy(g_aw8686x_s.chip_type, "AW86864");
	} else {
		*ch_num = IC_ERR;
		strcpy(g_aw8686x_s.chip_type, "AW8686X");
	}
	g_aw8686x_s.chip_type[7] = '\0';
	AWLOGD("%s : chipd_type = %s", __func__, g_aw8686x_s.chip_type);
}

static uint32_t aw8686x_read_chipid(uint8_t *version, uint8_t *ch_num)
{
	uint8_t cnt = 0;
	uint8_t chipid = 0;
	uint32_t ret0 = 0;
	uint32_t ret1 = 0;

	while (cnt < AW_READ_CHIPID_RETRIES) {
		ret0 = aw8686x_i2c_read(REG_CHIPID, &chipid);
		if (ret0 == AW_OK) {
			AWLOGD("the ic chipid is %x", chipid);
			if (chipid == AW8686X_TEST_CHIPID) {
				*version = AW8686XA;
				*ch_num = AW86862;
				return AW_OK;
			} else {
				ret1 = aw8686x_ic_confirm(chipid);
				if (ret1 == AW_OK) {
					aw8686x_confirm_version(chipid, version);
					aw8686x_confirm_ch_num(chipid, ch_num);
					AWLOGD("%s : chipid channel num = %d", __func__, *ch_num);
					return AW_OK;
				} else {
					AWLOGD("%s : read chipid time = %d", __func__, cnt);
					cnt++;
				}
			}
		} else {
			AWLOGD("%s : read chipid time =  %d", __func__, cnt);
			cnt++;
		}
	}

	AWLOGD("%s : err chipid is %x, ret0 = %d", __func__, chipid, ret0);

	return AW_ERR;
}

static void aw8668x_sw_reset(void)
{
	AWLOGD("aw8668x_sw_reset : enter!");

	aw8686x_i2c_write(REG_WR_UNLOCK, AW_WRITE_UNLOCK);
	aw8686x_i2c_write(REG_RST_CFG, AW8686X_RST);
}

static AW_BOOL aw_alg_param_coef_init(void)
{
	AW_BOOL re = AW_FALSE;
	AW_ALGO_PATAM_T *p_touch_param_alg = aw_touch_get_param_alg_lib();

	if (p_touch_param_alg != AW_NULL) {
		p_touch_param_alg->all_channel_num = TOUCH_CHANNEL;
		p_touch_param_alg->use_key_num = TOUCH_KEY_NUM;

		p_touch_param_alg->count_long = TOUCH_PRESS_LONG;
		p_touch_param_alg->count_vlong = TOUCH_PRESS_VLONG;
		p_touch_param_alg->count_vvlong = TOUCH_PRESS_VVLONG;
		p_touch_param_alg->count_double = TOUCH_CLICK_DOUBLE;
		p_touch_param_alg->data_smooth_range = TOUCH_DATA_RANGE;
		p_touch_param_alg->k_smooth_range = TOUCH_K_RANGE;
		p_touch_param_alg->start_sill = TOUCH_SILL_START;
		p_touch_param_alg->stop_sill = TOUCH_SILL_STOP;
		p_touch_param_alg->baseline_up = TOUCH_BASE_LINE;
		p_touch_param_alg->k_sill = TOUCH_SILL_K;

		p_touch_param_alg->key_channels[0] = KEY1_CHOOSE_CHANNEL;
		p_touch_param_alg->key_channels[1] = KEY2_CHOOSE_CHANNEL;
		p_touch_param_alg->key_channels[2] = KEY3_CHOOSE_CHANNEL;
		p_touch_param_alg->key_channels[3] = KEY4_CHOOSE_CHANNEL;
		p_touch_param_alg->key_channels[4] = 0;
		p_touch_param_alg->key_channels[5] = 0;
		p_touch_param_alg->key_channels[6] = 0;
		p_touch_param_alg->key_channels[7] = 0;
		p_touch_param_alg->count_data = TOUCH_COUNT_DATA;
		p_touch_param_alg->recount_data = TOUCH_RECOUNT_DATA;
		p_touch_param_alg->touch_heavy_sill = TOUCH_HEAVY_SILL;
		p_touch_param_alg->touch_heavy_reply_count = TOUCH_HEAVY_REPLY_COUNT;
		p_touch_param_alg->noise_pt = ADC_NOISE_RANGE;
		p_touch_param_alg->touch_st_cnt_len = TOUCH_ST_CAB_LEN;
		p_touch_param_alg->timeout_updata_baseline = TIMEOUT_UPDATA_BASE_LINE;

		g_aw8686x_s.g_ch_calib_factor_lib[0] = CH_CALIB_FACTOR;
		g_aw8686x_s.g_ch_calib_factor_lib[1] = CH1_CALIB_FACTOR;
		g_aw8686x_s.g_ch_calib_factor_lib[2] = CH2_CALIB_FACTOR;
		g_aw8686x_s.g_ch_calib_factor_lib[3] = CH3_CALIB_FACTOR;
		g_aw8686x_s.g_ch_calib_factor_lib[4] = ALGO_CALIB_FACTOR;
		g_aw8686x_s.g_ch_calib_factor_lib[5] = ALGO_CALIB_FACTOR;
		g_aw8686x_s.g_ch_calib_factor_lib[6] = ALGO_CALIB_FACTOR;
		g_aw8686x_s.g_ch_calib_factor_lib[7] = ALGO_CALIB_FACTOR;
#ifdef AW_FLASH_USED
		re = aw8686x_flash_read_val(AW_NORFLASH_INIT_ADD);
		if (re != AW_TRUE) {
			return AW_FALSE;
		}
#endif
		re = AW_TRUE;
	}

	return re;
}

static void aw_app_init_data(void)
{
	AW_BOOL set_param_flag;
	uint32_t alg_version = 0;

	alg_version = aw_get_version();
	AWLOGD("%s : algo version = 0x%08x", __func__, alg_version);
	alg_version = aw_get_creat_date();
	AWLOGD("%s : create version = 0x%08x", __func__, alg_version);

	p_touch_alg_out = (uint8_t *)os_mem_malloc(IOT_DRIVER_MID, aw_get_alg_size());
	if (p_touch_alg_out == NULL) {
		AWLOGD("%s : os_mem_malloc touch alg err", __func__);
		return;
	}
	memset(p_touch_alg_out, AW_ALG_BUFF_CLEAR, aw_get_alg_size());
	aw_get_alg_param_flag((void*)p_touch_alg_out, &set_param_flag);
	/*** Add algo init code ***/
	aw_alg_param_coef_init();

	aw_touch_alg_init(p_touch_alg_out); // Initialize ALGO data
	if (set_param_flag == AW_FALSE) {
		aw_alg_param_init(p_touch_alg_out, aw_touch_get_param_alg_lib()); // Set parameters to the algorithm library
		aw_alg_ch_calib_coef_init(p_touch_alg_out, g_aw8686x_s.g_ch_calib_factor_lib); // Set coef to the algorithm library
	}
	aw_touch_alg_clear(p_touch_alg_out);
	/*** Add algo init code ---End ***/
}

static uint8_t aw8686x_api_init(struct aw8686x_api_interface *api)
{
	{
		aw_delay_ms = api->aw_delay_ms_func;
		aw_send_key_event = api->p_snd_key_event;
		i2c_transfer_tx = api->p_i2c_tx;
		i2c_transfer_rx = api->p_i2c_rx;
		aw_timer0_start = api->p_timer0_start;
		aw_timer0_stop = api->p_timer0_stop;
		aw_signal0_set = api->p_set_signal0;
		aw_signal_wait = api->p_wait_signal;
		aw_signal_clear = api->p_clear_signal;
		thread0_create = api->p_thread0_create;
	}

#ifdef AW_SPP_USED
	if (api->p_aw_spp_write == NULL) {
		return AW_ERR;
	} else {
		aw_spp_write = api->p_aw_spp_write;
	}
#endif

#ifdef AW_FLASH_USED
	if ((api->p_aw_flash_write == NULL) || (api->p_aw_flash_read == NULL)) {
		return AW_ERR;
	} else {
		aw_flash_write = api->p_aw_flash_write;
		aw_flash_read = api->p_aw_flash_read;
	}
#endif
	//thread0_create(aw8686x_thread0_cb);

	return AW_OK;
}

#include "iot_wdt.h"

//ch_num : the number of channel
int32_t aw8686x_sensor_init(struct aw8686x_api_interface *api)
{
	int32_t ret = 0;
	uint8_t channel_status = 0;
	uint8_t chipid_ch_num = 0;
	uint8_t para_conf_ch_num = 0;
	uint8_t version = 0;
	uint8_t ch_num = 0;
	uint8_t channel_en[CHANNEL_MAX_NUM] = { 0 };
	uint32_t i = 0;
	uint8_t adc_code[REGADDR_REGVAL * CHANNEL_MAX_NUM] = { 0 };
	int16_t adc_data[CHANNEL_MAX_NUM] = { 0 };

	AWLOGD("[aw8686x_sensor_init]:driver version is %s", AW8686X_DRIVER_VERSION);

	ret = aw8686x_api_init(api);
	if (ret != AW_OK) {
		AWLOGD("%s : err init hardware api", __func__);
		return AW_OK;
	}
	
//	while(1){
//		ret = aw8686x_read_chipid(&version, &chipid_ch_num);
//		iot_wdt_do_feed();
//		AWLOGD("[%s,%d]:version=%d, chipid_ch_num=%d\n", __func__, __LINE__, version, chipid_ch_num);
//	}
	
	ret = aw8686x_read_chipid(&version, &chipid_ch_num);
	if (ret != AW_OK) {
		return ret;
	}
	aw8668x_sw_reset();
	aw_delay_ms(AW8686X_INIT_DELAY_TIME);
	aw_app_init_data();
	ret = aw8686x_parameter_loaded();
	if (ret != AW_OK) {
		AWLOGD("%s : load parameter failed", __func__);
		os_mem_free(p_touch_alg_out);
		return ret;
	}
	aw8686x_confirm_enable_ch(&channel_status, &para_conf_ch_num);
	if (chipid_ch_num < para_conf_ch_num) {
		AWLOGD("%s : the channel_num of config para is mor than the chipid", __func__);
		AWLOGD("%s : the program is not support!", __func__);
		os_mem_free(p_touch_alg_out);
		return AW_ERR;
	}
	g_aw8686x_s.ch_status = channel_status;
	g_aw8686x_s.version = version;
	aw8686x_close_irq();
	aw8686x_cali(channel_status, version);
	aw8686x_temperature_para_init();
	aw8686x_adst_trigger_init();
	for (ch_num = AW_LOOP_START; ch_num < CHANNEL_MAX_NUM; ch_num++) {
		channel_en[ch_num] = (channel_status >> (AW_BIT0 + ch_num)) & ONE_BIT;
		if (channel_en[ch_num] == EN) {
			for (i = AW_LOOP_START; i < GIVE_ALGO_DATA_TIME; i++) {
				aw_delay_ms(AW_TIMER_DELAY);
				aw8686x_data_get(adc_code, adc_data, ch_num);
				aw8686x_afe_data_handle(&adc_data[ch_num], ch_num);
			}
			aw8686x_touch_alg_no_press(ch_num, EN);
		}
	}

	AWLOGD("completed");

	return ret;
}

void aw8686x_sensor_free_memory(void)
{
	if(p_touch_alg_out != NULL)
	{
		os_mem_free(p_touch_alg_out);
		p_touch_alg_out = NULL;
	}
}
#endif

