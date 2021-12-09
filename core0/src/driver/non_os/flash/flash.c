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

#include "sfc.h"

#include "flash.h"
#include "iot_flash.h"
#include "iot_timer.h"
#include "iot_cache.h"
#include "riscv_cpu.h"

#if !defined(BUILD_OS_NON_OS)
#include "os_task.h"
#include "iot_ipc.h"
#include "iot_soc.h"
#include "iot_suspend_sched.h"
#include "os_utils.h"
#endif

extern struct flash_ctrl flash_gd_set;
extern struct flash_ctrl flash_puya_set;

typedef struct flash_special_info {
    uint32_t id;
    struct flash_ctrl *func;
} flash_special_info_t;

const flash_special_info_t flash_info[] IRAM_RODATA(flash_info) = {
    {PUYA_MANU_ID, &flash_puya_set},
    {GIGA_MANU_ID, &flash_gd_set},
};

static bool_t flash_special_qpi_mode = false;
static uint32_t flash_special_erase_wip_wait_time = 0;
static uint32_t flash_special_program_wip_wait_time = 0;

const struct flash_ctrl *flash_special_func = {NULL};

FLASH_PE_MODE flash_special_pe_mode = FLASH_PE_HW_MODE;

static void flash_special_read_info(void)
{
    assert(flash_special_func->read_info != NULL);
    flash_special_func->read_info();
}

FLASH_PE_MODE flash_special_get_pe_mode(void) IRAM_TEXT(flash_special_get_pe_mode);
FLASH_PE_MODE flash_special_get_pe_mode(void)
{
    return flash_special_pe_mode;
}

void flash_special_set_pe_mode(FLASH_PE_MODE mode)
{
    uint16_t id = flash_special_get_id();

    if((id & 0xFF) == GIGA_MANU_ID) {
        flash_special_pe_mode = mode;
    }
}

bool_t flash_special_is_quad_mode(void) IRAM_TEXT(flash_special_is_quad_mode);
bool_t flash_special_is_quad_mode(void)
{
    return flash_special_qpi_mode;
}

void flash_special_enable_quad_mode(void)
{
    assert(flash_special_func->set_quad_mode != NULL);

    flash_special_func->set_quad_mode();

    flash_special_qpi_mode = true;
}

void flash_special_set_wip_wait_time(uint32_t p_time, uint32_t e_time)
{
    flash_special_program_wip_wait_time = p_time;
    flash_special_erase_wip_wait_time = e_time;
}

uint32_t flash_special_get_program_wip_wait_time(void)
{
    return flash_special_program_wip_wait_time;
}

uint32_t flash_special_get_erase_wip_wait_time(void)
{
    return flash_special_erase_wip_wait_time;
}

void flash_special_disable_quad_mode(void)
{
    flash_special_qpi_mode = false;
}

void flash_special_set_cache_mode(void)
{
    sfc_set_cache_mode();
}

void flash_special_write_enable(void) IRAM_TEXT(flash_special_write_enable);
void flash_special_write_enable(void)
{
    sfc_cmd_t cmd = FLASH_CMD(WRITE_EN);
    cmd.addr = 0;

    sfc_send_cmd(&cmd);
}

bool_t flash_special_check_wip_status(void) IRAM_TEXT(flash_special_check_wip_status);
bool_t flash_special_check_wip_status(void)
{
    uint8_t sts = flash_special_read_status_reg1();

    return ((sts & STS_WIP_BIT_S0) == STS_WIP_BIT_S0);
}

void flash_special_wait_wip(void) IRAM_TEXT(flash_special_wait_wip);
void flash_special_wait_wip(void)
{
    while (flash_special_read_status_reg1() & STS_WIP_BIT_S0) {
        iot_timer_delay_us(20);
    }
}

