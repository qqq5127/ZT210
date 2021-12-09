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

#ifndef __DRIVER_HAL_I2C_H__
#define __DRIVER_HAL_I2C_H__

/**
 * @addtogroup HAL
 * @{
 * @addtogroup I2C
 * @{
 * This section introduces the I2C module's enum, structure, functions and how to use this driver.
 */

#include "iot_irq.h"
#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif
#include "i2c.h"

#define I2C_TX_FIFO_FULL_SIZE 0x0F
#define I2C_RX_FIFO_FULL_SIZE 0x10
/* if tx_fifo_data_num <= i2c_txfifo_thrs, then the interrupt will be triggered. */
#define I2C_TX_FIFO_THROSHOLD 0x01U
/* if rx_fifo_data_num >= i2c_rxfifo_thrs, then the interrupt will be triggered. */
#define I2C_RX_FIFO_THROSHOLD 0x08U

/**
 * @defgroup hal_i2c_enum Enum
 * @{
 */

/** @brief I2C port. */
typedef enum {
    IOT_I2C_PORT_0,
    IOT_I2C_PORT_1,
    IOT_I2C_PORT_MAX,
} IOT_I2C_PORT;

/** @brief I2C mode. */
typedef enum {

    IOT_I2C_MODE_NONE,
    IOT_I2C_MODE_MASTER,
    IOT_I2C_MODE_SLAVE,
} IOT_I2C_MODE;

/** @brief I2C memory address size. */
typedef enum {
    IOT_I2C_MEMORY_ADDR_8BIT,
    IOT_I2C_MEMORY_ADDR_16BIT,
    IOT_I2C_MEMORY_ADDR_32BIT,
} IOT_I2C_MEMORY_ADDR_SIZE;
/**
 * @}
 */

/**
 * @defgroup hal_i2c_typedef Typedef
 * @{
 */
typedef void (*iot_i2c_callback)(void);
/**
 * @}
 */

/**
 * @defgroup hal_i2c_struct Struct
 * @{
 */
typedef struct iot_i2c_gpio_cfg {
    uint8_t scl;
    uint8_t sda;
} iot_i2c_gpio_cfg_t;

typedef struct iot_i2c_config {
    uint8_t i2c_busrt_mode;
    /** Device baudrate */
    uint32_t baudrate;
    /**time-out if there is no ack during this setting time.
     *value is (wait_nack_max+1) i2c clk cycle. i2c clk is also APB CLK.
     */
    uint16_t wait_nack_max_time;
} iot_i2c_config_t;

typedef struct iot_i2c_transaction {
    iot_i2c_callback callback;
    uint8_t *buffer;
    uint32_t length;
} iot_i2c_transaction_t;
/**
 * @}
 */

/**
 * @brief This function is to init i2c.
 *
 * @param port is i2c port.
 * @param cfg is i2c configuration.
 * @return uint8_t RET_OK for success else error.
 */
uint8_t iot_i2c_init(IOT_I2C_PORT port, const iot_i2c_config_t *cfg);

/**
 * @brief This function is to deinitialize i2c.
 *
 * @param port is i2c port.
 * @return uint8_t RET_OK for success else error.
 */
uint8_t iot_i2c_deinit(IOT_I2C_PORT port);

/**
 * @brief This function is to open i2c.
 *
 * @param port is i2c port.
 * @param gpio is i2c gpio configuration.
 * @return uint8_t RET_OK for success else error.
 */
uint8_t iot_i2c_open(IOT_I2C_PORT port, const iot_i2c_gpio_cfg_t *gpio);

/**
 * @brief This function is to close i2c.
 *
 * @param port is i2c port.
 * @return uint8_t RET_OK for success else error.
 */
uint8_t iot_i2c_close(IOT_I2C_PORT port);

/**
 * @brief Save all i2c state
 *
 * @param data saved data
 * @return uint32_t RET_OK for succ otherwise fail
 */
uint32_t iot_i2c_save(uint32_t data);

/**
 * @brief Restore all i2c state
 *
 * @param data saved data
 * @return uint32_t RET_OK for succ otherwise fail
 */
uint32_t iot_i2c_restore(uint32_t data);

/**
 * @brief This function is to set interrupt configuration.
 *
 * @param port is i2c port.
 */
void iot_i2c_interrupt_config(IOT_I2C_PORT port);

