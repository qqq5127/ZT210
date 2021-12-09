/*
* Copyright © Shanghai Awinic Technology Co., Ltd. 2020-2020 . All rights reserved.
* Description: TOUCH related functions
*/
#ifndef __AW_ALGO_TOUCH_DATA_H__
#define __AW_ALGO_TOUCH_DATA_H__
#ifdef __cplusplus
extern "C" {
#endif // endif __cplusplus

#define TOUCH_CHANNEL					(0x04) // Number of all channels
#define TOUCH_KEY_NUM					(0x04) // Number of channels used

#define TOUCH_PRESS_LONG				(100) // *20ms After clicking, the count is less than this value and judged as long press
#define TOUCH_PRESS_VLONG				(200) // *20ms long_press_II
#define TOUCH_PRESS_VVLONG				(300) // *20ms long_press_III
#define TOUCH_CLICK_DOUBLE				(15) // *20ms After clicking, the count is less than this value and judged as double-click

#define TOUCH_DATA_RANGE				(0x08) // Data fluctuation does not exceed this value, complete baseline tracking
#define TOUCH_K_RANGE					(0x08) // Determine whether the data fluctuation is smooth

#define TOUCH_SILL_START		    	(60)  //(0x28) // Start threshold after baseline is exceeded
#define TOUCH_SILL_STOP					(30)   //(0x10) // Stop threshold after baseline is exceeded
#define TOUCH_BASE_LINE					(0x12) // BASE_LINE threshold

#define TOUCH_SILL_K					(0x14) // The threshold is the slope to respond

#define TOUCH_COUNT_DATA				(0x64) // The upper count limit for refreshing the baseline
#define TOUCH_RECOUNT_DATA				(0x32) // When the update fails, refresh the baseline count upper limit

#define TOUCH_HEAVY_SILL				(0x640) // Heavy_sill
#define TOUCH_HEAVY_REPLY_COUNT			(0x08) // The number of counts when the flag is set

#define ADC_NOISE_RANGE					(0x0c) // Noise range under standard conditions

#define TOUCH_ST_CAB_LEN				(0x14) // The count length for initialization
#define TIMEOUT_UPDATA_BASE_LINE		(0x4B0) // This count is set to determine whether the long press has timed out

#define KEY1_CHOOSE_CHANNEL				(0x01) // Four channel signs 0000 0001B
#define KEY2_CHOOSE_CHANNEL				(0x02) // Four channel signs 0000 0010B
#define KEY3_CHOOSE_CHANNEL				(0x04) // Four channel signs 0000 0100B
#define KEY4_CHOOSE_CHANNEL				(0x08) // Four channel signs 0000 1000B
//#define KEY5_CHOOSE_CHANNEL				(0x00) // Four channel signs 0001 0000B
//#define KEY6_CHOOSE_CHANNEL				(0x00) // Four channel signs 0010 0000B
//#define KEY7_CHOOSE_CHANNEL				(0x00) // Four channel signs 0100 0000B
//#define KEY8_CHOOSE_CHANNEL				(0x00) // Four channel signs 1000 0000B

#define CH_CALIB_FACTOR					(0x0064) // Calibration factor
#define CH1_CALIB_FACTOR				(0x0064) // Calibration factor
#define CH2_CALIB_FACTOR				(0x0064) // Calibration factor
#define CH3_CALIB_FACTOR				(0x0064) // Calibration factor
//#define CH4_CALIB_FACTOR				(0x0064) // Calibration factor
//#define CH5_CALIB_FACTOR				(0x0064) // Calibration factor
//#define CH6_CALIB_FACTOR				(0x0064) // Calibration factor
//#define CH7_CALIB_FACTOR				(0x0064) // Calibration factor

#ifdef __cplusplus
}
#endif // endif __cplusplus
#endif // endif __AW_ALGO_TOUCH_DATA_H__

