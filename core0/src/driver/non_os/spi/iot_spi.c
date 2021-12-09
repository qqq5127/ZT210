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
/* common includes */
#include "types.h"
#include "string.h"
#include "riscv_cpu.h"
#include "chip_irq_vector.h"

/* hal includes */
#include "iot_spi.h"
#include "iot_irq.h"

/* hw includes */
#include "apb.h"
#include "spi.h"
#include "gpio.h"

typedef struct iot_spi_state {
    bool_t is_initialized;
    bool_t auto_cs;
    IOT_SPI_MODE mode;
    iot_spi_gpio_cfg_t gpio_cfg;
    iot_irq_t irq_handler;
} iot_spi_state_t;

static uint32_t iot_spi_isr_handler(uint32_t vector, uint32_t data);
static iot_spi_state_t iot_spi_states[IOT_SPI_PORT_MAX];
static uint8_t *iot_spi_rx_buf = NULL;

static void iot_spi_interrupt_config(IOT_SPI_PORT port)
{
    uint32_t vector;
    switch (port) {
        case IOT_SPI_PORT0:
            vector = SPI_M0_INT;
            break;
        case IOT_SPI_PORT1:
#ifdef BUILD_CHIP_WQ7033
            vector = SPI_M1_INT;
#else
            vector = SPI_M0_INT;
#endif
            break;
        default:
            return;
    }

    iot_spi_states[port].irq_handler =
        iot_irq_create(vector, port, iot_spi_isr_handler);

#ifdef SPI_TEST_INT_RECV
    iot_irq_unmask(iot_spi_states[port].irq_handler);
    spi_interrupt_disable(port, SPI_INT_TXFIFO_EMPTY);
    spi_interrupt_disable(port, SPI_INT_TXFIFO_OVF);
    spi_interrupt_disable(port, SPI_INT_RXFIFO_UNDER_OVF);
#else
    iot_irq_mask(iot_spi_states[port].irq_handler);
#endif
}

void iot_spi_init(IOT_SPI_PORT port, IOT_SPI_MODE mode)
{
    spi_init((SPI_PORT)port, (SPI_MODE)mode);

    iot_spi_states[port].mode = mode;
    iot_spi_states[port].is_initialized = true;
}

void iot_spi_deinit(IOT_SPI_PORT port)
{
    UNUSED(port);
}

int iot_spi_open(IOT_SPI_PORT port, const iot_spi_cfg_t *cfg,
                 const iot_spi_gpio_cfg_t *gpio_cfg, iot_spi_dma_cfg_t *dma_cfg)
{
    spi_config_t spi_cfg;

    UNUSED(dma_cfg);

    if (!iot_spi_states[port].is_initialized) {
        return -1;
    }

    // Save gpio config
    iot_spi_states[port].gpio_cfg = *gpio_cfg;

    // Config GPIO
    iot_spi_pin_config(port, iot_spi_states[port].mode, gpio_cfg);
    if (cfg->auto_cs) {
    }

    iot_spi_states[port].auto_cs = cfg->auto_cs;

    spi_cfg.trans_mode = SPI_TMOD_TRANCIEVER;
    spi_cfg.frame_format = SPI_FRM_STD;
    // Only support for now
    spi_cfg.frame_size = SPI_DFRAME_SIZE_8;
    spi_cfg.frequency = IOT_SPI_DEFAULT_FREQUENCY;

#ifdef SPI_TEST_INT_RECV
    //set rx fifo threshould is 1 byte
    spi_cfg.rx_threshold = 1;
#else
    spi_cfg.rx_threshold = IOT_SPI_DEFAULT_RX_THRESHOULD;
#endif

    spi_cfg.tx_threshold = IOT_SPI_DEFAULT_TX_THRESHOULD;
    spi_cfg.auto_cs = cfg->auto_cs;

    spi_config((SPI_PORT)port, &spi_cfg);

    iot_spi_interrupt_config(port);

    spi_enable((SPI_PORT)port);
    return 0;
}

