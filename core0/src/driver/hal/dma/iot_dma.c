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
#include "riscv_cpu.h"
#include "chip_irq_vector.h"

/* hw includes */
#include "dma.h"
#include "apb.h"
#include "wic.h"

#include "iot_irq.h"
#include "iot_dma.h"

#include "iot_wic.h"
#include "iot_memory_config.h"

#include "iot_timer.h"
#include "iot_ipc.h"

#ifdef LOW_POWER_ENABLE
#include "power_mgnt.h"
#include "dev_pm.h"
#endif
#include "driver_dbglog.h"
#include "critical_sec.h"

#define IOT_DMA_DESCRIPTOR_NUM 32

enum {
    DMA_OWNER_CPU,
    DMA_OWNER_DMA,
};

struct mem_group_info {
    uint8_t *src;
    uint8_t *dst;
    uint32_t total_len;
};

typedef struct iot_dma_trans_info {
    struct iot_dma_trans_info *next;
    dma_descriptor_t rx_desc;
    dma_descriptor_t tx_desc;
    void *cb;
    void *param;
    bool_t in_use;

    uint32_t ts_us;
    struct mem_group_info info;
} iot_dma_trans_info_t;

typedef struct iot_dma_channel_info {
    iot_dma_trans_info_t *trans_head;
    iot_dma_trans_info_t *trans_tail;
    IOT_DMA_TRANS_TYPE work_mode;
} iot_dma_channel_info_t;

static const uint32_t iot_dma_int_vector[] = {
    DMA0_INT0,
    DMA1_INT0,
    DMA2_INT0,
};

#ifdef LOW_POWER_ENABLE
struct pm_operation dma_pm;
#endif

static uint32_t iot_dma_isr_handler(uint32_t vector, uint32_t data);
static bool_t iot_dma_ch_curr_desc_irq(
        IOT_DMA_CONTROLLER dma,
        IOT_DMA_CHANNEL_ID ch) IRAM_TEXT(iot_dma_ch_curr_desc_irq);

static dma_mem_peri_tx_ack_callback iot_dma_ch_tx_ack_cb[IOT_DMA_CHANNEL_MAX];
static iot_dma_trans_info_t iot_dma_trans[IOT_DMA_DESCRIPTOR_NUM];
static iot_dma_channel_info_t iot_dma_ch_info[IOT_DMA_CHANNEL_MAX];
static IOT_DMA_CHANNEL_ID iot_dma_mem_ch_soft =
    IOT_DMA_CHANNEL_NONE;   // software trigger
static iot_irq_t iot_dma_interrupt_irq;
static uint32_t iot_dma_ch_active_vect = 0; // active trans for this channel

static uint32_t iot_dma_ch_active_count[32] = {0};

static int32_t judge_domain_by_addr(uint32_t addr) IRAM_TEXT(judge_domain_by_addr);
static int32_t judge_domain_by_addr(uint32_t addr)
{
    if ((DCP_IRAM_START <= addr)
        && (addr < (DCP_IRAM_START + DCP_IRAM_LENGTH))) {
        return WIC_DTOP_CORE;

    } else if ((BT_IRAM_START <= addr)
               && (addr < (BT_IRAM_START + BT_IRAM_LENGTH))) {
        return WIC_BT_CORE;

    } else if ((AUDIO_IRAM_START <= addr)
               && (addr < (AUDIO_IRAM_START + AUDIO_IRAM_LENGTH))) {
        return WIC_AUDIO_CORE;
    }

    return WIC_SELF;
}

static int32_t judge_domain_by_addr2(uint32_t src, uint32_t dst) IRAM_TEXT(judge_domain_by_addr2);
static int32_t judge_domain_by_addr2(uint32_t src, uint32_t dst)
{
    int32_t core;

    core = judge_domain_by_addr(src);
    if (core != WIC_SELF)
        return core;

    return judge_domain_by_addr(dst);
}

static IOT_DMA_CONTROLLER iot_dma_get_controller(void)
{
    uint32_t cpu = cpu_get_mhartid();
    switch (cpu) {
        case 0:
            return IOT_DMA_CONTROLLER0;
        case 1:
        case 2:
            return IOT_DMA_CONTROLLER1;
        case 3:
            return IOT_DMA_CONTROLLER2;
        default:
            return IOT_DMA_CONTROLLER_MAX;
    }
}

static void iot_dma_free_trans(iot_dma_trans_info_t *trans) IRAM_TEXT(iot_dma_free_trans);
static void iot_dma_free_trans(iot_dma_trans_info_t *trans)
{
    if (trans) {
        trans->next = NULL;
        trans->rx_desc.buf_addr = 0;
        trans->rx_desc.buf_size = 0;
        trans->tx_desc.buf_addr = 0;
        trans->tx_desc.buf_size = 0;
        trans->in_use = false;
        trans->ts_us = 0;
        trans->cb = NULL;
    }
}

static iot_dma_trans_info_t *iot_dma_alloc_trans(void)
{
    iot_dma_trans_info_t *trans = NULL;

#if 0
    /**
     * This func will call from add desc or desc done int handler, free
     * idle trans first and alloc then.
     */
    for (IOT_DMA_CHANNEL_ID ch = DMA_CHANNEL_0; ch < IOT_DMA_CHANNEL_MAX; ch++) {
        trans = iot_dma_ch_info[ch].trans_head;
        /**
         * Have tx desc and tx desc owner is cpu, means this one trans done.
         * tx desc is NULL but rx desc owner is cpu, also means trans done.
         */
        while (trans != NULL && trans->in_use) {
            if ((trans->tx_desc.owner == DMA_OWNER_CPU
                 && trans->tx_desc.buf_size != 0)
                || (trans->rx_desc.owner == DMA_OWNER_CPU
                    && trans->rx_desc.buf_size != 0)) {
                if (trans->next == NULL
                    || trans == iot_dma_ch_info[ch].trans_tail) {
                    // All transfer done.
                    iot_dma_free_trans(trans);
                    iot_dma_ch_info[ch].trans_head = NULL;
                    iot_dma_ch_info[ch].trans_tail = NULL;
                    break;
                } else {
                    // Move to next
                    iot_dma_ch_info[ch].trans_head = trans->next;
                    iot_dma_free_trans(trans);
                    trans = iot_dma_ch_info[ch].trans_head;
                }
            } else {
                break;
            }
        }
    }
#endif
    uint32_t i;
    cpu_critical_enter();
    for (i = 0; i < IOT_DMA_DESCRIPTOR_NUM; i++) {
        if (!iot_dma_trans[i].in_use) {
            trans = &iot_dma_trans[i];
            memset(trans, 0, sizeof(*trans));
            trans->in_use = true;
            break;
        }
    }
    cpu_critical_exit();
    if (i == IOT_DMA_DESCRIPTOR_NUM) {
        for (IOT_DMA_CHANNEL_ID ch = IOT_DMA_CHANNEL_0; ch < IOT_DMA_CHANNEL_MAX; ch++) {
            trans = iot_dma_ch_info[ch].trans_head;
            /**
             * Have tx desc and tx desc owner is cpu, means this one trans done.
             * tx desc is NULL but rx desc owner is cpu, also means trans done.
             */
            while (trans != NULL) {
                DBGLOG_DRIVER_INFO("ch=%u,cb=0x%x,own=%u,dst=0x%x,ts=%u\n",
                        ch,
                        trans->cb,
                        trans->tx_desc.owner,
                        trans->tx_desc.buf_addr,
                        trans->ts_us);
                trans = trans->next;
            }
        }
        assert(0);
    }

    return trans;
}

