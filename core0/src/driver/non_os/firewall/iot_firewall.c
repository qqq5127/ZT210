/****************************************************************************

Copyright(c) 2021 by WuQi Technologies. ALL RIGHTS RESERVED.

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
#include "iot_irq.h"
#include "firewall.h"
#include "riscv_cpu.h"
#include "iot_firewall.h"

static iot_irq_t firewall_interrupt_irq;

static uint8_t iot_firewall_get_id(void)
{
    uint32_t cpu = cpu_get_mhartid();

    if (cpu == 0) {
        return IOT_FIREWALL_0;
    } else if (cpu == 1 || cpu == 2) {
        return IOT_FIREWALL_1;
    } else {
        return IOT_FIREWALL_2;
    }
}

static uint8_t iot_firewall_get_irq_id(uint8_t cpu)
{
    if (cpu == IOT_FIREWALL_0) {
        return FIREWALL_MTX_MIX_INT;
    } else if (cpu == IOT_FIREWALL_1) {
        return BCP_FIREWALL_INT;
    } else {
        return ACP_FIREWALL_INT;
    }
}

static uint32_t iot_firewall_irq_handler(uint32_t vector, uint32_t data) IRAM_TEXT(iot_firewall_irq_handler);
static uint32_t iot_firewall_irq_handler(uint32_t vector, uint32_t data)
{
    UNUSED(data);
    UNUSED(vector);

    iot_irq_mask(firewall_interrupt_irq);

    extern void exception(void);

    exception();

    return 0;
}

void iot_firewall_init(void)
{
    uint8_t firewall_id = iot_firewall_get_id();

    firewall_init(firewall_id);

    uint32_t firewall_irq_vector = iot_firewall_get_irq_id(firewall_id);

    /* register firewall irq */
    firewall_interrupt_irq = iot_irq_create(firewall_irq_vector, 0, iot_firewall_irq_handler);

    /* turn on firewall irq */
    iot_irq_unmask(firewall_interrupt_irq);
}

uint8_t iot_firewall_master_config(IOT_FIREWALL_WR_ID wr, uint32_t start, uint32_t end)
{
    uint8_t firewall_id = iot_firewall_get_id();

    return firewall_master_config(firewall_id, (FIREWALL_WR_ID)wr, start, end);
}

uint8_t iot_firewall_segment_get_num(void)
{
    return firewall_segment_get_num();
}

bool_t iot_firewall_slaver_config(uint8_t enable, uint8_t master, uint32_t addr)
{
    uint8_t firewall_id = iot_firewall_get_id();

    if (firewall_slaver_config(firewall_id, enable, master, addr)) {
        return true;
    } else {
        return false;
    }
}

void iot_firewall_deinit(void)
{
    uint8_t firewall_id = iot_firewall_get_id();

    firewall_deinit(firewall_id);

    /* turn off firewall irq */
    iot_irq_mask(firewall_interrupt_irq);
    iot_irq_delete(firewall_interrupt_irq);
}
