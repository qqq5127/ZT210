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

#include "flash.h"
#include "sfc.h"

#include "iot_timer.h"
#include "iot_soc.h"

/**                       cmd   opmode        swm_mode, swm_cycle, wtime */
#define GD_PAGE_PROGRAM   0x02, SFC_OP_PRM, 0x200, 0x08180000, 0x960      //PE=2.4ms
#define GD_QUAD_PAGE_PROGRAM 0x32, SFC_OP_PRM, 0x201, 0x08180000, 0x960      //PE=2.4ms
#define GD_SECTOR_ERASE      0x20, SFC_OP_ERASE, 0x200, 0x08180000, 0x493E0  //sector erase time=300ms
#define GD_OTP_ERASE         0x44, SFC_OP_ERASE, 0x200, 0x08180000, 0x2000
#define GD_OTP_PAGE_PROGRAM  0x42, SFC_OP_PRM, 0x200, 0x08180000, 0x2000
#define GD_CHIP_ERASE        0x60, SFC_OP_ERASE, 0x200, 0x08000000, 0x44AA20   //chip erase time=4.5s

#define PROG_TIME_SLICE 450
#define ERAS_TIME_SLICE 1000

static const sfc_pe_param_t gd_pe_param[] = {
    FLASH_PE_INFO(100, 30, 30, 250, 32, 2400, 300000),
    FLASH_PE_INFO(100, 30, 20, 250, 32, 2400, 300000)
};

const uint32_t flash_gd_uid[] IRAM_RODATA(flash_gd_uid) = {
    0x14c80000, //2MB
    0x15c80000, //4MB
};

static flash_id_info_t flash_gd_info = {
    0,      //initial state flag: invalid
    0,      //invalid
    0       //invalid flash uid
};

static void flash_gd_set_quad_mode(void)
{
    uint8_t sts1;
    uint8_t sts2;
    uint16_t sts;

    sts2 = flash_special_read_status_reg2();
    if (sts2 & STS_QE_BIT_S9) {
        return;
    }

    sts2 |= STS_QE_BIT_S9;
    sts1 = flash_special_read_status_reg1();
    sts = sts1 | (sts2 << 8);

    flash_special_write_enable();

    flash_special_write_status_reg1(sts, 2);

    flash_special_wait_wip();

    while (!(flash_special_read_status_reg2() & STS_QE_BIT_S9)) {
    }
}

static void flash_gd_enable_qpp_mode(void)
{
}

static void flash_gd_disable_qpp_mode(void)
{
}

static void flash_gd_otp_lock(uint8_t id)
{
    uint8_t sts1;
    uint8_t sts2;
    uint8_t mask = 0;
    uint16_t sts;

    switch (id) {
        case FLASH_OTP_REGION0:
            mask = STS_LB1_BIT_S11;
            break;
        case FLASH_OTP_REGION1:
            mask = STS_LB2_BIT_S12;
            break;
        case FLASH_OTP_REGION2:
            mask = STS_LB3_BIT_S13;
            break;
        default:
            return;
    }

    sts2 = flash_special_read_status_reg2();

    if (sts2 & mask) {
        return;
    }

    sts2 |= mask;
    sts1 = flash_special_read_status_reg1();
    sts = sts1 | (sts2 << 8);

    flash_special_write_enable();
    flash_special_write_status_reg1(sts, 2);
    flash_special_wait_wip();

    while (!(flash_special_read_status_reg2() & mask)) {
    }
}

static uint8_t flash_gd_page_program(uint32_t addr, const void *buf, size_t count)
{
    sfc_cmd_t pp = FLASH_CMD(GD_PAGE_PROGRAM);
    sfc_cmd_t qpp = FLASH_CMD(GD_QUAD_PAGE_PROGRAM);
    sfc_cmd_t *cmd = NULL;
    uint8_t ret;

    uint32_t wtime = flash_special_get_program_wip_wait_time();

    if (flash_special_is_quad_mode()) {
        cmd = &qpp;
    } else {
        cmd = &pp;
    }

    cmd->addr = addr;

    /* check WIP status */
    flash_special_wait_wip();

    uint8_t offset = flash_gd_info.param_offset;
    cmd->wtime = gd_pe_param[offset].page_program_time;

    if (wtime != 0) {
        cmd->wtime = wtime;
    }

    /* write enable cmd*/
    flash_special_write_enable();

    sfc_set_dynamic_suspend_resume_param(&gd_pe_param[offset], cmd->wtime);

    ret = sfc_send_write_cmd(cmd, buf, count);

    return ret;
}

