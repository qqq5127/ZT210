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
#include "types.h"
#include "modules.h"
#include "riscv_cpu.h"
#include "chip_irq_vector.h"
#include "os_lock.h"
#include "os_mem.h"
#include "iot_uart.h"
#include "iot_irq.h"
#include "uart.h"
#include "iot_gpio.h"
#include "iot_timer.h"
#include "dma.h"
#include "apb.h"

#define UNIQ_LINE_SWITCH_TO_RX_EXTRA_DELAY  5        //us

#if defined(LOW_POWER_ENABLE)
#include "dev_pm.h"
#endif
#if defined(LOW_POWER_ENABLE) && defined(BUILD_CORE_CORE0)
#include "power_mgnt.h"
#endif

typedef struct iot_uart_uniq_line_cfg {
    bool_t enable;
    int8_t current_direction;
    uint8_t uniq_pin;
    uint16_t switch_to_rx_delay;
    uint16_t switch_to_tx_delay;
} iot_uart_uniq_line_state_t;

typedef struct iot_uart_rx_state {
    iot_uart_rx_callback callback;
    uint8_t *buffer;
    uint32_t length;
    uint32_t receive_length;
} iot_uart_rx_state_t;

typedef struct iot_uart_tx_block {
    struct iot_uart_tx_block *next;
    const char *buffer;
    uint32_t length;
    iot_uart_write_done_callback cb;
    bool_t send_done;
} iot_uart_tx_block_t;

typedef struct iot_uart_tx_state {
    os_mutex_h mutex;
    iot_uart_tx_block_t *head;
    iot_uart_tx_block_t *tail;
    iot_uart_tx_block_t *trans;
    uint16_t send_count;
} iot_uart_tx_state_t;

typedef struct iot_uart_dma_state {
    IOT_DMA_CHANNEL_ID tx_ch;
    IOT_DMA_CHANNEL_ID rx_ch;
    bool_t tx_enable;
    bool_t rx_enable;
} iot_uart_dma_state_t;

typedef struct iot_uart_port_state {
    iot_uart_rx_state_t rx;
    iot_uart_tx_state_t tx;
    iot_uart_dma_state_t dma;
    iot_irq_t irq;
} iot_uart_port_state_t;

typedef struct iot_uart_port_restore_state {
    bool_t is_initialized;
    iot_uart_configuration_t cfg;
    iot_uart_pin_configuration_t pin_cfg;
    iot_uart_uniq_line_configuration_t uniq_line_cfg;
} iot_uart_port_restore_state_t;

#if defined(LOW_POWER_ENABLE)
static struct pm_operation uart_pm;
#endif

static uint32_t iot_uart_isr_handler(uint32_t vector, uint32_t data);
static iot_uart_port_state_t iot_uart_stats[IOT_UART_PORT_MAX];
static iot_uart_port_restore_state_t iot_uart_restore_stats[IOT_UART_PORT_MAX];
static iot_uart_uniq_line_state_t iot_uart_uniq_line_stats[IOT_UART_PORT_MAX];
static iot_uart_write_done_callback iot_uart_write_dma_cbs[IOT_UART_PORT_MAX];

static void iot_uart_interrupt_config(IOT_UART_PORT port)
{
    iot_uart_stats[port].irq =
        iot_irq_create(UART0_INT + port, port, iot_uart_isr_handler);
    uart_interrupt_clear_all((UART_PORT)port);
    iot_irq_unmask(iot_uart_stats[port].irq);
}

static uint8_t iot_uart_tx_dma_config(IOT_UART_PORT port)
{
    // Claim a dma channel
    uint8_t ret = iot_dma_claim_channel(&iot_uart_stats[port].dma.tx_ch);
    if (ret != RET_OK) {
        return ret;
    }

    DMA_PERI_REQ req;
    if (port == IOT_UART_PORT_0) {
        req = DMA_PERI_UART0_TX;
    } else {
        req = DMA_PERI_UART1_TX;
    }

    iot_dma_ch_peri_config(iot_uart_stats[port].dma.tx_ch,
                          (IOT_DMA_TRANS_TYPE)DMA_TRANS_MEM_TO_PERI, (IOT_DMA_DATA_WIDTH)DMA_DATA_WIDTH_BYTE,
                          (IOT_DMA_CH_PRIORITY)DMA_CH_PRIORITY_LOW, (IOT_DMA_PERI_REQ)DMA_PERI_REQ_NONE, (IOT_DMA_PERI_REQ)req);

    return RET_OK;
}

