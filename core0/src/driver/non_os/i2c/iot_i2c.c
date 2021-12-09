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
#include "riscv_cpu.h"
#include "string.h"

/* hal includes */
#include "iot_i2c.h"

/* hw includes */
#include "apb.h"
#include "gpio.h"

#ifdef LOW_POWER_ENABLE
#include "dev_pm.h"
#endif

typedef struct iot_i2c_state {
    bool_t in_use;
    bool_t is_initialized;
    i2c_config_t i2c_cfg;
    i2c_gpio_cfg_t gpio_cfg;
    iot_i2c_transaction_t tx_trans;
    iot_i2c_transaction_t rx_trans;
    iot_irq_t i2c_irq_handler;
} iot_i2c_state_t;

static uint32_t i2c_counter;
static iot_i2c_state_t iot_i2c_states[IOT_I2C_PORT_MAX];
static uint32_t iot_i2c_isr_handler(uint32_t vector, uint32_t data);

#ifdef LOW_POWER_ENABLE
static struct pm_operation iot_i2c_pm;
#endif

static bool_t i2c_check_timeout(uint32_t last_tick, uint32_t timeout)
{
    return (i2c_counter - last_tick > timeout ? true : false);
}

void iot_i2c_interrupt_config(IOT_I2C_PORT port)
{
    uint32_t vector = 0;
    I2C_PORT p = (I2C_PORT)port;
    switch (port) {
        case IOT_I2C_PORT_0:
            vector = I2C0_INT;
            break;
        case IOT_I2C_PORT_1:
            vector = I2C1_INT;
            break;
        case IOT_I2C_PORT_MAX:
            return;
        default:
            return;
    }
    iot_i2c_states[port].i2c_irq_handler = iot_irq_create(vector, port, iot_i2c_isr_handler);

    /* Config rx threshold */
    i2c_tx_fifo_throshold(p, I2C_TX_FIFO_THROSHOLD);
    i2c_rx_fifo_throshold(p, I2C_RX_FIFO_THROSHOLD);

    i2c_nack_it_clear(p, true);
    i2c_underflow_it_clear(p, true);
    i2c_overflow_it_clear(p, true);

    iot_irq_unmask(iot_i2c_states[port].i2c_irq_handler);
}

uint8_t iot_i2c_init(IOT_I2C_PORT port, const iot_i2c_config_t *cfg)
{
    if (iot_i2c_states[port].in_use) {
        return RET_INVAL;
    } else {
        i2c_clk_init((I2C_PORT)port);
        iot_i2c_states[port].in_use = true;
        iot_i2c_states[port].is_initialized = true;
        iot_i2c_states[port].i2c_cfg.baudrate = cfg->baudrate;
        iot_i2c_states[port].i2c_cfg.i2c_busrt_mode = cfg->i2c_busrt_mode;
        iot_i2c_states[port].i2c_cfg.wait_nack_max_time = cfg->wait_nack_max_time;
    }

#ifdef LOW_POWER_ENABLE
    list_init(&iot_i2c_pm.node);
    iot_i2c_pm.save = iot_i2c_save;
    iot_i2c_pm.restore = iot_i2c_restore;
    iot_dev_pm_register(&iot_i2c_pm);
#endif

    return RET_OK;
}

uint8_t iot_i2c_deinit(IOT_I2C_PORT port)
{
    iot_i2c_states[port].i2c_cfg.baudrate = 0;
    iot_i2c_states[port].i2c_cfg.i2c_busrt_mode = 0;
    iot_i2c_states[port].i2c_cfg.wait_nack_max_time = 0;
    iot_i2c_states[port].is_initialized = false;
    iot_i2c_states[port].in_use = false;

#ifdef LOW_POWER_ENABLE
    iot_dev_pm_unregister(&iot_i2c_pm);
#endif

    return RET_OK;
}

