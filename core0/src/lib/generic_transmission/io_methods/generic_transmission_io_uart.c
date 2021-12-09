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
#ifdef BUILD_CORE_CORE0

#include "types.h"
#include "string.h"
#include "lib_dbglog.h"

#include "os_lock.h"
#include "os_utils.h"

#include "iot_gpio.h"
#include "iot_dma.h"
#include "iot_uart.h"
#include "iot_resource.h"

#include "generic_transmission_api.h"
#include "generic_transmission_io_uart.h"

/******** Config Start ********/
#define CONFIG_GENERIC_TRANSMISSION_IO_DEBUG                0
#define CONFIG_GENERIC_TRANSMISSION_UNIQ_LINE_PIN           IOT_GPIO_66
#define CONFIG_GENERIC_TRANSMISSION_DMA_BUSY_WAIT           0
#if CONFIG_GENERIC_TRANSMISSION_DMA_BUSY_WAIT
#define CONFIG_GENERIC_TRANSMISSION_DMA_BUSY_WAIT_POLL_INTV 1   //ms
#endif

#ifndef CONFIG_GENERIC_TRANSMISSION_IO_UART_PORT
#define CONFIG_GENERIC_TRANSMISSION_IO_UART_PORT IOT_UART_PORT_0
#endif

#ifndef CONFIG_GENERIC_TRANSMISSION_IO_UART_USE_DMA
#define CONFIG_GENERIC_TRANSMISSION_IO_UART_USE_DMA 0
#endif
/******** Config End ********/

/* Log & Print configuration */
#if CONFIG_GENERIC_TRANSMISSION_IO_DEBUG
#include "stdio.h"
#define GENERIC_TRANSMISSION_IO_UART_LOGD(fmt, arg...) printf(fmt, ##arg)
#define GENERIC_TRANSMISSION_IO_UART_LOGI(fmt, arg...) printf(fmt, ##arg)
#else
#define GENERIC_TRANSMISSION_IO_UART_LOGD(fmt, arg...)
#define GENERIC_TRANSMISSION_IO_UART_LOGI(fmt, arg...)
#endif

/* Uart configuration */
#define GENERIC_TRANSMISSION_DMA_TX_SIZE_ONCE_THRESHOLD 640
#define GENERIC_TRANSMISSION_UART_RX_BUF_SIZE           256

/*lint -esym(754, generic_transmission_io_uart_cfg::*_pin) not referenced */
struct generic_transmission_io_uart_cfg {
    uint32_t baudrate;
    uint8_t data_bits;
    uint8_t parity;
    uint8_t stop_bits;
    uint8_t tx_pin;
    uint8_t rx_pin;
    uint8_t cts_pin;
    uint8_t rts_pin;
    uint8_t hw_fc;
    uint8_t use_dma;
    uint8_t use_uniq_line;
};

struct generic_transmission_io_uart_env_tag {
    struct generic_transmission_io_uart_cfg uart_cfg;
    uint8_t rx_buf[GENERIC_TRANSMISSION_UART_RX_BUF_SIZE];
    os_sem_h write_done_sem;
};

static struct generic_transmission_io_uart_env_tag s_generic_transmission_io_uart_env = {
    .uart_cfg =
        {
            .baudrate = 2000000,
            .data_bits = IOT_UART_DATA_BITS_8,
            .parity = IOT_UART_PARITY_NONE,
            .stop_bits = IOT_UART_STOP_BITS_1,
            .hw_fc = 0,
            .use_dma = CONFIG_GENERIC_TRANSMISSION_IO_UART_USE_DMA,
            .use_uniq_line = 1,
        },
    .rx_buf = {0},
    .write_done_sem = NULL,
};

//static function declare
static int32_t generic_transmission_io_uart_write(const uint8_t *buf, uint32_t len);
static int32_t generic_transmission_io_uart_write_panic(const uint8_t *buf, uint32_t len);
static void generic_transmission_io_uart_init(void);
static void generic_transmission_io_uart_deinit(void);

generic_transmission_io_method_t s_generic_transmission_io_uart_method = {
    .send = generic_transmission_io_uart_write,
    .send_panic = generic_transmission_io_uart_write_panic,
    .init = generic_transmission_io_uart_init,
    .deinit = generic_transmission_io_uart_deinit,
};

static void generic_transmission_io_uart_rx_handler(const void *buffer, uint32_t length)
    IRAM_TEXT(generic_transmission_io_uart_rx_handler);