static uint8_t iot_uart_rx_dma_config(IOT_UART_PORT port)
{
    // Claim a dma channel
    uint8_t ret = iot_dma_claim_channel(&iot_uart_stats[port].dma.rx_ch);
    if (ret != RET_OK) {
        return ret;
    }

    DMA_PERI_REQ req;
    if (port == IOT_UART_PORT_0) {
        req = DMA_PERI_UART0_RX;
    } else {
        req = DMA_PERI_UART1_RX;
    }

    iot_dma_ch_peri_config(iot_uart_stats[port].dma.rx_ch,
                          (IOT_DMA_TRANS_TYPE)DMA_TRANS_PERI_TO_MEM, (IOT_DMA_DATA_WIDTH)DMA_DATA_WIDTH_BYTE,
                          (IOT_DMA_CH_PRIORITY)DMA_CH_PRIORITY_LOW, (IOT_DMA_PERI_REQ)req, (IOT_DMA_PERI_REQ)DMA_PERI_REQ_NONE);

    return RET_OK;
}

static void iot_uart_pin_direction(IOT_UART_PORT port, int direction, bool_t in_isr)
                                                IRAM_TEXT(iot_uart_pin_direction);
static void iot_uart_pin_direction(IOT_UART_PORT port, int direction, bool_t in_isr)
{
    UART_PORT p = (UART_PORT)port;

    if (!iot_uart_uniq_line_stats[port].enable) {
        return;
    }

    if (iot_uart_uniq_line_stats[port].current_direction != direction) {
        iot_uart_uniq_line_stats[port].current_direction = (int8_t)direction;

        if (direction == 0) {   //change back to rx, need flush tx
            uart_flush(p);
            //for more safe, still do double check and delay is also for wait the uart line busy
            do {
                iot_timer_delay_us(iot_uart_uniq_line_stats[port].switch_to_rx_delay);
            } while (uart_get_tx_fifo_count(p) != 0);
        } else {
            if (!in_isr && cpu_get_int_enable()) {
                do {
                    while (uart_get_rx_fifo_count(p) != 0) {};
                    iot_timer_delay_us(iot_uart_uniq_line_stats[port].switch_to_tx_delay);
                } while (uart_get_rx_fifo_count(p) != 0);
            }
        }

        uart_pin_direction(p, iot_uart_uniq_line_stats[port].uniq_pin, (UART_DIRECTION_TYPE)direction);
    }
}

static uint8_t iot_uart_write_dma(IOT_UART_PORT port, const char *string,
                                  uint32_t length,
                                  iot_uart_write_done_callback cb)
{
    uint8_t ret;

    iot_uart_pin_direction(port, 1, in_irq());

    if (port == IOT_UART_PORT_0) {
        ret = iot_dma_mem_to_peri_with_tx_ack(iot_uart_stats[port].dma.tx_ch,
                        uart_get_fifo_addr((UART_PORT)port), string, length,
                        (dma_mem_peri_tx_ack_callback)cb);
    } else if (port == IOT_UART_PORT_1) {
        ret = iot_dma_mem_to_peri_with_tx_ack(iot_uart_stats[port].dma.tx_ch,
                        uart_get_fifo_addr((UART_PORT)port), string, length,
                        (dma_mem_peri_tx_ack_callback)cb);
    } else {
        ret = RET_INVAL;
    }

    return ret;
}

