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
#ifndef _DRIVER_NON_OS_FLASH_H
#define _DRIVER_NON_OS_FLASH_H
/**
 * @addtogroup HAL
 * @{
 * @addtogroup FLASH
 * @{
 * This section introduces the FLASH module's enum, structure, functions and how to use this driver.
 */

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define REGION_ADDR_OF_ID(id)          ((id + 1)*0x1000)
#define PAGE_PROGRAM_MASK              0xFF
#define SECTOR_ERASE_MASK              0xFFF
#define BLOCK_ERASE_32K_MASK           0x1FFFF
#define BLOCK_ERASE_64K_MASK           0xFFFF

#define PAGE_NUM_PER_SECTOR            0x10
#define PAGE_PROGRAM_SIZE              0x100
#define SECTOR_ERASE_SIZE              0x1000
#define BLOCK_ERASE_32K_SIZE           0x8000
#define BLOCK_ERASE_64K_SIZE           0x10000

#define IOT_FLASH_OTP_REGIN_MASK       0x3000
#define IOT_FLASH_OTP_REGIN_VALID_BYTE 0x1ff
#define IOT_FLASH_OTP_REGIN_SIZE       512

/** @defgroup hal_flash_enum Enum
  * @{
  */

/**
 * @brief flash otp region id.
 *
*/
typedef enum {
    IOT_FLASH_OTP_REGION0,
    IOT_FLASH_OTP_REGION1,
    IOT_FLASH_OTP_REGION2,
    IOT_FLASH_OTP_REGION_MAX,
} IOT_FLASH_OTP_REGION_ID;

typedef void (*iot_flash_pe_cb)(void);

/**
 * @brief flash program/erase mode
 *
 */
typedef enum{
    IOT_FLASH_PE_HW_MODE,
    IOT_FLASH_PE_SW_MODE,
}IOT_FLASH_PE_MODE;

/**
 * @}
 */

/**
 * @brief This function is to erase flash sector according to the sector id.
 *
 * @param sector_id is flash sector id.
 * @return uint8_t RET_INVAL or RET_OK,RET_OK for success else error..
 */
uint8_t iot_flash_erase_sector(uint16_t sector_id);

/**
 * @brief This function is to erase flash sector according to the sector addr.
 *
 * @param addr is erase flash addr
 * @return uint8_t RET_INVAL or RET_OK,RET_OK for success else error..
 */
uint8_t iot_flash_erase(uint32_t addr);

/**
 * @brief This function is to chip erase flash.
 *
 * @return uint8_t RET_OK
 */
uint8_t iot_flash_chip_erase(void);

/**
 * @brief This function is to read data from flash addr.
 *
 * @param addr is flash addr
 * @param buf is buffer for the data being read
 * @param count is buffer len
 * @return uint8_t RET_OK or RET_INVAL,RET_OK for success else error..
 */
uint8_t iot_flash_read(uint32_t addr, void *buf, size_t count);

/**
 * @brief This function is to write data into flash addr.
 *
 * @param addr is flash addr
 * @param buf is buffer ready write to flash
 * @param count is buffer len
 * @return uint8_t RET_OK or RET_INVAL,RET_OK for success else error..
 */
uint8_t iot_flash_write(uint32_t addr, void *buf, size_t count);

/**
 * @brief This function is to wtite data into flash addr without erase.
 *
 * @param addr is flash addr
 * @param buf is buffer ready write to flash
 * @param count is buffer len
 * @return uint8_t RET_OK or RET_INVAL,RET_OK for success else error..
 */
uint8_t iot_flash_write_without_erase(uint32_t addr, const void *buf, size_t count);

/**
 * @brief This function is to get flash id.
 *
 * @return uint16_t 16 bytes of flash id.
 */
uint16_t iot_flash_get_id(void);

/**
 * @brief This function is to init flash.
 *
 */
void iot_flash_init(void);

/**
 * @brief This function is to check if flash is inited.
 *
 * @return bool_t true or false
 */
bool_t iot_flash_is_init(void);

/**
 * @brief This function is to set flash io map.
 *
 * @param map is flash io map
 */
void iot_flash_set_io_map(uint32_t map);