static void generic_transmission_io_uart_rx_handler(const void *buffer, uint32_t length)
{
    IOT_UART_PORT port = CONFIG_GENERIC_TRANSMISSION_IO_UART_PORT + (IOT_UART_PORT)GENERIC_TRANSMISSION_IO_UART0;
    generic_transmission_io_recv((generic_transmission_io_t)port, buffer, length, true);
}

static void generic_transmission_io_gpio_cb(void)
{
    iot_uart_pin_configuration_t pin_cfg;
    iot_uart_uniq_line_configuration_t uart_uniq_line_cfg;

    uart_uniq_line_cfg.enable = false;
    uart_uniq_line_cfg.uniq_pin = CONFIG_GENERIC_TRANSMISSION_UNIQ_LINE_PIN;
    uart_uniq_line_cfg.baud_rate = s_generic_transmission_io_uart_env.uart_cfg.baudrate;

    // will fixed in UART0 in ear board, TX->GPIO1, RX->GPIO2
    pin_cfg.tx_pin = IOT_GPIO_01;
    pin_cfg.rx_pin = IOT_GPIO_02;

    // rx pin already claim as interrupt, tx should not been used.
    iot_gpio_close(pin_cfg.rx_pin);

    // tx pin may be cliaimed for other usasge, here we need to reclaimed to TX.
    iot_gpio_close(pin_cfg.tx_pin);

    // Disable uniq-line mode first
    iot_uart_uniq_line_config(CONFIG_GENERIC_TRANSMISSION_IO_UART_PORT, &uart_uniq_line_cfg);

    // Enable normal mode
    iot_uart_pin_config(CONFIG_GENERIC_TRANSMISSION_IO_UART_PORT, &pin_cfg);
}

