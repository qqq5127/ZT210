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

#ifndef _DRIVER_HAL_UART_H
#define _DRIVER_HAL_UART_H

/**
 * @addtogroup HAL
 * @{
 * @addtogroup UART
 * @{
 * This section introduces the UART module's enum, structure, functions and how to use this driver.
 */

#include "types.h"
#include "iot_dma.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    IOT_UART_PORT_0,
    IOT_UART_PORT_1,
    IOT_UART_PORT_MAX,
} IOT_UART_PORT;

/** Uart data bits */
typedef enum {
    IOT_UART_DATA_BITS_MIN_BITS = 0, /*!< Used for checking, the min value it can take */
    IOT_UART_DATA_BITS_5 = 0,        /*!< 5 Data bits */
    IOT_UART_DATA_BITS_6 = 1,        /*!< 6 Data bits */
    IOT_UART_DATA_BITS_7 = 2,        /*!< 7 Data bits */
    IOT_UART_DATA_BITS_8 = 3,        /*!< 8 Data bits */
    IOT_UART_DATA_BITS_MAX_BITS = 3, /*!< Used for checking, the max value it can take*/
} IOT_UART_DATA_BITS;

/** Uart parity modes */
typedef enum {
    IOT_UART_PARITY_NONE, /*!< No parity enabled */
    IOT_UART_PARITY_ODD,  /*!< Odd parity        */
    IOT_UART_PARITY_EVEN, /*!< Even parity       */
} IOT_UART_PARITY;

/** Uart stop bits modes */
typedef enum {
    IOT_UART_STOP_BITS_1 = 0, /*!< 1 Stop bit*/
    IOT_UART_STOP_BITS_2 = 1, /*!< 2 Stop bits */
} IOT_UART_STOP_BITS;

typedef enum {
    IOT_UART_FLOWCTRL_RTS,
    IOT_UART_FLOWCTRL_CTS,
    IOT_UART_FLOWCTRL_CTS_RTS,
} IOT_UART_FLOWCTRL;

/**
 * UART configuration
 */
typedef struct iot_uart_configuration {
    uint32_t baud_rate;
    IOT_UART_DATA_BITS data_bits;
    IOT_UART_PARITY parity;
    IOT_UART_STOP_BITS stop_bits;
} iot_uart_configuration_t;

/**
 * UART pin configuration
 */
typedef struct iot_uart_pin_configuration {
    uint16_t tx_pin;
    uint16_t rx_pin;
} iot_uart_pin_configuration_t;

// TODO: mix with iot_uart_pin_configuration_t
/**
 * UART Uniq line configuration
 */
typedef struct iot_uart_uniq_line_configuration {
    bool_t enable;
    uint16_t uniq_pin;
    uint32_t baud_rate;
} iot_uart_uniq_line_configuration_t;

/**
 * UART flow control configuration
 */
typedef struct iot_uart_flow_control_cfg {
    IOT_UART_FLOWCTRL type;
    uint16_t cts_pin;
    uint16_t rts_pin;
} iot_uart_flow_control_cfg_t;

/**
 * UART flow control configuration
 */
typedef struct iot_uart_dma_config {
    bool_t tx_use_dma;
    bool_t rx_use_dma;
    IOT_DMA_CH_PRIORITY tx_priority;
    IOT_DMA_CH_PRIORITY rx_priority;
} iot_uart_dma_config_t;

typedef void (*iot_uart_write_done_callback)(const void *buffer, uint32_t length);
typedef void (*iot_uart_rx_callback)(const void *buffer, uint32_t length);

void iot_uart_write_fifo_from_critical(IOT_UART_PORT port);

/**
 * @brief This function is to init uart module.
 *
 * @param port is uart port.
 */
void iot_uart_init(IOT_UART_PORT port);

/**
 * @brief This function is to open the uart flow.
 *
 * @param port is uart port.
 */
void iot_uart_flow_on(IOT_UART_PORT port);

/**
 * @brief This function is to close the uart flow.
 *
 * @param port is uart port.
 */
void iot_uart_flow_off(IOT_UART_PORT port);

/**
 * @brief This function is to config uart pin.
 *
 * @param port is uart port.
 * @param cfg is uart pin configuration.
 */
void iot_uart_pin_config(IOT_UART_PORT port, const iot_uart_pin_configuration_t *cfg);