static inline void iot_dma_fill_desc(dma_descriptor_t *desc, uint32_t buf_addr,
                                     uint32_t buf_size, uint8_t owner,
                                     bool_t int_en, bool_t start, bool_t end)
{
    desc->buf_addr = buf_addr;
    desc->buf_size = buf_size;
    desc->owner = owner == DMA_OWNER_DMA ? DMA_OWNER_DMA : DMA_OWNER_CPU;
    desc->start = start ? 1 : 0;
    desc->end = end ? 1 : 0;
    desc->int_en = int_en ? 1 : 0;
    desc->next = NULL;
}

static void iot_dma_clk_enable(void)
{
    IOT_DMA_CONTROLLER controller = iot_dma_get_controller();

    switch (controller) {
        case IOT_DMA_CONTROLLER0:
            apb_ahb_hclk_enable(APB_AHB_SW_DMA0);
            break;
        case IOT_DMA_CONTROLLER1:
            apb_clk_enable(APB_CLK_SW_DMA1);
            break;
        case IOT_DMA_CONTROLLER2:
            apb_clk_enable(APB_CLK_SW_DMA2);
            break;
        case IOT_DMA_CONTROLLER_MAX:
            break;
        default:
            break;
    }
}

uint8_t iot_dma_claim_channel(IOT_DMA_CHANNEL_ID *ch)
{
    IOT_DMA_CONTROLLER controller = iot_dma_get_controller();

    *ch = (IOT_DMA_CHANNEL_ID)dma_claim_channel((DMA_CONTROLLER)controller);
    if (*ch >= IOT_DMA_CHANNEL_MAX) {
        return RET_BUSY;
    }

    return RET_OK;
}

void iot_dma_free_channel(IOT_DMA_CHANNEL_ID ch)
{
    IOT_DMA_CONTROLLER controller = iot_dma_get_controller();

    iot_dma_ch_flush(ch);
    dma_release_channel((DMA_CONTROLLER)controller, (DMA_CHANNEL_ID)ch);
}

void iot_dma_channel_suspend(IOT_DMA_CHANNEL_ID ch)
{
    IOT_DMA_CONTROLLER controller = iot_dma_get_controller();

    dma_channel_suspend((DMA_CONTROLLER)controller, (DMA_CHANNEL_ID)ch);
}

void iot_dma_channel_reset(IOT_DMA_CHANNEL_ID ch)
{
    IOT_DMA_CONTROLLER controller = iot_dma_get_controller();

    dma_channel_reset((DMA_CONTROLLER)controller, (DMA_CHANNEL_ID)ch);
}

void iot_dma_mark_work_type(IOT_DMA_CHANNEL_ID ch, IOT_DMA_TRANS_TYPE type)
{
    iot_dma_ch_info[ch].work_mode = type;
}

uint32_t iot_dma_restore(uint32_t data)
{
    UNUSED(data);

    IOT_DMA_CONTROLLER controller = iot_dma_get_controller();

    // Enable dma clock first
    iot_dma_clk_enable();

    for (uint32_t i = 0; i < IOT_DMA_DESCRIPTOR_NUM; i++) {
        iot_dma_trans[i].in_use = false;
        iot_dma_trans[i].cb = NULL;
        iot_dma_trans[i].param = NULL;
        iot_dma_trans[i].next = NULL;
    }

    for (uint8_t i = 0; i < IOT_DMA_CHANNEL_MAX; i++) {
        iot_dma_ch_info[i].trans_head = NULL;
        iot_dma_ch_info[i].trans_tail = NULL;

        // Clear all int
        dma_channel_int_clear_all((DMA_CONTROLLER)controller, (DMA_CHANNEL_ID)i);

        // Default to set channel int to dma_int0
        dma_channel_int_select((DMA_CONTROLLER)controller, (DMA_CHANNEL_ID)i, DMA_INT_0);

        // clear tx ack callback
        iot_dma_ch_tx_ack_cb[i] = NULL;
    }

    iot_dma_mem_ch_soft = IOT_DMA_CHANNEL_NONE;     // software trigger

    iot_dma_ch_active_vect = 0;

    dma_init((DMA_CONTROLLER)controller);

    return RET_OK;
}

uint32_t iot_dma_get_act_ch_vect(void)
{
    return iot_dma_ch_active_vect;
}

void iot_dma_init(void)
{
    IOT_DMA_CONTROLLER controller = iot_dma_get_controller();

#ifdef LOW_POWER_ENABLE
    iot_dev_pm_node_init(&dma_pm);
#endif
    // Enable dma clock first
    iot_dma_clk_enable();

    for (uint32_t i = 0; i < IOT_DMA_DESCRIPTOR_NUM; i++) {
        iot_dma_trans[i].in_use = false;
        iot_dma_trans[i].cb = NULL;
        iot_dma_trans[i].param = NULL;
        iot_dma_trans[i].next = NULL;
    }

    for (uint8_t i = 0; i < IOT_DMA_CHANNEL_MAX; i++) {
        iot_dma_ch_info[i].trans_head = NULL;
        iot_dma_ch_info[i].trans_tail = NULL;

        // Clear all int
        dma_channel_int_clear_all((DMA_CONTROLLER)controller, (DMA_CHANNEL_ID)i);

        // Default to set channel int to dma_int0
        dma_channel_int_select((DMA_CONTROLLER)controller, (DMA_CHANNEL_ID)i, DMA_INT_0);

        // clear tx ack callback
        iot_dma_ch_tx_ack_cb[i] = NULL;
    }

    dma_init((DMA_CONTROLLER)controller);

    // Config interrupt
    iot_dma_interrupt_irq = iot_irq_create(iot_dma_int_vector[controller],
                                           controller, iot_dma_isr_handler);
    iot_irq_unmask(iot_dma_interrupt_irq);

#ifdef LOW_POWER_ENABLE
    dma_pm.restore = iot_dma_restore;
    iot_dev_pm_register(&dma_pm);
#endif
}

void iot_dma_deinit(void)
{
    IOT_DMA_CONTROLLER controller = iot_dma_get_controller();

    iot_irq_mask(iot_dma_interrupt_irq);
    iot_irq_delete(iot_dma_interrupt_irq);

    dma_deinit((DMA_CONTROLLER)controller);
}

