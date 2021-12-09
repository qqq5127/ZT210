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
#include "string.h"
#include "riscv_cpu.h"

#include "iot_ipc.h"
#include "intc.h"
#include "wic.h"

#include "iot_memory_config.h"
#include "iot_wic.h"
#include "iot_timer.h"

#include "driver_dbglog.h"

#ifdef LOW_POWER_ENABLE
#include "power_mgnt.h"
#endif

#ifdef CHECK_ISR_FLASH
#include "iot_soc.h"
#endif

#if !defined(NDEBUG) && !defined(RELEASE)
#include "iot_rtc.h"
#endif

enum {
    IPC_STATUS_IDLE,
    IPC_STATUS_READY,
    IPC_STATUS_PROCESSING,
};

/*lint -esym(754, iot_ipc_msg::reserved) */
typedef struct iot_ipc_msg {
    uint8_t status;
    uint8_t dst;
    uint8_t src;
    uint8_t need_ack : 1;
    uint8_t reserved : 7;
    uint16_t type;
    uint16_t len;
    uint8_t payload[];
} iot_ipc_msg_t;

#define IPC_SEND_TIMEOUT_US 1000000     // 1s

#ifdef IPC_MESSAGE_START

#define IPC_MSG_BASE IPC_MESSAGE_START
#define IPC_MSG_SIZE 0x100

#define IPC_DTOP_BT_MSG_ADDR  IPC_MSG_BASE
#define IPC_DTOP_AUD_MSG_ADDR (IPC_DTOP_BT_MSG_ADDR + IPC_MSG_SIZE)
#define IPC_BT_DTOP_MSG_ADDR  (IPC_DTOP_AUD_MSG_ADDR + IPC_MSG_SIZE)
#define IPC_BT_AUD_MSG_ADDR   (IPC_BT_DTOP_MSG_ADDR + IPC_MSG_SIZE)
#define IPC_BT2_DTOP_MSG_ADDR (IPC_BT_AUD_MSG_ADDR + IPC_MSG_SIZE)
#define IPC_BT2_AUD_MSG_ADDR  (IPC_BT2_DTOP_MSG_ADDR + IPC_MSG_SIZE)
#define IPC_AUD_DTOP_MSG_ADDR (IPC_BT2_AUD_MSG_ADDR + IPC_MSG_SIZE)
#define IPC_AUD_BT_MSG_ADDR   (IPC_AUD_DTOP_MSG_ADDR + IPC_MSG_SIZE)

static volatile iot_ipc_msg_t * const
    iot_ipc_msg_base[MAX_CORE][MAX_CORE] IRAM_RODATA(iot_ipc_msg_base) = {
        {
            (volatile iot_ipc_msg_t *)NULL,
            (volatile iot_ipc_msg_t *)IPC_DTOP_BT_MSG_ADDR,
            (volatile iot_ipc_msg_t *)NULL,
            (volatile iot_ipc_msg_t *)IPC_DTOP_AUD_MSG_ADDR,
        },

        {
            (volatile iot_ipc_msg_t *)IPC_BT_DTOP_MSG_ADDR,
            (volatile iot_ipc_msg_t *)NULL,
            (volatile iot_ipc_msg_t *)NULL,
            (volatile iot_ipc_msg_t *)IPC_BT_AUD_MSG_ADDR,
        },

        {
            (volatile iot_ipc_msg_t *)IPC_BT2_DTOP_MSG_ADDR,
            (volatile iot_ipc_msg_t *)NULL,
            (volatile iot_ipc_msg_t *)NULL,
            (volatile iot_ipc_msg_t *)IPC_BT2_AUD_MSG_ADDR,
        },

        {
            (volatile iot_ipc_msg_t *)IPC_AUD_DTOP_MSG_ADDR,
            (volatile iot_ipc_msg_t *)IPC_AUD_BT_MSG_ADDR,
            (volatile iot_ipc_msg_t *)NULL,
            (volatile iot_ipc_msg_t *)NULL,
        },
};

#if !defined(NDEBUG) && !defined(RELEASE)
typedef struct iot_ipc_msg_cnt {
    uint32_t send_cnt;
    uint32_t send_ts;
    uint32_t recv_cnt;
    uint32_t recv_ts;
} iot_ipc_msg_cnt_t;

