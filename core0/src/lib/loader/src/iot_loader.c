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
#include "string.h"

#include "iot_boot_map.h"
#include "iot_memory_config.h"
#include "iot_soc.h"
#include "iot_dsp.h"
#include "lib_dbglog.h"

#include "iot_loader.h"

#define BT_START_PC 0x10000000

uint32_t iot_loader_load_bt(void)
{
    iot_boot_map_t map;
    iot_boot_map_image_t image;
    uint32_t start_pc = BT_START_PC;

    if (iot_boot_map_get_valid_map(&map) == RET_OK) {
        iot_soc_bt_power_up();
        if (map.boot_type == IOT_BOOT_MODE_NORMAL) {
            if (iot_boot_map_get_image(&image, IMAGE_TYPE_CORE1_TBL0)
                == RET_OK) {
                memcpy((uint8_t *)image.code_vma,
                       (uint8_t *)(image.code_lma + FLASH_START
                                   + IMAGE_HEADER_LEN),
                       image.code_length);
            }
            if (iot_boot_map_get_image(&image, IMAGE_TYPE_CORE1_FW0)
                == RET_OK) {
                iot_flash_image_header_t *img_hdr =
                    (iot_flash_image_header_t *)(image.code_lma
                                                 + FLASH_START);
                if (img_hdr->guard == FLASH_IMAGE_MAGIC) {
                    if (image.code_vma != 0) {
                        memcpy((uint8_t *)image.code_vma,
                               (uint8_t *)(image.code_lma + FLASH_START
                                           + IMAGE_HEADER_LEN),
                               image.code_length);
                        start_pc = image.code_vma;
                    } else {
                        start_pc =
                            image.code_lma + FLASH_START + IMAGE_HEADER_LEN;
                    }
                } else {
                    DBGLOG_LIB_LOADER_INFO("BT image is bad\r\n");
                    return start_pc;
                }
                DBGLOG_LIB_LOADER_INFO("BT start PC:0x%08x,type:%d,img(load:0x%x/len:0x%x/st:0x%x)\r\n",
                                start_pc, map.boot_type, image.code_vma, image.code_length,
                                image.code_lma);
            }
        } else if (map.boot_type == IOT_BOOT_MODE_OTA) {
            if (iot_boot_map_get_image(&image, IMAGE_TYPE_CORE1_FW1)
                == RET_OK) {
                iot_flash_image_header_t *img_hdr =
                    (iot_flash_image_header_t *)(image.code_lma
                                                 + FLASH_START);
                if (img_hdr->guard == FLASH_IMAGE_MAGIC) {
                    if (image.code_vma != 0) {
                        memcpy((uint8_t *)image.code_vma,
                               (uint8_t *)(image.code_lma + FLASH_START
                                           + IMAGE_HEADER_LEN),
                               image.code_length);
                        start_pc = image.code_vma;
                    } else {
                        start_pc =
                            image.code_lma + FLASH_START + IMAGE_HEADER_LEN;
                    }
                } else {
                    DBGLOG_LIB_LOADER_INFO("BT image is bad\r\n");
                    return start_pc;
                }
                DBGLOG_LIB_LOADER_INFO("BT start PC:0x%08x,type:%d,img(load:0x%x/len:0x%x/st:0x%x)\r\n",
                                start_pc, map.boot_type, image.code_vma, image.code_length,
                                image.code_lma);
            }
        }
    }

    return start_pc;
}

uint32_t iot_loader_load_dsp(void)
{
    iot_boot_map_t map;
    iot_boot_map_image_t image;
    uint32_t start_pc = 0;

    if (iot_boot_map_get_valid_map(&map) == RET_OK) {
        if (map.boot_type == IOT_BOOT_MODE_NORMAL) {
            if (iot_boot_map_get_image(&image, IMAGE_TYPE_DSP_FW0) == RET_OK) {
                DBGLOG_LIB_LOADER_INFO(
                    "DSP start PC:0x%08x\n",
                    (image.code_lma + FLASH_START + IMAGE_HEADER_LEN));

                iot_dsp_stall_and_enable();
                start_pc = image.code_lma + FLASH_START + IMAGE_HEADER_LEN;

                iot_dsp_load_image((uint8_t *)start_pc);
            }
        }
    }

    return start_pc;
}

uint32_t iot_loader_load_tone(void)
{
    iot_boot_map_t map;
    iot_boot_map_image_t image;
    uint32_t offset = 0;

    if (iot_boot_map_get_valid_map(&map) == RET_OK) {
        if (iot_boot_map_get_image(&image, IMAGE_TYPE_TONE) == RET_OK) {

            DBGLOG_LIB_LOADER_INFO("Tone offset:0x%08x\n", (image.code_lma + FLASH_START));
            offset = image.code_lma + FLASH_START + IMAGE_HEADER_LEN;
        }
    }

    return offset;
}