uint8_t iot_i2c_open(IOT_I2C_PORT port, const iot_i2c_gpio_cfg_t *gpio)
{
    i2c_gpio_cfg_t cfg;
    cfg.scl = gpio->scl;
    cfg.sda = gpio->sda;

    if (i2c_gpio_config((I2C_PORT)port, &cfg) != RET_OK) {
        return RET_FAIL;
    }

    iot_i2c_states[port].gpio_cfg.scl = gpio->scl;
    iot_i2c_states[port].gpio_cfg.sda = gpio->sda;

    i2c_config((I2C_PORT)port, &iot_i2c_states[port].i2c_cfg);

    return RET_OK;
}

uint8_t iot_i2c_close(IOT_I2C_PORT port)
{
    i2c_transfer_stop((I2C_PORT)port);
    iot_i2c_deinit(port);
    return RET_OK;
}

uint32_t iot_i2c_save(uint32_t data)
{
    UNUSED(data);
    return RET_OK;
}

uint32_t iot_i2c_restore(uint32_t data)
{
    UNUSED(data);
    for (I2C_PORT i = (I2C_PORT)IOT_I2C_PORT_0; i < (I2C_PORT)IOT_I2C_PORT_MAX; i++) {
        if (iot_i2c_states[i].in_use) {
            // Enable clk
            i2c_clk_init(i);

            gpio_release(iot_i2c_states[i].gpio_cfg.scl);
            gpio_release(iot_i2c_states[i].gpio_cfg.sda);
            i2c_gpio_config(i, &iot_i2c_states[i].gpio_cfg);
            i2c_config(i, &iot_i2c_states[i].i2c_cfg);
        }
    }
    return RET_OK;
}

uint8_t iot_i2c_master_transmit_poll(IOT_I2C_PORT port, uint16_t dev_addr, uint8_t *data,
                                     uint16_t size, uint32_t timeout)
{
    i2c_counter = 0;
    uint8_t *buf = data;
    uint16_t dev_addr_temp;
    I2C_PORT p = (I2C_PORT)port;

    dev_addr_temp = (uint16_t)(dev_addr << 1);
    BIT_CLR(dev_addr_temp, 0);   //lint !e835:code style

    /*step2: set data length*/
    i2c_set_data_length(p, I2C_RW_DIR_WRITE, size + 1);
    /*step1: write config*/
    i2c_clear_status(p);
    i2c_set_mode(p, I2C_SET_WRITE);

    uint32_t last_timer = i2c_counter;
    /*step3: check if the i2c is idle*/
    while (i2c_get_status_txfifo_full(p)) {
        if (i2c_check_timeout(last_timer, timeout)) {
            return RET_INVAL;
        }
        i2c_counter++;
    }
    /*write*/
    i2c_tx_fifo_write_data(p, (uint8_t)dev_addr_temp);

    if (size < I2C_TX_FIFO_FULL_SIZE) {
        for (int i = 0; i < size; i++) {
            while (i2c_get_status_txfifo_full(p)) {
            }
            i2c_tx_fifo_write_data(p, *buf);
            buf++;
        }
        i2c_trans_start(p);
    } else {
        for (int i = 0; i < I2C_TX_FIFO_FULL_SIZE; i++) {
            while (i2c_get_status_txfifo_full(p)) {
            }
            i2c_tx_fifo_write_data(p, *buf);
            buf++;
        }
        i2c_trans_start(p);
        for (int i = I2C_TX_FIFO_FULL_SIZE; i < size; i++) {
            while (i2c_get_status_txfifo_full(p)) {
            }
            i2c_tx_fifo_write_data(p, *buf);
            buf++;
        }
    }
    /*step5: check transfer status*/
    i2c_counter = 0;
    do {
        if (i2c_get_nack_it_status(p)) {
            i2c_clear_status(p);
            return RET_INVAL;
        } else {
            if (i2c_check_timeout(last_timer, timeout)) {
                return RET_INVAL;
            }
            i2c_counter++;
        }
    } while (!(i2c_get_done_it_status(p) && i2c_get_status_txfifo_empty(p)));
    /*step5: clear status reg*/
    i2c_clear_status(p);

    return RET_OK;
}