static uint8_t flash_gd_erase(uint32_t addr)
{
    sfc_cmd_t cmd = FLASH_CMD(GD_SECTOR_ERASE);
    uint8_t ret;

    uint32_t wtime = flash_special_get_erase_wip_wait_time();

    cmd.addr = addr;

    /* check WIP status */
    flash_special_wait_wip();

    uint8_t offset = flash_gd_info.param_offset;

    cmd.wtime = gd_pe_param[offset].sector_erase_time;

    if (wtime != 0) {
        cmd.wtime = wtime;
    }

    flash_special_write_enable();

    sfc_set_dynamic_suspend_resume_param(&gd_pe_param[offset], cmd.wtime);
    ret = sfc_send_cmd(&cmd);

    return ret;
}

static uint8_t flash_gd_otp_page_program(uint32_t addr, const void *buf, size_t count)
{
    sfc_cmd_t cmd = FLASH_CMD(GD_OTP_PAGE_PROGRAM);
    uint8_t ret;

    cmd.addr = addr;

    /* check WIP status */
    flash_special_wait_wip();

    /* write enable cmd*/
    flash_special_write_enable();

    ret = sfc_send_write_cmd(&cmd, buf, count);

    return ret;
}

static uint8_t flash_gd_otp_erase(uint32_t addr)
{
    uint8_t ret;
    sfc_cmd_t cmd = FLASH_CMD(GD_OTP_ERASE);

    cmd.addr = addr;

    flash_special_wait_wip();

    flash_special_write_enable();
    ret = sfc_send_cmd(&cmd);

    return ret;
}

static uint8_t flash_gd_chip_erase(void)
{
    uint8_t ret;
    sfc_cmd_t cmd = FLASH_CMD(GD_CHIP_ERASE);

    cmd.addr = 0;

    flash_special_write_enable();
    ret = sfc_send_cmd(&cmd);

    flash_special_wait_wip();

    return ret;
}

static uint8_t flash_gd_sw_erase(uint32_t addr) IRAM_TEXT(flash_gd_sw_erase);
static uint8_t flash_gd_sw_erase(uint32_t addr)
{
    sfc_cmd_t cmd = FLASH_CMD(GD_SECTOR_ERASE);
    uint8_t ret;

    cmd.addr = addr;
    cmd.op_mode = SFC_OP_REG_WR;

    ret = sfc_send_cmd(&cmd);

    iot_timer_delay_us(ERAS_TIME_SLICE);

    if(flash_special_check_wip_status()){
        /*send suspend command*/
        flash_special_pe_suspend();
        ret = 1;
    } else {
        ret = 0;
    }

    return ret;
}

static uint8_t flash_gd_page_program_sw(uint32_t addr, const void *buf, size_t count) IRAM_TEXT(flash_gd_page_program_sw);
static uint8_t flash_gd_page_program_sw(uint32_t addr, const void *buf, size_t count)
{
    sfc_cmd_t cmd = FLASH_CMD(GD_QUAD_PAGE_PROGRAM);
    uint8_t ret;

    cmd.addr = addr;
    cmd.op_mode = SFC_OP_TRANS;

    sfc_send_write_cmd(&cmd, buf, count);

    iot_timer_delay_us(PROG_TIME_SLICE);

    if(flash_special_check_wip_status()){
        /*send suspend command*/
        flash_special_pe_suspend();
        ret = 1;
    } else {
        ret = 0;
    }

    return ret;
}

static uint8_t flash_gd_sw_pe_alg(void) IRAM_TEXT(flash_gd_sw_pe_alg);
static uint8_t flash_gd_sw_pe_alg(void)
{
    uint8_t ret;

    /* send resume command */
    flash_special_pe_resume();

    iot_timer_delay_us(ERAS_TIME_SLICE);

    if(flash_special_check_wip_status()){
        /*send suspend command*/
        flash_special_pe_suspend();
        ret = 1;
    } else {
        ret = 0;
    }

    return ret;
}

static void flash_gd_read_info(void)
{
    uint16_t id;
    uint8_t i;

    if(!(flash_gd_info.v_flag)) {
        id = flash_special_get_id();
        flash_gd_info.uid |= ((uint32_t)id << 16);

        for (i = 0; i < ARRAY_SIZE(flash_gd_uid); i++) {
            if (flash_gd_info.uid == flash_gd_uid[i]) {
                flash_gd_info.param_offset = i;

                break;
            }
        }

        flash_gd_info.v_flag = 1;
    }
}

struct flash_ctrl flash_gd_set = {
    .set_quad_mode = flash_gd_set_quad_mode,
    .enable_qpp_mode = flash_gd_enable_qpp_mode,
    .otp_lock = flash_gd_otp_lock,
    .disable_qpp_mode = flash_gd_disable_qpp_mode,
    .page_program = flash_gd_page_program,
    .sector_erase = flash_gd_erase,
    .otp_write = flash_gd_otp_page_program,
    .otp_erase = flash_gd_otp_erase,
    .chip_erase = flash_gd_chip_erase,
    .page_program_sw = flash_gd_page_program_sw,
    .sector_erase_sw = flash_gd_sw_erase,
    .sw_pe_alg = flash_gd_sw_pe_alg,
    .read_info = flash_gd_read_info,
};
