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
#ifndef _DRIVER_HAL_PDM_H
#define _DRIVER_HAL_PDM_H

/**
 * @addtogroup HAL
 * @{
 * @addtogroup PDM
 * @{
 * This section introduces the PDM module's enum, structure, functions and how to use this driver.
 */

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup hal_pdm_enum Enum
 * @{
 */

/** @brief PDM port. */
typedef enum {
    IOT_PDM_PORT_0,
    IOT_PDM_PORT_1,
    IOT_PDM_PORT_2,
    IOT_PDM_PORT_MAX,
} IOT_PDM_PORT;
/**
 * @}
 */

/**
 * @defgroup hal_pdm_struct Struct
 * @{
 */
typedef struct iot_pdm_gpio_cfg {
    uint16_t clk;
    uint16_t sd;
} iot_pdm_gpio_cfg_t;
/**
 * @}
 */

/**
 * @brief This function is to enable pdm module.
 *
 * @param port is pdm port.
 */
void iot_pdm_enable(IOT_PDM_PORT port);

/**
 * @brief This function is to disable pdm module.
 *
 * @param port is pdm port.
 */
void iot_pdm_disable(IOT_PDM_PORT port);

/**
 * @brief This function is to start pdm.
 *
 * @param port is pdm port.
 */
void iot_pdm_start(IOT_PDM_PORT port);

/**
 * @brief This function is to stop pdm.
 *
 * @param port is pdm port.
 */
void iot_pdm_stop(IOT_PDM_PORT port);

/**
 * @brief This function is to reset pdm.
 *
 * @param port is pdm port.
 */
void iot_pdm_reset(IOT_PDM_PORT port);

/**
 * @brief This function is to config pdm.
 *
 * @param port is pdm port.
 * @return uint8_t RET_INVAL or RET_OK.
 */
uint8_t iot_pdm_config(IOT_PDM_PORT port);

/**
 * @brief This function is to config pdm pin.
 *
 * @param port is pdm port.
 * @param gpio_cfg is pdm gpio configuration.
 * @return uint8_t RET_INVAL or RET_OK.
 */
uint8_t iot_pdm_pin_config(IOT_PDM_PORT port, const iot_pdm_gpio_cfg_t *gpio_cfg);

#ifdef __cplusplus
}
#endif

/**
 * @}
 * @}
 */
#endif   //_DRIVER_HAL_PDM_H