uint8_t iot_i2c_master_receive_poll(IOT_I2C_PORT port, uint16_t dev_addr, uint8_t *data,
                                    uint16_t size, uint32_t timeout)
{
    i2c_counter = 0;
    uint8_t *buf = data;
    uint16_t dev_addr_temp;
    I2C_PORT p = (I2C_PORT)port;

    /*step1: write config*/
    i2c_clear_status(p);
    i2c_set_data_length(p, I2C_RW_DIR_READ, size);
    i2c_set_data_length(p, I2C_RW_DIR_WRITE, 1);
    i2c_set_mode(p, I2C_SET_READ);

    /*step2: check if the i2c is idle*/
    uint32_t last_timer = i2c_counter;
    while (i2c_get_status_txfifo_full(p)) {
        if (i2c_check_timeout(last_timer, timeout)) {
            return RET_INVAL;
        }
        i2c_counter++;
    }
    /*read*/
    dev_addr_temp = (uint16_t)(dev_addr << 1);
    BIT_SET(dev_addr_temp, 0);   //lint !e835:code style
    i2c_tx_fifo_write_data(p, (uint8_t)dev_addr_temp);

    /*step3: start the fifo tx*/
    i2c_trans_start(p);

    while (size > 0) {
        if (!i2c_get_status_rxfifo_empty(p)) {
            *buf++ = i2c_rx_fifo_read_data(p);
            i2c_rx_fifo_we(p);
            size -= sizeof(uint8_t);
        } else {
            if (i2c_check_timeout(last_timer, timeout)) {
                return RET_INVAL;
            }
            i2c_counter++;
        }
    }
    return RET_OK;
}

uint8_t iot_i2c_master_receive_from_memory_poll(IOT_I2C_PORT port, uint16_t dev_addr,
                                                uint16_t mem_addr,
                                                IOT_I2C_MEMORY_ADDR_SIZE addr_size, uint8_t *data,
                                                uint16_t size, uint32_t timeout)
{
    uint8_t temp_buf[2] = {0};

    if (addr_size == IOT_I2C_MEMORY_ADDR_8BIT) {
        temp_buf[0] = (mem_addr & 0xFF);
        iot_i2c_master_transmit_poll(port, dev_addr, temp_buf, 1, timeout);
    } else if (addr_size == IOT_I2C_MEMORY_ADDR_16BIT) {
        temp_buf[0] = (uint8_t)((mem_addr & 0xFF00) >> 8);
        temp_buf[1] = (uint8_t)(mem_addr & 0x00FF);
        iot_i2c_master_transmit_poll(port, dev_addr, temp_buf, 2, timeout);
    } else {
        return RET_INVAL;
    }
    i2c_transfer_stop((I2C_PORT)port);
    return iot_i2c_master_receive_poll(port, dev_addr, data, size, timeout);
}

uint8_t iot_i2c_master_transmit_to_memory_poll(IOT_I2C_PORT port, uint16_t dev_addr,
                                               uint16_t mem_addr,
                                               IOT_I2C_MEMORY_ADDR_SIZE addr_size,
                                               const uint8_t *data, uint8_t size, uint32_t timeout)
{
    uint16_t total_size = 0;
    uint8_t temp_buf[2] = {0};
    uint8_t dataBuf[256];

    if (addr_size == IOT_I2C_MEMORY_ADDR_8BIT) {
        temp_buf[0] = (mem_addr & 0xFF);
        total_size += (size + 1);
        memcpy(&dataBuf[0], temp_buf, 1);
        memcpy(&dataBuf[1], data, size);
        iot_i2c_master_transmit_poll(port, dev_addr, dataBuf, total_size, timeout);
    } else if (addr_size == IOT_I2C_MEMORY_ADDR_16BIT) {
        temp_buf[0] = (uint8_t)((mem_addr & 0xFF00) >> 8);
        temp_buf[1] = (uint8_t)(mem_addr & 0x00FF);
        total_size += (size + 2);
        memcpy(&dataBuf[0], temp_buf, 2);
        memcpy(&dataBuf[2], data, size);
        iot_i2c_master_transmit_poll(port, dev_addr, dataBuf, total_size, timeout);
    } else {
        return RET_INVAL;
    }

    return RET_OK;
}