static uint8_t iot_dma_mem_config(IOT_DMA_CHANNEL_ID ch)
{
    IOT_DMA_CONTROLLER controller = iot_dma_get_controller();
    dma_ch_config_t cfg;

    cfg.trans_type = DMA_TRANS_MEM_TO_MEM;
    cfg.src_addr_increase = true;
    cfg.dst_addr_increase = true;
    cfg.src_data_width = DMA_DATA_WIDTH_WORD;
    cfg.dst_data_width = DMA_DATA_WIDTH_WORD;
    cfg.src_bit_order = DMA_MSB_LSB;
    cfg.dst_bit_order = DMA_MSB_LSB;
    cfg.src_word_order = DMA_LITTLE_ENDIAN;
    cfg.dst_word_order = DMA_LITTLE_ENDIAN;
    cfg.src_burst_length =
        dma_get_ch_max_burst_length((DMA_CONTROLLER)controller, (DMA_CHANNEL_ID)ch);
    cfg.dst_burst_length =
        dma_get_ch_max_burst_length((DMA_CONTROLLER)controller, (DMA_CHANNEL_ID)ch);

    dma_set_channel_config((DMA_CONTROLLER)controller, (DMA_CHANNEL_ID)ch, &cfg);
    dma_set_channel_priority((DMA_CONTROLLER)controller, (DMA_CHANNEL_ID)ch, DMA_CH_PRIORITY_LOW);

    // Enable channel interrupt
    dma_channel_int_enable((DMA_CONTROLLER)controller, (DMA_CHANNEL_ID)ch, DMA_INT_TX_CURR_DECR);

    // Save work mode
    iot_dma_ch_info[ch].work_mode = IOT_DMA_TRANS_MEM_TO_MEM;

    return RET_OK;
}

uint8_t iot_dma_ch_peri_config(IOT_DMA_CHANNEL_ID ch, IOT_DMA_TRANS_TYPE type,
                                IOT_DMA_DATA_WIDTH width, IOT_DMA_CH_PRIORITY pri,
                                IOT_DMA_PERI_REQ src_req, IOT_DMA_PERI_REQ dst_req)
{
    IOT_DMA_CONTROLLER controller = iot_dma_get_controller();

    if (ch >= IOT_DMA_CHANNEL_MAX) {
        return RET_INVAL;
    }

    if (src_req >= IOT_DMA_PERI_MAX && dst_req >= IOT_DMA_PERI_MAX) {
        return RET_INVAL;
    }

    dma_ch_config_t cfg;

    cfg.trans_type = (DMA_TRANS_TYPE)type;
    cfg.src_data_width = (DMA_DATA_WIDTH)width;
    cfg.dst_data_width = (DMA_DATA_WIDTH)width;
    cfg.src_bit_order = DMA_MSB_LSB;
    cfg.dst_bit_order = DMA_MSB_LSB;
    cfg.src_word_order = DMA_LITTLE_ENDIAN;
    cfg.dst_word_order = DMA_LITTLE_ENDIAN;
    cfg.src_burst_length =
        dma_get_ch_max_burst_length((DMA_CONTROLLER)controller, (DMA_CHANNEL_ID)ch);
    // TODO: will reconfig dst_burst_length if need better performance in uart
    if (dst_req == IOT_DMA_PERI_UART0_TX || dst_req == IOT_DMA_PERI_UART1_TX) {
        cfg.dst_burst_length = 0;
    } else {
        cfg.dst_burst_length =
            dma_get_ch_max_burst_length((DMA_CONTROLLER)controller, (DMA_CHANNEL_ID)ch);
    }

    if (type == IOT_DMA_TRANS_MEM_TO_MEM) {
        cfg.src_addr_increase = true;
        cfg.dst_addr_increase = true;
    } else if (type == IOT_DMA_TRANS_MEM_TO_PERI) {
        cfg.src_addr_increase = true;
        cfg.dst_addr_increase = false;
    } else if (type == IOT_DMA_TRANS_PERI_TO_MEM) {
        cfg.src_addr_increase = false;
        cfg.dst_addr_increase = true;
    } else if (type == IOT_DMA_TRANS_PERI_TO_PERI) {
        cfg.src_addr_increase = false;
        cfg.dst_addr_increase = false;
    } else {
        return RET_INVAL;
    }

    dma_set_channel_config((DMA_CONTROLLER)controller, (DMA_CHANNEL_ID)ch, &cfg);
    dma_set_channel_priority((DMA_CONTROLLER)controller, (DMA_CHANNEL_ID)ch, (DMA_CH_PRIORITY)pri);
    dma_set_channel_request((DMA_CONTROLLER)controller, (DMA_CHANNEL_ID)ch, (DMA_PERI_REQ)src_req,
                            (DMA_PERI_REQ)dst_req);

    // Enable channel interrupt
    if (type == IOT_DMA_TRANS_PERI_TO_MEM) {
        dma_channel_int_enable((DMA_CONTROLLER)controller, (DMA_CHANNEL_ID)ch,
                               DMA_INT_TX_CURR_DECR);
    } else {
        dma_channel_int_enable((DMA_CONTROLLER)controller, (DMA_CHANNEL_ID)ch,
                               DMA_INT_RX_CURR_DECR);
    }

    // Save work mode
    iot_dma_ch_info[ch].work_mode = type;

    return RET_OK;
}

static IOT_DMA_CHANNEL_ID iot_dma_select_soft_channel(void)
{
    uint8_t ret;

    if (iot_dma_mem_ch_soft != IOT_DMA_CHANNEL_NONE) {
        return iot_dma_mem_ch_soft;
    }

    // regular flow
    ret = iot_dma_claim_channel(&iot_dma_mem_ch_soft);
    if (ret != RET_OK) {
        return IOT_DMA_CHANNEL_NONE;
    }

    iot_dma_mem_config(iot_dma_mem_ch_soft);

    return iot_dma_mem_ch_soft;
}

