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
#include "types.h"
#include "string.h"

#include "iot_memory_origin.h"
#include "iot_flash.h"

#include "iot_boot_map.h"

#define IOT_BOOT_MAP_SIZE        FLASH_SECTOR_SIZE
#define IOT_BOOT_MAP_MAIN_ADDR   FLASH_BOOT_MAP_OFFSET
#define IOT_BOOT_MAP_BACKUP_ADDR (IOT_BOOT_MAP_MAIN_ADDR + IOT_BOOT_MAP_SIZE)

static uint8_t iot_boot_map_read(uint32_t addr, void *data, uint32_t length)
{
    return iot_flash_read(addr, data, length);
}

static uint8_t iot_boot_map_write(uint32_t addr, const void *data, uint32_t length)
{
    return iot_flash_write_without_erase(addr, data, length);
}

static uint8_t iot_boot_map_erase(uint32_t addr)
{
    return iot_flash_erase(addr);
}

/**
 * @brief Read backup boot map and write to main boot map
 */
static void iot_boot_map_sync(void)
{
    iot_boot_map_t map;
    iot_boot_map_image_t image;

    // Sync header first
    iot_boot_map_read(IOT_BOOT_MAP_BACKUP_ADDR, &map, sizeof(iot_boot_map_t));
    iot_boot_map_erase(IOT_BOOT_MAP_MAIN_ADDR);
    iot_boot_map_write(IOT_BOOT_MAP_MAIN_ADDR, &map, sizeof(iot_boot_map_t));

    // Sync all images
    for (uint32_t i = 0; i < map.image_num; i++) {
        uint32_t offset =
            sizeof(iot_boot_map_t) + i * sizeof(iot_boot_map_image_t);
        iot_boot_map_read(IOT_BOOT_MAP_BACKUP_ADDR + offset, &image,
                          sizeof(iot_boot_map_image_t));
        iot_boot_map_write(IOT_BOOT_MAP_MAIN_ADDR + offset, &image,
                           sizeof(iot_boot_map_image_t));
    }
}

/**
 * @brief Write main boot map to backup
 */
void iot_boot_map_update(void)
{
    iot_boot_map_t map;
    iot_boot_map_image_t image;

    // Sync header first
    iot_boot_map_read(IOT_BOOT_MAP_MAIN_ADDR, &map, sizeof(iot_boot_map_t));
    iot_boot_map_erase(IOT_BOOT_MAP_BACKUP_ADDR);
    iot_boot_map_write(IOT_BOOT_MAP_BACKUP_ADDR, &map, sizeof(iot_boot_map_t));

    // Sync all images
    for (uint32_t i = 0; i < map.image_num; i++) {
        uint32_t offset =
            sizeof(iot_boot_map_t) + i * sizeof(iot_boot_map_image_t);
        iot_boot_map_read(IOT_BOOT_MAP_MAIN_ADDR + offset, &image,
                          sizeof(iot_boot_map_image_t));
        iot_boot_map_write(IOT_BOOT_MAP_BACKUP_ADDR + offset, &image,
                           sizeof(iot_boot_map_image_t));
    }
}

uint8_t iot_boot_map_get_valid_map(iot_boot_map_t *map)
{
    // First read main boot map
    iot_boot_map_read(IOT_BOOT_MAP_MAIN_ADDR, map, sizeof(iot_boot_map_t));
    if (map->magic == BOOT_MAP_MAGIC) {
        return RET_OK;
    }

    // If main boot map is bad, read backup map
    iot_boot_map_read(IOT_BOOT_MAP_BACKUP_ADDR, map, sizeof(iot_boot_map_t));
    if (map->magic == BOOT_MAP_MAGIC) {
        // Main map is invalid, so copy backup map as main map
        iot_boot_map_sync();
        return RET_OK;
    }

    // All boot map is bad
    return RET_NOT_EXIST;
}

bool_t iot_boot_map_is_valid(void)
{
    iot_boot_map_t map;

    if (iot_boot_map_get_valid_map(&map) == RET_OK) {
        return true;
    } else {
        return false;
    }
}

uint8_t iot_boot_map_get_image(iot_boot_map_image_t *image, uint8_t id)
{
    iot_boot_map_t map;
    iot_boot_map_image_t cimage;
    uint8_t ret;

    ret = iot_boot_map_get_valid_map(&map);
    if (ret != RET_OK) {
        return ret;
    }

    for (uint32_t i = 0; i < map.image_num; i++) {
        uint32_t offset =
            sizeof(iot_boot_map_t) + i * sizeof(iot_boot_map_image_t);
        iot_boot_map_read(IOT_BOOT_MAP_MAIN_ADDR + offset, &cimage,
                          sizeof(iot_boot_map_image_t));
        if (cimage.image_type == id) {
            memcpy(image, &cimage, sizeof(iot_boot_map_image_t));
            return RET_OK;
        }
    }

    return RET_NOT_EXIST;
}