uint8_t iot_i2c_master_transmit_it(IOT_I2C_PORT port, uint16_t dev_addr,
                                   const iot_i2c_transaction_t *tx_trans)
{
    uint16_t i, dev_addr_temp;
    I2C_PORT p = (I2C_PORT)port;
    iot_i2c_states[port].tx_trans.buffer = tx_trans->buffer;
    iot_i2c_states[port].tx_trans.length = tx_trans->length;
    iot_i2c_states[port].tx_trans.callback = tx_trans->callback;
    dev_addr_temp = (uint16_t)(dev_addr << 1);
    BIT_CLR(dev_addr_temp, 0);   //lint !e835:code style

    if (!iot_i2c_states[port].tx_trans.length) {
        return RET_INVAL;
    }
    i2c_nack_it_clear(p, true);
    i2c_underflow_it_clear(p, true);
    i2c_overflow_it_clear(p, true);
    i2c_empty_it_clear(p, true);
    i2c_full_it_clear(p, true);
    i2c_done_it_clear(p, true);

    /*step2: set data length*/
    i2c_set_data_length(p, I2C_RW_DIR_WRITE, (uint16_t)(tx_trans->length + 1));
    /*step1: write config*/
    i2c_clear_status(p);
    i2c_set_mode(p, I2C_SET_WRITE);
    /*write the write mode*/
    i2c_tx_fifo_write_data(p, (uint8_t)dev_addr_temp);
    i2c_tx_fifo_throshold(p, (uint8_t)MIN(I2C_TX_FIFO_THROSHOLD, tx_trans->length));

    if (iot_i2c_states[port].tx_trans.length < I2C_TX_FIFO_FULL_SIZE) {
        while (iot_i2c_states[port].tx_trans.length != 0) {
            iot_i2c_states[port].tx_trans.length--;
            i2c_tx_fifo_write_data(p, *iot_i2c_states[port].tx_trans.buffer++);
        }
        i2c_done_it_enable(p, true);
        i2c_nack_it_enable(p, true);

    } else {
        for (i = 0; i < I2C_TX_FIFO_FULL_SIZE; i++) {
            i2c_tx_fifo_write_data(p, *iot_i2c_states[port].tx_trans.buffer++);
            iot_i2c_states[port].tx_trans.length--;
        }
        i2c_underflow_it_enable(p, true);
        i2c_nack_it_enable(p, true);
    }
    i2c_trans_start(p);

    return RET_OK;
}