int8_t iot_dma_memcpy(void *dst, void *src, uint32_t length,
                      dma_mem_mem_done_callback cb, void *param)
{
    IOT_DMA_CONTROLLER controller = iot_dma_get_controller();
    IOT_DMA_CHANNEL_ID ch;
    iot_dma_trans_info_t *trans;
    uint32_t mask;
    int32_t core;

    if (!cb) {
        DBGLOG_DRIVER_ERROR("dma_memcpy cb can't be NULL\n");
        assert(0);
    }

    ch = iot_dma_select_soft_channel();

    trans = iot_dma_alloc_trans();

    // No idle descriptor
    if (trans == NULL) {
        return RET_BUSY;
    }

    // Fill the descriptor as a single transfer
    iot_dma_fill_desc(&trans->rx_desc, (uint32_t)src, length, DMA_OWNER_DMA,
                      false, true, true);
    iot_dma_fill_desc(&trans->tx_desc, (uint32_t)dst, length, DMA_OWNER_DMA,
                      true, true, true);
    trans->cb = cb;
    trans->param = param;

    // judge whether need to be triggered by wic
    core = judge_domain_by_addr2((uint32_t)src, (uint32_t)dst);
    if (core != WIC_SELF) {
        if (iot_wic_query((IOT_WIC_CORE)core, true)) {
            iot_wic_poll((IOT_WIC_CORE)core);
        }
    }

    mask = cpu_disable_irq();
    // Add descriptor to dma channel
    if (iot_dma_ch_info[ch].trans_tail == NULL) {
        iot_dma_ch_info[ch].trans_tail = trans;
        iot_dma_ch_info[ch].trans_head = trans;
        dma_set_channel_rx_descriptor((DMA_CONTROLLER)controller, (DMA_CHANNEL_ID)ch,
                                      &trans->rx_desc);
        dma_set_channel_tx_descriptor((DMA_CONTROLLER)controller, (DMA_CHANNEL_ID)ch,
                                      &trans->tx_desc);
    } else {
        iot_dma_ch_info[ch].trans_tail->tx_desc.next = &trans->tx_desc;
        iot_dma_ch_info[ch].trans_tail->rx_desc.next = &trans->rx_desc;
        iot_dma_ch_info[ch].trans_tail->next = trans;
        iot_dma_ch_info[ch].trans_tail = trans;
    }

    // stop sleep
    iot_dma_ch_active_vect |= BIT(ch);
#if defined(LOW_POWER_ENABLE)
    power_mgnt_inc_module_refcnt(POWER_SLEEP_DMA);
    iot_dma_ch_active_count[ch]++;
#endif

    // Start channel
    dma_channel_start((DMA_CONTROLLER)controller, (DMA_CHANNEL_ID)ch);

    cpu_restore_irq(mask);

    return RET_OK;
}
static void iot_dma_mem_to_peri_cb(void *buf, uint32_t length) IRAM_TEXT(iot_dma_mem_to_peri_cb);
static void iot_dma_mem_to_peri_cb(void *buf, uint32_t length)
{
    UNUSED(buf);
    UNUSED(length);
}

uint8_t iot_dma_mem_to_peri(IOT_DMA_CHANNEL_ID ch, void *dst, const void *src,
                            uint32_t length, dma_mem_peri_done_callback cb)
{
    IOT_DMA_CONTROLLER controller = iot_dma_get_controller();
    iot_dma_trans_info_t *trans;
    uint32_t mask;

    if (!cb) {
        DBGLOG_DRIVER_ERROR("dma_mem2peri cb can't be NULL\n");
        assert(0);
    }

    mask = cpu_disable_irq();

    if (dma_is_channel_idle((DMA_CONTROLLER)controller, (DMA_CHANNEL_ID)ch)) {
        cpu_restore_irq(mask);
        return RET_NOT_READY;
    }

    trans = iot_dma_alloc_trans();

    // No idle descriptor
    if (trans == NULL) {
        cpu_restore_irq(mask);
        return RET_BUSY;
    }

    // Fill the descriptor
    iot_dma_fill_desc(&trans->rx_desc, (uint32_t)src, length, DMA_OWNER_DMA,
                      true, false, false);
    trans->cb = cb;
    trans->ts_us = iot_timer_get_time();

#if defined(LOW_POWER_ENABLE)
    // TODO : quick fix here
    int32_t core = judge_domain_by_addr((uint32_t)src);
    assert(core == WIC_DTOP_CORE);
#endif

    // Add descriptor to dma channel
    if (iot_dma_ch_info[ch].trans_tail == NULL) {
        iot_dma_ch_info[ch].trans_tail = trans;
        iot_dma_ch_info[ch].trans_head = trans;
        dma_set_channel_rx_descriptor((DMA_CONTROLLER)controller, (DMA_CHANNEL_ID)ch,
                                      &trans->rx_desc);
        dma_set_channel_tx_descriptor((DMA_CONTROLLER)controller, (DMA_CHANNEL_ID)ch, dst);
    } else {
        iot_dma_ch_info[ch].trans_tail->rx_desc.next = &trans->rx_desc;
        iot_dma_ch_info[ch].trans_tail->next = trans;
        iot_dma_ch_info[ch].trans_tail = trans;
    }

    // stop sleep
    iot_dma_ch_active_vect |= BIT(ch);
#if defined(LOW_POWER_ENABLE)
    power_mgnt_inc_module_refcnt(POWER_SLEEP_DMA);
    iot_dma_ch_active_count[ch]++;
#endif

    // Start channel
    dma_channel_start((DMA_CONTROLLER)controller, (DMA_CHANNEL_ID)ch);

    cpu_restore_irq(mask);

    return RET_OK;
}

uint8_t iot_dma_mem_to_peri_with_tx_ack(IOT_DMA_CHANNEL_ID ch, void *dst, const void *src,
                            uint32_t length, dma_mem_peri_tx_ack_callback cb)
{
    IOT_DMA_CONTROLLER controller = iot_dma_get_controller();
    if (!cb) {
        DBGLOG_DRIVER_ERROR("dma_mem2peri_with_ack cb can't be NULL\n");
        assert(0);
    }

    iot_dma_ch_tx_ack_cb[ch] = cb;

    // if callback is not NULL, then enable TX ACK Interrupt
    dma_channel_int_enable((DMA_CONTROLLER)controller, (DMA_CHANNEL_ID)ch, DMA_INT_TX_ACK);
    return iot_dma_mem_to_peri(ch, dst, src, length, iot_dma_mem_to_peri_cb);
}