static volatile iot_ipc_msg_cnt_t * const
    iot_ipc_msg_cnt_base[MAX_CORE][MAX_CORE] IRAM_RODATA(iot_ipc_msg_cnt_base) = {
        {
            (volatile iot_ipc_msg_cnt_t *)NULL,
            (volatile iot_ipc_msg_cnt_t *)(IPC_DTOP_BT_MSG_ADDR + IPC_MSG_SIZE
                                           - sizeof(iot_ipc_msg_cnt_t)),
            (volatile iot_ipc_msg_cnt_t *)NULL,
            (volatile iot_ipc_msg_cnt_t *)(IPC_DTOP_AUD_MSG_ADDR + IPC_MSG_SIZE
                                           - sizeof(iot_ipc_msg_cnt_t)),
        },

        {
            (volatile iot_ipc_msg_cnt_t *)(IPC_BT_DTOP_MSG_ADDR + IPC_MSG_SIZE
                                           - sizeof(iot_ipc_msg_cnt_t)),
            (volatile iot_ipc_msg_cnt_t *)NULL,
            (volatile iot_ipc_msg_cnt_t *)NULL,
            (volatile iot_ipc_msg_cnt_t *)(IPC_BT_AUD_MSG_ADDR + IPC_MSG_SIZE
                                           - sizeof(iot_ipc_msg_cnt_t)),
        },

        {
            (volatile iot_ipc_msg_cnt_t *)(IPC_BT2_DTOP_MSG_ADDR + IPC_MSG_SIZE
                                           - sizeof(iot_ipc_msg_cnt_t)),
            (volatile iot_ipc_msg_cnt_t *)NULL,
            (volatile iot_ipc_msg_cnt_t *)NULL,
            (volatile iot_ipc_msg_cnt_t *)(IPC_BT2_AUD_MSG_ADDR + IPC_MSG_SIZE
                                           - sizeof(iot_ipc_msg_cnt_t)),
        },

        {
            (volatile iot_ipc_msg_cnt_t *)(IPC_AUD_DTOP_MSG_ADDR + IPC_MSG_SIZE
                                           - sizeof(iot_ipc_msg_cnt_t)),
            (volatile iot_ipc_msg_cnt_t *)(IPC_AUD_BT_MSG_ADDR + IPC_MSG_SIZE
                                           - sizeof(iot_ipc_msg_cnt_t)),
            (volatile iot_ipc_msg_cnt_t *)NULL,
            (volatile iot_ipc_msg_cnt_t *)NULL,
        },
};
#endif
#else
static volatile iot_ipc_msg_t *iot_ipc_msg_base[MAX_CORE][MAX_CORE] = {0};
#endif

static const WIC_CORE core_to_domain[MAX_CORE] IRAM_RODATA(core_to_domain) = {
    WIC_DTOP_CORE,
    WIC_BT_CORE,
    WIC_BT_CORE,
    WIC_AUDIO_CORE,
};

static int ipc_freq_send[IPC_TYPE_MAX] = {0};
static int ipc_freq_recv[IPC_TYPE_MAX] = {0};

static iot_ipc_ack_cb_t ipc_ack_cbs[IPC_TYPE_MAX];

static iot_ipc_msg_handler ipc_handlers[IPC_TYPE_MAX];
static int32_t ipc_ack_handler(IPC_CORES src_cpu, const void *payload) IRAM_TEXT(ipc_ack_handler);
static int32_t ipc_ack_handler(IPC_CORES src_cpu, const void *payload)
{
    uint16_t type;
    iot_ipc_ack_cb_t cb;

#ifdef LOW_POWER_ENABLE
    power_mgnt_dec_module_refcnt(POWER_SLEEP_IPC);
#endif

    type = *(uint16_t *)payload;

    if (type >= IPC_TYPE_MAX) {
        DBGLOG_DRIVER_INFO("IPC: ack type err src %d,  %d/%d\n", src_cpu, type, IPC_TYPE_MAX);
        assert(0);
    }

    cb = ipc_ack_cbs[type];

    if (cb) {
        cb(src_cpu);
    }

    return RET_OK;
}

void iot_ipc_init(void)
{
#ifdef IPC_MESSAGE_START
    IPC_CORES src_core = (IPC_CORES)cpu_get_mhartid();
    volatile const iot_ipc_msg_t *msg;
#endif
    int i;

    /* clear all IPC handlers */
    for (i = 0; i < IPC_TYPE_MAX; i++) {
        ipc_handlers[i] = NULL;
    }
    iot_ipc_register_handler(IPC_TYPE_IPC_ACK, ipc_ack_handler);

#ifdef IPC_MESSAGE_START
    /* clear all IPC message */
    for (i = 0; i < MAX_CORE; i++) {
        msg = iot_ipc_msg_base[src_core][i];
        if (!msg) {
            continue;
        }

        memset((void *)msg, 0, sizeof(iot_ipc_msg_t));
    }

    /* enable interrupt */
    cpu_enable_software_irq();
#endif
}

