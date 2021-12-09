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
#ifndef _DRIVER_NON_OS_OTP_H
#define _DRIVER_NON_OS_OTP_H
/**
 * @addtogroup HAL
 * @{
 * @addtogroup OTP
 * @{
 * This section introduces the OTP module's enum, structure, functions and how to use this driver.
 */

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief This function is to write soc id into flash. soc id ared stored with two words.
 *
 * @param[in] soc_id is array, e.g uint32_t soc_id[2];
 * @return uint8_t RET_OK.
 */
uint8_t iot_otp_set_soc_id(const uint32_t *soc_id);

/**
 * @brief This function is to read soc id from flash. soc id ared stored with two words.
 *
 * @param[out] soc_id is array, eng. uint32_t soc id[2].
 * @return uint8_t RET_OK.
 */
uint8_t iot_otp_get_soc_id(uint32_t *soc_id);

/**
 * @brief This function is to write device id to flash.
 *
 * @param device_id is chip device id.
 * @return uint8_t RET_OK.
 */
uint8_t iot_otp_set_device_id(const uint32_t *device_id);

/**
 * @brief This function is to read device id from flash.
 *
 * @param[out] device_id is device id saved in the otp.
 * @return uint8_t RET_NOT_EXIST or RET_OK.
 */
uint8_t iot_otp_get_device_id(uint32_t *device_id);

/**
 * @brief This function is to get flash iomap.
 *
 * @param[out] iomap is flash iomap looked up from IOMAP table.
 * @return uint8_t RET_INVAL or RET_OK.
 */
uint8_t iot_otp_get_flash_iomap(uint32_t *iomap);

#ifdef __cplusplus
}
#endif

/**
 * @}
 * addtogroup OTP
 * @}
 * addtogroup HAL
 */

#endif /* _DRIVER_NON_OS_OTP_H */