static void generic_transmission_io_uart_cfg_init(void)
{
    int32_t ret;

    iot_uart_uniq_line_configuration_t uart_uniq_line_cfg;
    iot_uart_configuration_t uart_cfg;

    uart_cfg.baud_rate = s_generic_transmission_io_uart_env.uart_cfg.baudrate;
    uart_cfg.data_bits = (IOT_UART_DATA_BITS)s_generic_transmission_io_uart_env.uart_cfg.data_bits;
    uart_cfg.parity = (IOT_UART_PARITY)s_generic_transmission_io_uart_env.uart_cfg.parity;
    uart_cfg.stop_bits = (IOT_UART_STOP_BITS)s_generic_transmission_io_uart_env.uart_cfg.stop_bits;

    iot_uart_pin_configuration_t pin_cfg;
    // will fixed in UART0 in ear board
    pin_cfg.tx_pin = iot_resource_lookup_gpio(GPIO_UART0_TXD);
    pin_cfg.rx_pin = iot_resource_lookup_gpio(GPIO_UART0_RXD);

    iot_uart_init(CONFIG_GENERIC_TRANSMISSION_IO_UART_PORT);

    if (s_generic_transmission_io_uart_env.uart_cfg.hw_fc) {
        iot_uart_flow_control_cfg_t flow_cfg;

        flow_cfg.cts_pin = iot_resource_lookup_gpio(GPIO_UART0_CTS);
        flow_cfg.rts_pin = iot_resource_lookup_gpio(GPIO_UART0_RTS);
        flow_cfg.type = IOT_UART_FLOWCTRL_CTS_RTS;

        iot_uart_flow_control_config(CONFIG_GENERIC_TRANSMISSION_IO_UART_PORT, &flow_cfg);

        GENERIC_TRANSMISSION_IO_UART_LOGI("[GTP] Uart use HW Flow Control\n");
    }

    if (s_generic_transmission_io_uart_env.uart_cfg.use_dma) {
        iot_uart_dma_config_t dma_cfg;

        dma_cfg.rx_use_dma = false;
        dma_cfg.tx_use_dma = true;
        dma_cfg.tx_priority = IOT_DMA_CH_PRIORITY_HIGH;
        iot_uart_dma_config(CONFIG_GENERIC_TRANSMISSION_IO_UART_PORT, &dma_cfg);

        GENERIC_TRANSMISSION_IO_UART_LOGI("[GTP] Uart use dma\n");
    }

    if (s_generic_transmission_io_uart_env.uart_cfg.use_uniq_line) {
        /**
         * Set uniq-line by default, and add a gpio int at normal mode rx line,
         * if receive the interrupt, switch the rx pin
         */
        iot_gpio_open_as_interrupt(pin_cfg.rx_pin, IOT_GPIO_INT_EDGE_FALLING,
                                   generic_transmission_io_gpio_cb);
        iot_gpio_set_pull_mode(pin_cfg.rx_pin, IOT_GPIO_PULL_UP);

        uart_uniq_line_cfg.enable = true;
        uart_uniq_line_cfg.uniq_pin = CONFIG_GENERIC_TRANSMISSION_UNIQ_LINE_PIN;
        uart_uniq_line_cfg.baud_rate = s_generic_transmission_io_uart_env.uart_cfg.baudrate;

        // re-configure tx/rx pin, overwrite the configuration above
        pin_cfg.tx_pin = CONFIG_GENERIC_TRANSMISSION_UNIQ_LINE_PIN;
        pin_cfg.rx_pin = IOT_GPIO_INVALID;
        iot_uart_pin_config(CONFIG_GENERIC_TRANSMISSION_IO_UART_PORT, &pin_cfg);
        iot_uart_uniq_line_config(CONFIG_GENERIC_TRANSMISSION_IO_UART_PORT, &uart_uniq_line_cfg);
        iot_gpio_set_pull_mode(CONFIG_GENERIC_TRANSMISSION_UNIQ_LINE_PIN, IOT_GPIO_PULL_UP);

        GENERIC_TRANSMISSION_IO_UART_LOGI("[GTP] Uart Unique Line\n");
    }
    iot_uart_open(CONFIG_GENERIC_TRANSMISSION_IO_UART_PORT, &uart_cfg, &pin_cfg);

    ret = iot_uart_register_rx_callback(
        CONFIG_GENERIC_TRANSMISSION_IO_UART_PORT, s_generic_transmission_io_uart_env.rx_buf,
        GENERIC_TRANSMISSION_UART_RX_BUF_SIZE, generic_transmission_io_uart_rx_handler);

    if (ret != RET_OK) {
        GENERIC_TRANSMISSION_IO_UART_LOGI("[GTP] Register rx call back fail %d\n", ret);
        return;
    }

    GENERIC_TRANSMISSION_IO_UART_LOGI(
        "[GTP] Uart port init: rate %d, data bits %d, parity %d, stop bits %d\n",
        uart_cfg.baud_rate, uart_cfg.data_bits, uart_cfg.parity, uart_cfg.stop_bits);

    GENERIC_TRANSMISSION_IO_UART_LOGI("[GTP] Uart pin: tx %d, rx %d\n", pin_cfg.tx_pin,
                                      pin_cfg.rx_pin);
}

static void generic_transmission_io_uart_cfg_deinit(void)
{
    iot_uart_close(CONFIG_GENERIC_TRANSMISSION_IO_UART_PORT);
    iot_uart_deinit(CONFIG_GENERIC_TRANSMISSION_IO_UART_PORT);
}

static void generic_transmission_io_uart_write_done_cb(const void *buffer, uint32_t length)
    IRAM_TEXT(generic_transmission_io_uart_write_done_cb);
static void generic_transmission_io_uart_write_done_cb(const void *buffer, uint32_t length)
{
    UNUSED(buffer);
    UNUSED(length);

    os_post_semaphore_from_isr(s_generic_transmission_io_uart_env.write_done_sem);
}

static void generic_transmission_io_uart_write_via_dma(const uint8_t *buf, uint32_t len)
{
    uint32_t remain_len = len;
    uint32_t write_len;
    int32_t ret;

    while (remain_len > 0) {
        write_len = (remain_len < GENERIC_TRANSMISSION_DMA_TX_SIZE_ONCE_THRESHOLD
                         ? remain_len
                         : GENERIC_TRANSMISSION_DMA_TX_SIZE_ONCE_THRESHOLD);

        GENERIC_TRANSMISSION_IO_UART_LOGD("[GTP] Write Via DMA, buf %p, len %d, write_len %d\n",
                                          buf, len, write_len);

        ret = iot_uart_write(CONFIG_GENERIC_TRANSMISSION_IO_UART_PORT,
                             (const char *)buf + (len - remain_len), write_len,
                             generic_transmission_io_uart_write_done_cb);

#if CONFIG_GENERIC_TRANSMISSION_DMA_BUSY_WAIT
        while (ret != RET_OK) {
            //delay 1ms avoid to trigger wdt
            os_delay(CONFIG_GENERIC_TRANSMISSION_DMA_BUSY_WAIT_POLL_INTV);

            ret = iot_uart_write(CONFIG_GENERIC_TRANSMISSION_IO_UART_PORT,
                                 (const char *)buf + (len - remain_len), write_len,
                                 generic_transmission_io_uart_write_done_cb);
        }
#endif

        remain_len -= write_len;

        if (ret == RET_OK) {
            os_pend_semaphore(s_generic_transmission_io_uart_env.write_done_sem, 0xFFFFFFFF);
        }

        GENERIC_TRANSMISSION_IO_UART_LOGD("[GTP] Write Via DMA TX Done\n");
    }
}

