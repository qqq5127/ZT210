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
#include "ddr.h"
#include "ddr_special.h"
#include "iot_memory_origin.h"

static void ddr_special_info_cfg(ddr_cfg_t *cfg)
{
#if (PSRAM_DDR_LENGTH == AP_DDR_SPECIAL_4MB_SIZE)
    /*0:ap type*/
    cfg->ddr_pmc_info.wb_ap_sel = 0;
    /*DQS and DQ is separate pad*/
    cfg->ddr_pmc_info.dqs_dm_shared = 0;

    /*ap memory instruct type*/
    cfg->ddr_pmc_info.inst_type = 1;
    cfg->ddr_pmc_info.chip_size = 5;
    cfg->ddr_pmc_info.total_size = 5;
    cfg->ddr_pmc_info.bank_size = 10;

    /*disable tranmit*/
    cfg->ddr_tx_dl_handle.trim_ena = 0;
    cfg->ddr_tx_dl_handle.dl_eb = 1;
    cfg->ddr_tx_dl_handle.dl_step = 2;
    cfg->ddr_timing.tx_req_timing = 0;
    cfg->ddr_timing.tcph_tim = 4;
    cfg->ddr_timing.rece_early_num = 1;

    /*additional initial access latency in transaction*/
    cfg->ddr_trans_info.add_latency_en = 0;

    /*additional initial access latency after transaction*/
    cfg->ddr_trans_info.add_latency = 0;

    cfg->ddr_trans_info.spread_cycle_num = 2;
    cfg->ddr_trans_info.the_last_dm_active = 1;
    cfg->ddr_trans_info.the_last_have_dm = NULL;

    /*1 cycles in write transaction*/
    cfg->ddr_trans_info.wr_latency = 1;

    /*read instruction*/
    cfg->ddr_inst_type.inst_rd = 0;

    /*write instruction*/
    cfg->ddr_inst_type.inst_wr = 0x8000;
#else
    /*0:ap type*/
    cfg->ddr_pmc_info.wb_ap_sel = 1;
    /*DQS and DQ is same pad*/
    cfg->ddr_pmc_info.dqs_dm_shared = 1;
    /*ap memory instruct type*/
    cfg->ddr_pmc_info.inst_type = 0;
    cfg->ddr_pmc_info.chip_size = 6;
    cfg->ddr_pmc_info.total_size = 6;

    /*bank size:2**10=1KB*/
    cfg->ddr_pmc_info.bank_size = 10;

    cfg->ddr_tx_dl_handle.trim_ena = 0;
    cfg->ddr_tx_dl_handle.dl_eb = 1;
    cfg->ddr_tx_dl_handle.dl_step = 2;

    cfg->ddr_timing.tx_req_timing = 0;
    cfg->ddr_timing.tcph_tim = 4;
    cfg->ddr_timing.rece_early_num = 1;

    /*additional initial access latency in transaction*/
    cfg->ddr_trans_info.add_latency_en = 0;

    /*additional initial access latency after transaction*/
    cfg->ddr_trans_info.add_latency = 2;

    cfg->ddr_trans_info.spread_cycle_num = 0;
    cfg->ddr_trans_info.the_last_dm_active = 1;
    cfg->ddr_trans_info.the_last_have_dm = NULL;

    /*3 cycles in write transaction*/
    cfg->ddr_trans_info.wr_latency = 3;

    /*read instruction*/
    cfg->ddr_inst_type.inst_rd = 0x2020;

    /*write instruction*/
    cfg->ddr_inst_type.inst_wr = 0xA0A0;
#endif
}

void ddr_special_init(void)
{
    uint16_t rdata;
    ddr_cfg_t cfg;

    ddr_special_info_cfg(&cfg);

    /*chip info cfg*/
    ddr_info_cfg(&cfg.ddr_pmc_info);

    /*timimg*/
    ddr_trans_timing_cfg(&cfg.ddr_tx_dl_handle);

    /*work mode:normal*/
    ddr_normal_mode_cfg();

    /*ctrl parameter*/
    ddr_ctrl_cfg(&cfg.ddr_timing);
#if (PSRAM_DDR_LENGTH == AP_DDR_SPECIAL_4MB_SIZE)
    /*dev parameter0*/
    ddr_dev_trans_cfg(&cfg.ddr_trans_info);

    /*dev parameter1*/
    ddr_dev_inst_cfg(&cfg.ddr_inst_type);

    ddr_soft_reset();

    rdata = (uint16_t)ddr_reg_read(DDR_SPECIAL_AP_32M_WRADDR, REG_RD_INST_AP_32M);

    /******************bit12 bit11 bit10***************
    *********************1     0      0***********133M
    *********************0     1      1***********109M
    *********************0     1      0************66M
    **************************************************/
    rdata = (rdata & ~(0x7 << 10)) | (2 << 10);
    ddr_reg_write(DDR_SPECIAL_AP_32M_WRADDR, REG_WR_INST_AP_32M, rdata);

#else

    uint8_t tmpdm = 1;
    cfg.ddr_trans_info.the_last_have_dm = &tmpdm;
    ddr_dev_trans_cfg(&cfg.ddr_trans_info);

    /*dev parameter1*/
    ddr_dev_inst_cfg(&cfg.ddr_inst_type);

    ddr_soft_reset();

    rdata = ddr_reg_read(DDR_SPECIAL_AP_64M_WRADDR1, REG_RD_INST_AP_64M);
    rdata = (rdata & ~(0x7 << 10)) | (0 << 10);
    ddr_reg_write(DDR_SPECIAL_AP_64M_WRADDR1, REG_WR_INST_AP_64M, rdata);

    rdata = ddr_reg_read(DDR_SPECIAL_AP_64M_WRADDR2, REG_RD_INST_AP_64M);
    rdata = (rdata & ~(0x7 << 13)) | (0 < 13);
    ddr_reg_write(DDR_SPECIAL_AP_64M_WRADDR2, REG_WR_INST_AP_64M, rdata);

#endif
}
