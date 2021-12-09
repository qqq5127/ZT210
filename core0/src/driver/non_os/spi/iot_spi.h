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

#ifndef _DRIVER_NON_OS_SPI_H
#define _DRIVER_NON_OS_SPI_H

/**
 * @addtogroup HAL
 * @{
 * @addtogroup SPI
 * @{
 * This section introduces the SPI module's enum, structure, functions and how to use this driver.
 */

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define IOT_SPI_DEFAULT_RX_THRESHOULD 12
#define IOT_SPI_DEFAULT_TX_THRESHOULD 5
#define IOT_SPI_DEFAULT_FREQUENCY     (100 * 1000)
#define IOT_SPI_SEND_DUMMY_BYTE       0xFF

/**
 * @defgroup hal_spi_enum Enum
 * @{
 */

/** @brief SPI port. */
typedef enum {
    IOT_SPI_PORT0,
    IOT_SPI_PORT_SLAVE_MAX,
    IOT_SPI_PORT1 = 1,
    IOT_SPI_PORT_MASTER_MAX,
    IOT_SPI_PORT_MAX = IOT_SPI_PORT_MASTER_MAX,
} IOT_SPI_PORT;

/** @brief SPI mode. */
typedef enum {
    IOT_SPI_MASTER,
    IOT_SPI_SLAVE,
} IOT_SPI_MODE;
/**
 * @}
 */

/**
 * @defgroup hal_spi_struct Struct
 * @{
 */
typedef struct iot_spi_gpio_cfg {
    uint16_t clk;
    uint16_t cs;
    uint16_t mosi;
    uint16_t miso;
} iot_spi_gpio_cfg_t;

typedef struct iot_spi_dma_cfg {
    bool_t use_dma;
} iot_spi_dma_cfg_t;

typedef struct iot_spi_cfg {
    bool_t auto_cs;
} iot_spi_cfg_t;
/**
 * @}
 */

/**
 * @defgroup hal_spi_typedef Typedfe
 * @{
 */
typedef void (*iot_spi_callback)(void);
/**
 * @}
 */

/**
 * @brief This function is to initialize SPI module.
 *
 * @param port is the SPI port.
 * @param mode is the SPI mode.
 */
void iot_spi_init(IOT_SPI_PORT port, IOT_SPI_MODE mode);

/**
 * @brief This function is to deinitialize SPI module.
 *
 * @param port is the SPI port.
 */
void iot_spi_deinit(IOT_SPI_PORT port);

/**
 * @brief This function is to open SPI.
 *
 * @param port is the SPI port.
 * @param cfg is configuration of the SPI.
 * @param gpio_cfg is configuration of the SPI gpio.
 * @param dma_cfg is configuration of the SPI dma.
 * @return int -1 or 0.
 */
int iot_spi_open(IOT_SPI_PORT port, const iot_spi_cfg_t *cfg,
                 const iot_spi_gpio_cfg_t *gpio_cfg, iot_spi_dma_cfg_t *dma_cfg);

/**
 * @brief This function is to close the SPI module.
 *
 * @param port is the SPI port.
 * @return int 0.
 */
int iot_spi_close(IOT_SPI_PORT port);

/**
 * @brief This function is to config the SPI pin.
 *
 * @param port is the SPI port.
 * @param mode is the SPI mode.
 * @param gpio_cfg is configuration of the SPI gpio.
 * @return bool_t true or false.
 */
bool_t iot_spi_pin_config(IOT_SPI_PORT port, IOT_SPI_MODE mode,
                          const iot_spi_gpio_cfg_t *gpio_cfg);

/**
 * @brief This function is to send data.
 *
 * @param port is the SPI port.
 * @param cmd_buff is the address of command buffer.
 * @param cmd_len is the length of command.
 * @param data_buff is the address of data buffer.
 * @param data_len is the length of data.
 * @param callback is SPI callback function.
 * @return bool_t true if send success otherwise false.
 */
bool_t iot_spi_send_data(IOT_SPI_PORT port, uint8_t *cmd_buff, uint16_t cmd_len,
                         uint8_t *data_buff, uint16_t data_len,
                         iot_spi_callback callback);

/**
 * @brief This function is to receive data.
 *
 * @param port is the SPI port.
 * @param cmd_buff is the address of command buffer.
 * @param cmd_len is the length of command.
 * @param data_buff is the address of data buffer.
 * @param data_len is the address of data buffer.
 * @param callback is SPI callback function.
 * @return bool_t true if receive success otherwise false.
 */
bool_t iot_spi_recv_data(IOT_SPI_PORT port, uint8_t *cmd_buff, uint16_t cmd_len,
                         uint8_t *data_buff, uint16_t data_len,
                         iot_spi_callback callback);

#ifdef __cplusplus
}
#endif

/**
 * @}
 * @}
 */
#endif /* _DRIVER_NON_OS_SPI_H */
