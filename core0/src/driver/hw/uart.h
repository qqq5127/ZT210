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
#ifndef _DRIVER_HW_UART_H
#define _DRIVER_HW_UART_H

#define UART_TX_FIFO_LEN        256U
#define UART_FULL_THRESH        128U
#define UART_FLOWCTRL_THRSH     128U
#define UART_TOUT_THRESH        8U
#define UART_EMPTY_THRESH       8U

typedef enum {
    UART_PORT_0,
    UART_PORT_1,
    UART_PORT_MAX,
} UART_PORT;

/** Uart data bits */
typedef enum {
    /*!< Used for checking, the min value it can take */
    UART_DATA_BITS_MIN_BITS = 0,
    UART_DATA_BITS_5 = 0, /*!< 5 Data bits */
    UART_DATA_BITS_6 = 1, /*!< 6 Data bits */
    UART_DATA_BITS_7 = 2, /*!< 7 Data bits */
    UART_DATA_BITS_8 = 3, /*!< 8 Data bits */
    /*!< Used for checking, the max value it can take*/
    UART_DATA_BITS_MAX_BITS = 3,
} UART_DATA_BITS;

/** Uart parity modes */
typedef enum {
    UART_PARITY_NONE, /*!< No parity enabled */
    UART_PARITY_EVEN, /*!< Even parity       */
    UART_PARITY_ODD,  /*!< Odd parity        */
} UART_PARITY;

/** Uart stop bits modes */
typedef enum {
    UART_STOP_BITS_1 = 0, /*!< 1 Stop bit*/
    UART_STOP_BITS_2 = 1, /*!< 2 Stop bits */
} UART_STOP_BITS;

typedef enum {
    UART_THR_FLOWCTRL,
    UART_THR_RXTIMEOUT,
    UART_THR_RXFULL,
    UART_THR_TXEMPTY,
} UART_THRESHOLD;

typedef enum {
    UART_FLOWCTRL_RTS,
    UART_FLOWCTRL_CTS,
    UART_FLOWCTRL_CTS_RTS,
} UART_FLOWCTRL;

typedef enum {
    UART_INT_RXFIFO_TOUT,
    UART_INT_RS485_PARITY_ERR,
    UART_INT_RS485_FRM_ERR,
    UART_INT_RS485_CLASH,
    UART_INT_SW_XON,
    UART_INT_SW_XOFF,
    UART_INT_GLITCH_DET,
    UART_INT_TX_BRK_DONE,
    UART_INT_TX_BRK_IDLE_DONE,
    UART_INT_TX_DONE,
    UART_INT_CMD_AT_CHAR_DET,
    UART_INT_RXFIFO_FULL,
    UART_INT_TXFIFO_EMPTY,
    UART_INT_PARITY_ERR,
    UART_INT_FRM_ERR,
    UART_INT_RXFIFO_OVF,
    UART_INT_DSR_CHG,
    UART_INT_CTS_CHG,
    UART_INT_BRK_DET,

    UART_INT_MAX
} UART_INT_TYPE;

typedef enum _uart_direction_t {
    UART_IN,                 /*!< as uart rx*/
    UART_OUT                 /*!< as uart tx*/
} UART_DIRECTION_TYPE;

/**
 * UART configuration
 */
typedef struct uart_configuration {
    uint32_t baud_rate;
    UART_DATA_BITS data_bits;
    UART_PARITY parity;
    UART_STOP_BITS stop_bits;
} uart_configuration_t;

/**
 * UART pin configuration
 */
typedef struct uart_pin_configuration {
    uint16_t tx_pin;
    uint16_t rx_pin;
} uart_pin_configuration_t;

/**
 * UART flow control configuration
 */
typedef struct uart_flow_control_cfg {
    UART_FLOWCTRL type;
    uint16_t cts_pin;
    uint16_t rts_pin;
} uart_flow_control_cfg_t;

uint32_t* uart_get_fifo_addr(UART_PORT port);
void uart_init(UART_PORT port);
void uart_deinit(UART_PORT port);
void uart_auto_lr_enable(UART_PORT port , bool_t enable);
bool_t uart_is_tx_fifo_full(UART_PORT port);
void uart_set_rx_fifo(UART_PORT port, uint16_t fifo_depth);
void uart_set_baudrate(UART_PORT port, uint32_t baudrate);
void uart_set_data_bit(UART_PORT port, UART_DATA_BITS num);
void uart_set_parity(UART_PORT port, UART_PARITY parity);
void uart_set_stop_bit(UART_PORT port, UART_STOP_BITS num);
void uart_dma_enable(UART_PORT port);
void uart_config(UART_PORT port, const uart_configuration_t *cfg);
bool_t uart_pin_config(UART_PORT port, const uart_pin_configuration_t *cfg);
void uart_reset_pin(UART_PORT port, uint16_t pin);
void uart_pin_direction(UART_PORT port, uint16_t pin, UART_DIRECTION_TYPE direction);
void uart_pin_release(UART_PORT port);
bool_t uart_flow_control_config(UART_PORT port, const uart_flow_control_cfg_t *cfg);
void uart_flow_disable(UART_PORT port);
void uart_set_threshold(UART_PORT port, UART_THRESHOLD thr, uint32_t value);
void uart_reset_tx_fifo(UART_PORT port);
void uart_reset_rx_fifo(UART_PORT port);
uint32_t uart_read(UART_PORT port, uint8_t* buffer, uint32_t size);
void uart_put_char(UART_PORT port, uint8_t c);
void uart_flush(UART_PORT port);
uint32_t uart_get_interrupt_statue(UART_PORT port);
void uart_interrupt_enable(UART_PORT port, UART_INT_TYPE int_type);
void uart_interrupt_disable(UART_PORT port, UART_INT_TYPE int_type);
void uart_interrupt_disable_all(UART_PORT port);
void uart_interrupt_clear(UART_PORT port, UART_INT_TYPE int_type);
void uart_interrupt_clear_all(UART_PORT port);

uint16_t uart_get_rx_fifo_count(UART_PORT port);
uint16_t uart_get_tx_fifo_count(UART_PORT port);

#endif /* _DRIVER_HW_UART_H */