uint8_t iot_dma_ch_flush(IOT_DMA_CHANNEL_ID ch)
{
    assert(ch < IOT_DMA_CHANNEL_MAX);

    /* currently we still use busy wait for the flush */
    iot_dma_trans_info_t *trans;
    uint32_t work_mode = iot_dma_ch_info[ch].work_mode;
    dma_descriptor_t *curr_desc;
    iot_dma_trans_info_t *head = iot_dma_ch_info[ch].trans_head;

    if (work_mode == IOT_DMA_TRANS_PERI_TO_PERI) {
        /* do nothing */
        return RET_OK;
    }

    cpu_critical_enter();
    /*
     * force buf sz to 4 to speed up the flush
     */
    for (trans = head;
            trans != NULL;
            trans = trans->next) {
        if (work_mode == IOT_DMA_TRANS_PERI_TO_MEM) {
            curr_desc = &trans->tx_desc;
        } else if (work_mode == IOT_DMA_TRANS_MEM_TO_PERI) {
            curr_desc = &trans->rx_desc;
        } else {
            curr_desc = &trans->tx_desc;
        }
        if (curr_desc->owner == DMA_OWNER_DMA) {
            /*
             * < little endian >
             * because buf_size is 24 bit, so we
             * use the byte pointer to write or else
             * may write the whole 32 bit,which may
             * result in owner bit overwrite!
            */
            if (curr_desc->buf_size > MAX_UINT16) {
                // Attention:
                //   Because of 8 highest-significant bits ignored,
                //   so it isn't compatible with data len over than 64kB.
                assert(0);
                continue;   //lint !e527 assert might be ignored
            }
            uint16_t *p_halfword = (uint16_t *)curr_desc;   //lint !e740 RISCV compatible with half-word operations
            p_halfword[0] = 4;
        }
    }
    cpu_critical_exit();

    /*
     * wait done
     */
    uint32_t i = 0;
    uint32_t last_ts = 0;
    IOT_DMA_CONTROLLER controller = iot_dma_get_controller();
    for (trans = iot_dma_ch_info[ch].trans_head;
            trans != NULL;
            trans = iot_dma_ch_info[ch].trans_head) {

        if (work_mode == IOT_DMA_TRANS_PERI_TO_MEM) {
            curr_desc = &trans->tx_desc;
        } else if (work_mode == IOT_DMA_TRANS_MEM_TO_PERI) {
            curr_desc = &trans->rx_desc;
        } else {
            curr_desc = &trans->tx_desc;
        }

        if (i == 0) {
            last_ts = trans->ts_us;
        }
        if (++i > 0) {
            if (last_ts != trans->ts_us) {
                DBGLOG_DRIVER_INFO("ch=%u,cb=0x%x,own=%u,dst=0x%x,ts=%u\n", ch,
                    trans->cb, curr_desc->owner, curr_desc->buf_addr,
                    trans->ts_us);
                last_ts = trans->ts_us;
            }
            if (i > 100) {
                assert(0);
            }
            /* TODO: quick fix here, better to have a async event to
             * handle this situation - called by iot_dma_free_channel()
             */
            iot_timer_delay_us(1000);
        }

        cpu_critical_enter();
        if (trans == iot_dma_ch_info[ch].trans_tail
                    && curr_desc->owner == DMA_OWNER_CPU
#if defined(LOW_POWER_ENABLE)
                    /* need all trans INT done */
                    && iot_dma_ch_active_count[ch] == 0
#endif
        ) {
            /*
             * the last desc and owner = cpu means done
             */
            /* free the last dummy trans */
            iot_dma_free_trans(iot_dma_ch_info[ch].trans_head);
            iot_dma_ch_info[ch].trans_tail = NULL;
            iot_dma_ch_info[ch].trans_head = NULL;
            cpu_critical_exit();
            break;
        } else {
            if (curr_desc->owner == DMA_OWNER_CPU) {
                /*
                 * if have desc can reap, then do it
                 * now, this can WAR the HW miss IRQ issue
                 */
                iot_dma_ch_curr_desc_irq(controller, ch);
            }
        }
        cpu_critical_exit();
    }

    return RET_OK;
}

uint8_t iot_dma_chs_flush(uint32_t ch_bmp)
{
    IOT_DMA_CHANNEL_ID ch;
    iot_dma_trans_info_t *trans;
    uint32_t work_mode;
    dma_descriptor_t *curr_desc;
    iot_dma_trans_info_t *head;

    cpu_critical_enter();
    for (ch = IOT_DMA_CHANNEL_0; ch < IOT_DMA_CHANNEL_MAX; ch++) {
        if ((BIT(ch) & ch_bmp) == 0) {
            continue;
        }

        work_mode = iot_dma_ch_info[ch].work_mode;
        if (work_mode == IOT_DMA_TRANS_PERI_TO_PERI) {
            continue;
        }

        /*
         * force buf sz to 4 to speed up the flush
         */
        head = iot_dma_ch_info[ch].trans_head;
        for (trans = head;
                trans != NULL;
                trans = trans->next) {
            if (work_mode == IOT_DMA_TRANS_PERI_TO_MEM) {
                curr_desc = &trans->tx_desc;
            } else if (work_mode == IOT_DMA_TRANS_MEM_TO_PERI) {
                curr_desc = &trans->rx_desc;
            } else {
                curr_desc = &trans->tx_desc;
            }
            if (curr_desc->owner == DMA_OWNER_DMA) {
                /*
                 * < little endian >
                 * because buf_size is 24 bit, so we
                 * use the byte pointer to write or else
                 * may write the whole 32 bit,which may
                 * result in owner bit overwrite!
                */
                if (curr_desc->buf_size > MAX_UINT16) {
                    // Attention:
                    //   Because of 8 highest-significant bits ignored,
                    //   so it isn't compatible with data len over than 64kB.
                    assert(0);
                    continue;   //lint !e527 assert might be ignored
                }
                uint16_t *p_halfword = (uint16_t *)curr_desc;   //lint !e740 RISCV compatible with half-word operations
                p_halfword[0] = 4;
            }
        }
    }
    cpu_critical_exit();

    /*
     * wait done
     */
    uint32_t i;
    uint32_t last_ts;
    IOT_DMA_CONTROLLER controller = iot_dma_get_controller();
    for (ch = IOT_DMA_CHANNEL_0; ch < IOT_DMA_CHANNEL_MAX; ch++) {
        if ((BIT(ch) & ch_bmp) == 0) {
            continue;
        }

        work_mode = iot_dma_ch_info[ch].work_mode;
        if (work_mode == IOT_DMA_TRANS_PERI_TO_PERI) {
            continue;
        }

        i = 0;
        last_ts = 0;
        for (trans = iot_dma_ch_info[ch].trans_head;
                trans != NULL;
                trans = iot_dma_ch_info[ch].trans_head) {

            if (work_mode == IOT_DMA_TRANS_PERI_TO_MEM) {
                curr_desc = &trans->tx_desc;
            } else if (work_mode == IOT_DMA_TRANS_MEM_TO_PERI) {
                curr_desc = &trans->rx_desc;
            } else {
                curr_desc = &trans->tx_desc;
            }

            if (i == 0) {
                last_ts = trans->ts_us;
            }
            if (++i > 0) {
                if (last_ts != trans->ts_us) {
                    DBGLOG_DRIVER_INFO("ch=%u,cb=0x%x,own=%u,dst=0x%x,ts=%u\n", ch,
                        trans->cb, curr_desc->owner, curr_desc->buf_addr,
                        trans->ts_us);
                    last_ts = trans->ts_us;
                }
                if (i > 100) {
                    assert(0);
                }
                /* TODO: quick fix here, better to have a async event to
                 * handle this situation - called by iot_dma_free_channel()
                 */
                iot_timer_delay_us(1000);
            }

            cpu_critical_enter();
            if (trans == iot_dma_ch_info[ch].trans_tail
                    && curr_desc->owner == DMA_OWNER_CPU
#if defined(LOW_POWER_ENABLE)
                    /* need all trans INT done */
                    && iot_dma_ch_active_count[ch] == 0
#endif
            ) {
                /*
                 * the last desc and owner = cpu means done
                 */
                /* free the last dummy trans */
                iot_dma_free_trans(iot_dma_ch_info[ch].trans_head);
                iot_dma_ch_info[ch].trans_tail = NULL;
                iot_dma_ch_info[ch].trans_head = NULL;
                cpu_critical_exit();
                break;
            } else {
                if (curr_desc->owner == DMA_OWNER_CPU) {
                    /*
                     * if have desc can reap, then do it
                     * now, this can WAR the HW miss IRQ issue
                     */
                    iot_dma_ch_curr_desc_irq(controller, ch);
                }
            }
            cpu_critical_exit();
        }

    }

    return RET_OK;
}