static void generic_transmission_io_uart_write_via_pio(const uint8_t *buf, uint32_t len)
{
    uint32_t remain_len = len;
    uint32_t write_len;
    int32_t ret;

    while (remain_len > 0) {
        write_len = (remain_len < GENERIC_TRANSMISSION_DMA_TX_SIZE_ONCE_THRESHOLD
                         ? remain_len
                         : GENERIC_TRANSMISSION_DMA_TX_SIZE_ONCE_THRESHOLD);

        GENERIC_TRANSMISSION_IO_UART_LOGD("[GTP] Write Via PIO, buf %p, len %d, write_len %d\n",
                                          buf, len, write_len);

        ret = iot_uart_write(CONFIG_GENERIC_TRANSMISSION_IO_UART_PORT,
                             (const char *)buf + (len - remain_len), write_len,
                             generic_transmission_io_uart_write_done_cb);

#if CONFIG_GENERIC_TRANSMISSION_DMA_BUSY_WAIT
        while (ret != RET_OK) {
            //delay 1ms avoid to trigger wdt
            os_delay(CONFIG_GENERIC_TRANSMISSION_DMA_BUSY_WAIT_POLL_INTV);

            ret = iot_uart_write(CONFIG_GENERIC_TRANSMISSION_IO_UART_PORT,
                                 (const char *)buf + (len - remain_len), write_len,
                                 generic_transmission_io_uart_write_done_cb);
        }
#endif

        iot_uart_flush(CONFIG_GENERIC_TRANSMISSION_IO_UART_PORT);

        remain_len -= write_len;

        if (ret == RET_OK) {
            os_pend_semaphore(s_generic_transmission_io_uart_env.write_done_sem, 0xFFFFFFFF);
        }

        GENERIC_TRANSMISSION_IO_UART_LOGD("[GTP] Write Via PIO TX Done\n");
    }
}

static int32_t generic_transmission_io_uart_write(const uint8_t *buf, uint32_t len)
{
    if (s_generic_transmission_io_uart_env.uart_cfg.use_dma) {
        generic_transmission_io_uart_write_via_dma(buf, len);
    } else {
        generic_transmission_io_uart_write_via_pio(buf, len);
    }

    return RET_OK;
}

static int32_t generic_transmission_io_uart_write_panic(const uint8_t *buf, uint32_t len)
{
    iot_uart_write_buffer(CONFIG_GENERIC_TRANSMISSION_IO_UART_PORT, buf, len);

    return RET_OK;
}

static void generic_transmission_io_uart_init(void)
{
    generic_transmission_io_uart_cfg_init();

    s_generic_transmission_io_uart_env.write_done_sem =
        os_create_semaphore(IOT_GENERIC_TRANSMISSION_MID, 1, 0);
}

static void generic_transmission_io_uart_deinit(void)
{
    generic_transmission_io_uart_cfg_deinit();

    if (s_generic_transmission_io_uart_env.write_done_sem) {
        os_delete_semaphore(s_generic_transmission_io_uart_env.write_done_sem);
        s_generic_transmission_io_uart_env.write_done_sem = NULL;
    }
}

/* This function here is for example.
 * As uart is the basic IO of generic transmission, so generic transmission
 * IO layer register uart method by directly assign the method address.
 */
void generic_transmission_io_uart_method_register(void)
{
    IOT_UART_PORT port = CONFIG_GENERIC_TRANSMISSION_IO_UART_PORT + (IOT_UART_PORT)GENERIC_TRANSMISSION_IO_UART0;
    generic_transmission_io_method_register((generic_transmission_io_t)port, &s_generic_transmission_io_uart_method);
}

#endif /* BUILD_CORE_CORE0 */