int iot_spi_close(IOT_SPI_PORT port)
{
    UNUSED(port);
    return 0;
}

bool_t iot_spi_pin_config(IOT_SPI_PORT port, IOT_SPI_MODE mode,
                          const iot_spi_gpio_cfg_t *gpio_cfg)
{
    spi_gpio_cfg_t spi_gpio_cfg;
    spi_gpio_cfg.clk = gpio_cfg->clk;
    spi_gpio_cfg.cs = gpio_cfg->cs;
    spi_gpio_cfg.miso = gpio_cfg->miso;
    spi_gpio_cfg.mosi = gpio_cfg->mosi;

    return spi_pin_config((SPI_PORT)port, (SPI_MODE)mode, &spi_gpio_cfg);
}

static void iot_spi_chip_select(IOT_SPI_PORT port)
{
    gpio_write(iot_spi_states[port].gpio_cfg.cs, 0);
}

static void iot_spi_chip_dis_select(IOT_SPI_PORT port)
{
    gpio_write(iot_spi_states[port].gpio_cfg.cs, 1);
}

bool_t iot_spi_send_data(IOT_SPI_PORT port, uint8_t *cmd_buff, uint16_t cmd_len,
                         uint8_t *data_buff, uint16_t data_len,
                         iot_spi_callback callback)
{
    SPI_PORT p = (SPI_PORT)port;

    // Set cs pin
    if (iot_spi_states[port].auto_cs) {
    }
    //CS
    iot_spi_chip_select(port);

    // Clear RX fifo
    spi_rx_fifo_clear(p);

    // Send cmd buffer
    spi_poll_write(p, cmd_buff, cmd_len);

    // Send data
    spi_poll_write(p, data_buff, data_len);

    // Wait data trans done
    while (!spi_is_tx_fifo_empty(p)) {
    }

    spi_poll_read(p, data_buff, cmd_len + data_len);

    // Set cs pin
    if (iot_spi_states[port].auto_cs) {
    }

    if (callback) {
        callback();
    }
    //DIS CS
    iot_spi_chip_dis_select(port);

    return true;
}

bool_t iot_spi_recv_data(IOT_SPI_PORT port, uint8_t *cmd_buff, uint16_t cmd_len,
                         uint8_t *data_buff, uint16_t data_len,
                         iot_spi_callback callback)
{
    uint8_t i;
    SPI_PORT p = (SPI_PORT)port;
    iot_spi_rx_buf = (uint8_t *)data_buff;

    // Set cs pin
    if (iot_spi_states[port].auto_cs) {
    }
    //CS
    iot_spi_chip_select(port);

    // Clear RX fifo
    spi_rx_fifo_clear(p);

    // Send cmd buffer
    spi_poll_write(p, cmd_buff, cmd_len);

    //fill dummy byte
    for (i = 0; i < data_len; i++) {
        *(cmd_buff + i) = IOT_SPI_SEND_DUMMY_BYTE;
    }

    spi_poll_write(p, cmd_buff, data_len);

    // Wait data trans done
    while (!spi_is_tx_fifo_empty(p)) {
    }

#ifndef SPI_TEST_INT_RECV
    //first Read data during ctrl section trans
    spi_poll_read(p, data_buff, cmd_len);

    //second read valid data
    spi_poll_read(p, data_buff, data_len);
#endif

    if (callback) {
        callback();
    }
    //DIS CS
    iot_spi_chip_dis_select(port);
    return true;
}

static uint32_t iot_spi_isr_handler(uint32_t vector, uint32_t data) IRAM_TEXT(iot_spi_isr_handler);
static uint32_t iot_spi_isr_handler(uint32_t vector, uint32_t data)
{
    SPI_PORT port = (SPI_PORT)data;
    UNUSED(vector);

    while (!spi_is_rx_fifo_empty(port)) {
        *iot_spi_rx_buf = (uint8_t)spi_read_byte(port);
    }
    spi_interrupt_clear_all(port);

    return 0;
}