/**
 * @brief This function is to transmit poll to master.
 *
 * @param port is i2c port.
 * @param dev_addr Device address.
 * @param data Data to transmit.
 * @param size Length of the data to transmit.
 * @param timeout Time out of allowed.
 * @return uint8_t RET_OK for success else error.
 */
uint8_t iot_i2c_master_transmit_poll(IOT_I2C_PORT port, uint16_t dev_addr,
                                     uint8_t *data, uint16_t size,
                                     uint32_t timeout);

/**
 * @brief This function is to recevie poll from master.
 *
 * @param port is i2c port.
 * @param dev_addr Device address.
 * @param data Data to receive.
 * @param size Length of the data to receive.
 * @param timeout Time out of allowed.
 * @return uint8_t RET_OK for success else error.
 */
uint8_t iot_i2c_master_receive_poll(IOT_I2C_PORT port, uint16_t dev_addr,
                                    uint8_t *data, uint16_t size,
                                    uint32_t timeout);

/**
 * @brief This function is to transmit it to master.
 *
 * @param port is i2c port.
 * @param dev_addr Device address.
 * @param tx_trans Transaction to send.
 * @return uint8_t RET_OK for success else error.
 */
uint8_t iot_i2c_master_transmit_it(IOT_I2C_PORT port, uint16_t dev_addr,
                                   const iot_i2c_transaction_t *tx_trans);

/**
 * @brief This function is to receive it from master.
 *
 * @param port is i2c port.
 * @param dev_addr Device address.
 * @param rx_trans Transaction to receive.
 * @return uint8_t RET_OK for success or RTE_INVAL for rx_trans invalid or RET_AGAIN for port exist.
 */
uint8_t iot_i2c_master_receive_it(IOT_I2C_PORT port, uint16_t dev_addr,
                                  const iot_i2c_transaction_t *rx_trans);

/**
 * @brief This function is to receive poll from memory.
 *
 * @param port is i2c port.
 * @param dev_addr Device address.
 * @param mem_addr Memory address.
 * @param addr_size Size of address.
 * @param data Data to receive.
 * @param size Length of the data to receive.
 * @param timeout Time out of allowed.
 * @return uint8_t RET_OK for success else error.
 */
uint8_t iot_i2c_master_receive_from_memory_poll(
    IOT_I2C_PORT port, uint16_t dev_addr, uint16_t mem_addr,
    IOT_I2C_MEMORY_ADDR_SIZE addr_size, uint8_t *data, uint16_t size,
    uint32_t timeout);

/**
 * @brief This function is to transmit poll to memory.
 *
 * @param port I2C port.
 * @param dev_addr Device address.
 * @param mem_addr Memory address.
 * @param addr_size Size of address.
 * @param data Data to transmit.
 * @param size Length of the data to transmit.
 * @param timeout Time out of allowed.
 * @return uint8_t RET_OK for success else error.
 */
uint8_t iot_i2c_master_transmit_to_memory_poll(
    IOT_I2C_PORT port, uint16_t dev_addr, uint16_t mem_addr,
    IOT_I2C_MEMORY_ADDR_SIZE addr_size, const uint8_t *data, uint8_t size,
    uint32_t timeout);

/**
 * @brief This function is to receive it from memory.
 *
 * @param port is i2c port.
 * @param dev_addr Device address.
 * @param mem_addr Memory address.
 * @param addr_size Size of address.
 * @param rx_trans Transaction to receive.
 * @return uint8_t RET_OK for success else error.
 */
uint8_t iot_i2c_master_receive_from_memory_it(
    IOT_I2C_PORT port, uint16_t dev_addr, uint16_t mem_addr,
    IOT_I2C_MEMORY_ADDR_SIZE addr_size, const iot_i2c_transaction_t *rx_trans);

/**
 * @brief This function is to transmit it to memory.
 *
 * @param port is i2c port.
 * @param dev_addr Device address.
 * @param mem_addr Memory address.
 * @param addr_size Size of address.
 * @param tx_trans Transaction to transmit.
 * @return uint8_t RET_OK for success else error.
 */
uint8_t iot_i2c_master_transmit_to_memory_it(IOT_I2C_PORT port,
                                             uint16_t dev_addr,
                                             uint16_t mem_addr,
                                             IOT_I2C_MEMORY_ADDR_SIZE addr_size,
                                             const iot_i2c_transaction_t *tx_trans);

#ifdef __cplusplus
}
#endif

/**
 * @}
 * @}
 */
#endif