void iot_ipc_deinit(void)
{
    cpu_disable_software_irq();
}

int32_t iot_ipc_register_handler(IPC_TYPE type, iot_ipc_msg_handler handler)
{
    if (ipc_handlers[type] == NULL) {
        ipc_handlers[type] = handler;
        return RET_OK;
    }
    return RET_EXIST;
}

int32_t iot_ipc_unregister_handler(IPC_TYPE type)
{
    if (ipc_handlers[type] != NULL) {
        ipc_handlers[type] = NULL;
        return RET_OK;
    }
    return RET_NOT_EXIST;
}

int32_t iot_ipc_register_ack_cb(IPC_TYPE type, iot_ipc_ack_cb_t cb)
{
    if (ipc_ack_cbs[type] == NULL) {
        ipc_ack_cbs[type] = cb;
        return RET_OK;
    }

    return RET_EXIST;
}

int32_t iot_ipc_unregister_ack_cb(IPC_TYPE type)
{
    if (ipc_ack_cbs[type] != NULL) {
        ipc_ack_cbs[type] = NULL;
        return RET_OK;
    }

    return RET_NOT_EXIST;
}

int32_t iot_ipc_send_message(IPC_CORES dst_core, IPC_TYPE type, const void *payload, uint16_t len,
                             bool_t keep_wakeup) IRAM_TEXT(iot_ipc_send_message);
int32_t iot_ipc_send_message(IPC_CORES dst_core, IPC_TYPE type, const void *payload, uint16_t len,
                             bool_t keep_wakeup)
{
    uint32_t retry_cnt = 0;
    uint32_t mask;
    IPC_CORES src_core;
    volatile iot_ipc_msg_t *msg;
#if defined(IPC_MESSAGE_START) && !defined(NDEBUG) && !defined(RELEASE)
    volatile iot_ipc_msg_cnt_t *msg_cnt;
#endif

    /* As far as now, only PM LOCK type use true of keep_wakeup, other type always input false.
     * add assert here to avoid other type input true value.
     */
    if (keep_wakeup) {
        assert(type == IPC_TYPE_PM_LOCK);
    }

    mask = cpu_disable_irq();

    src_core = (IPC_CORES)cpu_get_mhartid();

    msg = iot_ipc_msg_base[src_core][dst_core];
    if (!msg) {
        cpu_restore_irq(mask);
        return RET_INVAL;
    }

    uint32_t start = iot_timer_get_time();
    while (msg->status != IPC_STATUS_IDLE) {
        // handle irq & ticks
        cpu_restore_irq(mask);

        retry_cnt++;

        uint32_t current = iot_timer_get_time();
        if (current - start > IPC_SEND_TIMEOUT_US) {
            iot_ipc_info_dump();

            DBGLOG_DRIVER_ERROR("[auto]WARNING: IPC blocked:%u,status:%u,%u->%u len %d ack %d retry %d\n",
                                msg->type, msg->status, msg->src, msg->dst, msg->len, msg->need_ack,
                                retry_cnt);

            start = current;
        }

        mask = cpu_disable_irq();
    }

    msg->dst = dst_core;
    msg->src = src_core;
    msg->type = (uint16_t)type;
    msg->len = len;

    if (payload != NULL) {
        memcpy((void *)msg->payload, payload, len);
    }

    if (len && msg->type != IPC_TYPE_IPC_ACK) {
        msg->need_ack = 1;
    } else {
        msg->need_ack = 0;
    }

    msg->status = IPC_STATUS_READY;

    intc_send_software_int(dst_core);

#if defined(IPC_MESSAGE_START) && !defined(NDEBUG) && !defined(RELEASE)
    msg_cnt = iot_ipc_msg_cnt_base[src_core][dst_core];
    msg_cnt->send_cnt++;
    msg_cnt->send_ts = iot_rtc_get_time_ms();
#endif

    iot_wic_query((IOT_WIC_CORE)core_to_domain[dst_core], keep_wakeup);
    if (len && msg->type != IPC_TYPE_IPC_ACK) {
#ifdef LOW_POWER_ENABLE
        power_mgnt_inc_module_refcnt(POWER_SLEEP_IPC);
#endif
    }

    ipc_freq_send[msg->type]++;
    cpu_restore_irq(mask);
    return RET_OK;
}