/**
 * @brief This function is to open the uart module.
 *
 * @param port is uart port.
 * @param cfg is uart configuration.
 * @param pin_cfg is uart pin configuration.
 */
void iot_uart_open(IOT_UART_PORT port, const iot_uart_configuration_t *cfg,
                   const iot_uart_pin_configuration_t *pin_cfg);

/**
 * @brief This function is to config the uart flow control.
 *
 * @param port is uart port.
 * @param cfg is uart flow control configuration.
 */
void iot_uart_flow_control_config(IOT_UART_PORT port, const iot_uart_flow_control_cfg_t *cfg);

/**
 * @brief This function is to config the uart dma.
 *
 * @param port is uart port.
 * @param cfg is uart dma configuration.
 * @return uint8_t RET_OK for success else error.
 */
uint8_t iot_uart_dma_config(IOT_UART_PORT port, const iot_uart_dma_config_t *cfg);

/**
 * @brief This function is to close the uart module.
 *
 * @param port is uart port.
 */
void iot_uart_close(IOT_UART_PORT port);

/**
 * @brief This function is to deinitialize the uart module.
 *
 * @param port is uart port.
 */
void iot_uart_deinit(IOT_UART_PORT port);

/**
 * @brief This function is to write through the uart.
 *
 * @param port is uart port.
 * @param string is the input content.
 * @param length is the length of the input.
 * @param cb is the uart write done callback variable quantity.
 * @return int8_t RET_OK for success else error.
 */
uint8_t iot_uart_write(IOT_UART_PORT port, const char *string, uint32_t length,
                       iot_uart_write_done_callback cb);

/**
 * @brief This function is to register the rx callback of the uart.
 *
 * @param port is uart port.
 * @param buffer is the buffer of the received content.
 * @param length is the length of the received content.
 * @param callback is the uart rx callback variable quantity.
 * @return int8_t RET_INVAL or RET_AGAIN or RET_OK.
 */
int8_t iot_uart_register_rx_callback(IOT_UART_PORT port, uint8_t *buffer, uint32_t length,
                                     iot_uart_rx_callback callback);

/**
 * @brief This function is to set the uart rx buffer.
 *
 * @param port is uart port.
 * @param buffer is the buffer of the received content.
 * @param length is the length of the received content.
 * @return int8_t RET_OK for success else error.
 */
int8_t iot_uart_set_rx_buffer(IOT_UART_PORT port, uint8_t *buffer, uint32_t length);

/**
 * @brief This function is to unregister the rx callback of the uart.
 *
 * @param port is uart port.
 */
void iot_uart_unregister_rx_callback(IOT_UART_PORT port);

/**
 * @brief This function is to put a character through the uart.
 *
 * @param port is uart port.
 * @param c is the character to put.
 */
void iot_uart_putc(IOT_UART_PORT port, char c);

/**
 * @brief This function is to put a string of characters through the uart.
 *
 * @param port is uart port.
 * @param s is the first pointer of the string.
 */
void iot_uart_puts(IOT_UART_PORT port, const char *s);

/**
 * @brief This function is to flush the uart.
 *
 * @param port is uart port.
 */
void iot_uart_flush(IOT_UART_PORT port);

/**
 * @brief This function is to restore driver.
 *
 * @param data is the data structure needed when this driver restore.
 * @return uint32_t Return status of the restore operation.
 */
uint32_t iot_uart_restore(uint32_t data);

/**
 * @brief This function is to send data in buffer.
 *
 * @param port is uart port
 * @param buffer is data to send
 * @param length is data lenth
 */
void iot_uart_write_buffer(IOT_UART_PORT port, const uint8_t *buffer, uint32_t length);

/**
 * @brief This function is to config uniq line uart
 *
 * @param port is uart port
 * @param cfg is uniq line uart config
 */
void iot_uart_uniq_line_config(IOT_UART_PORT port, const iot_uart_uniq_line_configuration_t *cfg);

/**
 * @brief This function is to clear rx fifo.
 *
 * @param port is uart port
 */
void iot_uart_rx_reset(IOT_UART_PORT port);

#ifdef __cplusplus
}
#endif

/**
 * @}
 * addtogroup UART
 * @}
 * addtogroup HAL
 */

#endif /* _DRIVER_HAL_UART_H */