static iot_uart_tx_block_t *iot_tx_block_alloc(IOT_UART_PORT port)
{
    iot_uart_tx_block_t *block = iot_uart_stats[port].tx.head;

    while (block != NULL) {
        if (block->send_done) {
            if (iot_uart_stats[port].tx.head->next != NULL) {
                iot_uart_stats[port].tx.head =
                    iot_uart_stats[port].tx.head->next;
                os_mem_free(block);
                block = iot_uart_stats[port].tx.head;
            } else {
                iot_uart_stats[port].tx.head = NULL;
                iot_uart_stats[port].tx.tail = NULL;
                os_mem_free(block);
                break;
            }
        } else {
            break;
        }
    }

    block = os_mem_malloc(IOT_DRIVER_MID, sizeof(iot_uart_tx_block_t));
    if (block == NULL) {
        return NULL;
    }

    return block;
}

static void iot_uart_tx_block_add(IOT_UART_PORT port,
                                  iot_uart_tx_block_t *block)
{
    if (iot_uart_stats[port].tx.tail == NULL) {
        iot_uart_stats[port].tx.head = block;
        iot_uart_stats[port].tx.tail = block;
        iot_uart_stats[port].tx.trans = block;
    } else {
        iot_uart_stats[port].tx.tail->next = block;
        iot_uart_stats[port].tx.tail = block;
    }
}

static void iot_uart_write_fifo(IOT_UART_PORT port) IRAM_TEXT(iot_uart_write_fifo);
static void iot_uart_write_fifo(IOT_UART_PORT port)
{
    uint32_t write_count = iot_uart_stats[port].tx.send_count;
    uint8_t *write_buffer = NULL;
    uint32_t *fifo = uart_get_fifo_addr((UART_PORT)port);
    uint32_t mask;

    mask = cpu_disable_irq();
    //iot_uart_write_fifo may be called in TX EMPTY ISR Hnalder
    iot_uart_pin_direction(port, 1, in_irq());

    if (iot_uart_stats[port].tx.trans != NULL) {
        write_buffer = (uint8_t *)iot_uart_stats[port].tx.trans->buffer;
    } else {
        // All data send done.
#if defined(LOW_POWER_ENABLE) && defined(BUILD_CORE_CORE0)
        power_mgnt_dec_module_refcnt(POWER_SLEEP_UART);
#endif
        iot_uart_pin_direction(port, 0, in_irq());
        cpu_restore_irq(mask);
        return;
    }

    while (!uart_is_tx_fifo_full((UART_PORT)port)) {
        *fifo = (uint32_t)write_buffer[write_count];
        write_count++;

        if (write_count >= iot_uart_stats[port].tx.trans->length) {
            // Send block done, invoke the callback
            if (iot_uart_stats[port].tx.trans->cb != NULL) {
                iot_uart_stats[port].tx.trans->cb(
                    (void *)iot_uart_stats[port].tx.trans->buffer,
                    iot_uart_stats[port].tx.trans->length);
            }
            iot_uart_stats[port].tx.trans->send_done = true;

            // Move to next block
            if (iot_uart_stats[port].tx.trans->next != NULL) {
                iot_uart_stats[port].tx.trans =
                    iot_uart_stats[port].tx.trans->next;
                write_buffer = (uint8_t *)iot_uart_stats[port].tx.trans->buffer;
                write_count = 0;
            } else {
                iot_uart_stats[port].tx.trans = NULL;
                // All block send done, just stop
                write_count = 0;
                break;
            }
        }
    }

    // There is byte left
    iot_uart_stats[port].tx.send_count = (uint16_t)write_count;
    uart_interrupt_enable((UART_PORT)port, UART_INT_TXFIFO_EMPTY);
#if defined(LOW_POWER_ENABLE) && defined(BUILD_CORE_CORE0)
    power_mgnt_inc_module_refcnt(POWER_SLEEP_UART);
#endif
    iot_uart_pin_direction(port, 0, in_irq());
    cpu_restore_irq(mask);
}