static void flash_special_wait_suspend(void) IRAM_TEXT(flash_special_wait_suspend);
static void flash_special_wait_suspend(void)
{
    uint32_t count = 0;
    while (flash_special_read_status_reg1() & STS_WIP_BIT_S0) {
        iot_timer_delay_us(20);

        count++;
        if(count > 100){
            assert(0);
        }
    }
}


void flash_special_write_disable(void)
{
    sfc_cmd_t cmd = FLASH_CMD(WRITE_DIS);
    cmd.addr = 0;

    sfc_send_cmd(&cmd);
}

static uint8_t flash_special_page_read(uint32_t addr, void *buf, size_t count)
{
    sfc_cmd_t pr = FLASH_CMD(READ_DATA);
    sfc_cmd_t qpr = FLASH_CMD(QUAD_READ_DATA);
    sfc_cmd_t *cmd = NULL;

    if (flash_special_is_quad_mode()) {
        cmd = &qpr;
    } else {
        cmd = &pr;
    }

    cmd->addr = addr;
    return sfc_send_read_cmd(cmd, buf, count);
}

uint8_t flash_special_read_status_reg1(void) IRAM_TEXT(flash_special_read_status_reg1);
uint8_t flash_special_read_status_reg1(void)
{
    sfc_cmd_t cmd = FLASH_CMD(READ_STS_REG1);
    uint8_t status = 0;

    cmd.addr = 0;
    sfc_send_read_cmd(&cmd, &status, sizeof(uint8_t));

    return status;
}

void flash_special_write_status_reg1(uint32_t data, uint8_t len)
{
    sfc_cmd_t cmd = FLASH_CMD(WRITE_STS_REG1);

    cmd.addr = 0;
    sfc_send_write_cmd(&cmd, (uint8_t *)&data, len);
}

uint8_t flash_special_read_status_reg2(void) IRAM_TEXT(flash_special_read_status_reg2);
uint8_t flash_special_read_status_reg2(void)
{
    uint8_t status = 0;
    sfc_cmd_t cmd = FLASH_CMD(READ_STS_REG2);

    cmd.addr = 0;
    sfc_send_read_cmd(&cmd, &status, sizeof(uint8_t));

    return status;
}

void flash_special_write_status_reg2(uint32_t data, uint8_t len)
    IRAM_TEXT(flash_special_write_status_reg2);
void flash_special_write_status_reg2(uint32_t data, uint8_t len)
{
    sfc_cmd_t cmd = FLASH_CMD(WRITE_STS_REG2);

    cmd.addr = 0;
    sfc_send_write_cmd(&cmd, (uint8_t *)&data, len);
}


uint8_t flash_special_read(uint32_t addr, void *buf, size_t count)
{
    uint32_t left = count;
    uint32_t r_len = count;
    uint32_t r_addr = addr;
    uint8_t *r_buf = buf;
    uint8_t ret = RET_OK;

    if (r_addr % FLASH_PAGE_SIZE != 0) {
        r_len = MIN(left, FLASH_PAGE_SIZE - (r_addr % FLASH_PAGE_SIZE));
        ret = flash_special_page_read(r_addr, r_buf, r_len);
        if (ret != RET_OK) {
            return ret;
        }
        left -= r_len;
        r_addr += r_len;
        r_buf += r_len;
    }

    while (left > 0) {
        r_len = MIN(left, FLASH_PAGE_SIZE);
        ret = flash_special_page_read(r_addr, r_buf, r_len);
        if (ret != RET_OK) {
            return ret;
        }

        left -= r_len;
        r_addr += r_len;
        r_buf += r_len;
    }

    return ret;
}

uint16_t flash_special_get_id(void) IRAM_TEXT(flash_special_get_id);
uint16_t flash_special_get_id(void)
{
    sfc_cmd_t cmd = FLASH_CMD(MANU_DEV_ID);
    uint8_t id[2];

    cmd.addr = 0;
    sfc_send_read_cmd(&cmd, id, sizeof(id));

    return (id[0] | id[1] << 8);
}

