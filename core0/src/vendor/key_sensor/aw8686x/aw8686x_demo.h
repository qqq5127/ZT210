#ifndef _AW8686X_DOME_H_
#define _AW8686X_DOME_H_

#include "key_base.h"

#if KEY_DRIVER_SELECTION == KEY_DRIVER_AW8686X
//#include "aw8686x.h"

//#include "hal_trace.h"
//#include "hal_gpio.h"
#ifdef __cplusplus
extern "C" {
#endif // endif __cplusplus

#define AWLOGD(s, ...)		DBGLOG_LIB_RAW("[aw8686x]" s, ##__VA_ARGS__)

//#define AW_POLLING_MODE
//#define AW_SPP_USED
//#define AW_FLASH_USED

#ifndef AW_DELAY_MS_T
#define AW_DELAY_MS_T
typedef void (*aw_delay_ms_t)(uint32_t);
#endif

#ifndef AW_SEND_KEY_EVENT_T
#define AW_SEND_KEY_EVENT_T
typedef void (*aw_send_key_event_t)(uint8_t);
#endif

#ifndef AW_I2C_TRANSFER_TX_T
#define AW_I2C_TRANSFER_TX_T
typedef uint32_t (*aw_i2c_transfer_tx_t)(uint8_t *, uint8_t, uint8_t *, uint16_t);
#endif

#ifndef AW_I2C_TRANSFER_RX_T
#define AW_I2C_TRANSFER_RX_T
typedef uint32_t (*aw_i2c_transfer_rx_t)(uint8_t, uint8_t *, uint16_t);
#endif

#ifndef AW_TIMER_T
#define AW_TIMER_T
typedef void (*aw_timer_start_t)(uint32_t);
#endif

#ifndef AW_TIMER_STOP_T
#define AW_TIMER_STOP_T
typedef void (*aw_timer_stop_t)(void);
#endif

#ifndef AW_SIGNAL_T
#define AW_SIGNAL_T
typedef void (*aw_signal_set_t)(uint32_t);
#endif

#ifndef AW_SIGNAL_CLEAR_T
#define AW_SIGNAL_CLEAR_T
typedef void (*aw_signal_clear_t)(uint32_t);
#endif

#ifndef AW_SIGNAL_WAIT_T
#define AW_SIGNAL_WAIT_T
typedef uint32_t (*aw_signal_wait_t)(void);
#endif

#ifndef AW_THREAD_T
#define AW_THREAD_T
typedef void (*aw_thread_create_t)(void (*)(void));
#endif

#ifndef AW_PRINTF_T
#define AW_PRINTF_T
typedef int (*aw_printf_t)(const char *fmt, ...);
#endif

#ifndef AW_SPP_T
#define AW_SPP_T
typedef void (*aw_spp_t)(void);
#endif

#ifndef AW_SPP_WRITE_T
#define AW_SPP_WRITE_T
typedef void (*aw_spp_write_t)(uint8_t *buf, uint16_t length);
#endif

#ifndef AW_FLASH_HANDLE_T
#define AW_FLASH_HANDLE_T
typedef int32_t (*aw_flash_handle_t)(uint32_t addr_offset, uint8_t *buf, uint32_t len);
#endif

typedef enum aw8686x_i2c_speed {
	LOW_SPEED = 100000,
	HIGH_SPEED = 400000,
	SUPHIGH_SPEED = 1000000,
} I2C_SPEED;

struct aw8686x_api_interface {
	aw_delay_ms_t aw_delay_ms_func;
	aw_send_key_event_t p_snd_key_event;
	aw_i2c_transfer_tx_t p_i2c_tx;
	aw_i2c_transfer_rx_t p_i2c_rx;
	aw_timer_start_t p_timer0_start;
	aw_timer_stop_t p_timer0_stop;
	aw_signal_set_t p_set_signal0;
	aw_signal_wait_t p_wait_signal;
	aw_signal_clear_t p_clear_signal;
	aw_thread_create_t p_thread0_create;
	aw_spp_write_t p_aw_spp_write;
	aw_flash_handle_t p_aw_flash_write;
	aw_flash_handle_t p_aw_flash_read;
};

extern void aw8686x_init(key_callback_t callback);
extern void aw8686x_deinit(bool_t wakeup_enable);

#ifdef __cplusplus
}
#endif // endif __cplusplus

#endif

#endif