void iot_uart_write_fifo_from_critical(IOT_UART_PORT port)
{
    uint32_t write_count;
    uint8_t *write_buffer = NULL;

    if (iot_uart_stats[port].tx.trans != NULL) {
        write_buffer = (uint8_t *)iot_uart_stats[port].tx.trans->buffer;
    } else {
        return;
    }

    write_count = iot_uart_stats[port].tx.send_count;

    while (write_buffer) {
        while (write_count < iot_uart_stats[port].tx.trans->length) {
            uart_put_char((UART_PORT)port, *write_buffer++);
            write_count++;
        }

        // Don't invoke the callback in critical

        iot_uart_stats[port].tx.trans->send_done = true;

        // Move to next block
        if (iot_uart_stats[port].tx.trans->next != NULL) {
            iot_uart_stats[port].tx.trans =
                iot_uart_stats[port].tx.trans->next;
            write_buffer = (uint8_t *)iot_uart_stats[port].tx.trans->buffer;
            write_count = 0;
        } else {
            iot_uart_stats[port].tx.trans = NULL;
            // All block send done, just stop
            break;
        }
    }
}

uint32_t iot_uart_restore(uint32_t data)
{
    UNUSED(data);

    UART_PORT port;
    uart_configuration_t uart_cfg;
    uart_pin_configuration_t uart_pin_cfg;
    iot_uart_uniq_line_configuration_t *uniq_line_cfg;
    for (port = UART_PORT_0; port < UART_PORT_MAX; port++) {

        if (!iot_uart_restore_stats[port].is_initialized) {
            continue;
        }

        uniq_line_cfg = &iot_uart_restore_stats[port].uniq_line_cfg;

        uart_cfg.baud_rate = iot_uart_restore_stats[port].cfg.baud_rate;
        uart_cfg.data_bits = (UART_DATA_BITS)iot_uart_restore_stats[port].cfg.data_bits;
        uart_cfg.parity = (UART_PARITY)iot_uart_restore_stats[port].cfg.parity;
        uart_cfg.stop_bits = (UART_STOP_BITS)iot_uart_restore_stats[port].cfg.stop_bits;

        uart_pin_cfg.rx_pin = iot_uart_restore_stats[port].pin_cfg.rx_pin;
        uart_pin_cfg.tx_pin = iot_uart_restore_stats[port].pin_cfg.tx_pin;

        apb_m_s_enable(APB_M_S_UART_MEM);
        apb_m_s_reset(APB_M_S_UART_MEM);

        apb_clk_enable(APB_CLK_UART0 + (APB_CLK)port);

        uart_config(port, &uart_cfg);
        uart_auto_lr_enable(port, true);
        uart_reset_tx_fifo(port);
        uart_reset_rx_fifo(port);

        uart_pin_config(port, &uart_pin_cfg);

        // iot_uart_interrupt_config(port);
        uart_set_threshold(port, UART_THR_TXEMPTY, UART_EMPTY_THRESH);

        if (iot_uart_stats[port].rx.callback) {
            // Config rx threshold
            uart_set_threshold(port, UART_THR_RXTIMEOUT, UART_TOUT_THRESH);
            uart_set_threshold(port, UART_THR_RXFULL, UART_FULL_THRESH);

            uart_interrupt_enable(port, UART_INT_RXFIFO_FULL);
            uart_interrupt_enable(port, UART_INT_RXFIFO_TOUT);
            uart_interrupt_enable(port, UART_INT_RXFIFO_OVF);
            uart_interrupt_enable(port, UART_INT_BRK_DET);
            uart_interrupt_enable(port, UART_INT_GLITCH_DET);
            uart_interrupt_enable(port, UART_INT_FRM_ERR);
        }
        if (iot_uart_stats[port].dma.tx_enable || iot_uart_stats[port].dma.rx_enable) {
            uart_dma_enable(port);
        }
        if (iot_uart_stats[(IOT_UART_PORT)port].dma.tx_enable) {
            iot_uart_tx_dma_config((IOT_UART_PORT)port);
        }
        if (iot_uart_stats[(IOT_UART_PORT)port].dma.rx_enable) {
            iot_uart_rx_dma_config((IOT_UART_PORT)port);
        }
        if (uniq_line_cfg->enable) {
            iot_uart_uniq_line_config((IOT_UART_PORT)port, (iot_uart_uniq_line_configuration_t *)uniq_line_cfg);
        }
    }

    return RET_OK;
}