uint8_t flash_special_get_sfdp(void) IRAM_TEXT(flash_special_get_sfdp);
uint8_t flash_special_get_sfdp(void)
{
    uint8_t id;
    sfc_cmd_t cmd = FLASH_CMD(RDSFDP_ID);

    cmd.addr = FLASH_SFDP_INTERNAL_ADDRESS;
    sfc_send_read_cmd(&cmd, &id, sizeof(id));

    return id;
}

uint8_t flash_special_get_uid(void *data) IRAM_TEXT(flash_special_get_uid);
uint8_t flash_special_get_uid(void *data)
{
    sfc_cmd_t cmd = FLASH_CMD(RD_UNIQ_ID);

    cmd.addr = 0;
    sfc_send_read_cmd(&cmd, data, FLASH_UNIQUE_ID_LEN);

    return RET_OK;
}

uint8_t flash_special_otp_erase(uint32_t addr)
{
    assert(flash_special_func->otp_erase != NULL);

    return flash_special_func->otp_erase(addr);
}

static uint8_t flash_special_otp_page_read(uint32_t addr, void *buf, size_t count)
{
    sfc_cmd_t cmd = FLASH_CMD(OTP_READ_DATA);

    cmd.addr = addr;
    return sfc_send_read_cmd(&cmd, buf, count);
}

uint8_t flash_special_otp_write(uint32_t addr, void *buf, size_t count)
{
    uint32_t left = count;
    uint32_t w_len = 0;
    uint32_t w_addr = addr;
    uint8_t *w_buf = buf;
    uint8_t ret = RET_OK;

    assert(flash_special_func->otp_write != NULL);

    while (left > 0) {
        w_len = MIN(left, FLASH_PAGE_SIZE);

        ret = flash_special_func->otp_write(w_addr, w_buf, w_len);

        if (ret != RET_OK) {
            return ret;
        }

        left -= w_len;
        w_addr += w_len;
        w_buf += w_len;
    }

    return ret;
}

uint8_t flash_special_otp_read(uint32_t addr, void *buf, size_t count)
{
    uint32_t left = count;
    uint32_t r_len = count;
    uint32_t r_addr = addr;
    uint8_t *r_buf = buf;
    uint8_t ret = RET_OK;

    while (left > 0) {
        r_len = MIN(left, FLASH_PAGE_SIZE);
        ret = flash_special_otp_page_read(r_addr, r_buf, r_len);
        if (ret != RET_OK) {
            return ret;
        }

        left -= r_len;
        r_addr += r_len;
        r_buf += r_len;
    }

    return ret;
}

void flash_special_enable_qpp_mode(void)
{
    assert(flash_special_func->enable_qpp_mode != NULL);
    flash_special_func->enable_qpp_mode();
}

void flash_special_disable_qpp_mode(void)
{
    assert(flash_special_func->disable_qpp_mode != NULL);
    flash_special_func->disable_qpp_mode();
}

void flash_special_otp_lock(uint8_t id)
{
    assert(flash_special_func->otp_lock != NULL);
    flash_special_func->otp_lock(id);
}

uint8_t flash_special_read_config_reg(void)
{
    sfc_cmd_t cmd = FLASH_CMD(READ_CFG_REG);
    uint8_t cfg = 0;

    cmd.addr = 0;
    sfc_send_read_cmd(&cmd, &cfg, 1);

    return cfg;
}

void flash_special_write_config_reg(uint8_t data)
{
    sfc_cmd_t cmd = FLASH_CMD(WRITE_CFG_REG);

    cmd.addr = 0;
    sfc_send_write_cmd(&cmd, &data, 1);
}

void flash_special_pe_suspend(void) IRAM_TEXT(flash_special_pe_suspend);
void flash_special_pe_suspend(void)
{
    sfc_cmd_t cmd = FLASH_CMD(PE_SUSPEND);
    cmd.addr = 0;

    sfc_send_cmd(&cmd);

    flash_special_wait_suspend();
}

