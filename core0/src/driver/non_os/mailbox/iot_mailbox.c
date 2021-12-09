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
#include "iot_irq.h"
#include "iot_mailbox.h"
#include "mailbox.h"
#include "riscv_cpu.h"

typedef struct iot_mailbox_info {
    iot_irq_t irq;
    iot_mailbox_msg_cb cb;
} iot_mailbox_info_t;

const uint32_t iot_mailbox_irq_vector[IOT_MAILBOX_CH_MAX] = {
    MAILBOX0_INT0, MAILBOX0_INT1, MAILBOX0_INT2, MAILBOX0_INT3,
    MAILBOX0_INT4, MAILBOX0_INT5, MAILBOX1_INT0, MAILBOX1_INT1};

const IOT_MAILBOX_CH iot_mailbox_ch[IOT_MAILBOX_CORE_MAX][IOT_MAILBOX_CORE_MAX] = {
    {IOT_MAILBOX_CH_0, IOT_MAILBOX_CH_0, IOT_MAILBOX_CH_0, IOT_MAILBOX_CH_2},
    {IOT_MAILBOX_CH_1, IOT_MAILBOX_CH_0, IOT_MAILBOX_CH_0, IOT_MAILBOX_CH_4},
    {IOT_MAILBOX_CH_0, IOT_MAILBOX_CH_0, IOT_MAILBOX_CH_0, IOT_MAILBOX_CH_0},
    {IOT_MAILBOX_CH_3, IOT_MAILBOX_CH_5, IOT_MAILBOX_CH_0, IOT_MAILBOX_CH_0},
};

iot_mailbox_info_t g_mb_info[MAILBOX_CH_MAX];

static uint32_t iot_mailbox_isr_handler(uint32_t vector, iot_addrword_t data)
{
    IOT_MAILBOX_CH ch = (IOT_MAILBOX_CH)data;
    UNUSED(vector);
    if (g_mb_info[ch].cb) {
        g_mb_info[ch].cb();
    }

    return 0;
}

static IOT_MAILBOX_CH iot_mailbox_get_channel(uint32_t source, uint32_t dest)
{
    return iot_mailbox_ch[source][dest];
}

static uint32_t iot_mailbox_get_irq_vector(IOT_MAILBOX_CH ch)
{
    return iot_mailbox_irq_vector[ch];
}

uint32_t iot_mailbox_open(uint32_t source, iot_mailbox_msg_cb cb)
{
    /*dest is current cpu core*/
    uint32_t dest = cpu_get_mhartid();

    IOT_MAILBOX_CH ch = iot_mailbox_get_channel(source, dest);
    uint32_t vector = iot_mailbox_get_irq_vector(ch);

    g_mb_info[ch].irq = iot_irq_create(vector, ch, iot_mailbox_isr_handler);
    g_mb_info[ch].cb = cb;
    iot_irq_unmask(g_mb_info[ch].irq);

    return RET_OK;
}

uint32_t iot_mailbox_close(uint32_t source)
{
    uint32_t dest = cpu_get_mhartid();

    IOT_MAILBOX_CH ch = iot_mailbox_get_channel(source, dest);
    iot_irq_mask(g_mb_info[ch].irq);
    iot_irq_delete(g_mb_info[ch].irq);
    g_mb_info[ch].irq = 0;
    g_mb_info[ch].cb = 0;

    return RET_OK;
}
/*Disable mailbox rev interrupt from other core*/
uint32_t iot_mailbox_mask(uint32_t source)
{
    uint32_t target = cpu_get_mhartid();

    IOT_MAILBOX_CH ch = iot_mailbox_get_channel(source, target);
    iot_irq_mask(g_mb_info[ch].irq);

    return RET_OK;
}
/*Enable mailbox rev interrupt from other core*/
uint32_t iot_mailbox_unmask(uint32_t source)
{
    uint32_t target = cpu_get_mhartid();

    IOT_MAILBOX_CH ch = iot_mailbox_get_channel(source, target);
    iot_irq_unmask(g_mb_info[ch].irq);

    return RET_OK;
}

uint32_t iot_mailbox_send(uint32_t target, uint32_t data)
{
    uint32_t ret;

    uint32_t source = cpu_get_mhartid();

    IOT_MAILBOX_CH ch = iot_mailbox_get_channel(source, target);
    ret = mailbox_write(ch, data);

    return ret;
}

uint32_t iot_mailbox_recint_clean(uint32_t source)
{
    uint32_t ret;

    uint32_t target = cpu_get_mhartid();

    IOT_MAILBOX_CH ch = iot_mailbox_get_channel(source, target);
    ret = mailbox_recvint_clean(ch);

    return ret;
}

uint32_t iot_mailbox_read(uint32_t source, uint32_t *p_data)
{
    uint32_t data_num;

    uint32_t target = cpu_get_mhartid();

    IOT_MAILBOX_CH ch = iot_mailbox_get_channel(source, target);
    data_num = iot_mailbox_get_msg_num(source);

    for (uint32_t i = 0; i < data_num; i++) {
        mailbox_read(ch, p_data + i);
    }

    return data_num;
}

uint32_t iot_mailbox_get_msg_num(uint32_t source)
{
    uint32_t target = cpu_get_mhartid();

    IOT_MAILBOX_CH ch = iot_mailbox_get_channel(source, target);

    return mailbox_get_read_space(ch);
}

void iot_mailbox_init(uint8_t iot_mailbox_id)
{
    mailbox_init(iot_mailbox_id);
}