uint8_t iot_i2c_master_receive_it(IOT_I2C_PORT port, uint16_t dev_addr,
                                  const iot_i2c_transaction_t *rx_trans)
{
    I2C_PORT p = (I2C_PORT)port;

    if (rx_trans->callback == NULL || rx_trans->buffer == NULL || rx_trans->length == 0) {
        return RET_INVAL;
    }

    if (iot_i2c_states[port].rx_trans.callback || iot_i2c_states[port].rx_trans.buffer
        || iot_i2c_states[port].rx_trans.length) {
        return RET_AGAIN;
    }

    iot_i2c_states[port].rx_trans.buffer = rx_trans->buffer;
    iot_i2c_states[port].rx_trans.length = rx_trans->length;
    iot_i2c_states[port].rx_trans.callback = rx_trans->callback;

    uint16_t dev_addr_temp;
    dev_addr_temp = (uint16_t)(dev_addr << 1);
    BIT_SET(dev_addr_temp, 0);   //lint !e835:code style

    i2c_nack_it_clear(p, true);
    i2c_underflow_it_clear(p, true);
    i2c_overflow_it_clear(p, true);
    i2c_empty_it_clear(p, true);
    i2c_full_it_clear(p, true);
    i2c_done_it_clear(p, true);

    /*step1: write config*/
    i2c_set_data_length(p, I2C_RW_DIR_READ, (uint16_t)iot_i2c_states[port].rx_trans.length);
    i2c_set_data_length(p, I2C_RW_DIR_WRITE, 1);
    i2c_set_mode(p, I2C_SET_READ);
    /* set rx fifo throshold */
    i2c_rx_fifo_throshold(p, (uint8_t)MIN(I2C_RX_FIFO_THROSHOLD, rx_trans->length));
    /*write the read mode, enable it*/
    i2c_tx_fifo_write_data(p, (uint8_t)dev_addr_temp);
    /*it enable */
    i2c_overflow_it_enable(p, true);
    i2c_nack_it_enable(p, true);
    /*step3: start the fifo tx*/
    i2c_trans_start(p);

    return RET_OK;
}

uint8_t iot_i2c_master_receive_from_memory_it(IOT_I2C_PORT port, uint16_t dev_addr,
                                              uint16_t mem_addr, IOT_I2C_MEMORY_ADDR_SIZE addr_size,
                                              const iot_i2c_transaction_t *rx_trans)
{
    uint8_t temp_buf[2] = {0};

    if (addr_size == IOT_I2C_MEMORY_ADDR_8BIT) {
        temp_buf[0] = (mem_addr & 0xFF);
        iot_i2c_master_transmit_poll(port, dev_addr, temp_buf, 1, 1000);
    } else if (addr_size == IOT_I2C_MEMORY_ADDR_16BIT) {
        temp_buf[0] = (uint8_t)((mem_addr & 0xFF00) >> 8);
        temp_buf[1] = (uint8_t)(mem_addr & 0x00FF);
        iot_i2c_master_transmit_poll(port, dev_addr, temp_buf, 2, 1000);
    } else {
        return RET_INVAL;
    }
    i2c_transfer_stop((I2C_PORT)port);
    return iot_i2c_master_receive_it(port, dev_addr, rx_trans);
}

uint8_t iot_i2c_master_transmit_to_memory_it(IOT_I2C_PORT port, uint16_t dev_addr,
                                             uint16_t mem_addr, IOT_I2C_MEMORY_ADDR_SIZE addr_size,
                                             const iot_i2c_transaction_t *tx_trans)
{
    uint8_t temp_buf[2] = {0};
    uint8_t dataBuf[256];
    iot_i2c_transaction_t current_tx;
    current_tx.buffer = dataBuf;
    current_tx.length = 0;

    if (addr_size == IOT_I2C_MEMORY_ADDR_8BIT) {
        temp_buf[0] = (mem_addr & 0xFF);
        current_tx.length += (tx_trans->length + 1);
        memcpy(current_tx.buffer, temp_buf, 1);
        current_tx.buffer++;
        memcpy(current_tx.buffer, tx_trans->buffer, tx_trans->length);
        iot_i2c_master_transmit_it(port, dev_addr, &current_tx);
    } else if (addr_size == IOT_I2C_MEMORY_ADDR_16BIT) {
        temp_buf[0] = (uint8_t)((mem_addr & 0xFF00) >> 8);
        temp_buf[1] = (uint8_t)(mem_addr & 0x00FF);
        current_tx.length += (tx_trans->length + 2);
        memcpy(current_tx.buffer, temp_buf, 2);
        current_tx.buffer += 2;
        memcpy(current_tx.buffer, tx_trans->buffer, tx_trans->length);
        iot_i2c_master_transmit_it(port, dev_addr, &current_tx);
    } else {
        return RET_INVAL;
    }

    return RET_OK;
}

