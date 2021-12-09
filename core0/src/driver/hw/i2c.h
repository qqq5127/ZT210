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

#ifndef __DRIVER_HW_I2C_H__
#define __DRIVER_HW_I2C_H__

#include "types.h"
#include "errno.h"

#ifdef __cplusplus
extern "C" {
#endif

#define BIT_SET(data, bit) ((data) |= (0x01 << (bit)))
#define BIT_CLR(data, bit) ((data) &= (~(0x01 << (bit))))
/*
 * write mode, active high,there is only one high for wr and rd
 */
#define I2C_MODE_TRANS_WRITE_ENABLE  0x01U
#define I2C_MODE_TRANS_WRITE_DISABLE 0x00U

/*
  *write mode, active high,there is only one high for wr and rd
  */
#define I2C_MODE_TRANS_READ_ENABLE  0x01U
#define I2C_MODE_TRANS_READ_DISABLE 0x00U

/*
 * 1:burst read/write(address incr 1 per byte);
 * 0/10/11: not burst mode, stop after one data.
 */
#define I2C_MODE_BUSRT_ENABLE  0x01U
#define I2C_MODE_BUSRT_DISABLE 0x00U

/*
 * 0:normal read;
 * 1:have write device phase(include dummy two transmit)
 */
#define I2C_MODE_READ_NORMAL 0x00U
#define I2C_MODE_READ_DUMMY  0x01U

/*
 * 1:stop finish during STOP state；
 * 0:not stop for restart next transmit during STOP state
 */
#define I2C_MODE_TRANS_STOP_ENABLE  0x01U
#define I2C_MODE_TRANS_STOP_DISABLE 0x00U

typedef enum {
    I2C_RW_DIR_WRITE = 0x00U,
    I2C_RW_DIR_READ = 0x01U,
} I2C_RW_DIR;

typedef enum {
    I2C_PORT_0,
    I2C_PORT_1,
    I2C_PORT_MAX,
} I2C_PORT;

typedef enum {
    I2C_SET_WRITE,
    I2C_SET_READ,
    I2C_SET_BUSRT,
    I2C_SET_READ_DUMMY,
    I2C_SET_STOP,
} I2C_MODE_SET;

typedef enum {
    I2C_STATUS_TX_FIFO_EMPTY = 0x00,
    I2C_STATUS_TX_FIFO_FULL = 0x01,
    I2C_STATUS_RX_FIFO_EMPTY = 0x02,
    I2C_STATUS_RX_FIFO_FULL = 0x03,
    I2C_STATUS_NO_ACK_TIMEOUT = 0x04,
    I2C_STATUS_TRANS_COMPLETE = 0x05,
    I2C_STATUS_NO_ACK_CAUSE = 0x06,
} I2C_STATUS_BIT;

typedef enum {
    I2C_NO_CORRECT_ACK,
    I2C_NO_CONTROL_RESPONSE,
    I2C_NO_ADDRESS_RESPONSE,
    I2C_NO_TX_DATA_RESPONSE,
} I2C_STATUS_NO_ACK_CAUS_T;

typedef struct i2c_gpio_cfg {
    uint8_t scl;
    uint8_t sda;
} i2c_gpio_cfg_t;

typedef struct i2c_config {
    /* Device baudrate */
    uint32_t baudrate;
    /* time-out if there is no ack during this setting time.
     * value is (wait_nack_max+1) i2c clk cycle. i2c clk is also APB CLK.
     */
    uint16_t wait_nack_max_time;
    uint8_t i2c_busrt_mode;
    /* have write device phase(include dummy two transmit */
    uint8_t i2c_read_mode;
} i2c_config_t;

void i2c_clk_init(I2C_PORT port);

/*
 * @param port 0-i2c0, 1-i2c1
 * @param gpio  SCL SDA line gpio selected
 *
 * @return 0: success
 * @return 1: error
 */
uint8_t i2c_gpio_config(I2C_PORT port, const i2c_gpio_cfg_t *gpio);

/*
 * @brief i2c_send_data_num() - set the sent number
 *
 * @param port 0-i2c0, 1-i2c1
 * @param dir write or read
 * @param len set number
 */
void i2c_set_data_length(I2C_PORT port, I2C_RW_DIR dir, uint16_t len);

/*
 * @param port 0-i2c0, 1-i2c1
 *
 * @return 0: success
 * @return 1: error
 */
uint8_t i2c_config(I2C_PORT port, const i2c_config_t *cfg);

/**
 * @brief i2c_txfifo_full() - check the txfifo is full or not
 *
 * @param port 0-i2c0, 1-i2c1
 *
 * @return false: not full
 * @return true: full
 */
bool_t i2c_get_status_txfifo_full(I2C_PORT port);

/**
 * @brief i2c_txfifo_empty() - check the txfifo is empty or not
 *
 * @param port 0-i2c0, 1-i2c1
 *
 * @return false: not empty
 * @return true: empty
 */
bool_t i2c_get_status_txfifo_empty(I2C_PORT port);

/**
 * @brief i2c_rxfifo_full() - check the rxfifo is full or not
 *
 * @param port 0-i2c0, 1-i2c1
 *
 * @return false: not full
 * @return true: full
 */
bool_t i2c_get_status_rxfifo_full(I2C_PORT port);

/**
 * @brief i2c_rxfifo_empty() - check the rxfifo is empty or not
 *
 * @param port 0-i2c0, 1-i2c1
 *
 * @return false: not empty
 * @return true: empty
 */
bool_t i2c_get_status_rxfifo_empty(I2C_PORT port);
bool_t i2c_get_status_nack_timeout(I2C_PORT port);
bool_t i2c_get_status_trans_complete(I2C_PORT port);

bool_t i2c_get_nack_it_raw(I2C_PORT port);
bool_t i2c_get_empty_it_raw(I2C_PORT port);
bool_t i2c_get_full_it_raw(I2C_PORT port);
bool_t i2c_get_underflow_it_raw(I2C_PORT port);
bool_t i2c_get_overflow_it_raw(I2C_PORT port);
bool_t i2c_get_done_it_raw(I2C_PORT port);

bool_t i2c_get_nack_it_status(I2C_PORT port);
bool_t i2c_get_empty_it_status(I2C_PORT port);
bool_t i2c_get_full_it_status(I2C_PORT port);
bool_t i2c_get_underflow_it_status(I2C_PORT port);
bool_t i2c_get_overflow_it_status(I2C_PORT port);
bool_t i2c_get_done_it_status(I2C_PORT port);

void i2c_nack_it_enable(I2C_PORT port, bool_t enable);
void i2c_empty_it_enable(I2C_PORT port, bool_t enable);
void i2c_full_it_enable(I2C_PORT port, bool_t enable);
void i2c_underflow_it_enable(I2C_PORT port, bool_t enable);
void i2c_overflow_it_enable(I2C_PORT port, bool_t enable);
void i2c_done_it_enable(I2C_PORT port, bool_t enable);

void i2c_nack_it_clear(I2C_PORT port, bool_t clear);
void i2c_empty_it_clear(I2C_PORT port, bool_t clear);
void i2c_full_it_clear(I2C_PORT port, bool_t clear);
void i2c_underflow_it_clear(I2C_PORT port, bool_t clear);
void i2c_overflow_it_clear(I2C_PORT port, bool_t clear);
void i2c_done_it_clear(I2C_PORT port, bool_t clear);
void i2c_rx_fifo_throshold(I2C_PORT port, uint8_t throshold);
void i2c_tx_fifo_throshold(I2C_PORT port, uint8_t throshold);

void i2c_set_mode(I2C_PORT port, I2C_MODE_SET mode);
void i2c_clear_status(I2C_PORT port);
void i2c_transfer_stop(I2C_PORT port);
void i2c_trans_start(I2C_PORT port);
void i2c_rx_fifo_we(I2C_PORT port);
void i2c_tx_fifo_write_data(I2C_PORT port, uint8_t data);
uint8_t i2c_rx_fifo_read_data(I2C_PORT port);
uint8_t i2c_get_rx_fifo_data_num(I2C_PORT port);
uint8_t i2c_get_tx_fifo_data_num(I2C_PORT port);

#ifdef __cplusplus
}
#endif

#endif