void iot_uart_init(IOT_UART_PORT port)
{
    uart_init((UART_PORT)port);
    iot_uart_restore_stats[port].is_initialized = true;

#if defined(LOW_POWER_ENABLE)
    list_init(&uart_pm.node);
    uart_pm.restore = iot_uart_restore;
    iot_dev_pm_register(&uart_pm);
#endif
}

void iot_uart_flow_on(IOT_UART_PORT port)
{
    UNUSED(port);
}

void iot_uart_flow_off(IOT_UART_PORT port)
{
    UNUSED(port);
}

void iot_uart_pin_config(IOT_UART_PORT port, const iot_uart_pin_configuration_t *cfg)
{
    uart_pin_configuration_t uart_cfg;
    uart_cfg.rx_pin = cfg->rx_pin;
    uart_cfg.tx_pin = cfg->tx_pin;

    iot_uart_restore_stats[port].pin_cfg = *cfg;
    uart_pin_config((UART_PORT)port, &uart_cfg);
}

void iot_uart_uniq_line_config(IOT_UART_PORT port, const iot_uart_uniq_line_configuration_t *cfg)
{
    iot_uart_uniq_line_stats[port].enable = cfg->enable;
    iot_uart_uniq_line_stats[port].uniq_pin = (uint8_t)cfg->uniq_pin;
    //The delay is used for wait uart line busy
    iot_uart_uniq_line_stats[port].switch_to_rx_delay = (uint16_t)(10 * 1000 * 1000 / cfg->baud_rate + UNIQ_LINE_SWITCH_TO_RX_EXTRA_DELAY);
    //The delay is 400us, adapt to high cpu freq
    iot_uart_uniq_line_stats[port].switch_to_tx_delay = 400;

    iot_uart_restore_stats[port].uniq_line_cfg = *cfg;
    //trigger direction switch to make sure in RX status as default
    iot_uart_pin_direction(port, 1, in_irq());  //switch to tx
    iot_uart_pin_direction(port, 0, in_irq());  //switch to rx
}

void iot_uart_open(IOT_UART_PORT port, const iot_uart_configuration_t *cfg,
                   const iot_uart_pin_configuration_t *pin_cfg)
{
    UART_PORT p = (UART_PORT)port;
    uart_configuration_t uart_cfg;
    uart_pin_configuration_t uart_pin_cfg;

    uart_cfg.baud_rate = cfg->baud_rate;
    uart_cfg.data_bits = (UART_DATA_BITS)cfg->data_bits;
    uart_cfg.parity = (UART_PARITY)cfg->parity;
    uart_cfg.stop_bits = (UART_STOP_BITS)cfg->stop_bits;

    uart_pin_cfg.rx_pin = pin_cfg->rx_pin;
    uart_pin_cfg.tx_pin = pin_cfg->tx_pin;

    uart_config(p, &uart_cfg);
    uart_auto_lr_enable(p, true);
    uart_reset_tx_fifo(p);
    uart_reset_rx_fifo(p);

    if (pin_cfg) {
        uart_pin_config(p, &uart_pin_cfg);
    }
    iot_uart_interrupt_config(port);
    uart_set_threshold(p, UART_THR_TXEMPTY, UART_EMPTY_THRESH);

    iot_uart_stats[port].tx.mutex = os_create_mutex(IOT_DRIVER_MID);

    iot_uart_restore_stats[port].cfg = *cfg;
    if (pin_cfg) {
        iot_uart_restore_stats[port].pin_cfg = *pin_cfg;
    }

    if (iot_uart_uniq_line_stats[port].enable) {
        //trigger direction switch to make sure in RX status as default
        iot_uart_pin_direction(port, 1, in_irq());  //switch to tx
        iot_uart_pin_direction(port, 0, in_irq());  //switch to rx
    }
}

void iot_uart_flow_control_config(IOT_UART_PORT port,
                                  const iot_uart_flow_control_cfg_t *cfg)
{
    uart_flow_control_cfg_t uart_cfg;

    uart_cfg.cts_pin = cfg->cts_pin;
    uart_cfg.rts_pin = cfg->rts_pin;
    uart_cfg.type = (UART_FLOWCTRL)cfg->type;

    uart_flow_control_config((UART_PORT)port, &uart_cfg);
}

