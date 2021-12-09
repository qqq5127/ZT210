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
#include "iot_flash.h"
#include "iot_otp.h"

#define IOT_OTP_SOC_ID_OFFSET    0x8
#define IOT_OTP_DEVICE_ID_OFFSET 0x10

#define FLASH_VENDOR_PUYA             0x85
#define FLASH_VENDOR_GD               0xc8
#define CHIP_PUYA_DEFAULT_FLASH_IOMAP 0x14577320   //FLASH_PUYA_DEFAULT_IO_MAP
#define CHIP_GD_DEFAULT_FLASH_IOMAP   0x14527730   //FLASH_GD_DEFAULT_IO_MAP
#define CHIP_REV_4_FLASH_IOMAP        0x17537420
#define CHIP_REV_2_FLASH_IOMAP        0x14577320
#define CHIP_REV_3_FLASH_IOMAP        0x74527130
#define CHIP_REV_5_FLASH_IOMAP        0x41527370
#define CHIP_REV_6_FLASH_IOMAP        0x74527130

#define CHIP_ID_MASK                0xF
#define CHIP_ID_VALID_OFFSET        8
#define CHIP_DEVICE_ID_VALID_FLAG   0
#define CHIP_DEVICE_ID_VALUE_MASK   0x3FFFFFFF
#define CHIP_DEVICE_ID_VALID_OFFSET 30
#define CHIP_DEVICE_ID_VALUE_OFFSET 0

static const uint32_t iomap_table[] = {
    CHIP_PUYA_DEFAULT_FLASH_IOMAP,
    CHIP_PUYA_DEFAULT_FLASH_IOMAP,
    CHIP_REV_2_FLASH_IOMAP,
    CHIP_REV_3_FLASH_IOMAP,
    CHIP_REV_4_FLASH_IOMAP,
    CHIP_REV_5_FLASH_IOMAP,
    CHIP_REV_6_FLASH_IOMAP,
};

uint8_t iot_otp_set_device_id(const uint32_t *device_id)
{
    iot_flash_otp_write(IOT_FLASH_OTP_REGION0, IOT_OTP_DEVICE_ID_OFFSET, (uint8_t *)device_id, 4);

    return RET_OK;
}

uint8_t iot_otp_get_device_id(uint32_t *device_id)
{
    uint32_t id = 0;
    iot_flash_otp_read(IOT_FLASH_OTP_REGION0, IOT_OTP_DEVICE_ID_OFFSET, (uint8_t *)&id, 4);

    if ((id >> CHIP_DEVICE_ID_VALID_OFFSET) == CHIP_DEVICE_ID_VALID_FLAG) {
        //chip device id is valid
        *device_id = (id >> CHIP_DEVICE_ID_VALUE_OFFSET)   //lint !e835 :code style
            & CHIP_DEVICE_ID_VALUE_MASK;

        return RET_OK;
    } else {
        return RET_NOT_EXIST;
    }
}

uint8_t iot_otp_set_soc_id(const uint32_t *soc_id)
{
    iot_flash_otp_write(IOT_FLASH_OTP_REGION0, IOT_OTP_SOC_ID_OFFSET, (uint8_t *)soc_id, 8);

    return RET_OK;
}

uint8_t iot_otp_get_soc_id(uint32_t *soc_id)
{
    iot_flash_otp_read(IOT_FLASH_OTP_REGION0, IOT_OTP_SOC_ID_OFFSET, (uint8_t *)soc_id, 8);

    return RET_OK;
}

uint8_t iot_otp_get_flash_iomap(uint32_t *iomap)
{
    uint32_t device_id = 0;

    if (iot_otp_get_device_id(&device_id) == RET_OK) {
        //device id is valid
        uint8_t flash_id = (device_id >> CHIP_ID_VALID_OFFSET) & CHIP_ID_MASK;
        assert(flash_id < ARRAY_SIZE(iomap_table));
        *iomap = iomap_table[flash_id];
    } else {
        //device id is invalid
        uint32_t vendor = iot_flash_get_vendor();
        if (vendor == FLASH_VENDOR_PUYA) {
            //default puya but not ATE
            *iomap = CHIP_PUYA_DEFAULT_FLASH_IOMAP;
        } else if (vendor == FLASH_VENDOR_GD) {
            //default gd
            *iomap = CHIP_GD_DEFAULT_FLASH_IOMAP;
        }
    }
    return RET_OK;
}