uint8_t iot_boot_map_clear_image(uint8_t id)
{
    iot_boot_map_t map;
    iot_boot_map_image_t zimage = {0};
    uint8_t ret;

    ret = iot_boot_map_get_valid_map(&map);
    if (ret != RET_OK) {
        return ret;
    }
    iot_boot_map_image_t cimage[MAX_IMAGE_NUM] = {0};
    iot_boot_map_read(IOT_BOOT_MAP_MAIN_ADDR + sizeof(iot_boot_map_t) , cimage,
                          sizeof(iot_boot_map_image_t) * map.image_num);
    for (uint32_t i = 0; i < map.image_num; i++) {
        if (cimage[i].image_type == id) {
            memcpy(&cimage[i], &zimage, sizeof(iot_boot_map_image_t));
            iot_boot_map_erase(IOT_BOOT_MAP_MAIN_ADDR);
            iot_boot_map_write(IOT_BOOT_MAP_MAIN_ADDR, &map,
                                sizeof(iot_boot_map_t));
            iot_boot_map_write(IOT_BOOT_MAP_MAIN_ADDR + sizeof(iot_boot_map_t), cimage,
                                   sizeof(iot_boot_map_image_t) * MAX_IMAGE_NUM);
            return RET_OK;
        }
    }

    return RET_NOT_EXIST;
}

uint8_t iot_boot_map_set_image(iot_boot_map_image_t *image)
{
    iot_boot_map_t map;
    uint8_t ret;

    ret = iot_boot_map_get_valid_map(&map);
    if (ret != RET_OK) {
        return ret;
    }

    // Check image type is already exist
    iot_boot_map_image_t cimage[MAX_IMAGE_NUM] = {0};
    iot_boot_map_read(IOT_BOOT_MAP_MAIN_ADDR + sizeof(iot_boot_map_t) , cimage,
                          sizeof(iot_boot_map_image_t) * map.image_num);
    for (uint8_t i = 0; i < map.image_num; i++) {
        if (cimage[i].image_type == image->image_type) {
            if (cimage[i].code_lma != image->code_lma
                || cimage[i].code_vma != image->code_vma
                || cimage[i].code_length != image->code_length
                || cimage[i].code_area_size != image->code_area_size) {
                memcpy(&cimage[i], image, sizeof(iot_boot_map_image_t));
                iot_boot_map_erase(IOT_BOOT_MAP_MAIN_ADDR);
                iot_boot_map_write(IOT_BOOT_MAP_MAIN_ADDR, &map,
                                   sizeof(iot_boot_map_t));
                iot_boot_map_write(IOT_BOOT_MAP_MAIN_ADDR + sizeof(iot_boot_map_t), cimage,
                                   sizeof(iot_boot_map_image_t) * MAX_IMAGE_NUM);
                return RET_OK;
            }
        }
    }
    memcpy(&cimage[map.image_num], (const void *)image, sizeof(iot_boot_map_image_t));
    // Image type not exist, add as new one
    map.image_num++;
    iot_boot_map_erase(IOT_BOOT_MAP_MAIN_ADDR);
    iot_boot_map_write(IOT_BOOT_MAP_MAIN_ADDR, &map, sizeof(iot_boot_map_t));
    iot_boot_map_write(IOT_BOOT_MAP_MAIN_ADDR + sizeof(iot_boot_map_t), cimage,
                                   sizeof(iot_boot_map_image_t) * MAX_IMAGE_NUM);

    return RET_OK;
}//lint !e818 image don't need be const

uint8_t iot_boot_map_get_boot_type(void)
{
    iot_boot_map_t map;

    if (iot_boot_map_get_valid_map(&map) != RET_OK) {
        return IOT_BOOT_MODE_UNKONWN;
    }

    return map.boot_type;
}

uint8_t iot_boot_map_set_boot_type(uint8_t boot_type)
{
    iot_boot_map_t map;
    uint8_t ret;

    ret = iot_boot_map_get_valid_map(&map);
    if (ret != RET_OK) {
        return ret;
    }

    map.boot_type = boot_type;
    iot_boot_map_image_t cimage[MAX_IMAGE_NUM] = {0};
    iot_boot_map_read(IOT_BOOT_MAP_MAIN_ADDR + sizeof(iot_boot_map_t) , cimage,
                          sizeof(iot_boot_map_image_t) * MAX_IMAGE_NUM);
    iot_boot_map_erase(IOT_BOOT_MAP_MAIN_ADDR);
    iot_boot_map_write(IOT_BOOT_MAP_MAIN_ADDR, &map, sizeof(iot_boot_map_t));
    iot_boot_map_write(IOT_BOOT_MAP_MAIN_ADDR + sizeof(iot_boot_map_t), cimage, sizeof(iot_boot_map_image_t) * MAX_IMAGE_NUM);
    return RET_OK;
}

uint8_t iot_boot_map_init(bool_t force)
{
    iot_boot_map_t map;

    if (force) {
        iot_flash_erase(IOT_BOOT_MAP_MAIN_ADDR);
        memset(&map, 0, sizeof(iot_boot_map_t));
        map.magic = BOOT_MAP_MAGIC;
        map.image_num = 0;
        map.boot_type = IOT_BOOT_MODE_NORMAL;
        return iot_boot_map_write(IOT_BOOT_MAP_MAIN_ADDR, &map,
                                  sizeof(iot_boot_map_t));
    }

    if (iot_boot_map_get_valid_map(&map) != RET_OK) {
        iot_flash_erase(IOT_BOOT_MAP_MAIN_ADDR);
        memset(&map, 0, sizeof(iot_boot_map_t));
        map.magic = BOOT_MAP_MAGIC;
        map.image_num = 0;
        map.boot_type = IOT_BOOT_MODE_NORMAL;
        return iot_boot_map_write(IOT_BOOT_MAP_MAIN_ADDR, &map,
                                  sizeof(iot_boot_map_t));
    }

    // Not force init, and there is a valid boot map
    return RET_OK;
}