int32_t iot_ipc_recv_message(IPC_CORES src_core) IRAM_TEXT(iot_ipc_recv_message);
int32_t iot_ipc_recv_message(IPC_CORES src_core)
{
    int32_t ret = RET_OK;
    IPC_CORES this_core = (IPC_CORES)cpu_get_mhartid();
    volatile iot_ipc_msg_t *msg;
    iot_ipc_msg_handler handler;
    uint8_t need_ack;
#if defined(IPC_MESSAGE_START) && !defined(NDEBUG) && !defined(RELEASE)
    volatile iot_ipc_msg_cnt_t *msg_cnt;
#endif

    msg = iot_ipc_msg_base[src_core][this_core];
    if (!msg)
        return RET_FAIL;

    if (msg->status != IPC_STATUS_READY) {
        return RET_FAIL;
    }

    msg->status = IPC_STATUS_PROCESSING;
    handler = ipc_handlers[msg->type];

    if (handler) {
        ret = handler(src_core, (void *)msg->payload);
    } else {
        assert(msg->type == IPC_TYPE_PM_LOCK);
    }

    ipc_freq_recv[msg->type]++;

    need_ack = msg->need_ack;
    msg->status = IPC_STATUS_IDLE;
#if defined(IPC_MESSAGE_START) && !defined(NDEBUG) && !defined(RELEASE)
    msg_cnt = iot_ipc_msg_cnt_base[src_core][this_core];
    msg_cnt->recv_cnt++;
    msg_cnt->recv_ts = iot_rtc_get_time_ms();
#endif

    if (need_ack) {
        uint16_t type = msg->type;

        while (iot_ipc_send_message(src_core, IPC_TYPE_IPC_ACK,
                    &type, sizeof(uint16_t), false) == RET_BUSY) {
        }
    }

    return ret;
}

void iot_ipc_finsh_keep_wakeup(IPC_CORES dst_core)
{
    iot_wic_finish((IOT_WIC_CORE)core_to_domain[dst_core]);
}

void machine_soft_interrupt_handler(void) IRAM_TEXT(machine_soft_interrupt_handler);
void machine_soft_interrupt_handler(void)
{
    uint32_t cpu = cpu_get_mhartid();
    uint32_t src, flag;

#ifdef CHECK_ISR_FLASH
    iot_soc_cpu_access_enable(IOT_SOC_CPU_ACCESS_DTOP_FLASH, false);
#endif

    while ((flag = intc_get_software_int_status(cpu)) != 0) {

        for (src = 0; src < MAX_CORE; src++) {
            if (flag & (1U << src)) {
                intc_clear_software_int(src);
                iot_ipc_recv_message((IPC_CORES)src);
            }
        }
    }

#ifdef CHECK_ISR_FLASH
    iot_soc_cpu_access_enable(IOT_SOC_CPU_ACCESS_DTOP_FLASH, true);
#endif
}

void iot_ipc_info_dump(void)
{
    uint32_t cpu = cpu_get_mhartid();
    volatile iot_ipc_msg_t *msg;

    DBGLOG_DRIVER_INFO("core %d soft irq status %x\n", cpu, intc_get_software_int_status(cpu));

    for (int src = 0; src < MAX_CORE; src++) {
        msg = iot_ipc_msg_base[src][cpu];
        if (!msg) {
            continue;
        }

        DBGLOG_DRIVER_INFO("recv: ipc from %d->%d status %d type %d len %d\n",
                            msg->src, msg->dst, msg->status, msg->type, msg->len);
    }

    for (int dst = 0; dst < MAX_CORE; dst++) {
        msg = iot_ipc_msg_base[cpu][dst];
        if (!msg) {
            continue;
        }

        DBGLOG_DRIVER_INFO("send: ipc from %d->%d status %d type %d len %d\n",
                            msg->src, msg->dst, msg->status, msg->type, msg->len);
    }
}

void iot_ipc_freq_detect(void)
{
    int sum, i;
    uint32_t cpu = cpu_get_mhartid();

    sum = 0;
    for (i = 0; i < IPC_TYPE_MAX; i++) {
        if (ipc_freq_send[i]) {
            DBGLOG_DRIVER_INFO("ipc %d: %d\n", i, ipc_freq_send[i]);
            sum += ipc_freq_send[i];
            ipc_freq_send[i] = 0;
        }
    }
    DBGLOG_DRIVER_INFO("IPC: CPU%d Number:%d Send Total: %d\n",
                        cpu, IPC_TYPE_MAX, sum);

    sum = 0;
    for (i = 0; i < IPC_TYPE_MAX; i++) {
        if (ipc_freq_recv[i]) {
            DBGLOG_DRIVER_INFO("ipc %d: %d\n", i, ipc_freq_recv[i]);
            sum += ipc_freq_recv[i];
            ipc_freq_recv[i] = 0;
        }
    }
    DBGLOG_DRIVER_INFO("IPC: CPU%d Number:%d Recv Total: %d\n",
                        cpu, IPC_TYPE_MAX, sum);
}