uint8_t iot_dma_peri_to_mem(IOT_DMA_CHANNEL_ID ch, void *dst, void *src,
                            uint32_t length, dma_peri_mem_done_callback cb)
{
    assert(ch < IOT_DMA_CHANNEL_MAX);
    IOT_DMA_CONTROLLER controller = iot_dma_get_controller();
    iot_dma_trans_info_t *trans;
    uint32_t mask;

    if (!cb) {
        DBGLOG_DRIVER_ERROR("dma_peri2mem cb can't be NULL\n");
        assert(0);
    }

    mask = cpu_disable_irq();
    if (dma_is_channel_idle((DMA_CONTROLLER)controller, (DMA_CHANNEL_ID)ch)) {
        cpu_restore_irq(mask);
        return RET_NOT_READY;
    }

    trans = iot_dma_alloc_trans();

    // No idle descriptor
    if (trans == NULL) {
        cpu_restore_irq(mask);
        return RET_BUSY;
    }

    // Fill the descriptor
    iot_dma_fill_desc(&trans->tx_desc, (uint32_t)dst, length, DMA_OWNER_DMA,
                      true, false, false);
    trans->cb = cb;
    trans->ts_us = iot_timer_get_time();

#if defined(LOW_POWER_ENABLE)
    // TODO : quick fix here
    int32_t core = judge_domain_by_addr((uint32_t)dst);
    assert(core == WIC_DTOP_CORE);
#endif

    // Add descriptor to dma channel
    if (iot_dma_ch_info[ch].trans_tail == NULL) {
        iot_dma_ch_info[ch].trans_tail = trans;
        iot_dma_ch_info[ch].trans_head = trans;
        dma_set_channel_rx_descriptor((DMA_CONTROLLER)controller, (DMA_CHANNEL_ID)ch, src);
        dma_set_channel_tx_descriptor((DMA_CONTROLLER)controller, (DMA_CHANNEL_ID)ch,
                                      &trans->tx_desc);
    } else {
        iot_dma_ch_info[ch].trans_tail->tx_desc.next = &trans->tx_desc;
        iot_dma_ch_info[ch].trans_tail->next = trans;
        iot_dma_ch_info[ch].trans_tail = trans;
    }

    // stop sleep
    iot_dma_ch_active_vect |= BIT(ch);
#if defined(LOW_POWER_ENABLE)
    power_mgnt_inc_module_refcnt(POWER_SLEEP_DMA);
    iot_dma_ch_active_count[ch]++;
#endif

    // Start channel
    dma_channel_start((DMA_CONTROLLER)controller, (DMA_CHANNEL_ID)ch);

    cpu_restore_irq(mask);

    return RET_OK;
}

/**
 * Every new group must have tx desc, so that next transfer can add desc as next
 *   ________________________________________________
 *  |     trans_tail      |     rx     |     tx     |
 *  |---------|-----------|------|-----|------|-----|
 *  |   new trans 0       |   new rx   |   new tx   |
 *  |         |           |      |     |      |     |
 *  |   new trans 1       |   new rx   |   new tx   |
 *  |         |           |      |     |      |     |
 *  |   new trans 2       |     ...    |      |     |
 *  |         |           |      |     |      |     |
 *  |   new trans end     | new rx end | new tx end |
 *  |-----------------------------------------------|
 */
uint32_t iot_dma_memcpy_group(const iot_dma_buf_entry_t src_buf_list[],
                              const iot_dma_buf_entry_t dst_buf_list[],
                              dma_mem_mem_done_callback cb, void *param)
{
    IOT_DMA_CONTROLLER controller = iot_dma_get_controller();
    IOT_DMA_CHANNEL_ID ch;
    iot_dma_trans_info_t *curr_head = NULL;
    iot_dma_trans_info_t *trans = NULL;
    dma_descriptor_t *rx_desc_head = NULL;
    dma_descriptor_t *tx_desc_head = NULL;
    dma_descriptor_t *rx_desc = NULL;
    dma_descriptor_t *tx_desc = NULL;
    bool_t is_rx_last = false;
    bool_t is_tx_last = false;
    uint8_t src_index = 0;
    uint8_t dst_index = 0;
    uint8_t ret = RET_OK;
    uint32_t total_len = 0;
    int32_t core;
    int32_t i;
    uint32_t len_src = 0;
    uint32_t len_dst = 0;
    if (!cb) {
        DBGLOG_DRIVER_ERROR("dma_memcpy_group cb can't be NULL\n");
        assert(0);
    }

    for (i = 0; src_buf_list[i].addr; i++) {
        len_src += src_buf_list[i].len;
    }
    for (i = 0; dst_buf_list[i].addr; i++) {
        len_dst += dst_buf_list[i].len;
    }
    if (len_src > len_dst) {
        assert(0);
    }

    ch = iot_dma_select_soft_channel();

    // make a group of descriptor
    while (src_buf_list[src_index].addr != NULL
           || dst_buf_list[dst_index].addr != NULL) {
        if (src_index == 0 && dst_index == 0) {
            trans = iot_dma_alloc_trans();
            curr_head = trans;
        } else {
            if (trans) {
                trans->next = iot_dma_alloc_trans();
                trans = trans->next;
            } else {
                assert(trans);
            }
        }

        // No idle descriptor
        if (trans == NULL) {
            ret = RET_NOMEM;
            break;
        }
        trans->ts_us = iot_timer_get_time();

        is_rx_last = src_buf_list[src_index + 1].addr == NULL;
        is_tx_last = dst_buf_list[dst_index + 1].addr == NULL;

        if (src_buf_list[src_index].addr) {
            bool_t is_first = src_index == 0;

            // Not the last or src and dst are all the last
            if (!is_rx_last || is_tx_last) {
                // Fill the descriptor
                iot_dma_fill_desc(&trans->rx_desc,
                                  (uint32_t)src_buf_list[src_index].addr,
                                  src_buf_list[src_index].len, DMA_OWNER_DMA,
                                  false, is_first, is_rx_last);
                if (rx_desc == NULL) {
                    rx_desc_head = &trans->rx_desc;
                    rx_desc = &trans->rx_desc;
                } else {
                    rx_desc->next = &trans->rx_desc;
                    rx_desc = rx_desc->next;
                }
                src_index++;
            }
        } else {
            // Set owner to cpu so it can be free
            trans->rx_desc.owner = DMA_OWNER_CPU;
        }

        if (dst_buf_list[dst_index].addr) {
            bool_t is_first = dst_index == 0;

            // Not the last or src and dst are all the last
            if (!is_tx_last || is_rx_last) {
                // Fill the descriptor
                iot_dma_fill_desc(&trans->tx_desc,
                                  (uint32_t)dst_buf_list[dst_index].addr,
                                  dst_buf_list[dst_index].len, DMA_OWNER_DMA,
                                  is_tx_last, is_first, is_tx_last);
                if (tx_desc == NULL) {
                    tx_desc_head = &trans->tx_desc;
                    tx_desc = &trans->tx_desc;
                } else {
                    tx_desc->next = &trans->tx_desc;
                    tx_desc = tx_desc->next;
                }

                total_len += dst_buf_list[dst_index].len;
                if (is_tx_last) {
                    trans->info.src = src_buf_list[0].addr;
                    trans->info.dst = dst_buf_list[0].addr;
                    trans->info.total_len = total_len;
                }

                trans->cb = is_tx_last ? cb : NULL;
                trans->param = is_tx_last ? param : NULL;
                dst_index++;
            }
        } else {
            // Set owner to cpu so it can be free
            trans->tx_desc.owner = DMA_OWNER_CPU;
        }
    }

    if (curr_head == NULL) {
        return RET_INVAL;
    }

    // judge whether need to be triggered by wic
    core = judge_domain_by_addr2((uint32_t)src_buf_list[0].addr,
                                 (uint32_t)dst_buf_list[0].addr);
    if (core != WIC_SELF) {
        if (iot_wic_query((IOT_WIC_CORE)core, true)) {
            iot_wic_poll((IOT_WIC_CORE)core);
        }
    }

    uint32_t mask = cpu_disable_irq();
    // Add descriptor to dma channel
    if (iot_dma_ch_info[ch].trans_tail == NULL) {
        iot_dma_ch_info[ch].trans_tail = trans;
        iot_dma_ch_info[ch].trans_head = curr_head;
        dma_set_channel_rx_descriptor((DMA_CONTROLLER)controller, (DMA_CHANNEL_ID)ch, rx_desc_head);
        dma_set_channel_tx_descriptor((DMA_CONTROLLER)controller, (DMA_CHANNEL_ID)ch, tx_desc_head);
    } else {
        iot_dma_ch_info[ch].trans_tail->rx_desc.next = rx_desc_head;
        iot_dma_ch_info[ch].trans_tail->tx_desc.next = tx_desc_head;
        iot_dma_ch_info[ch].trans_tail->next = curr_head;
        iot_dma_ch_info[ch].trans_tail = trans;
    }

    // stop sleep
    iot_dma_ch_active_vect |= BIT(ch);
#if defined(LOW_POWER_ENABLE)
    power_mgnt_inc_module_refcnt(POWER_SLEEP_DMA);
    iot_dma_ch_active_count[ch]++;
#endif

    // Start channel
    dma_channel_start((DMA_CONTROLLER)controller, (DMA_CHANNEL_ID)ch);

    cpu_restore_irq(mask);

    return ret;
}

