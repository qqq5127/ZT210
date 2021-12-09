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

/* hw includes */
#include "apb.h"
#include "iot_rom_patch.h"

#include "exception.h"
#include "encoding.h"
#include "iot_memory_config.h"

#ifdef LOW_POWER_ENABLE
#include "dev_pm.h"
#endif

#define IOT_DTOP_PATCH_NUM_BOUNDRY 3
#define IOT_BT_PATCH_NUM_BOUNDRY   7
#define IOT_DSP_PATCH_NUM_BOUNDRY  3
#define IOT_ROM_PATCH_MAX          16

#ifdef LOW_POWER_ENABLE
struct pm_operation rom_patch_pm;
#endif

static rompatch_func_t rom_patch_fun_info[IOT_ROM_PATCH_MAX];
static uint8_t iot_rom_patch_dtop_used_flag = 0;
static uint16_t iot_rom_patch_bt_used_flag = 0;
static uint8_t iot_rom_patch_dsp_used_flag = 0;

static cpu_exception_handler old_exception_handler = NULL;

void iot_rom_patch_controller_enable(IOT_ROM_PATCH_GROUP group,
                                     IOT_ROM_PATCH_ID patch_id)
{
    if (group == IOT_DTOP_BOOT_ROM_PATCH) {
        UNUSED(group);
        UNUSED(patch_id);
        return;
    }

    if (apb_rom_patch_get_status((APB_ROM_PATCH_GROUP)group, (APB_ROM_PATCH_ID)patch_id) == 0) {
        apb_rom_patch_enable((APB_ROM_PATCH_GROUP)group, (APB_ROM_PATCH_ID)patch_id);
    }
}

void iot_rom_patch_controller_disable(IOT_ROM_PATCH_GROUP group,
                                      IOT_ROM_PATCH_ID patch_id)
{
    apb_rom_patch_disable((APB_ROM_PATCH_GROUP)group, (APB_ROM_PATCH_ID)patch_id);
}

void iot_rom_patch_points_disable(IOT_ROM_PATCH_GROUP group, uint8_t member)
{
    uint32_t tmp_rom_addr = 0;
    IOT_ROM_PATCH_ID patch_id;
    if (IOT_DTOP_BOOT_ROM_PATCH == group) {
        if (member <= IOT_DTOP_PATCH_NUM_BOUNDRY) {
            patch_id = IOT_ROM_PATCH0;
        } else {
            patch_id = IOT_ROM_PATCH1;
            /* DTOP rom patch4~patch7 is DSP rom patch1,so should be disable DSP rom patch1 */
            apb_rom_patch_disable((APB_ROM_PATCH_GROUP)IOT_DSP_ROM_PATCH, (APB_ROM_PATCH_ID)patch_id);
        }
        /* disable DTOP rom patch points */
        apb_set_rom_patch_addr((APB_ROM_PATCH_GROUP)group, (APB_ROM_PATCH_ID)patch_id, member, tmp_rom_addr);
    } else if (IOT_BT_ROM_PATCH == group) {
        if (member <= IOT_BT_PATCH_NUM_BOUNDRY) {
            patch_id = IOT_ROM_PATCH0;
        } else {
            patch_id = IOT_ROM_PATCH1;
        }
        apb_set_rom_patch_addr((APB_ROM_PATCH_GROUP)group, (APB_ROM_PATCH_ID)patch_id, member - 7, tmp_rom_addr);
    }
}

static void iot_rom_patch_fill(IOT_ROM_PATCH_GROUP group,
                               IOT_ROM_PATCH_ID patch_id, uint8_t patch_number,
                               uint32_t rom_addr, uint32_t data)
{
    apb_set_rom_patch_addr((APB_ROM_PATCH_GROUP)group, (APB_ROM_PATCH_ID)patch_id, patch_number, rom_addr);
    apb_set_rom_patch_data((APB_ROM_PATCH_GROUP)group, (APB_ROM_PATCH_ID)patch_id, patch_number, data);
}