void flash_special_pe_resume(void) IRAM_TEXT(flash_special_pe_resume);
void flash_special_pe_resume(void)
{
    sfc_cmd_t cmd = FLASH_CMD(PE_RESUME);
    cmd.addr = 0;

    sfc_send_cmd(&cmd);
}

static void flash_special_reset_en(void)
{
    sfc_cmd_t cmd = FLASH_CMD(RESET_EN);
    cmd.addr = 0;

    sfc_send_cmd(&cmd);
}

static void flash_special_reset(void)
{
    sfc_cmd_t cmd = FLASH_CMD(RESET);
    cmd.addr = 0;

    sfc_send_cmd(&cmd);
}

void flash_special_soft_reset(void)
{
    flash_special_reset_en();

    flash_special_reset();
}

void flash_init(void)
{
    sfc_init();
    uint16_t id = flash_special_get_id();

    for (uint8_t i = 0; i < ARRAY_SIZE(flash_info); i++) {
        if ((id & 0xFF) == flash_info[i].id) {
            flash_special_func = (struct flash_ctrl *)(flash_info[i].func);

            break;
        }
    }

    flash_special_read_info();
}

bool_t flash_special_is_pe_in_progress(void)
{
    return sfc_is_pe_in_progress();
}

static uint8_t flash_special_sw_page_program(uint32_t addr, const void *buf, size_t count) IRAM_TEXT(flash_special_sw_page_program);
static uint8_t flash_special_sw_page_program(uint32_t addr, const void *buf, size_t count)
{
    bool_t first_time = true;
    uint8_t time_out;

    for (;;) {
#if !defined(BUILD_OS_NON_OS)
        os_task_suspend_all();

        uint32_t mask = cpu_disable_irq();

        /* suspend bt core's task*/
        iot_suspend_sched_wait_core_suspend(BT_CORE);
#endif

#ifdef CHECK_ISR_FLASH
        iot_soc_cpu_access_enable(IOT_SOC_CPU_ACCESS_DTOP_FLASH, false);
#endif

        apb_btb_enable((uint8_t)cpu_get_mhartid(), false);
        if (first_time) {
            flash_special_write_enable();
            time_out = flash_special_func->page_program_sw(addr, buf, count);
            first_time = false;
        } else {
            time_out = flash_special_func->sw_pe_alg();
        }
        apb_btb_enable((uint8_t)cpu_get_mhartid(), true);

#ifdef CHECK_ISR_FLASH
        iot_soc_cpu_access_enable(IOT_SOC_CPU_ACCESS_DTOP_FLASH, true);
#endif

#if !defined(BUILD_OS_NON_OS)
        /* resume bt core's task*/
        iot_suspend_sched_notify_core_resume(BT_CORE);

        cpu_restore_irq(mask);

        os_task_resume_all();
#endif

        /* erase finish */
        if (!time_out) {
            break;
        }

#if !defined(BUILD_OS_NON_OS)
        os_delay(1);
#endif
    }

    return RET_OK;
}


static uint8_t flash_special_sw_erase(uint32_t addr) IRAM_TEXT(flash_special_sw_erase);
static uint8_t flash_special_sw_erase(uint32_t addr)
{
    bool_t first_time = true;
    uint8_t time_out;

    for (;;) {
#if !defined(BUILD_OS_NON_OS)
        os_task_suspend_all();

        uint32_t mask = cpu_disable_irq();

        /* suspend bt core's task*/
        iot_suspend_sched_wait_core_suspend(BT_CORE);
#endif

#ifdef CHECK_ISR_FLASH
        iot_soc_cpu_access_enable(IOT_SOC_CPU_ACCESS_DTOP_FLASH, false);
#endif

        apb_btb_enable((uint8_t)cpu_get_mhartid(), false);
        if (first_time) {
            flash_special_write_enable();
            time_out = flash_special_func->sector_erase_sw(addr);
            first_time = false;
        } else {
            time_out = flash_special_func->sw_pe_alg();
        }
        apb_btb_enable((uint8_t)cpu_get_mhartid(), true);

#ifdef CHECK_ISR_FLASH
        iot_soc_cpu_access_enable(IOT_SOC_CPU_ACCESS_DTOP_FLASH, true);
#endif

#if !defined(BUILD_OS_NON_OS)
        /* resume bt core's task*/
        iot_suspend_sched_notify_core_resume(BT_CORE);

        cpu_restore_irq(mask);

        os_task_resume_all();
#endif

        /* erase finish */
        if (!time_out) {
            break;
        }

#if !defined(BUILD_OS_NON_OS)
        os_delay(1);
#endif
    }

    return RET_OK;
}