/**
 * @brief This function is to enable flash's quad mode.
 *
 */
void iot_flash_enable_quad_mode(void);

/**
 * @brief This function is to enable flash's qpp mode.
 *
 */
void iot_flash_enable_qpp_mode(void);

/**
 * @brief This function is to write data into flash through otp certification.
 *
 * @param id is flash otp region id
 * @param addr is otp region inter addr
 * @param buf is buffer ready write into flash
 * @param count is data buffer lenth
 * @return uint8_t RET_OK or RET_INVAL,RET_OK for success else error..
 */
uint8_t iot_flash_otp_write(IOT_FLASH_OTP_REGION_ID id, uint32_t addr, const void *buf, size_t count);

/**
 * @brief This function is to read data from flash through otp certification.
 *
 * @param id is flash otp region id
 * @param addr is otp region inter addr
 * @param buf is buffer for the data being read
 * @param count is buffer len
 * @return uint8_t RET_OK or RET_INVAL,RET_OK for success else error..
 */
uint8_t iot_flash_otp_read(IOT_FLASH_OTP_REGION_ID id, uint32_t addr, void *buf, size_t count);

/**
 * @brief This function is to lock flsh region through otp certification.
 *
 * @param id is Flash otp region id
 */
void iot_flash_otp_lock(IOT_FLASH_OTP_REGION_ID id);

/**
 * @brief This function is to enable flash download mode.
 *
 * @param enable is enable download mode
 */
void iot_flash_set_download_mode(bool_t enable);

/**
 * @brief This function is to set flash cache mode.
 *
 */
void iot_flash_set_cache_mode(void);

/**
 * @brief This function is to get flash size.
 *
 * @return uint32_t flash size.
 */
uint32_t iot_flash_get_size(void);

/**
 * @brief This function is to get vendor id.
 *
 * @return uint32_t vendor id.
 */
uint32_t iot_flash_get_vendor(void);

/**
 * @brief This function is to write soc id into flash.
 *
 * @param soc_id is soc id
 */
void iot_flash_set_soc_id(const uint32_t *soc_id);

/**
 * @brief This function is to read soc id from flash.
 *
 * @param soc_id is soc id
 */
void iot_flash_get_soc_id(uint32_t *soc_id);

/**
 * @brief This function is to disable QPP mode.
 *
 */
void iot_flash_disable_qpp_mode(void);

/**
 * @brief This function is to get flash's version.
 *
 * @return uint32_t is flash version.
 */
uint32_t iot_flash_get_version(void);


/**
 * @brief This function is to set WIP wait time when erase.
 *
 * @param p_time is WIP wait time when program, unit is us.
 * @param e_time is WIP wait time when erase, unit is us.
 */
void iot_flash_set_wip_wait_time(uint32_t p_time, uint32_t e_time);

/**
 * @brief This function is to register program/erase's callback function.
 *
 * @param cb is the callback.
 * @return uint8_t RET_OK for success else error.
 */
uint8_t iot_flash_register_pe_callback(iot_flash_pe_cb cb);

/**
 * @brief This function is to unregister program/erase's callback function.
 *
 * @return uint8_t RET_OK for success else error.
 */
uint8_t iot_flash_unregister_pe_callback(void);

/**
 * @brief This function is to get wheter program/erase is in progress.
 *
 * @return bool_t is true when run program/erase command.
 */
bool_t iot_flash_is_pe_in_progress(void);

/**
 * @brief This function is to set program/erase mode.
 *
 * @param mode is IOT_FLASH_PE_SW_MODE or IOT_FLASH_PE_HW_MODE.
 */
void iot_flash_set_pe_mode(IOT_FLASH_PE_MODE mode);

/**
 * @brief This function is to get program/erase mode.
 *
 * @return mode is IOT_FLASH_PE_SW_MODE or IOT_FLASH_PE_HW_MODE.
 */
IOT_FLASH_PE_MODE iot_flash_get_pe_mode(void);
#ifdef __cplusplus
}
#endif

/**
 * @}
 * addtogroup FLASH
 * @}
 * addtogroup HAL
 */

#endif /* _DRIVER_NON_OS_FLASH_H */
