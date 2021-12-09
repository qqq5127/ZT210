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
#ifndef _DRIVER_HW_SPI_H
#define _DRIVER_HW_SPI_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SPI_DFRAME_SIZE_8  7
#define SPI_DFRAME_SIZE_16 0xf
#define SPI_DFRAME_SIZE_32 32 /* NOT available for NOW */

typedef enum {
    SPI_PORT0,
    SPI_PORT_SLAVE_MAX,
    SPI_PORT1 = 1,
    SPI_PORT2,
    SPI_PORT_MASTER_MAX,
} SPI_PORT;

typedef enum {
    SPI_MASTER,
    SPI_SLAVE,
} SPI_MODE;

typedef enum {
    SPI_TMOD_TRANCIEVER, /**< transmit and receive */
    SPI_TMOD_TRANSMITER, /**< transmit only */
    SPI_TMOD_RECIEVER,   /**< receive only */
} SPI_TRANS_MODE;

typedef enum {
    SPI_FRM_STD, /**< Only mode supported. */
    SPI_FRM_DUAL,
    SPI_FRM_QUAD,
    SPI_FRM_OCTAL
} SPI_FRAME_FORMAT;

typedef enum {
    SPI_INT_TXFIFO_EMPTY,
    SPI_INT_TXFIFO_OVF,
    SPI_INT_RXFIFO_UNDER_OVF,
    SPI_INT_RXFIFO_OVF,
    SPI_INT_RXFIFO_FULL,
    SPI_INT_MAX
} SPI_INT_TYPE;

typedef struct spi_config {
    /** trans_mode */
    SPI_TRANS_MODE trans_mode;
    /** frame_format */
    SPI_FRAME_FORMAT frame_format;
    /** Data frame size, SPI_DFRAME_SIZE_8 suggested. */
    uint8_t frame_size;
    /** slave select enable*/
    bool_t auto_cs;
    /** Device frequency. Like DEVICE_SPI_DEFAULT_FREQUENCY  */
    uint32_t frequency;
    /**< Threshold for reviecing fifo. 0xF is max */
    uint8_t rx_threshold;
    /**< Threshold for transmitting fifo. 0xF is max */
    uint8_t tx_threshold;
} spi_config_t;

typedef struct spi_gpio_cfg {
    uint16_t clk;
    uint16_t cs;
    uint16_t mosi;
    uint16_t miso;
} spi_gpio_cfg_t;

void spi_enable(SPI_PORT port);
void spi_disable(SPI_PORT port);
void spi_init(SPI_PORT port, SPI_MODE mode);
void spi_deinit(SPI_PORT port);
void spi_config(SPI_PORT port, const spi_config_t *cfg);
bool_t spi_pin_config(SPI_PORT port, SPI_MODE mode, const spi_gpio_cfg_t *gpio_cfg);
bool_t spi_is_rx_fifo_empty(SPI_PORT port);
bool_t spi_is_tx_fifo_full(SPI_PORT port);
bool_t spi_is_tx_fifo_empty(SPI_PORT port);
int32_t spi_poll_write(SPI_PORT port, void *buffer, uint32_t size);
int32_t spi_poll_read(SPI_PORT port, void *buffer, uint32_t size);
void spi_rx_fifo_clear(SPI_PORT port);

void spi_interrupt_disable(UART_PORT port, SPI_INT_TYPE int_type);
void spi_interrupt_enable(UART_PORT port, SPI_INT_TYPE int_type);
void spi_interrupt_clear_all(SPI_PORT port);
uint32_t spi_read_byte(SPI_PORT port);
#ifdef __cplusplus
}
#endif

#endif /* _DRIVER_HW_SPI_H */