static uint8_t iot_rom_patch_config(IOT_ROM_PATCH_GROUP group,
                                    uint8_t patch_number, uint32_t rom_addr)
{
    uint32_t data = 0;
    uint32_t tmp_rom_val = 0;
    uint8_t inst_length;
    IOT_ROM_PATCH_ID patch_id = IOT_ROM_PATCH0;

    if ((group >= IOT_ROM_PATCH_GROUP_MAX)) {
        return RET_INVAL;
    }

    inst_length = cpu_get_instruction_length(rom_addr);

    if (inst_length == 4) {
        data = OS_LONG_EBREAK_TRIGER;
    } else if ((rom_addr & 3) == 0 && inst_length == 2) {
        data = OS_EBREAK_TRIGER;
    } else if ((rom_addr & 3) != 0 && inst_length == 2) {
        tmp_rom_val = *((volatile uint32_t *)(rom_addr - (rom_addr & 3)));
        tmp_rom_val &= 0xffff;
        data = tmp_rom_val | ((uint32_t)(OS_EBREAK_TRIGER) << 16);
    }

    if (IOT_DTOP_BOOT_ROM_PATCH == group) {
        /*word align*/
        rom_addr >>= 2;
        rom_addr &= 0xffff;
        /* bit15 is enable bit */
        rom_addr |= (1 << 15);
        if (patch_number <= IOT_DTOP_PATCH_NUM_BOUNDRY) {
            patch_id = IOT_ROM_PATCH0;

            iot_rom_patch_fill(group, patch_id, patch_number, rom_addr, data);
        } else {
            patch_id = IOT_ROM_PATCH0;

            apb_set_rom_patch_addr((APB_ROM_PATCH_GROUP)group, (APB_ROM_PATCH_ID)patch_id, patch_number, rom_addr);
            group = IOT_DSP_ROM_PATCH;
            patch_id = IOT_ROM_PATCH1;
            apb_set_rom_patch_data((APB_ROM_PATCH_GROUP)group, (APB_ROM_PATCH_ID)patch_id, patch_number - 4, data);

            /*enable dsp patch1 controller*/
            iot_rom_patch_controller_enable(group, patch_id);
        }
    } else if (IOT_BT_ROM_PATCH == group) {
        /*word align*/
        rom_addr >>= 2;
        /* 1M address space */
        rom_addr &= 0xfffff;
        /* bit31 is enable bit */
        rom_addr |= BIT(31);
        /* BT ROM have 14 patch points */
        if (patch_number <= IOT_BT_PATCH_NUM_BOUNDRY) {
            patch_id = IOT_ROM_PATCH0;
            iot_rom_patch_fill(group, patch_id, patch_number, rom_addr, data);
        } else {
            patch_id = IOT_ROM_PATCH1;
            patch_number -= 8;
            iot_rom_patch_fill(group, patch_id, patch_number, rom_addr, data);
        }
    } else if (IOT_DSP_ROM_PATCH == group) {
        /* DSP have 8 patch points patch0:0~3 patch1:4~7 */
        if (patch_number <= IOT_DSP_PATCH_NUM_BOUNDRY) {
            patch_id = IOT_ROM_PATCH0;
        } else {
            patch_id = IOT_ROM_PATCH1;
            patch_number -= 4;
        }
        iot_rom_patch_fill(group, patch_id, patch_number, rom_addr, data);
    }

    return RET_OK;
}

static uint8_t iot_rom_patch_deconfig(IOT_ROM_PATCH_GROUP group,
                                      uint8_t patch_number)
{
    IOT_ROM_PATCH_ID patch_id = IOT_ROM_PATCH0;
    uint32_t rom_addr = 0;

    if ((group >= IOT_ROM_PATCH_GROUP_MAX)) {
        return RET_INVAL;
    }

    if (IOT_DTOP_BOOT_ROM_PATCH == group) {
        if (patch_number <= IOT_DTOP_PATCH_NUM_BOUNDRY) {
            patch_id = IOT_ROM_PATCH0;
        } else {
            patch_id = IOT_ROM_PATCH0;
        }

        apb_set_rom_patch_addr((APB_ROM_PATCH_GROUP)group, (APB_ROM_PATCH_ID)patch_id, patch_number, rom_addr);
    } else if (IOT_BT_ROM_PATCH == group) {
        /* BT ROM have 14 patch points */
        if (patch_number <= IOT_BT_PATCH_NUM_BOUNDRY) {
            patch_id = IOT_ROM_PATCH0;
        } else {
            patch_id = IOT_ROM_PATCH1;
            patch_number -= 8;
        }

        apb_set_rom_patch_addr((APB_ROM_PATCH_GROUP)group, (APB_ROM_PATCH_ID)patch_id, patch_number, rom_addr);
    } else if (IOT_DSP_ROM_PATCH == group) {
        /* DSP have 8 patch points patch0:0~3 patch1:4~7 */
        if (patch_number <= IOT_DSP_PATCH_NUM_BOUNDRY) {
            patch_id = IOT_ROM_PATCH0;
        } else {
            patch_id = IOT_ROM_PATCH1;
        }
        iot_rom_patch_controller_disable(group, patch_id);
    }
    return RET_OK;
}

static bool_t iot_rom_patch_find_func(uint32_t rom_address,
                                      uint32_t *patch_address)
{
    for (uint8_t i = 0; i < IOT_ROM_PATCH_MAX; i++) {
        if ((rom_patch_fun_info[i].used_flag == USED)
            && (rom_patch_fun_info[i].rom_addr == rom_address)) {

            *patch_address = rom_patch_fun_info[i].func_addr;
            return true;
        }
    }

    return false;
}

