/****************************************************************************

Copyright(c) 2016 by WuQi Technologies. ALL RIGHTS RESERVED.

This Information is proprietary to WuQi Technologies and MAY NOT
be copied by any method or incorporated into another program without
the express written consent of WuQi. This Information or any portion
thereof remains the property of WuQi. The Information contained herein
is believed to be accurate and WuQi assumes no responsibility or
liability for its use in any way and conveys no license or title under
any patent or copyright and makes no representation or warranty that this
Information is free from patent or copyright infringement.

****************************************************************************/
#ifndef DRIVER_NON_OS_BOOT_MAP_H
#define DRIVER_NON_OS_BOOT_MAP_H
/**
 * @addtogroup HAL
 * @{
 * @addtogroup BOOT_MAP
 * @{
 * This section introduces the BOOT_MAP module's enum, structure, functions and how to use this driver.
 */

#ifdef __cplusplus
extern "C" {
#endif

#define BOOT_MAP_MAGIC 0x5771424D   // WqBM
#define FLASH_IMAGE_MAGIC 0xa4e49a17

#define BOOT_SECTION_OTA      0x0
#define BOOT_SECTION_TRY      0xFF
#define BOOT_SECTION_SBL      0xFE
#define BOOT_SECTION_STANDARD 0x80
#define IMAGE_NUM             6
#define MAX_IMAGE_NUM         16

/** @defgroup hal_image_type_enum Enum
  * @{
  */

/** @brief IMAGE TYPE.*/
typedef enum {
    IMAGE_TYPE_ROM = 0,
    IMAGE_TYPE_PBL,
    IMAGE_TYPE_OEM,
    IMAGE_TYPE_CUS,
    IMAGE_TYPE_OTA,
    IMAGE_TYPE_TONE,

    IMAGE_TYPE_SBL0 = 0x10,
    IMAGE_TYPE_SBL1,

    IMAGE_TYPE_CORE0_FW0 = 0x20,
    IMAGE_TYPE_CORE0_FW1 = 0x21,
    IMAGE_TYPE_CORE0_TBL0 = 0x22,
    IMAGE_TYPE_CORE0_TBL1 = 0x23,
    IMAGE_TYPE_CORE1_FW0 = 0x30,
    IMAGE_TYPE_CORE1_FW1 = 0x31,
    IMAGE_TYPE_CORE1_TBL0 = 0x32,
    IMAGE_TYPE_CORE1_TBL1 = 0x33,
    IMAGE_TYPE_CORE2_FW0 = 0x40,
    IMAGE_TYPE_CORE2_FW1 = 0x41,
    IMAGE_TYPE_CORE2_TBL0 = 0x42,
    IMAGE_TYPE_CORE2_TBL1 = 0x43,

    IMAGE_TYPE_DSP_FW0 = 0x70,
    IMAGE_TYPE_DSP_FW1 = 0x71,

    IMAGE_TYPE_MAX = 0xFF,
} IOT_IMAGE_TYPE;

/** @brief BOOT mode.*/
typedef enum {
    IOT_BOOT_MODE_NORMAL,
    IOT_BOOT_MODE_OTA,

    IOT_BOOT_MODE_UNKONWN = 0xFF,
} IOT_BOOT_MODE;
/**
 * @}
 */

/** @defgroup hal_boot_map_struct Struct
  * @{
  */
typedef struct iot_boot_map_image {
    uint8_t image_type;
    uint8_t reserved[3];
    /*load offset of this image */
    uint32_t code_lma;
    /*run address of this imag*/
    uint32_t code_vma;
    /*image size*/
    uint32_t code_length;
    /*image max size*/
    uint32_t code_area_size;
} iot_boot_map_image_t;

typedef struct iot_boot_map {
    uint32_t magic;
    uint8_t image_num;
    uint8_t boot_type;
    uint8_t reserved[2];
} iot_boot_map_t;

typedef struct iot_flash_image_header {
    uint32_t guard;
    uint32_t length;
    uint32_t version;
    uint32_t start;
    uint32_t crc;
    uint8_t tlv_count;
    uint8_t reserved[11];
} iot_flash_image_header_t;
/**
  * @}
  */

/**
 * @brief This function is to get bootmap's vaildity.
 *
 * @param map is boot map
 * @return uint8_t RET_OK for success else for error.
 */
uint8_t iot_boot_map_get_valid_map(iot_boot_map_t *map);

/**
 * @brief This function is to check boot map's vaildity.
 *
 * @return bool_t true is vaild false is invaild.
 */
bool_t iot_boot_map_is_valid(void);

/**
 * @brief This function is to get boot map's image.
 *
 * @param image is bootmap's image
 * @param id is image id
 * @return uint8_t RET_OK for success else for error.
 */
uint8_t iot_boot_map_get_image(iot_boot_map_image_t *image, uint8_t id);

/**
 * @brief This function is to set boot map's image.
 *
 * @param image is image info
 * @return uint8_t RET_OK for success else for error.
 */
uint8_t iot_boot_map_set_image(iot_boot_map_image_t *image);

/**
 * @brief This function is to get boot map's type.
 *
 * @return uint8_t RET_OK for success else for error.
 */
uint8_t iot_boot_map_get_boot_type(void);

/**
 * @brief This function is to set boot map's boot type.
 *
 * @param boot_type is boot map's bppt type
 * @return uint8_t RET_OK for success else for error.
 */
uint8_t iot_boot_map_set_boot_type(uint8_t boot_type);

/**
 * @brief This function is to update boot map.
 *
 */
void iot_boot_map_update(void);

/**
 * @brief  This function is to init boot map.
 *
 * @param force is force to erase bootmap
 * @return uint8_t RET_OK for success else error.
 */
uint8_t iot_boot_map_init(bool_t force);

/**
 * @brief  This function is to clear valid boot map in image.
 *
 * @param id is image id
 * @return uint8_t RET_OK or RET_NOT_EXIST,RET_OK for success else error.
 */
uint8_t iot_boot_map_clear_image(uint8_t id);

#ifdef __cplusplus
}
#endif
/**
* @}
* @}
*/
#endif /* DRIVER_NON_OS_BOOT_MAP_H */
