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
#include "crc.h"

#include "os_mem.h"
#include "os_utils.h"

#include "generic_list.h"
#include "lib_dbglog.h"
#include "boot_reason.h"

#include "iot_memory_config.h"
#include "iot_flash.h"
#include "iot_boot_map.h"
#include "oem.h"
#include "ota.h"

#define OTA_LIMIT_SIZE     (FLASH_FOTA_LENGTH)
#define OTA_OFFSET         FLASH_FOTA_OFFSET
#define OTA_WRITE_OFFSET   (FLASH_FOTA_OFFSET - FLASH_START)
#define SPI_FLASH_SEC_SIZE 4096
#define OTA_ANC_BIN_TYPE   (0x00000080UL)

typedef struct ota_operation {
    struct list_head node;
    uint32_t handle;
    uint32_t erased_size;
    uint32_t wrote_size;
} ota_operation_t;

static struct list_head ota_opt_list;
static uint32_t s_ota_ops_last_handle = 0;
static image_desc image_desc_data[16];
static ota_anc_data_process_cb anc_process_cb = NULL;
static int partition_erase_range(uint32_t start, uint32_t length)
{
    int ret = RET_OK;
    assert(length <= OTA_LIMIT_SIZE);
    uint16_t sec_num = (uint16_t)((length +  SPI_FLASH_SEC_SIZE -1) / SPI_FLASH_SEC_SIZE);
    uint16_t start_sec = (uint16_t)(start / SPI_FLASH_SEC_SIZE);
    for (uint16_t num = 0; num < sec_num; num++) {
        ret |= iot_flash_erase_sector(start_sec + num);
    }
    return ret;
}

static int partition_write(uint32_t offset, const uint8_t *buf, size_t length)
{
    assert((offset+length) < OTA_LIMIT_SIZE);
    return iot_flash_write_without_erase(offset + OTA_WRITE_OFFSET, buf, length);
}

static int ota_image_verify(void)
{
    pack_desc_t package_header;
    memcpy((uint8_t *)&package_header, (uint8_t *)OTA_OFFSET, sizeof(pack_desc_t));
    if (package_header.magic_word != IMAGE_DESC_MAGIC_WORD) {
        DBGLOG_LIB_OTA_ERROR("OTA image has invalid magic byte saw 0x%08x\n", package_header.magic_word);
        return RET_CRC_FAIL;
    }

    // get package header mete data
    uint32_t p_type = package_header.image_pack_types;

    uint32_t num = 0;
    for (; p_type != 0; p_type &= (p_type - 1)) {
        num++;
    }
    assert(num > 0);

    memcpy(image_desc_data, (uint8_t *)(OTA_OFFSET + sizeof(pack_desc_t)),
           num * sizeof(image_desc));

    // check header crc
    uint32_t cal_crc = getcrc32((const uint8_t *)&image_desc_data[0], num * sizeof(image_desc));
    uint32_t saved_crc = 0;
    memcpy(&saved_crc, (uint8_t *)(OTA_OFFSET + sizeof(pack_desc_t) + num * sizeof(image_desc)),
           sizeof(uint32_t));

    if (cal_crc != saved_crc) {
        DBGLOG_LIB_OTA_ERROR("OTA image header crc check failed 0x%08x != 0x%08x\n", cal_crc, saved_crc);
        return RET_CRC_FAIL;
    }

    // compress data len = sum of compress len
    uint32_t compress_data_offset =
        sizeof(pack_desc_t) + num * sizeof(image_desc) + sizeof(uint32_t);
    uint32_t compress_total_len = 0;
    for (uint32_t i = 0; i < num; i++) {
        compress_total_len += image_desc_data[i].image_compress_size;
    }
    assert(compress_total_len < OTA_LIMIT_SIZE);

    // check crc of compressed data
    cal_crc = getcrc32((const uint8_t *)(OTA_OFFSET + compress_data_offset), compress_total_len);
    memcpy(&saved_crc, (uint8_t *)(OTA_OFFSET + compress_data_offset + compress_total_len),
           sizeof(uint32_t));

    if (cal_crc != saved_crc) {
        DBGLOG_LIB_OTA_ERROR("OTA compressed data check failed 0x%08x != 0x%08x\n", cal_crc, saved_crc);
        return RET_CRC_FAIL;
    }
    return RET_OK;
}

