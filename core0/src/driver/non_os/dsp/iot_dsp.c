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
/* common includes */
#include "types.h"
#include "string.h"

#include "iot_memory_config.h"
#include "iot_dsp.h"

/* hw includes */
#include "apb.h"
#include "apb_cache.h"
#include "pmm.h"

#define DSP_START_ENTRY  DSP_IRAM_START

/*lint -esym(754, section_info::type) */
typedef struct section_info {
    uint32_t type;    /* section type */
    uint32_t offset;  /* offset in binary image */
    uint32_t dest;    /* load address in dsp memory */
    uint32_t len;     /* section length */
} sect_info_t;

/*lint -esym(754, dsp_bin_head::image_len) */
/*lint -esym(754, dsp_bin_head::reserved) */
typedef struct dsp_bin_head {
    uint32_t magic;
#define DSP_HEAD_MAGIC  0xfacecafe
    uint32_t sect_num;  /* section number */
    uint32_t image_len; /* total length of image */
    uint32_t reserved;
    sect_info_t sections[];
} dsp_bin_head_t;

void iot_dsp_load_image(uint8_t *pbin)
{
    dsp_bin_head_t *ph;
    sect_info_t *ps;
    uint32_t entrys, i;

    ph = (dsp_bin_head_t *)pbin;   //lint !e826:Suspicious pointer-to-pointer conversion
    if (NTOHL(ph->magic) != DSP_HEAD_MAGIC) {
        return;
    }

    entrys = NTOHL(ph->sect_num);
    pbin += sizeof(dsp_bin_head_t) + sizeof(sect_info_t) * entrys;

    for (i = 0; i < entrys; i++) {
        ps = &(ph->sections[i]);
        memcpy((void *)(NTOHL(ps->dest)), pbin + NTOHL(ps->offset), NTOHL(ps->len));
    }
}

void iot_dsp_power_on(void)
{
    pmm_power_domain_poweron(PMM_PD_AUDIO);
}

void iot_dsp_power_off(void)
{
    pmm_power_domain_poweroff(PMM_PD_AUDIO);
}

void iot_dsp_stall_and_enable(void)
{
    iot_dsp_power_on();
    /* set dsp start entry */
    pmm_set_cpu_boot_addr(0x3, DSP_START_ENTRY);
    //apb_dsp_misc_ctl_in(0x10);

    /* reset dsp first */
    apb_dsp_disable();

    /* enable dsp memory, but stall dsp core */
    apb_dsp_stall_enable();
    apb_dsp_enable();

    /* set dsp rom layout */
    apb_dsp_switch_rom_layout(DSP_ROM_MEM_LAYOUT_MODE_0);

    /* set dsp ram layout */
    apb_dsp_switch_ram_layout(DSP_RAM_MEM_LAYOUT_MODE_2);

    /* config ddr cache, used for sram */
    apb_cache_enable_ram(APB_CACHE_SMC_ID);
}

void iot_dsp_start(void)
{
    /* set dsp rom layout */
    apb_dsp_switch_rom_layout(DSP_ROM_MEM_LAYOUT_MODE_1);

    apb_dsp_stall_disable();
}

void iot_dsp_load_and_start(uint8_t *pimage)
{
    if (!pimage) {
        return;
    }

    iot_dsp_stall_and_enable();

    iot_dsp_load_image(pimage);

    iot_dsp_start();
}