uint8_t iot_uart_dma_config(IOT_UART_PORT port, const iot_uart_dma_config_t *cfg)
{
    uart_dma_enable((UART_PORT)port);

    if (cfg->tx_use_dma) {
        if (iot_uart_tx_dma_config(port) == RET_OK) {
            iot_uart_stats[port].dma.tx_enable = cfg->tx_use_dma;
        }
    }

    if (cfg->rx_use_dma) {
        if (iot_uart_rx_dma_config(port) == RET_OK) {
            iot_uart_stats[port].dma.rx_enable = cfg->rx_use_dma;
        }
    }

    return RET_OK;
}

void iot_uart_close(IOT_UART_PORT port)
{
    // Disable dma and release dma channel
    if (iot_uart_stats[port].dma.tx_enable && iot_uart_stats[port].dma.tx_ch) {
        iot_uart_stats[port].dma.tx_enable = false;
        iot_dma_free_channel(iot_uart_stats[port].dma.tx_ch);
    }

    if (iot_uart_stats[port].dma.rx_enable && iot_uart_stats[port].dma.rx_ch) {
        iot_uart_stats[port].dma.rx_enable = false;
        iot_dma_free_channel(iot_uart_stats[port].dma.rx_ch);
    }

    iot_uart_unregister_rx_callback(port);

    // Mask interrupt and delete irq
    uart_interrupt_disable_all((UART_PORT)port);
    iot_irq_mask(iot_uart_stats[port].irq);
    iot_irq_delete(iot_uart_stats[port].irq);
    iot_uart_stats[port].irq = 0;

    uart_pin_release((UART_PORT)port);
}

void iot_uart_deinit(IOT_UART_PORT port)
{
    uart_deinit((UART_PORT)port);
}

static void iot_uart_write_dma_done_callback0(const void *buffer, uint32_t length) IRAM_TEXT(iot_uart_write_dma_done_callback0);
static void iot_uart_write_dma_done_callback0(const void *buffer, uint32_t length)
{
    iot_uart_pin_direction(IOT_UART_PORT_0, 0, in_irq());

    if (iot_uart_write_dma_cbs[IOT_UART_PORT_0]) {
        iot_uart_write_dma_cbs[IOT_UART_PORT_0](buffer, length);
    }
}

static void iot_uart_write_dma_done_callback1(const void *buffer, uint32_t length) IRAM_TEXT(iot_uart_write_dma_done_callback1);
static void iot_uart_write_dma_done_callback1(const void *buffer, uint32_t length)
{
    iot_uart_pin_direction(IOT_UART_PORT_1, 0, in_irq());

    if (iot_uart_write_dma_cbs[IOT_UART_PORT_1]) {
        iot_uart_write_dma_cbs[IOT_UART_PORT_1](buffer, length);
    }
}

uint8_t iot_uart_write(IOT_UART_PORT port, const char *string, uint32_t length,
                      iot_uart_write_done_callback cb)
{
    UNUSED(cb);

    if (iot_uart_stats[port].dma.tx_enable) {
        iot_uart_write_dma_cbs[port] = cb;
        if (port == IOT_UART_PORT_0) {
            return iot_uart_write_dma(port, string, length, iot_uart_write_dma_done_callback0);
        } else if (port == IOT_UART_PORT_1) {
            return iot_uart_write_dma(port, string, length, iot_uart_write_dma_done_callback1);
        } else{
            //nothing
        }
    } else {
        os_acquire_mutex(iot_uart_stats[port].tx.mutex);

        iot_uart_tx_block_t *block = iot_tx_block_alloc(port);

        if (block == NULL) {
            os_release_mutex(iot_uart_stats[port].tx.mutex);
            return RET_NOMEM;
        }

        block->next = NULL;
        block->buffer = string;
        block->length = length;
        block->cb = cb;
        block->send_done = false;

        uart_interrupt_disable((UART_PORT)port, UART_INT_TXFIFO_EMPTY);
        iot_uart_tx_block_add(port, block);

        // Add buffer to tail
        if (iot_uart_stats[port].tx.trans == NULL) {
            iot_uart_stats[port].tx.trans = block;
        }
        iot_uart_write_fifo(port);

        os_release_mutex(iot_uart_stats[port].tx.mutex);
    }
    return RET_OK;
}