static bool_t iot_rom_patch_handler(const exception_info_t *info)
{
    trap_stack_registers_t *frame = (void *)info->regs;
    uint32_t rom_address = frame->mepc - 4;
    uint32_t patch_address = 0;

    if (iot_rom_patch_find_func(rom_address, &patch_address)) {
        /* patch hit*/
        frame->mepc = patch_address;
        return true;
    }

    /*patch miss*/
    return false;
}

static void exception_with_rom_patch(void) IRAM_TEXT(exception_with_rom_patch);
static void exception_with_rom_patch(void)
{
    exception_info_t *info = cpu_get_exception_data();

    if ((info->mcause & MCAUSE_CAUSE) == CAUSE_BREAKPOINT) {
        if (iot_rom_patch_handler(info)) {
            return;
        }
    }

    /* original exception handler */
    if (old_exception_handler) {
        old_exception_handler();
    }
}

static rom_patch_info_t rom_patch_info;
static rom_patch_info_t iot_rom_patch_alloc(uint32_t rom_address)
{
    uint8_t i;

    if (AON_S0_BOOT_ROM_BASEADDR <= rom_address
        && rom_address <= AON_S0_BOOT_ROM_ENDADDR) {
        rom_patch_info.rom_patch_group = IOT_DTOP_BOOT_ROM_PATCH;
        for (i = 0; i < 8; i++) {
            if (0 == (iot_rom_patch_dtop_used_flag & BIT(i))) {
                if (i < 4) {
                    iot_rom_patch_dtop_used_flag |= (uint8_t)BIT(i);
                    rom_patch_info.patch_number = i;
                    rom_patch_info.valid = true;

                    return rom_patch_info;
                } else {
                    if (iot_rom_patch_dsp_used_flag & 0xf0) {
                        rom_patch_info.valid = false;
                    } else {
                        rom_patch_info.patch_number = i;
                        rom_patch_info.valid = true;
                    }

                    return rom_patch_info;
                }
            }
        }
    } else if (BT_S0_DROM_BASEADDR <= rom_address
               && rom_address < BT_S1_DROM_BASEADDR) {
        rom_patch_info.rom_patch_group = IOT_BT_ROM_PATCH;
        rom_patch_info.patch_id = IOT_ROM_PATCH0;
        for (i = 0; i < 8; i++) {
            if (0 == (iot_rom_patch_bt_used_flag & BIT(i))) {
                iot_rom_patch_bt_used_flag |= (uint8_t)BIT(i);
                rom_patch_info.patch_number = i;
                rom_patch_info.valid = true;

                return rom_patch_info;
            }
        }
    } else if (BT_S1_DROM_BASEADDR <= rom_address
               && rom_address <= BT_S1_DROM_ENDADDR) {
        rom_patch_info.rom_patch_group = IOT_BT_ROM_PATCH;
        rom_patch_info.patch_id = IOT_ROM_PATCH1;
        for (i = 8; i < 16; i++) {
            if (0 == (iot_rom_patch_bt_used_flag & BIT(i))) {
                iot_rom_patch_bt_used_flag |= (uint8_t)BIT(i);
                rom_patch_info.patch_number = i;
                rom_patch_info.valid = true;
                return rom_patch_info;
            }
        }
    } else if (AUD_S0_TCM_IROM_BASEADDR <= rom_address
               && rom_address <= AUD_S0_TCM_IROM_ENDADDR) {
        rom_patch_info.rom_patch_group = IOT_DSP_ROM_PATCH;
        rom_patch_info.patch_id = IOT_ROM_PATCH0;
        for (i = 0; i < 4; i++) {
            if (0 == (iot_rom_patch_dsp_used_flag & BIT(i))) {
                iot_rom_patch_dsp_used_flag |= (uint8_t)BIT(i);
                rom_patch_info.patch_number = i;
                rom_patch_info.valid = true;

                return rom_patch_info;
            }
        }
    } else if (AUD_S0_TCM_DROM_BASEADDR <= rom_address
               && rom_address < AUD_S0_TCM_DROM_ENDADDR) {
        rom_patch_info.rom_patch_group = IOT_DSP_ROM_PATCH;
        rom_patch_info.patch_id = IOT_ROM_PATCH1;
        for (i = 4; i < 8; i++) {
            if (0 == (iot_rom_patch_dsp_used_flag & BIT(i))) {
                if (iot_rom_patch_dtop_used_flag & 0xf0) {
                    rom_patch_info.valid = false;
                } else {
                    iot_rom_patch_dsp_used_flag |= (uint8_t)BIT(i);
                    rom_patch_info.patch_number = i;
                    rom_patch_info.valid = true;
                }
                return rom_patch_info;
            }
        }
    }
    rom_patch_info.valid = false;

    return rom_patch_info;
}