static bool_t iot_dma_ch_curr_desc_irq(IOT_DMA_CONTROLLER dma, IOT_DMA_CHANNEL_ID ch) IRAM_TEXT(iot_dma_ch_curr_desc_irq);
static bool_t iot_dma_ch_curr_desc_irq(IOT_DMA_CONTROLLER dma, IOT_DMA_CHANNEL_ID ch)
{
    iot_dma_trans_info_t *head = iot_dma_ch_info[ch].trans_head;
    iot_dma_trans_info_t *tail = iot_dma_ch_info[ch].trans_tail;

    assert(head && tail);
    void *callback = head->cb;
    void *param = head->param;
    IOT_DMA_TRANS_TYPE work_mode = iot_dma_ch_info[ch].work_mode;
    dma_descriptor_t *curr_desc;
    int32_t core;
    bool_t is_last_desc = false;

    UNUSED(dma);

    if (work_mode == IOT_DMA_TRANS_PERI_TO_PERI) {
        if (callback != NULL) {
            ((dma_peri_peri_done_callback)callback)();  //lint !e611 :suspicious cast
        }
        return is_last_desc;
    } else if (work_mode == IOT_DMA_TRANS_PERI_TO_MEM) {
        curr_desc = &head->tx_desc;
    } else if (work_mode == IOT_DMA_TRANS_MEM_TO_PERI) {
        curr_desc = &head->rx_desc;
    } else {
        curr_desc = &head->tx_desc;
    }

    while (curr_desc->owner == DMA_OWNER_CPU) {
        if (callback != NULL) {
            if (work_mode == IOT_DMA_TRANS_MEM_TO_MEM) {
                if (!head->info.total_len) {
                    ((dma_mem_mem_done_callback)callback)(   //lint !e611 :suspicious cast
                        (void *)head->tx_desc.buf_addr, (void *)head->rx_desc.buf_addr,
                        head->tx_desc.buf_size, param);
                    core = judge_domain_by_addr2(head->rx_desc.buf_addr,
                                                    head->tx_desc.buf_addr);
                } else {
                    ((dma_mem_mem_done_callback)callback)(   //lint !e611 :suspicious cast
                        head->info.dst, head->info.src, head->info.total_len, param);
                    core = judge_domain_by_addr2((uint32_t)head->info.src,
                                                    (uint32_t)head->info.dst);
                }

                if (core != WIC_SELF) {
                    iot_wic_finish((IOT_WIC_CORE)core);
                }
            } else {
                ((dma_peri_mem_done_callback)callback)(   //lint !e611 :suspicious cast
                    (void *)curr_desc->buf_addr, curr_desc->buf_size);
            }

            head->cb = NULL;
#if defined(LOW_POWER_ENABLE)
            // TODO : fix me for no-callback dma trans
            power_mgnt_dec_module_refcnt(POWER_SLEEP_DMA);
            iot_dma_ch_active_count[ch]--;
#endif
        }

        if (head->tx_desc.end) {
            head->tx_desc.end = 0;
        }

        if (head->next == NULL || head == tail) {
            // All transfer done.
            // just quit, left a dummy descriptor
            is_last_desc = true;

            iot_dma_ch_active_vect &= ~BIT(ch);
            if (!iot_dma_ch_active_vect) {
#if defined(LOW_POWER_ENABLE)
                uint32_t count = power_mgnt_get_module_refcnt(POWER_SLEEP_DMA);
                if (count) {
                    ASSERT_FAILED_DUMP(iot_dma_ch_active_count, ARRAY_SIZE(iot_dma_ch_active_count));
                }

                // if all channel done, dma vote to sleep
                power_mgnt_clr_module_refcnt(POWER_SLEEP_DMA);
#endif
            }
            break;
        } else {
            // Move to next
            iot_dma_ch_info[ch].trans_head = head->next;
            iot_dma_free_trans(head);
            head = iot_dma_ch_info[ch].trans_head;
            callback = head->cb;
            param = head->param;
            if (work_mode == IOT_DMA_TRANS_PERI_TO_MEM) {
                curr_desc = &head->tx_desc;
            } else if (work_mode == IOT_DMA_TRANS_MEM_TO_PERI) {
                curr_desc = &head->rx_desc;
            } else {
                curr_desc = &head->tx_desc;
            }
        }
    }

    return is_last_desc;
}

