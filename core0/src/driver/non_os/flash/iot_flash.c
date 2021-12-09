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
#include "iot_memory_origin.h"

#if !defined(BUILD_OS_NON_OS)
#include "os_lock.h"
#include "os_mem.h"
#endif

#include "sfc.h"
#include "apb.h"

#include "flash.h"
#include "iot_flash.h"
#include "iot_cache.h"

#define IOT_FLASH_OTP_SOC_ID_OFFSET 0x8

#if !defined(BUILD_OS_NON_OS)
#else
static uint8_t iot_flash_tmp_buf[FLASH_SECTOR_SIZE];
#endif
static bool_t iot_flash_inited = false;
#if !defined(BUILD_OS_NON_OS)
static os_mutex_h flash_op_mutex = NULL;
#endif

static iot_flash_pe_cb flash_pe_cb = NULL;

static uint32_t iot_flash_get_sector_id(uint32_t addr)
{
    return addr / FLASH_SECTOR_SIZE;
}

static uint8_t iot_flash_sector_write(uint32_t addr, const void *buf, uint32_t length)
{
    // Only write the data in same sector
    if (iot_flash_get_sector_id(addr)
        != iot_flash_get_sector_id(addr + length - 1)) {
        return RET_INVAL;
    }

    // Write a whole sector, just write it
    if (length == FLASH_SECTOR_SIZE) {
        flash_special_erase(addr);
        return flash_special_write(addr, buf, length);
    }

    uint32_t sector_start = addr & ~(FLASH_SECTOR_SIZE - 1);
    uint32_t sector_offset = addr % FLASH_SECTOR_SIZE;

    // read this sector's data */
#if !defined(BUILD_OS_NON_OS)
    uint8_t* iot_flash_tmp_buf = os_mem_malloc(IOT_DRIVER_MID,FLASH_SECTOR_SIZE);
    assert(iot_flash_tmp_buf);
#endif
    flash_special_read(sector_start, iot_flash_tmp_buf, FLASH_SECTOR_SIZE);

    memcpy(iot_flash_tmp_buf + sector_offset, buf, length);

    flash_special_erase(sector_start);

    uint8_t ret = flash_special_write(sector_start, iot_flash_tmp_buf,
                               FLASH_SECTOR_SIZE);
#if !defined(BUILD_OS_NON_OS)
    os_mem_free(iot_flash_tmp_buf);
#endif
    return ret;
}

uint8_t iot_flash_erase_sector(uint16_t sector_id)
{
    uint8_t ret;

    ret = flash_special_erase_sector(sector_id);

    if(flash_pe_cb){
        flash_pe_cb();
    }
    iot_cache_invalidate(IOT_CACHE_SFC_ID,
                         ((sector_id * FLASH_SECTOR_SIZE + FLASH_START) & ~(FLASH_SECTOR_SIZE - 1)),
                         FLASH_SECTOR_SIZE);
    return ret;
}

uint8_t iot_flash_erase(uint32_t addr)
{
    uint8_t ret;

    ret = flash_special_erase(addr);

    if(flash_pe_cb){
        flash_pe_cb();
    }
    iot_cache_invalidate(IOT_CACHE_SFC_ID, ((addr + FLASH_START) & ~(FLASH_SECTOR_SIZE - 1)), FLASH_SECTOR_SIZE);
    return ret;
}

uint8_t iot_flash_chip_erase(void)
{
    uint8_t ret;

    ret = flash_special_chip_erase();

    if(flash_pe_cb){
        flash_pe_cb();
    }

    return ret;
}

uint8_t iot_flash_read(uint32_t addr, void *buf, size_t count)
{
#if !defined(BUILD_OS_NON_OS)
    os_acquire_mutex(flash_op_mutex);
#endif

    uint8_t ret = flash_special_read(addr, buf, count);

#if !defined(BUILD_OS_NON_OS)
    os_release_mutex(flash_op_mutex);
#endif

    return ret;
}

uint8_t iot_flash_write_without_erase(uint32_t addr, const void *buf, size_t count)
{
    uint8_t ret;

#if !defined(BUILD_OS_NON_OS)
    os_acquire_mutex(flash_op_mutex);
#endif

    ret = flash_special_write(addr, buf, count);

    if(flash_pe_cb){
        flash_pe_cb();
    }
    iot_cache_invalidate(IOT_CACHE_SFC_ID, addr + FLASH_START, count);
#if !defined(BUILD_OS_NON_OS)
    os_release_mutex(flash_op_mutex);
#endif

    return ret;
}

uint8_t iot_flash_write(uint32_t addr, void *buf, size_t count)
{
    uint32_t left = count;
    uint32_t waddr = addr;
    uint8_t *p = buf;
    uint8_t ret = RET_OK;

#if !defined(BUILD_OS_NON_OS)
    os_acquire_mutex(flash_op_mutex);
#endif

    while (left) {
        uint32_t sector_left =
            ((waddr + FLASH_SECTOR_SIZE) & ~(FLASH_SECTOR_SIZE - 1)) - waddr;
        uint32_t wlen = MIN(sector_left, left);
        ret = iot_flash_sector_write(waddr, p, wlen);

        left -= wlen;
        waddr += wlen;
        p += wlen;
    }

    if(flash_pe_cb){
        flash_pe_cb();
    }
    iot_cache_invalidate(IOT_CACHE_SFC_ID, addr + FLASH_START, count);
#if !defined(BUILD_OS_NON_OS)
    os_release_mutex(flash_op_mutex);
#endif

    return ret;
}