static IOT_ROM_PATCH_GROUP iot_rom_patch_get_group(uint32_t rom_address)
{
    IOT_ROM_PATCH_GROUP group = IOT_DTOP_BOOT_ROM_PATCH;

    if (AON_S0_BOOT_ROM_BASEADDR <= rom_address
        && rom_address <= AON_S0_BOOT_ROM_ENDADDR) {
        group = IOT_DTOP_BOOT_ROM_PATCH;
    } else if (BT_S0_DROM_BASEADDR <= rom_address
               && rom_address <= BT_S1_DROM_ENDADDR) {
        group = IOT_BT_ROM_PATCH;
    } else if (AUD_S0_TCM_IROM_BASEADDR <= rom_address
               && rom_address <= AUD_S0_TCM_IROM_ENDADDR) {
        group = IOT_DSP_ROM_PATCH;
    } else if (AUD_S0_TCM_DROM_BASEADDR <= rom_address
               && rom_address <= AUD_S0_TCM_DROM_ENDADDR) {
        group = IOT_DSP_ROM_PATCH;
    }

    return group;
}

void iot_rom_patch_install(uint32_t rom_address, uint32_t func_address)
{
    rom_patch_info_t patch_info;

    patch_info = iot_rom_patch_alloc(rom_address);

    if (patch_info.valid == false) {
        return;
    }

    iot_rom_patch_controller_enable(patch_info.rom_patch_group, patch_info.patch_id);

    iot_rom_patch_config(patch_info.rom_patch_group, patch_info.patch_number, rom_address);

    for (uint8_t i = 0; i < IOT_ROM_PATCH_MAX; i++) {

        if (rom_patch_fun_info[i].used_flag == NOUSED) {
            rom_patch_fun_info[i].rom_addr = rom_address;
            rom_patch_fun_info[i].func_addr = func_address;
            rom_patch_fun_info[i].patch_number = patch_info.patch_number;
            rom_patch_fun_info[i].patch_group = patch_info.rom_patch_group;
            rom_patch_fun_info[i].patch_id = patch_info.patch_id;
            rom_patch_fun_info[i].used_flag = USED;

            return;
        }
    }
}

void iot_rom_patch_unstall(uint32_t rom_address)
{
    IOT_ROM_PATCH_GROUP group = iot_rom_patch_get_group(rom_address);

    for (uint8_t i = 0; i < IOT_ROM_PATCH_MAX; i++) {

        if ((rom_patch_fun_info[i].rom_addr == rom_address)
            && (rom_patch_fun_info[i].used_flag == USED)) {
            rom_patch_fun_info[i].used_flag = NOUSED;
            iot_rom_patch_deconfig(group, rom_patch_fun_info[i].patch_number);
            switch (group) {
                case IOT_DTOP_BOOT_ROM_PATCH:
                    iot_rom_patch_dtop_used_flag &=
                        ~BIT(rom_patch_fun_info[i].patch_number);
                    break;
                case IOT_BT_ROM_PATCH:
                    iot_rom_patch_bt_used_flag &=
                        ~BIT(rom_patch_fun_info[i].patch_number);
                    break;
                case IOT_DSP_ROM_PATCH:
                    iot_rom_patch_dsp_used_flag &=
                        ~BIT(rom_patch_fun_info[i].patch_number);
                    break;
                case IOT_ROM_PATCH_GROUP_MAX:
                    break;
                default:
                    break;
            }

            return;
        }
    }
}

uint32_t iot_rom_patch_restore(uint32_t data)
{
    UNUSED(data);
    for (uint8_t i = 0; i < IOT_ROM_PATCH_MAX; i++) {
        if (rom_patch_fun_info[i].used_flag == USED) {
            iot_rom_patch_controller_enable((IOT_ROM_PATCH_GROUP)rom_patch_fun_info[i].patch_group,
                                            (IOT_ROM_PATCH_ID)rom_patch_fun_info[i].patch_id);
            iot_rom_patch_config((IOT_ROM_PATCH_GROUP)rom_patch_fun_info[i].patch_group,
                                 rom_patch_fun_info[i].patch_number,
                                 rom_patch_fun_info[i].rom_addr);
        }
    }

    return RET_OK;
}

void iot_rom_patch_init(void)
{
#ifdef LOW_POWER_ENABLE
    iot_dev_pm_node_init(&rom_patch_pm);
#endif
    old_exception_handler = cpu_get_exception_handler();

    cpu_register_exception_handler(exception_with_rom_patch);
#ifdef LOW_POWER_ENABLE
    rom_patch_pm.restore = iot_rom_patch_restore;
    iot_dev_pm_register(&rom_patch_pm);
#endif
}