RET_TYPE ota_begin(size_t image_size, ota_handle_t *out_handle)
{
    ota_operation_t *it;

    if ((image_size > OTA_LIMIT_SIZE && image_size != 0xFFFFFFFF) || (out_handle == NULL)) {
        return RET_INVAL;
    }

    it = (ota_operation_t *)os_mem_malloc(IOT_OTA_MID, sizeof(ota_operation_t));
    if (it == NULL) {
        return RET_NOMEM;
    }
    // reset boot mode to normal alway when ota begin, save main boot map firstly
    iot_boot_map_update();
    RET_TYPE ret = (RET_TYPE)iot_boot_map_set_boot_type(IOT_BOOT_MODE_NORMAL);
    if (ret != RET_OK) {
        os_mem_free(it);
        return ret;
    }

    // If input image size is 0 or OTA_SIZE_UNKNOWN, erase entire partition
    if ((image_size == 0) || (image_size == OTA_SIZE_UNKNOWN)) {
        ret = (RET_TYPE)partition_erase_range(OTA_WRITE_OFFSET, OTA_LIMIT_SIZE);
        it->erased_size = OTA_LIMIT_SIZE;
    } else {
        ret = (RET_TYPE)partition_erase_range(
            OTA_WRITE_OFFSET, (image_size / SPI_FLASH_SEC_SIZE + 1) * SPI_FLASH_SEC_SIZE);
        it->erased_size = (image_size / SPI_FLASH_SEC_SIZE + 1) * SPI_FLASH_SEC_SIZE;
    }

    if (ret != RET_OK) {
        os_mem_free(it);
        return ret;
    }

    it->wrote_size = 0;
    it->handle = ++s_ota_ops_last_handle;
    list_init(&ota_opt_list);
    list_add_tail(&it->node, &ota_opt_list);

    *out_handle = it->handle;
    return RET_OK;
}

RET_TYPE ota_write(ota_handle_t handle, const void *data, size_t size)
{
    uint8_t *data_bytes = (uint8_t *)data;
    ota_operation_t *it;
    struct list_head *saved;
    RET_TYPE ret;

    if (data == NULL || handle == 0 || size == 0) {
        DBGLOG_LIB_OTA_ERROR("write parameter is invalid\n");
        return RET_INVAL;
    }

    // find ota handle in linked list
    list_for_each_entry_safe (it, &ota_opt_list, node, saved) {
        if (it->handle == handle) {
            // must erase the partition before writing to it
            assert(it->erased_size > 0);
            if (it->wrote_size == 0 && size > 0
                && data_bytes[0] != (IMAGE_DESC_MAGIC_WORD & 0x000000FF)) {
                DBGLOG_LIB_OTA_ERROR("OTA image has invalid magic byte (expected 0x43, saw 0x%02x)\n",
                                 data_bytes[0]);
                return RET_INVAL;
            }
            ret = (RET_TYPE)partition_write(it->wrote_size, data_bytes, size);
            if (ret == RET_OK) {
                it->wrote_size += size;
            }
            return ret;
        }
    }

    //if go to here ,means don't find the handle
    DBGLOG_LIB_OTA_ERROR("not found the handle\n");
    return RET_NOT_EXIST;
}

RET_TYPE ota_write_data(ota_handle_t handle, const void *data, size_t size, uint32_t offset)
{
    uint8_t *data_bytes = (uint8_t *)data;
    ota_operation_t *it;
    struct list_head *saved;
    RET_TYPE ret;

    if (data == NULL || handle == 0 || size == 0) {
        DBGLOG_LIB_OTA_ERROR("write parameter is invalid\n");
        return RET_INVAL;
    }

    // find ota handle in linked list
    list_for_each_entry_safe (it, &ota_opt_list, node, saved) {
        if (it->handle == handle) {
            // must erase the partition before writing to it
            assert(it->erased_size > 0);
            if (offset == 0 && size > 0
                && data_bytes[0] != (IMAGE_DESC_MAGIC_WORD & 0x000000FF)) {
                DBGLOG_LIB_OTA_ERROR("OTA image has invalid magic byte (expected 0x43, saw 0x%02x)\n",
                                 data_bytes[0]);
                return RET_INVAL;
            }
            if(offset != it->wrote_size) {
                DBGLOG_LIB_OTA_INFO("ota packet missing!\n");
            }
            ret = (RET_TYPE)partition_write(offset, data_bytes, size);
            if (ret == RET_OK) {
                it->wrote_size += size;
            }
            return ret;
        }
    }

    //if go to here ,means don't find the handle
    DBGLOG_LIB_OTA_ERROR("not found the handle\n");
    return RET_NOT_EXIST;
}

RET_TYPE ota_read_data(void *data, size_t size, uint32_t offset)
{
    if (data == NULL || size == 0) {
        DBGLOG_LIB_OTA_ERROR("ota_read_data parameter is invalid\n");
        return RET_INVAL;
    }
    assert((offset + size) < OTA_LIMIT_SIZE);

    memcpy(data, (uint32_t *)(OTA_OFFSET + offset), size);
    return RET_OK;
}