uint16_t iot_flash_get_id(void)
{
    return flash_special_get_id();
}

void iot_flash_init(void)
{
#if !defined(BUILD_OS_NON_OS)
    flash_op_mutex = os_create_mutex(IOT_DRIVER_MID);

    assert(flash_op_mutex);
#endif
    if (iot_flash_inited) {
        return;
    }

    flash_init();

    iot_flash_inited = true;

    flash_special_enable_quad_mode();
}

void iot_flash_enable_quad_mode(void)
{
    /* enable flash's quad mode */
    flash_special_enable_quad_mode();
}

void iot_flash_set_cache_mode(void)
{
    flash_special_set_cache_mode();
}

void iot_flash_enable_qpp_mode(void)
{
    flash_special_enable_qpp_mode();
}

void iot_flash_set_wip_wait_time(uint32_t p_time, uint32_t e_time)
{
    flash_special_set_wip_wait_time(p_time, e_time);
}

void iot_flash_disable_qpp_mode(void)
{
    flash_special_disable_qpp_mode();
}

bool_t iot_flash_is_init(void)
{
    return iot_flash_inited;
}

uint8_t iot_flash_otp_write(IOT_FLASH_OTP_REGION_ID id, uint32_t addr, const void *buf, size_t count)
{
    assert(id < IOT_FLASH_OTP_REGION_MAX);

    uint32_t r_addr = REGION_ADDR_OF_ID(id);
#if !defined(BUILD_OS_NON_OS)
    uint8_t* iot_flash_tmp_buf = os_mem_malloc(IOT_DRIVER_MID,FLASH_SECTOR_SIZE);
    assert(iot_flash_tmp_buf);
#endif
    flash_special_otp_read(r_addr, iot_flash_tmp_buf, IOT_FLASH_OTP_REGIN_SIZE);
    memcpy(iot_flash_tmp_buf + addr, buf, count);
    flash_special_otp_erase(r_addr);

    uint8_t ret = flash_special_otp_write(r_addr,iot_flash_tmp_buf, IOT_FLASH_OTP_REGIN_SIZE);
#if !defined(BUILD_OS_NON_OS)
    os_mem_free(iot_flash_tmp_buf);
#endif
    return ret;
}

uint8_t iot_flash_otp_read(IOT_FLASH_OTP_REGION_ID id, uint32_t addr, void *buf, size_t count)
{
    assert(id < IOT_FLASH_OTP_REGION_MAX);

    uint32_t r_addr = REGION_ADDR_OF_ID(id) + addr;
    flash_special_otp_read(r_addr, buf, count);

    return RET_OK;
}

void iot_flash_otp_lock(IOT_FLASH_OTP_REGION_ID id)
{
    if (id >= IOT_FLASH_OTP_REGION_MAX) {
        return;
    }

    flash_special_otp_lock(id);
}

void iot_flash_set_io_map(uint32_t map)
{
    sfc_set_io_map(map);
}

void iot_flash_set_download_mode(bool_t enable)
{
    apb_misc_remapsfc_enable(enable);
}

uint32_t iot_flash_get_size(void)
{
    uint32_t id = iot_flash_get_id();
    uint32_t size = (id >>8)&0xff;

    if(size > 0x12){
        return BIT(size - 0x13);
    } else {
        return 0;
    }
}

uint32_t iot_flash_get_vendor(void)
{
    uint32_t id = iot_flash_get_id();

    return id & 0xff;
}

void iot_flash_set_soc_id(const uint32_t *soc_id)
{
    iot_flash_otp_write(IOT_FLASH_OTP_REGION0,IOT_FLASH_OTP_SOC_ID_OFFSET, (uint8_t*)soc_id, 8);
}

void iot_flash_get_soc_id(uint32_t *soc_id)
{
    iot_flash_otp_read(IOT_FLASH_OTP_REGION0,IOT_FLASH_OTP_SOC_ID_OFFSET, (uint8_t*)soc_id, 8);
}

uint32_t iot_flash_get_version(void)
{
    return flash_special_get_sfdp();
}

uint8_t iot_flash_register_pe_callback(iot_flash_pe_cb cb)
{
    flash_pe_cb = cb;

    return RET_OK;
}

uint8_t iot_flash_unregister_pe_callback(void)
{
    flash_pe_cb = NULL;

    return RET_OK;
}

bool_t iot_flash_is_pe_in_progress(void)
{
    return flash_special_is_pe_in_progress();
}

void iot_flash_set_pe_mode(IOT_FLASH_PE_MODE mode)
{
    flash_special_set_pe_mode((FLASH_PE_MODE)mode);
}

IOT_FLASH_PE_MODE iot_flash_get_pe_mode(void)
{
    return (IOT_FLASH_PE_MODE)flash_special_get_pe_mode();
}