int8_t iot_uart_register_rx_callback(IOT_UART_PORT port, uint8_t *buffer,
                                     uint32_t length,
                                     iot_uart_rx_callback callback)
{
    UART_PORT p = (UART_PORT)port;
    if (callback == NULL || buffer == NULL || length == 0) {
        return RET_INVAL;
    }

    if (iot_uart_stats[port].rx.callback || iot_uart_stats[port].rx.buffer
            || iot_uart_stats[port].rx.length) {
        return RET_AGAIN;
    }

    iot_uart_stats[port].rx.callback = callback;
    iot_uart_stats[port].rx.buffer = buffer;
    iot_uart_stats[port].rx.length = length;
    iot_uart_stats[port].rx.receive_length = 0;

    // Config rx threshold
    uart_set_threshold(p, UART_THR_RXTIMEOUT, UART_TOUT_THRESH);
    uart_set_threshold(p, UART_THR_RXFULL, UART_FULL_THRESH);

    uint32_t mask = cpu_disable_irq();
    uart_interrupt_enable(p, UART_INT_RXFIFO_FULL);
    uart_interrupt_enable(p, UART_INT_RXFIFO_TOUT);
    uart_interrupt_enable(p, UART_INT_RXFIFO_OVF);
    uart_interrupt_enable(p, UART_INT_BRK_DET);
    uart_interrupt_enable(p, UART_INT_GLITCH_DET);
    uart_interrupt_enable(p, UART_INT_FRM_ERR);
    cpu_restore_irq(mask);

    return RET_OK;
}

int8_t iot_uart_set_rx_buffer(IOT_UART_PORT port, uint8_t *buffer,
                uint32_t length) IRAM_TEXT(iot_uart_set_rx_buffer);
int8_t iot_uart_set_rx_buffer(IOT_UART_PORT port, uint8_t *buffer,
                              uint32_t length)
{
    if (buffer == NULL || length == 0) {
        return RET_INVAL;
    }

    uint32_t mask = cpu_disable_irq();
    iot_uart_stats[port].rx.buffer = buffer;
    iot_uart_stats[port].rx.length = length;
    cpu_restore_irq(mask);
    return RET_OK;
}

void iot_uart_unregister_rx_callback(IOT_UART_PORT port)
{
    uint32_t mask = cpu_disable_irq();
    uart_interrupt_disable((UART_PORT)port, UART_INT_RXFIFO_FULL);
    uart_interrupt_disable((UART_PORT)port, UART_INT_RXFIFO_TOUT);
    uart_interrupt_disable((UART_PORT)port, UART_INT_RXFIFO_OVF);
    uart_interrupt_disable((UART_PORT)port, UART_INT_BRK_DET);
    uart_interrupt_disable((UART_PORT)port, UART_INT_GLITCH_DET);
    uart_interrupt_disable((UART_PORT)port, UART_INT_FRM_ERR);
    uart_interrupt_clear_all((UART_PORT)port);
    cpu_restore_irq(mask);

    iot_uart_stats[port].rx.callback = NULL;
    iot_uart_stats[port].rx.buffer = NULL;
    iot_uart_stats[port].rx.length = 0;
    iot_uart_stats[port].rx.receive_length = 0;
}

void iot_uart_putc(IOT_UART_PORT port, char c)
{
    iot_uart_pin_direction(port, 1, in_irq());

    uart_put_char((UART_PORT)port, (uint8_t)c);

    iot_uart_pin_direction(port, 0, in_irq());
}

void iot_uart_puts(IOT_UART_PORT port, const char *s)
{
    iot_uart_pin_direction(port, 1, in_irq());

    while (*s != 0) {
        uart_put_char((UART_PORT)port, *((uint8_t*)s));
        s++;
    }

    iot_uart_pin_direction(port, 0, in_irq());
}