static IOT_DMA_TRANS_TYPE iot_dma_ch_get_workmode(IOT_DMA_CHANNEL_ID ch) IRAM_TEXT(iot_dma_ch_get_workmode);
static IOT_DMA_TRANS_TYPE iot_dma_ch_get_workmode(IOT_DMA_CHANNEL_ID ch)
{
    return iot_dma_ch_info[ch].work_mode;
}

static void iot_dma_ch_all_desc_irq(IOT_DMA_CONTROLLER dma, IOT_DMA_CHANNEL_ID ch) IRAM_TEXT(iot_dma_ch_all_desc_irq);
static void iot_dma_ch_all_desc_irq(IOT_DMA_CONTROLLER dma, IOT_DMA_CHANNEL_ID ch)
{
    UNUSED(dma);
    UNUSED(ch);
}

static void iot_dma_ack_irq(IOT_DMA_CONTROLLER dma, IOT_DMA_CHANNEL_ID ch) IRAM_TEXT(iot_dma_ack_irq);
static void iot_dma_ack_irq(IOT_DMA_CONTROLLER dma, IOT_DMA_CHANNEL_ID ch)
{
    IOT_DMA_TRANS_TYPE type = iot_dma_ch_info[ch].work_mode;
    dma_mem_peri_tx_ack_callback cb;

    UNUSED(dma);

    // Dsiable channel interrupt
    if (type == IOT_DMA_TRANS_MEM_TO_PERI) {
        cb = iot_dma_ch_tx_ack_cb[ch];
        iot_dma_ch_tx_ack_cb[ch] = NULL;
        dma_channel_int_disable((DMA_CONTROLLER)dma, (DMA_CHANNEL_ID)ch, DMA_INT_TX_ACK);
    } else {
        dma_channel_int_disable((DMA_CONTROLLER)dma, (DMA_CHANNEL_ID)ch, DMA_INT_RX_ACK);
        //not support yet
        cb = NULL;
    }

    if (cb) {
        cb(ch);
    }
}

static uint32_t iot_dma_isr_handler(uint32_t vector, uint32_t data) IRAM_TEXT(iot_dma_isr_handler);
static uint32_t iot_dma_isr_handler(uint32_t vector, uint32_t data)
{
    IOT_DMA_CONTROLLER controller = (IOT_DMA_CONTROLLER)data;
    uint32_t int_st;
    bool_t is_last_desc = false;
    UNUSED(vector);

    for (uint8_t i = 0; i < IOT_DMA_CHANNEL_MAX; i++) {
        int_st = dma_get_ch_int_status((DMA_CONTROLLER)controller, (DMA_CHANNEL_ID)i);
        if (!int_st)
            continue;

        if (int_st & BIT(DMA_INT_TX_CURR_DECR)) {
            dma_channel_int_clear((DMA_CONTROLLER)controller, (DMA_CHANNEL_ID)i,
                                  DMA_INT_TX_CURR_DECR);
            iot_dma_ch_curr_desc_irq(controller, (IOT_DMA_CHANNEL_ID)i);
            int_st &= ~BIT(DMA_INT_TX_CURR_DECR);
        }

        if (int_st & BIT(DMA_INT_RX_CURR_DECR)) {
            if (iot_dma_ch_get_workmode((IOT_DMA_CHANNEL_ID)i) != IOT_DMA_TRANS_MEM_TO_PERI) {
                iot_dma_ch_curr_desc_irq(controller, (IOT_DMA_CHANNEL_ID)i);
            } else {
                is_last_desc = iot_dma_ch_curr_desc_irq(controller, (IOT_DMA_CHANNEL_ID)i);
                if (is_last_desc && iot_dma_ch_tx_ack_cb[i] != NULL) {
                    dma_channel_int_disable((DMA_CONTROLLER)controller, (DMA_CHANNEL_ID)i,
                                            DMA_INT_RX_CURR_DECR);
                    int_st &= ~BIT(DMA_INT_RX_CURR_DECR);
                } else {
                    dma_channel_int_clear((DMA_CONTROLLER)controller, (DMA_CHANNEL_ID)i,
                                          DMA_INT_RX_CURR_DECR);
                    /* call curr desc handler again to avoid interrupt missing.
                     * While next interrupt happen between previous curr desc handler and
                     * clear interrupt here */
                    iot_dma_ch_curr_desc_irq(controller, (IOT_DMA_CHANNEL_ID)i);
                    /* There's a HW limit that if the last desc interrupt is clear, then
                     * the tx ack interrupt won't happen. So tx ack interrupt user should
                     * user dma desc one by one until tx ack happen.
                     * Now only uniqline uart use tx ack and there's always one desc in
                     * list until tx ack happen. So assert tx ack callback is NULL here.*/
                    assert(iot_dma_ch_tx_ack_cb[i] == NULL);
                }
                int_st &= ~BIT(DMA_INT_RX_CURR_DECR);
            }
        }

        if (int_st & BIT(DMA_INT_TX_ALL_DECR)) {
            dma_channel_int_clear((DMA_CONTROLLER)controller, (DMA_CHANNEL_ID)i,
                                  DMA_INT_TX_ALL_DECR);
            iot_dma_ch_all_desc_irq(controller, (IOT_DMA_CHANNEL_ID)i);
            int_st &= ~BIT(DMA_INT_TX_ALL_DECR);
        }

        if (iot_dma_ch_get_workmode((IOT_DMA_CHANNEL_ID)i) != IOT_DMA_TRANS_MEM_TO_PERI) {
            dma_channel_int_clear_all((DMA_CONTROLLER)controller, (DMA_CHANNEL_ID)i);
            /* call curr desc handler again to avoid interrupt missing.
             * While next interrupt happen between previous curr desc handler and
             * clear interrupt here */
            iot_dma_ch_curr_desc_irq(controller, (IOT_DMA_CHANNEL_ID)i);
        } else {
            if (int_st & BIT(DMA_INT_RX_ACK)) {
                dma_channel_int_clear((DMA_CONTROLLER)controller, (DMA_CHANNEL_ID)i,
                                      DMA_INT_RX_ACK);
                iot_dma_ack_irq(controller, (IOT_DMA_CHANNEL_ID)i);
                int_st &= ~BIT(DMA_INT_RX_ACK);
            }

            if (int_st & BIT(DMA_INT_TX_ACK)) {
                dma_channel_int_enable((DMA_CONTROLLER)controller, (DMA_CHANNEL_ID)i,
                                       DMA_INT_RX_CURR_DECR);
                dma_channel_int_clear((DMA_CONTROLLER)controller, (DMA_CHANNEL_ID)i,
                                      DMA_INT_RX_CURR_DECR);
                dma_channel_int_clear((DMA_CONTROLLER)controller, (DMA_CHANNEL_ID)i,
                                      DMA_INT_TX_ACK);
                iot_dma_ack_irq(controller, (IOT_DMA_CHANNEL_ID)i);
                int_st &= ~BIT(DMA_INT_TX_ACK);
            }

            dma_channel_int_clear_bits((DMA_CONTROLLER)controller, (DMA_CHANNEL_ID)i, int_st);
        }
    }

    return RET_OK;
}