RET_TYPE ota_end(ota_handle_t handle)
{
    ota_operation_t *it;
    struct list_head *saved;
    bool_t found = false;
    if (handle == 0) {
        return RET_FAIL;
    }
    list_for_each_entry_safe (it, &ota_opt_list, node, saved) {
        if (it->handle == handle) {
            found = true;
            break;
        }
    }

    if (!found) {
        return RET_NOT_EXIST;
    }

    // ota_end() is only valid if some data was written to this handle
    if ((it->erased_size == 0) || (it->wrote_size == 0)) {
        list_del(&it->node);
        os_mem_free(it);
        return RET_INVAL;
    }

    if (ota_image_verify() != RET_OK) {
        list_del(&it->node);
        os_mem_free(it);
        return RET_CRC_FAIL;
    }

    list_del(&it->node);
    os_mem_free(it);
    return RET_OK;
}

RET_TYPE ota_commit(bool_t reboot)
{
    // save main boot map firstly
    iot_boot_map_update();
    RET_TYPE ret = (RET_TYPE)iot_boot_map_set_boot_type(IOT_BOOT_MODE_OTA);
    if (ret != RET_OK) {
        return ret;
    }

    if (reboot) {
        DBGLOG_LIB_OTA_INFO("OTA finished, will reset system\n");
        boot_reason_do_soft_reset(BOOT_REASON_SOFT_REASON_APP, 0);
        while (1) {     //lint !e716 reboot path, no need exit
            os_delay(100);
        }
    }
    return ret;
}

RET_TYPE ota_register_anc_data_process_cb(ota_anc_data_process_cb cb)
{
    anc_process_cb = cb;
    return RET_OK;
}

RET_TYPE ota_anc_data_process(void)
{
    RET_TYPE ret = RET_OK;
    pack_desc_t package_header;

    memcpy((uint8_t *)&package_header, (uint8_t *)OTA_OFFSET, sizeof(pack_desc_t));
    if (package_header.magic_word != IMAGE_DESC_MAGIC_WORD) {
        DBGLOG_LIB_OTA_ERROR("OTA image has invalid magic byte saw 0x%08x\n", package_header.magic_word);
        return RET_CRC_FAIL;
    }

    uint32_t p_type = package_header.image_pack_types;
    // there is anc data need ota?
    if (!(p_type & OTA_ANC_BIN_TYPE)) {
        DBGLOG_LIB_OTA_INFO("no anc.bin need ota,p_type 0x%08x\n", p_type);
        return RET_OK;
    }

    // get package header mete data
    uint32_t num = 0;
    for (; p_type != 0; p_type &= (p_type - 1)) {
        num++;
    }
    assert(num > 0);

    memcpy(image_desc_data, (uint8_t *)(OTA_OFFSET + sizeof(pack_desc_t)),
           num * sizeof(image_desc));

    // check header crc
    uint32_t cal_crc = getcrc32((const uint8_t *)&image_desc_data[0], num * sizeof(image_desc));
    uint32_t saved_crc = 0;
    memcpy(&saved_crc, (uint8_t *)(OTA_OFFSET + sizeof(pack_desc_t) + num * sizeof(image_desc)),
           sizeof(uint32_t));

    if (cal_crc != saved_crc) {
        DBGLOG_LIB_OTA_ERROR("OTA image header crc check failed 0x%08x != 0x%08x\n", cal_crc, saved_crc);
        return RET_CRC_FAIL;
    }

    uint32_t anc_data_offset =
        sizeof(pack_desc_t) + num * sizeof(image_desc) + sizeof(uint32_t);
    for (uint32_t i = 0; i < num - 1; i++) {
        anc_data_offset += image_desc_data[i].image_compress_size;
    }
    uint8_t* p_anc_data = (uint8_t*)(anc_data_offset + FLASH_FOTA_OFFSET);

    if (anc_process_cb) {
        ret = anc_process_cb(p_anc_data);
    }

    return ret;
}

static int32_t ota_anc_data_check(const oem_data_anc_t* p_anc_data)
{
    if (p_anc_data->header.magic == OEM_DATA_MAGIC) {
        uint32_t crc = getcrc32(p_anc_data->data, p_anc_data->header.length);

        if (crc == p_anc_data->header.crc) {
            return RET_OK;
        }
    }
    return RET_FAIL;
}

RET_TYPE ota_anc_data_recover(void)
{
    if (RET_OK == ota_anc_data_check((oem_data_anc_t *)(FLASH_START + FLASH_ANC_DATA_OFFSET))) {
        /* if origin anc oem data is valid, don't need recover*/
        return RET_OK;
    } else if (RET_OK == ota_anc_data_check((oem_data_anc_t *)(FLASH_FOTA_OFFSET))){
        /* if origin anc oem data is invalid, ota zone anc data is valid, then recover*/
        if (RET_OK != iot_flash_write(FLASH_ANC_DATA_OFFSET, (void *)FLASH_FOTA_OFFSET, FLASH_ANC_DATA_LENGTH)) {
            return RET_FAIL;
        }
    } else {
        DBGLOG_LIB_OTA_INFO("no valid anc data\n");
        return RET_FAIL;
    }
    return RET_OK;
}