void iot_uart_flush(IOT_UART_PORT port)
{
    uart_flush((UART_PORT)port);
}

void iot_uart_write_buffer(IOT_UART_PORT port, const uint8_t *buffer, uint32_t length)
{
    iot_uart_pin_direction(port, 1, in_irq());

    for (uint32_t i = 0; i < length; i++) {
        uart_put_char((UART_PORT)port, buffer[i]);
    }

    iot_uart_pin_direction(port, 0, in_irq());
}

static void iot_uart_rx_int_handler(IOT_UART_PORT port)
{
    uint32_t read_size = iot_uart_stats[port].rx.length;

    read_size = uart_read((UART_PORT)port, iot_uart_stats[port].rx.buffer, read_size);

    if (iot_uart_stats[port].rx.callback != NULL) {
        iot_uart_stats[port].rx.callback(iot_uart_stats[port].rx.buffer,
                                         read_size);
    }
}

static void iot_uart_tx_fifo_empty_handler(IOT_UART_PORT port)
{
    uart_interrupt_disable((UART_PORT)port, UART_INT_TXFIFO_EMPTY);
    iot_uart_write_fifo(port);
}

static void iot_uart_tx_done_handler(IOT_UART_PORT port)
{
    UNUSED(port);
}

static void iot_uart_rx_fifo_overflow_handler(IOT_UART_PORT port)
{
    uart_reset_rx_fifo((UART_PORT)port);
}

static void iot_uart_tx_break_handler(IOT_UART_PORT port)
{
    UNUSED(port);
}

static void iot_uart_rx_break_handler(IOT_UART_PORT port)
{
    uart_reset_rx_fifo((UART_PORT)port);
}

static uint32_t iot_uart_isr_handler(uint32_t vector, uint32_t data) IRAM_TEXT(iot_uart_isr_handler);
static uint32_t iot_uart_isr_handler(uint32_t vector, uint32_t data)
{
    IOT_UART_PORT port = (IOT_UART_PORT)data;
    UART_PORT p = (UART_PORT)port;
    uint32_t int_status = uart_get_interrupt_statue((UART_PORT)port);
    UNUSED(vector);

    if (int_status & (BIT(UART_INT_RXFIFO_FULL) | BIT(UART_INT_RXFIFO_TOUT))) {
        uart_interrupt_clear((UART_PORT)port, UART_INT_RXFIFO_FULL);
        uart_interrupt_clear((UART_PORT)port, UART_INT_RXFIFO_TOUT);
        iot_uart_rx_int_handler(port);
    }

    if (int_status & BIT(UART_INT_TXFIFO_EMPTY)) {
        iot_uart_tx_fifo_empty_handler(port);
        uart_interrupt_clear(p, UART_INT_TXFIFO_EMPTY);
    }

    if (int_status & BIT(UART_INT_TX_DONE)) {
        iot_uart_tx_done_handler(port);
        uart_interrupt_clear(p, UART_INT_TX_DONE);
    }

    if (int_status & BIT(UART_INT_RXFIFO_OVF)) {
        iot_uart_rx_fifo_overflow_handler(port);
        uart_interrupt_clear(p, UART_INT_RXFIFO_OVF);
    }

    if (int_status & BIT(UART_INT_TX_BRK_DONE)) {
        iot_uart_tx_break_handler(port);
        uart_interrupt_clear(p, UART_INT_TX_BRK_DONE);
    }

    if (int_status & (BIT(UART_INT_BRK_DET) | BIT(UART_INT_GLITCH_DET) | BIT(UART_INT_FRM_ERR))) {
        uart_interrupt_clear(p, UART_INT_BRK_DET);
        uart_interrupt_clear(p, UART_INT_GLITCH_DET);
        uart_interrupt_clear(p, UART_INT_FRM_ERR);
        iot_uart_rx_break_handler(port);
    }

    return 0;
}
void iot_uart_rx_reset(IOT_UART_PORT port) IRAM_TEXT(iot_uart_rx_reset);
void iot_uart_rx_reset(IOT_UART_PORT port)
{
    uart_reset_rx_fifo((UART_PORT)port);
}