static uint8_t flash_special_hw_page_program(uint32_t addr, const void *buf, size_t count) IRAM_TEXT(flash_special_hw_page_program);
static uint8_t flash_special_hw_page_program(uint32_t addr, const void *buf, size_t count)
{
    assert(flash_special_func->page_program != NULL);

    flash_special_func->page_program(addr, buf, count);

    return RET_OK;
}

static uint8_t flash_special_hw_erase(uint32_t addr) IRAM_TEXT(flash_special_hw_erase);
static uint8_t flash_special_hw_erase(uint32_t addr)
{
    assert(flash_special_func->sector_erase != NULL);

    flash_special_func->sector_erase(addr);

    return RET_OK;
}

static uint8_t flash_special_page_program(uint32_t addr, const void *buf, size_t count) IRAM_TEXT(flash_special_page_program);
static uint8_t flash_special_page_program(uint32_t addr, const void *buf, size_t count)
{
    uint8_t ret = RET_OK;
    FLASH_PE_MODE mode = flash_special_get_pe_mode();

    if (mode == FLASH_PE_SW_MODE) {
        ret = flash_special_sw_page_program(addr, buf, count);
    } else {
        ret = flash_special_hw_page_program(addr, buf, count);
    }

    return ret;
}

uint8_t flash_special_erase(uint32_t addr) IRAM_TEXT(flash_special_erase);
uint8_t flash_special_erase(uint32_t addr)
{
    uint8_t ret = RET_OK;
    FLASH_PE_MODE mode = flash_special_get_pe_mode();

    if (mode == FLASH_PE_SW_MODE) {
        ret = flash_special_sw_erase(addr);
    }  else {
        ret = flash_special_hw_erase(addr);
    }

    return ret;
}

uint8_t flash_special_chip_erase(void)
{
    assert(flash_special_func->chip_erase != NULL);

    flash_special_func->chip_erase();

    return RET_OK;
}

uint8_t flash_special_write(uint32_t addr, const void *buf, size_t count)
{
    uint32_t left = count;
    uint32_t w_len = count;
    uint32_t w_addr = addr;
    const uint8_t *w_buf = buf;
    uint8_t ret = RET_OK;

    if (w_addr % FLASH_PAGE_SIZE != 0) {
        w_len = MIN(left, FLASH_PAGE_SIZE - (w_addr % FLASH_PAGE_SIZE));

        ret = flash_special_page_program(w_addr, w_buf, w_len);

        if (ret != RET_OK) {
            return ret;
        }

        left -= w_len;
        w_addr += w_len;
        w_buf += w_len;
    }

    while (left > 0) {
        w_len = MIN(left, FLASH_PAGE_SIZE);

        ret = flash_special_page_program(w_addr, w_buf, w_len);
        if (ret != RET_OK) {
            return ret;
        }

        left -= w_len;
        w_addr += w_len;
        w_buf += w_len;
    }

    return ret;
}

uint8_t flash_special_erase_sector(uint16_t sector_id)
{
    return flash_special_erase(sector_id * FLASH_SECTOR_SIZE);
}