static void iot_i2c_overflow_cb(IOT_I2C_PORT port)
{
    uint16_t i;
    I2C_PORT p = (I2C_PORT)port;

    uint8_t read_size = (uint8_t)MIN(iot_i2c_states[port].rx_trans.length, I2C_RX_FIFO_THROSHOLD);
    i2c_overflow_it_enable(p, false);
    for (i = 0; i < read_size; i++) {
        *iot_i2c_states[port].rx_trans.buffer++ = i2c_rx_fifo_read_data(p);
        i2c_rx_fifo_we(p);
        iot_i2c_states[port].rx_trans.length -= sizeof(uint8_t);
    }
    if (iot_i2c_states[port].rx_trans.length > 0) {
        if (iot_i2c_states[port].rx_trans.length < I2C_RX_FIFO_THROSHOLD) {
            i2c_rx_fifo_throshold(p, (uint8_t)iot_i2c_states[port].rx_trans.length);
        }
        i2c_overflow_it_enable(p, true);
    } else {
        if (iot_i2c_states[port].rx_trans.callback != NULL) {
            iot_i2c_states[port].rx_trans.callback();
        }
    }
}

static void iot_i2c_underflow_cb(IOT_I2C_PORT port)
{
    uint16_t i;
    I2C_PORT p = (I2C_PORT)port;

    i2c_underflow_it_enable(p, false);
    if (iot_i2c_states[port].tx_trans.length < I2C_TX_FIFO_FULL_SIZE) {
        while (iot_i2c_states[port].tx_trans.length != 0) {
            iot_i2c_states[port].tx_trans.length--;
            i2c_tx_fifo_write_data(p, *iot_i2c_states[port].tx_trans.buffer++);
        }
    } else {
        for (i = 0; i < I2C_TX_FIFO_FULL_SIZE; i++) {
            i2c_tx_fifo_write_data(p, *iot_i2c_states[port].tx_trans.buffer++);
            iot_i2c_states[port].tx_trans.length--;
        }
        i2c_underflow_it_enable(p, true);
    }
}

static void iot_i2c_tx_complete_cb(IOT_I2C_PORT port)
{
    i2c_done_it_enable((I2C_PORT)port, false);
}

static void iot_i2c_full_cb(IOT_I2C_PORT port)
{
    UNUSED(port);
}

static void iot_i2c_empty_cb(IOT_I2C_PORT port)
{
    UNUSED(port);
}

static void iot_i2c_nack_timeout_cb(IOT_I2C_PORT port)
{
    UNUSED(port);
}

static uint32_t iot_i2c_isr_handler(uint32_t vector, uint32_t data) IRAM_TEXT(iot_i2c_isr_handler);
static uint32_t iot_i2c_isr_handler(uint32_t vector, uint32_t data)
{
    UNUSED(vector);
    IOT_I2C_PORT port = (IOT_I2C_PORT)data;
    I2C_PORT p = (I2C_PORT)data;

    if (i2c_get_underflow_it_status(p)) {
        iot_i2c_underflow_cb(port);
        i2c_underflow_it_clear(p, true);
    } else if (i2c_get_overflow_it_status(p)) {
        iot_i2c_overflow_cb(port);
        i2c_overflow_it_clear(p, true);
    } else if (i2c_get_done_it_status(p)) {
        iot_i2c_tx_complete_cb(port);
        i2c_done_it_clear(p, true);
    } else if (i2c_get_empty_it_status(p)) {
        iot_i2c_empty_cb(port);
        return RET_INVAL;
    } else if (i2c_get_full_it_status(p)) {
        iot_i2c_full_cb(port);
        return RET_INVAL;
    } else if (i2c_get_nack_it_status(p)) {
        iot_i2c_nack_timeout_cb(port);
        return RET_INVAL;
    }
    return RET_OK;
}
