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
#include "crc.h"
#include "lib_dbglog.h"
#include "generic_list.h"
#include "critical_sec.h"

#include "os_task.h"
#include "os_lock.h"
#include "os_timer.h"
#include "os_mem.h"

#include "iot_timer.h"

#include "generic_transmission_api.h"
#include "generic_transmission_profile.h"
#include "generic_transmission_io.h"
#include "generic_transmission_protocol.h"

/******** Config Start ********/
#define CONFIG_GENERIC_TRANSMISSION_PRF_DEBUG              0U
#define CONFIG_GENERIC_TRANSMISSION_PRF_TEST_CHECK         0U

#define CONFIG_GENERIC_TRANSMISSION_PRF_TX_TASK_PRIO       5U
#define CONFIG_GENERIC_TRANSMISSION_PRF_RX_TASK_PRIO       8U

#define CONFIG_GENERIC_TRANSMISSION_PRF_IO_TX_FAIL_ABANDON 0U
/******** Config End ********/

/* Log & Print configuration */
#if CONFIG_GENERIC_TRANSMISSION_PRF_DEBUG
#define GENERIC_TRANSMISSION_PRF_LOGD(fmt, arg...)   iot_printf(fmt, ##arg)
#define GENERIC_TRANSMISSION_PRF_LOGI(fmt, arg...)   iot_printf(fmt, ##arg)
#define GENERIC_TRANSMISSION_PRF_LOGE(fmt, arg...)   iot_printf(fmt, ##arg)
#else
#define GENERIC_TRANSMISSION_PRF_LOGD(fmt, arg...)
#define GENERIC_TRANSMISSION_PRF_LOGI(fmt, arg...)
#define GENERIC_TRANSMISSION_PRF_LOGE(fmt, arg...)
#endif

/* Transport layer configuration */
// RX MTU
#define GENERIC_TRANSMISSION_PRF_PKT_MAX_RX_MTU_SIZE       512UL
#define GENERIC_TRANSMISSION_PRF_PKT_MAX_RX_SIZE           (GENERIC_TRANSMISSION_PRF_PKT_MAX_RX_MTU_SIZE + GTP_PKT_PREAMBLE_LEN + GTP_PKT_HDR_LEN + GTP_PKT_CRC_LEN)
// TX MTU
#define GENERIC_TRANSMISSION_PRF_PKT_MIN_TX_MTU_SIZE       512U
#define GENERIC_TRANSMISSION_PRF_PKT_DFT_TX_MTU_SIZE       512U
#define GENERIC_TRANSMISSION_PRF_PKT_MAX_TX_MTU_SIZE       512U
#define GENERIC_TRANSMISSION_MS_TO_US(ms)                  ((uint32_t)((((ms) << 10) - ((ms) << 4)) - ((ms) << 3)))
#define GENERIC_TRANSMISSION_PRF_MIN_ACK_TIMEOUT_MS        32U
#define GENERIC_TRANSMISSION_PRF_MAX_ACK_TIMEOUT_MS        256U

/* Common configuration */
/* According to how long the PC reply packet delay, the maximum throughput need different free heap size.
 * This maximum 16K max threshold is an experience result of windows10 pyserial API with 1ms timeout.
 * Now, generic transmission use dynamic total packet size threshold, according to the free heap size.
 * If the total packet size reduce, it may cause throughput decrease.
 */
#define GENERIC_TRANSMISSION_PRF_TOTAL_DATA_PKT_SIZE_MAX_THRESHOLD          16384U     //16KB
#define GENERIC_TRANSMISSION_PRF_TOTAL_DATA_PKT_SIZE_MIN_THRESHOLD          1024U      //1KB
#define GENERIC_TRANSMISSION_PRF_FREE_HEAP_SIZE_MAINTAIN_THRESHOLD          4096U      //4KB
#define GENERIC_TRANSMISSION_PRF_TOTAL_CTRL_PKT_SIZE_MAX_THRESHOLD          512U       //512B
#define GENERIC_TRANSMISSION_PRF_TOTAL_CTRL_PKT_SIZE_MIN_THRESHOLD          128U       //128B

/* TX Priority configuration */
/* 1. retry list is always higheset priority.
 * 2. the list over up watermark is more prior, more over more prior
 * 3. the list not reach watermark, but more approach, more prior
 * 4. if the prio is the same, follow the order core2/1/0
 * 5. the list will handle until the num reach low watermark, then
 *    it will handler another list.
 * 6. until all the list handle over, task yield.
 */
#define GENERIC_TRANSMISSION_PRF_TX_DATA_RETRY_LIST_LOW_WATERMARK        0U
#define GENERIC_TRANSMISSION_PRF_TX_CTRL_RETRY_LIST_LOW_WATERMARK        0U

// TID Num definition
#define GENERIC_TRANSMISSION_PRF_DATA_TXQ_NUM               8U
#define GENERIC_TRANSMISSION_PRF_CTRL_TXQ_NUM               1U
#define GENERIC_TRANSMISSION_PRF_MGMT_TXQ_NUM               1U
#define GENERIC_TRANSMISSION_PRF_ALL_TXQ_NUM                (GENERIC_TRANSMISSION_PRF_DATA_TXQ_NUM +    \
                                                             GENERIC_TRANSMISSION_PRF_CTRL_TXQ_NUM +    \
                                                             GENERIC_TRANSMISSION_PRF_MGMT_TXQ_NUM)
// Assign to CTRL packet
#define GENERIC_TRANSMISSION_PRF_TXQ_DATA_MAX_IDX           (GENERIC_TRANSMISSION_PRF_DATA_TXQ_NUM - 1)
#define GENERIC_TRANSMISSION_PRF_TXQ_CTRL_IDX               (GENERIC_TRANSMISSION_PRF_TXQ_DATA_MAX_IDX + 1)
#define GENERIC_TRANSMISSION_PRF_TXQ_MGMT_IDX               (GENERIC_TRANSMISSION_PRF_TXQ_CTRL_IDX + 1)

// RX config
#define GENERIC_TRANSMISSION_PRF_RX_CACHE_PREFIX_RSVD_SIZE  GENERIC_TRANSMISSION_PRF_PKT_MAX_RX_SIZE
#define GENERIC_TRANSMISSION_PRF_RX_CACHE_SIZE              (GENERIC_TRANSMISSION_IO_RECV_CACHE_SIZE - GENERIC_TRANSMISSION_PRF_RX_CACHE_PREFIX_RSVD_SIZE)

#define GENERIC_TRANSMISSION_PRF_TID_TO_QIDX(type, tid)     (GTP_PKT_TYPE_MAJOR_GET((type)) == GTP_PKT_TYPE_MAJOR_CTRL ?    \
                                                             GENERIC_TRANSMISSION_PRF_TXQ_CTRL_IDX :                        \
                                                             (GTP_PKT_TYPE_MAJOR_GET((type)) == GTP_PKT_TYPE_MAJOR_MGMT ?   \
                                                              GENERIC_TRANSMISSION_PRF_TXQ_MGMT_IDX : (tid)))

// Need ack check
#define GENERIC_TRANSMISSION_PRF_NEED_ACK_ST_INVALID        0xFF

// type define
struct generic_transmission_prf_list_item {
    struct list_head node;
    uint32_t timestamp;     //us
    uint8_t *pkt;
    uint16_t pkt_size;      //pkt size is means the whole packet size, include padding, >= pkt_len
    uint8_t io;
};

struct generic_transmission_prf_pkt_pack_param {
    uint8_t type;
    uint8_t tid;
    uint8_t frag;
    bool_t need_ack;
};

struct generic_transmission_prf_pkt_tx_param {
    uint8_t io;
    uint8_t mode;
    uint8_t type;
    uint8_t tid;
    bool_t need_ack;
};

struct generic_transmission_prf_ctrl_tx_param {
    uint8_t io;
    union {
        struct {
            uint16_t ack_seq;
            uint16_t ack_status;
            uint8_t ack_type;
            uint8_t ack_tid;
        } single_ack;
    } u;
};

struct generic_transmission_prf_fc_tag {
    uint32_t last_ajust_time;     //us
    uint16_t dyn_timeout_ms;
    uint16_t dyn_mtu;
    uint16_t cont_miss_ack_num;
};

struct generic_transmission_prf_tx_env_tag {
    struct {
        struct list_head retry_list;
        struct list_head wait_list;
        struct list_head sending_list;
        uint16_t retry_list_num;
        uint16_t wait_list_num;
        uint16_t sending_list_num;
        uint16_t next_tx_seq;
    } tx_list_grp[GENERIC_TRANSMISSION_PRF_ALL_TXQ_NUM]; /* the last one is for tx control packet */
    struct generic_transmission_prf_fc_tag fc_env;
    os_task_h tx_task_hdl;
    os_sem_h tx_notify_sem;
    os_sem_h data_push_sem;
    os_sem_h ctrl_push_sem;
    timer_id_t ack_timer;
    int16_t total_data_pkt_size; /* record total packet length in the list */
    int16_t total_data_pkt_size_threshold;
    int16_t total_ctrl_pkt_size; /* record total packet length in the list */
    int16_t total_ctrl_pkt_size_threshold;
    uint8_t tid_prio[GENERIC_TRANSMISSION_TID_NUM];
    uint8_t txq_order[GENERIC_TRANSMISSION_PRF_ALL_TXQ_NUM];
    uint8_t tx_need_ack_st[GENERIC_TRANSMISSION_PRF_ALL_TXQ_NUM];
};

struct generic_transmission_prf_rx_cache {
    uint8_t *rx_cache;
    uint16_t write_idx;
    uint16_t read_idx;
    uint16_t write_size;
    uint16_t read_size;
};

struct generic_transmission_prf_rx_env_tag {
    struct {
        uint16_t expect_rx_seq;
    } rx_list_grp[GENERIC_TRANSMISSION_PRF_ALL_TXQ_NUM]; /* the last one is for rx control packet */
    struct generic_transmission_prf_rx_cache rx_cache_grp[GENERIC_TRANSMISSION_IO_NUM];
    os_task_h rx_task_hdl;
    os_sem_h rx_notify_sem;
    generic_transmission_data_rx_cb_t rx_cb[GENERIC_TRANSMISSION_TID_NUM];
    uint8_t rx_need_ack_st[GENERIC_TRANSMISSION_PRF_ALL_TXQ_NUM];
};

struct generic_transmission_prf_env_tag {
    struct generic_transmission_prf_tx_env_tag tx_env;
    struct generic_transmission_prf_rx_env_tag rx_env;
    uint8_t *preamble_next;   //prepare preamble next array to accelerate KMP search algorithm
};

static struct generic_transmission_prf_env_tag s_generic_transmission_prf_env = {
    /* .tx_env */
    .tx_env = {
        .tx_task_hdl = NULL,
        .tx_notify_sem = NULL,
        .data_push_sem = NULL,
        .ctrl_push_sem = NULL,
        .ack_timer = 0,
        .total_data_pkt_size = 0,
        .total_data_pkt_size_threshold = GENERIC_TRANSMISSION_PRF_TOTAL_DATA_PKT_SIZE_MAX_THRESHOLD,
        .total_ctrl_pkt_size = 0,
        .total_ctrl_pkt_size_threshold = GENERIC_TRANSMISSION_PRF_TOTAL_CTRL_PKT_SIZE_MAX_THRESHOLD,
        .tid_prio = {0, 0, 0, 0, 0, 0, 0, 0},
     },
     .rx_env = {
        .rx_cache_grp = {{0}},
        .rx_task_hdl = NULL,
        .rx_notify_sem = NULL,
        .rx_cb = {NULL},
     },
};

//static function declare
static int32_t generic_transmission_prf_rx_handler(uint8_t io, const uint8_t *buf, uint32_t len, bool_t in_isr);

// function implement
#if CONFIG_GENERIC_TRANSMISSION_PRF_TEST_CHECK
static uint32_t expect_seq = 0xAB000000;

static void generic_transmission_prf_test_data_check(const uint8_t *data, int32_t data_len)
{
    uint8_t *data_align = (uint8_t *)(((uint32_t)data) & ~0x3);
    uint32_t data_mod = (((uint32_t)data) & 0x3);
    uint32_t len_align = (uint32_t)data_len & ~0x3;
    uint32_t len_mod = (uint32_t)data_len & 0x3;

    for (uint32_t *p = (uint32_t *)data_align; p < (uint32_t *)(data_align + len_align); p += 1) {
        if (*p != expect_seq) {
            GENERIC_TRANSMISSION_PRF_LOGD("[GTP] check seq error %08x, expect %08x\n", *p, expect_seq);
        }

        expect_seq++;

        if (expect_seq >= 0xAB000190) {
            expect_seq -= 0x190;
        }
    }

    expect_seq += ((len_mod + data_mod + 3) >> 2);

    if (expect_seq >= 0xAB000190) {
        expect_seq -= 0x190;
    }
}
#endif

static uint8_t *_kmp_gen_next(const uint8_t *pattern, uint32_t pattern_len)
{
    uint8_t *next;
    uint32_t i = 0;
    uint8_t k = 0;

    next = (uint8_t *)os_mem_malloc(IOT_GENERIC_TRANSMISSION_MID, pattern_len);
    if (next == NULL) {
        return NULL;
    }

    //initial
    next[i] = 0;
    i++;

    while (i < pattern_len) {
        if (pattern[k] == pattern[i]) {
            next[i] = k;
            k++;
            i++;
        } else {
            if (k > 0) {
                k = next[k-1];
            } else {
                next[i] = 0;
                i++;
            }
        }
    }

    return next;
}

static int32_t _kmp_search(const uint8_t *search, uint32_t search_len,
                       const uint8_t *pattern, uint32_t pattern_len,
                       const uint8_t *pre_next)
{
    bool_t found = false;
    uint32_t search_idx = 0;
    uint32_t pattern_idx = 0;
    const uint8_t *next;

    if (pre_next == NULL) {
        next = _kmp_gen_next(pattern, pattern_len);
        if (next == NULL) {
            return -RET_NOMEM;
        }
    } else {
        next = pre_next;
    }

    while (search_idx < search_len) {
        if (search[search_idx] == pattern[pattern_idx]) {
            search_idx++;
            pattern_idx++;
        } else {
            if (pattern_idx > 0) {
                pattern_idx = next[pattern_idx - 1];
            } else {
                search_idx++;
            }
        }

        if (pattern_idx == pattern_len) {
            found = true;
            break;
        }
    }

    return (found ? (int32_t)(search_idx - pattern_idx) : -RET_FAIL);
}

static void generic_transmission_prf_next_tx_seq_inc(uint8_t idx)
{
    s_generic_transmission_prf_env.tx_env.tx_list_grp[idx].next_tx_seq =
        (s_generic_transmission_prf_env.tx_env.tx_list_grp[idx].next_tx_seq + 1) & 0xFFFF;
}

static void generic_transmission_prf_expect_rx_seq_inc(uint8_t idx)
{
    s_generic_transmission_prf_env.rx_env.rx_list_grp[idx].expect_rx_seq =
        (s_generic_transmission_prf_env.rx_env.rx_list_grp[idx].expect_rx_seq + 1) & 0xFFFF;
}

static void generic_transmission_prf_expect_rx_seq_update(uint8_t idx, uint16_t new_seq)
{
    s_generic_transmission_prf_env.rx_env.rx_list_grp[idx].expect_rx_seq = (new_seq + 1) & 0xFFFF;
}

static bool_t generic_transmission_prf_is_retry_list_under_low_watermark(uint32_t txq_idx)
{
    if (txq_idx == GENERIC_TRANSMISSION_PRF_TXQ_CTRL_IDX &&
            s_generic_transmission_prf_env.tx_env.tx_list_grp[txq_idx].retry_list_num > GENERIC_TRANSMISSION_PRF_TX_CTRL_RETRY_LIST_LOW_WATERMARK) {
        return false;
    } else if (txq_idx != GENERIC_TRANSMISSION_PRF_TXQ_CTRL_IDX &&
            s_generic_transmission_prf_env.tx_env.tx_list_grp[txq_idx].retry_list_num > GENERIC_TRANSMISSION_PRF_TX_DATA_RETRY_LIST_LOW_WATERMARK) {
        return false;
    }

    return true;
}

static uint8_t generic_transmission_prf_get_max_prio_retry_list_idx(void)
{
    uint8_t txq_idx;

    for (uint32_t i = 0; i < GENERIC_TRANSMISSION_PRF_ALL_TXQ_NUM; i++) {
        txq_idx = s_generic_transmission_prf_env.tx_env.txq_order[i];
        if (!generic_transmission_prf_is_retry_list_under_low_watermark(txq_idx)) {
            break;
        }
    }

    return txq_idx;
}

static bool_t generic_transmission_prf_is_all_retry_list_empty(void)
{
    for (uint32_t i = 0; i < GENERIC_TRANSMISSION_PRF_ALL_TXQ_NUM; i++) {

        GENERIC_TRANSMISSION_PRF_LOGD("[GTP] retry empty check %d, num %d\n", i, s_generic_transmission_prf_env.tx_env.tx_list_grp[i].retry_list_num);

        if (s_generic_transmission_prf_env.tx_env.tx_list_grp[i].retry_list_num > 0) {
            return false;
        } else {
            //assert(list_empty(&s_generic_transmission_prf_env.tx_env.tx_list_grp[i].retry_list));
        }
    }
    return true;
}

static bool_t generic_transmission_prf_is_all_retry_list_under_low_watermark(void)
{
    for (uint32_t i = 0; i < GENERIC_TRANSMISSION_PRF_ALL_TXQ_NUM; i++) {
        if (!generic_transmission_prf_is_retry_list_under_low_watermark(i)) {
            break;
        }
    }
    return true;
}

static bool_t generic_transmission_prf_is_wait_list_under_low_watermark(uint32_t txq_idx)
{
    if (txq_idx == GENERIC_TRANSMISSION_PRF_TXQ_CTRL_IDX &&
            s_generic_transmission_prf_env.tx_env.tx_list_grp[txq_idx].wait_list_num > GENERIC_TRANSMISSION_PRF_TX_CTRL_RETRY_LIST_LOW_WATERMARK) {
        return false;
    } else if (txq_idx != GENERIC_TRANSMISSION_PRF_TXQ_CTRL_IDX &&
            s_generic_transmission_prf_env.tx_env.tx_list_grp[txq_idx].wait_list_num > GENERIC_TRANSMISSION_PRF_TX_DATA_RETRY_LIST_LOW_WATERMARK) {
        return false;
    }

    return true;
}

static uint8_t generic_transmission_prf_get_max_prio_wait_list_idx(void)
{
    uint8_t txq_idx;

    for (uint32_t i = 0; i < GENERIC_TRANSMISSION_PRF_ALL_TXQ_NUM; i++) {
        txq_idx = s_generic_transmission_prf_env.tx_env.txq_order[i];
        if (!generic_transmission_prf_is_wait_list_under_low_watermark(txq_idx)) {
            break;
        }
    }

    GENERIC_TRANSMISSION_PRF_LOGD("[GTP] select qid %d\n", txq_idx);

    return txq_idx;
}

static bool_t generic_transmission_prf_is_all_wait_list_empty(void)
{
    for (uint32_t i = 0; i < GENERIC_TRANSMISSION_PRF_ALL_TXQ_NUM; i++) {

        GENERIC_TRANSMISSION_PRF_LOGD("[GTP] wait empty check %d, num %d\n", i, s_generic_transmission_prf_env.tx_env.tx_list_grp[i].wait_list_num);

        if (s_generic_transmission_prf_env.tx_env.tx_list_grp[i].wait_list_num > 0) {
            return false;
        } else {
            //assert(list_empty(&s_generic_transmission_prf_env.tx_env.tx_list_grp[i].wait_list));
        }
    }
    return true;
}

static bool_t generic_transmission_prf_is_all_wait_list_under_low_watermark(void)
{
    for (uint32_t i = 0; i < GENERIC_TRANSMISSION_PRF_ALL_TXQ_NUM; i++) {
        if (!generic_transmission_prf_is_wait_list_under_low_watermark(i)) {
            break;
        }
    }

    return true;
}

static bool_t generic_transmission_prf_is_all_sending_list_empty(void)
{
    for (uint32_t i = 0; i < GENERIC_TRANSMISSION_PRF_ALL_TXQ_NUM; i++) {

        GENERIC_TRANSMISSION_PRF_LOGD("[GTP] sending empty check %d, num %d\n", i, s_generic_transmission_prf_env.tx_env.tx_list_grp[i].sending_list_num);

        if (s_generic_transmission_prf_env.tx_env.tx_list_grp[i].sending_list_num > 0) {
            return false;
        } else {
            //assert(list_empty(&s_generic_transmission_prf_env.tx_env.tx_list_grp[i].retry_list));
        }
    }
    return true;
}

static void generic_transmission_prf_free_item(struct generic_transmission_prf_list_item *item)
{
    if (item == NULL) {
        return;
    }

    if (GTP_PKT_TYPE_MAJOR_GET(GTP_PROTO_PKT_HDR_TYPE_GET(item->pkt + GTP_PKT_PREAMBLE_SYNC_LEN))
        == GTP_PKT_TYPE_MAJOR_DATA) {   //lint !e826 Suspicious pointer-to-pointer area too small
        cpu_critical_enter();
        s_generic_transmission_prf_env.tx_env.total_data_pkt_size -= (int16_t)item->pkt_size;
        cpu_critical_exit();

        os_mem_free(item->pkt);
        os_mem_free(item);

        if (s_generic_transmission_prf_env.tx_env.total_data_pkt_size
            < s_generic_transmission_prf_env.tx_env.total_data_pkt_size_threshold) {
            os_post_semaphore(s_generic_transmission_prf_env.tx_env.data_push_sem);
        }
    } else {
        cpu_critical_enter();
        s_generic_transmission_prf_env.tx_env.total_ctrl_pkt_size -= (int16_t)item->pkt_size;
        cpu_critical_exit();

        os_mem_free(item->pkt);
        os_mem_free(item);

        if (s_generic_transmission_prf_env.tx_env.total_ctrl_pkt_size < s_generic_transmission_prf_env.tx_env.total_ctrl_pkt_size_threshold) {
            os_post_semaphore(s_generic_transmission_prf_env.tx_env.ctrl_push_sem);
        }
    }
}

static void generic_transmission_prf_pack_data(uint8_t *pkt, uint32_t pkt_size, const uint8_t *data, uint32_t data_len,
                                               const struct generic_transmission_prf_pkt_pack_param *param)
{
    uint32_t crc;
    uint8_t hec;
    uint8_t txq_idx = GENERIC_TRANSMISSION_PRF_TID_TO_QIDX(param->type, param->tid);
    uint32_t pkt_len = GTP_PKT_HDR_LEN + data_len + GTP_PKT_CRC_LEN;

    // pack preamble
    GTP_PROTO_PKT_PREAMBLE_PACK(pkt);   //lint !e826 Suspicious pointer-to-pointer area too small
    pkt += GTP_PKT_PREAMBLE_LEN;

    // pack hdr
    GTP_PROTO_PKT_HDR_PACK(
        pkt, (uint16_t)pkt_len, param->type, param->tid, param->frag, param->need_ack ? 1 : 0,
        s_generic_transmission_prf_env.tx_env.tx_list_grp[txq_idx]
            .next_tx_seq);   //lint !e826 Suspicious pointer-to-pointer area too small

    //pack hec
    hec = getcrc8(pkt, GTP_PKT_HDR_LEN - GTP_PKT_HEC_LEN);

    GTP_PROTO_PKT_HDR_HEC_SET(pkt, hec);  //lint !e826 Suspicious pointer-to-pointer area too small

    //pack payload
    GTP_PROTO_PKT_DATA_PACK(pkt, data, data_len);   //lint !e826 Suspicious pointer-to-pointer area too small

    //pack crc
    crc = getcrc32(pkt, pkt_len - GTP_PKT_CRC_LEN);

    GTP_PROTO_PKT_CRC_PACK(pkt, pkt_len, crc);

    assert((pkt_len + GTP_PKT_PREAMBLE_LEN) == pkt_size);

    generic_transmission_prf_next_tx_seq_inc(txq_idx);
}

static int32_t generic_transmission_prf_pkt_tx(const uint8_t *data, uint32_t data_len, struct generic_transmission_prf_pkt_tx_param *param, bool_t blocking)
{
    struct generic_transmission_prf_pkt_pack_param pack_param;
    int32_t remain_len = (int32_t)data_len;
    int32_t sent_len = 0;

    while (remain_len > 0) {
        int32_t pdu_len = MIN(remain_len, s_generic_transmission_prf_env.tx_env.fc_env.dyn_mtu);
        uint32_t pkt_size = GTP_PKT_PREAMBLE_SYNC_LEN + GTP_PKT_HDR_LEN + (uint32_t)pdu_len + GTP_PKT_CRC_LEN;
        uint8_t *pkt;
        struct generic_transmission_prf_list_item *item;
        uint32_t free_heap_size = os_mem_get_heap_free();
        uint8_t txq_idx = GENERIC_TRANSMISSION_PRF_TID_TO_QIDX(param->type, param->tid);

        /* Just support unique need_ack value in the same TXQ, as well as the same TID.
         * The code below is used for need_ack value checking
         */
        if (s_generic_transmission_prf_env.tx_env.tx_need_ack_st[txq_idx] == GENERIC_TRANSMISSION_PRF_NEED_ACK_ST_INVALID) {
            //first tx packet in this TX queue, save the need_ack value
            s_generic_transmission_prf_env.tx_env.tx_need_ack_st[txq_idx] = param->need_ack ? 1 : 0;
        } else if ((s_generic_transmission_prf_env.tx_env.tx_need_ack_st[txq_idx] != 0 && param->need_ack)
            || (s_generic_transmission_prf_env.tx_env.tx_need_ack_st[txq_idx] == 0 && !param->need_ack)) {
            GENERIC_TRANSMISSION_PRF_LOGE("[GTP] Error: TX QID %d use multiple need_ack value %d, first is %d!\n",
                                          txq_idx, param->need_ack, s_generic_transmission_prf_env.tx_env.tx_need_ack_st[txq_idx]);
            //use the first need_ack value to instead of current one
            param->need_ack = s_generic_transmission_prf_env.tx_env.tx_need_ack_st[txq_idx] != 0;
        } else {
            //correct, do nothing
        }

        if (GTP_PKT_TYPE_MAJOR_GET(param->type) == GTP_PKT_TYPE_MAJOR_DATA) {
            // dynamic update total packet size threshold, according to free heap size, the throughput may decrease if there's no large free memory
            if (free_heap_size < GENERIC_TRANSMISSION_PRF_TOTAL_DATA_PKT_SIZE_MIN_THRESHOLD) {
                GENERIC_TRANSMISSION_PRF_LOGE("[GTP] free Heap size is less than %d, wait more free heap!\n", GENERIC_TRANSMISSION_PRF_TOTAL_DATA_PKT_SIZE_MIN_THRESHOLD);
                return -RET_NOMEM;
            } else if (free_heap_size < GENERIC_TRANSMISSION_PRF_TOTAL_DATA_PKT_SIZE_MIN_THRESHOLD + GENERIC_TRANSMISSION_PRF_FREE_HEAP_SIZE_MAINTAIN_THRESHOLD) {
                s_generic_transmission_prf_env.tx_env.total_data_pkt_size_threshold = GENERIC_TRANSMISSION_PRF_TOTAL_DATA_PKT_SIZE_MIN_THRESHOLD;
            } else if (free_heap_size < GENERIC_TRANSMISSION_PRF_TOTAL_DATA_PKT_SIZE_MAX_THRESHOLD + GENERIC_TRANSMISSION_PRF_FREE_HEAP_SIZE_MAINTAIN_THRESHOLD) {
                s_generic_transmission_prf_env.tx_env.total_data_pkt_size_threshold = (int16_t)(free_heap_size - GENERIC_TRANSMISSION_PRF_FREE_HEAP_SIZE_MAINTAIN_THRESHOLD);
            } else {
                s_generic_transmission_prf_env.tx_env.total_data_pkt_size_threshold = GENERIC_TRANSMISSION_PRF_TOTAL_DATA_PKT_SIZE_MAX_THRESHOLD;
            }

            if (s_generic_transmission_prf_env.tx_env.total_data_pkt_size >= s_generic_transmission_prf_env.tx_env.total_data_pkt_size_threshold) {
                GENERIC_TRANSMISSION_PRF_LOGD("[GTP] wait sem, type %d!\n", param->type);
                os_pend_semaphore(s_generic_transmission_prf_env.tx_env.data_push_sem, 0xFFFFFFFF);
            }
        } else {
            if (free_heap_size < GENERIC_TRANSMISSION_PRF_TOTAL_CTRL_PKT_SIZE_MAX_THRESHOLD) {
                return -RET_NOMEM;
            } else if (free_heap_size < GENERIC_TRANSMISSION_PRF_TOTAL_CTRL_PKT_SIZE_MIN_THRESHOLD + GENERIC_TRANSMISSION_PRF_FREE_HEAP_SIZE_MAINTAIN_THRESHOLD) {
                s_generic_transmission_prf_env.tx_env.total_ctrl_pkt_size_threshold = GENERIC_TRANSMISSION_PRF_TOTAL_CTRL_PKT_SIZE_MIN_THRESHOLD;
            } else if (free_heap_size < GENERIC_TRANSMISSION_PRF_TOTAL_CTRL_PKT_SIZE_MAX_THRESHOLD + GENERIC_TRANSMISSION_PRF_FREE_HEAP_SIZE_MAINTAIN_THRESHOLD) {
                s_generic_transmission_prf_env.tx_env.total_ctrl_pkt_size_threshold = (int16_t)(free_heap_size - GENERIC_TRANSMISSION_PRF_FREE_HEAP_SIZE_MAINTAIN_THRESHOLD);
            } else {
                s_generic_transmission_prf_env.tx_env.total_ctrl_pkt_size_threshold = GENERIC_TRANSMISSION_PRF_TOTAL_CTRL_PKT_SIZE_MAX_THRESHOLD;
            }

            if (s_generic_transmission_prf_env.tx_env.total_ctrl_pkt_size >= s_generic_transmission_prf_env.tx_env.total_ctrl_pkt_size_threshold) {
                if (blocking) {
                    /*  Now, Rx protocol process is in RX task, so Both RX ack handler and TX ack process are in RX task,
                     *  if TX ack blocking here, it may cause RX task cannot do RX protocol process, it cause more worse.
                     */
                    GENERIC_TRANSMISSION_PRF_LOGD("[GTP] wait sem, type %d!\n", param->type);
                    os_pend_semaphore(s_generic_transmission_prf_env.tx_env.ctrl_push_sem, 0xFFFFFFFF);
                } else {
                    return -RET_PENDING;
                }
            }
        }

        pkt = os_mem_malloc(IOT_GENERIC_TRANSMISSION_MID, pkt_size);
        if (pkt == NULL) {
            GENERIC_TRANSMISSION_PRF_LOGE("[GTP] No memory to alloc pkt!\n");
            return -RET_NOMEM;
        }

        uint8_t frag = GTP_PKT_FC_FRAG_INVALID;
        if (param->mode == GENERIC_TRANSMISSION_TX_MODE_LAZY) {
            switch (frag) {
            case GTP_PKT_FC_FRAG_INVALID:
                frag = (remain_len > pdu_len) ? GTP_PKT_FC_FRAG_FIRST : GTP_PKT_FC_FRAG_COMPLETE;
                break;
            case GTP_PKT_FC_FRAG_FIRST:
            case GTP_PKT_FC_FRAG_CONTINUE:
                frag = (remain_len > pdu_len) ? GTP_PKT_FC_FRAG_CONTINUE : GTP_PKT_FC_FRAG_END;
                break;
            case GTP_PKT_FC_FRAG_COMPLETE:
            case GTP_PKT_FC_FRAG_END:
            default:
                //do nothing:
                break;
            }
        } else {
            //ASAP mode, keep frag type complete
            frag = GTP_PKT_FC_FRAG_COMPLETE;
        }

        //set pack param
        pack_param.type = param->type;
        pack_param.tid = (param->tid < GENERIC_TRANSMISSION_TID_NUM ? param->tid : 0);
        pack_param.frag = frag;
        pack_param.need_ack = param->need_ack;

        generic_transmission_prf_pack_data(pkt, pkt_size, data + sent_len, (uint32_t)pdu_len, &pack_param);

        item = os_mem_malloc(IOT_GENERIC_TRANSMISSION_MID, sizeof(struct generic_transmission_prf_list_item));
        if (item == NULL) {
            GENERIC_TRANSMISSION_PRF_LOGE("[GTP] No memory to alloc list header!\n");
            os_mem_free(pkt);
            return -RET_NOMEM;
        }

        item->pkt = pkt;
        item->pkt_size = (uint16_t)pkt_size;
        item->io = param->io;

        GENERIC_TRANSMISSION_PRF_LOGD("[GTP] add to wait list %08x, tid %d, qidx %d\n", pkt, param->tid, txq_idx);
        cpu_critical_enter();
        list_add_tail(&item->node, &s_generic_transmission_prf_env.tx_env.tx_list_grp[txq_idx].wait_list);
        s_generic_transmission_prf_env.tx_env.tx_list_grp[txq_idx].wait_list_num++;
        cpu_critical_exit();

        if (GTP_PKT_TYPE_MAJOR_GET(param->type) == GTP_PKT_TYPE_MAJOR_DATA) {
            cpu_critical_enter();
            s_generic_transmission_prf_env.tx_env.total_data_pkt_size += (int16_t)pkt_size;
            cpu_critical_exit();
        } else {
            cpu_critical_enter();
            s_generic_transmission_prf_env.tx_env.total_ctrl_pkt_size += (int16_t)pkt_size;
            cpu_critical_exit();
        }

        sent_len += pdu_len;
        remain_len -= pdu_len;

        os_post_semaphore(s_generic_transmission_prf_env.tx_env.tx_notify_sem);
    }

    return RET_OK;
}

static int32_t generic_transmission_prf_pkt_tx_panic(const uint8_t *data, uint32_t data_len, const struct generic_transmission_prf_pkt_tx_param *param)
{
    struct generic_transmission_prf_pkt_pack_param pack_param;
    int32_t remain_len = (int32_t)data_len;
    int32_t sent_len = 0;

    if (param->tid >= GENERIC_TRANSMISSION_TID_NUM) {
        return -RET_INVAL;
    }

    while (remain_len > 0) {
        int32_t pdu_len = MIN(remain_len, s_generic_transmission_prf_env.tx_env.fc_env.dyn_mtu);
        uint32_t pkt_size = GTP_PKT_PREAMBLE_SYNC_LEN + GTP_PKT_HDR_LEN + (uint32_t)pdu_len + GTP_PKT_CRC_LEN;
        uint8_t pkt[pkt_size];  //variable array

        uint8_t frag = GTP_PKT_FC_FRAG_INVALID;
        if (param->mode == GENERIC_TRANSMISSION_TX_MODE_LAZY) {
            switch (frag) {
            case GTP_PKT_FC_FRAG_INVALID:
                frag = (remain_len > pdu_len) ? GTP_PKT_FC_FRAG_FIRST : GTP_PKT_FC_FRAG_COMPLETE;
                break;
            case GTP_PKT_FC_FRAG_FIRST:
            case GTP_PKT_FC_FRAG_CONTINUE:
                frag = (remain_len > pdu_len) ? GTP_PKT_FC_FRAG_CONTINUE : GTP_PKT_FC_FRAG_END;
                break;
            case GTP_PKT_FC_FRAG_COMPLETE:
            case GTP_PKT_FC_FRAG_END:
            default:
                //do nothing:
                break;
            }
        } else {
            //ASAP mode, keep frag type complete
            frag = GTP_PKT_FC_FRAG_COMPLETE;
        }

        //set pack param
        pack_param.type = param->type;
        pack_param.tid = (param->tid < GENERIC_TRANSMISSION_TID_NUM ? param->tid : 0);
        pack_param.frag = frag;
        pack_param.need_ack = param->need_ack;

        generic_transmission_prf_pack_data(pkt, pkt_size, data + sent_len, (uint32_t)pdu_len, &pack_param);

        generic_transmission_io_send_panic(param->io, pkt, pkt_size);

        sent_len += pdu_len;
        remain_len -= pdu_len;
    }

    return sent_len;
}

int32_t generic_transmission_prf_data_tx(const uint8_t *data, uint32_t data_len, const generic_transmission_prf_data_tx_param_t *param)
{
    struct generic_transmission_prf_pkt_tx_param tx_param;

    if (param->tid >= GENERIC_TRANSMISSION_TID_NUM) {
        return -RET_INVAL;
    }

    tx_param.mode = param->mode;
    tx_param.type = GTP_PKT_TYPE_BUILD(GTP_PKT_TYPE_MAJOR_DATA, param->type);
    tx_param.tid = param->tid;
    tx_param.need_ack = param->need_ack;
    tx_param.io = param->io;

    return generic_transmission_prf_pkt_tx(data, data_len, &tx_param, true);
}

int32_t generic_transmission_prf_data_tx_panic(const uint8_t *data, uint32_t data_len, const generic_transmission_prf_data_tx_param_t *param)
{
    struct generic_transmission_prf_pkt_tx_param tx_param;

    if (param->tid >= GENERIC_TRANSMISSION_TID_NUM) {
        return -RET_INVAL;
    }

    tx_param.mode = param->mode;
    tx_param.type = GTP_PKT_TYPE_BUILD(GTP_PKT_TYPE_MAJOR_DATA, param->type);
    tx_param.tid = param->tid;
    tx_param.need_ack = false; //for need_ack zero
    tx_param.io = param->io;

    return generic_transmission_prf_pkt_tx_panic(data, data_len, &tx_param);
}

static int32_t generic_transmission_prf_ctrl_tx(uint8_t type, const struct generic_transmission_prf_ctrl_tx_param *param)
{
    struct generic_transmission_prf_pkt_tx_param tx_param;
    int32_t ret;

    tx_param.mode = GENERIC_TRANSMISSION_TX_MODE_LAZY;
    tx_param.type = GTP_PKT_TYPE_BUILD(GTP_PKT_TYPE_MAJOR_CTRL, type);
    tx_param.tid = 0;
    tx_param.need_ack = false;
    tx_param.io = param->io;

    if (type == GTP_PKT_TYPE_CTRL_SUB_SINGLE_ACK)  {
        uint8_t single_ack_pld[GTP_PLD_SINGLE_ACK_LEN];
        uint16_t ack_seq = param->u.single_ack.ack_seq;
        uint16_t ack_status = param->u.single_ack.ack_status;
        uint8_t ack_type = param->u.single_ack.ack_type;
        uint8_t ack_tid = param->u.single_ack.ack_tid;
        uint16_t single_ack_pld_len = GTP_PLD_SINGLE_ACK_LEN;

        GENERIC_TRANSMISSION_PRF_LOGD("[GTP] single ack pack, seq %d, st %d, type %d, tid %d\n", ack_seq, ack_status, ack_type, ack_tid);

        GTP_PROTO_PLD_CTRL_SINGLE_ACK_PACK(single_ack_pld, ack_seq, ack_status, ack_type, ack_tid);

        ret = generic_transmission_prf_pkt_tx(single_ack_pld, single_ack_pld_len, &tx_param, false);
    } else {
        //do nothing, not support yet.
        return -RET_NOSUPP;
    }

    return ret;
}

void generic_transmission_prf_tx_flush_panic(void)
{
    uint8_t list_idx;
    struct list_head *list_hdl;
    struct list_head *saved;
    struct generic_transmission_prf_list_item *item;

    GENERIC_TRANSMISSION_PRF_LOGD("[GTP] tx flush panic\n");

    // handle retry list firstly
    for (list_idx = 0; list_idx < GENERIC_TRANSMISSION_PRF_ALL_TXQ_NUM; list_idx++) {
        list_hdl = &s_generic_transmission_prf_env.tx_env.tx_list_grp[list_idx].retry_list;

        list_for_each_entry_safe(item, list_hdl, node, saved) {
            generic_transmission_io_send_panic(item->io, item->pkt, item->pkt_size);
            cpu_critical_enter();
            list_del(&item->node);
            cpu_critical_exit();
        }
    }

    // handle wait list
    for (list_idx = 0; list_idx < GENERIC_TRANSMISSION_PRF_ALL_TXQ_NUM; list_idx++) {
        list_hdl = &s_generic_transmission_prf_env.tx_env.tx_list_grp[list_idx].wait_list;

        list_for_each_entry_safe(item, list_hdl, node, saved) {
            generic_transmission_io_send_panic(item->io, item->pkt, item->pkt_size);
            cpu_critical_enter();
            list_del(&item->node);
            cpu_critical_exit();
        }
    }
}

static int32_t generic_transmission_prf_rx_proto_process(uint8_t io, uint8_t *pkt, uint16_t pkt_len)
{
    uint16_t ack_status = GTP_ACK_ST_OK;
    uint8_t type = GTP_PROTO_PKT_HDR_TYPE_GET(pkt);     //lint !e826 Suspicious pointer-to-pointer area too small
    uint8_t tid = GTP_PROTO_PKT_HDR_FC_TID_GET(pkt);    //lint !e826 Suspicious pointer-to-pointer area too small
    uint8_t need_ack = GTP_PROTO_PKT_HDR_FC_NEED_ACK_GET(pkt);  //lint !e826 Suspicious pointer-to-pointer area too small
    uint16_t seq = GTP_PROTO_PKT_HDR_SEQ_GET(pkt);  //lint !e826 Suspicious pointer-to-pointer area too small
    bool_t do_cb = false;
    uint8_t q_idx = GENERIC_TRANSMISSION_PRF_TID_TO_QIDX(type, tid);
    uint16_t expect_seq = s_generic_transmission_prf_env.rx_env.rx_list_grp[q_idx].expect_rx_seq;
    uint32_t calc_crc;
    int32_t ret;

    // check crc
    calc_crc =  getcrc32(pkt, pkt_len - GTP_PKT_CRC_LEN);
    if (calc_crc != GTP_PROTO_PKT_CRC_GET(pkt, pkt_len)) {
        ack_status = GTP_ACK_ST_CRC_ERR;
        ret = -RET_CRC_FAIL;
    } else {
        /* ACK packet doesn't need to dispatch packet to other module,
        * so only ACK packet is handled in this function, other type packet
        * should be dispatch to RX task then handle it in RX task
        */
        if (GTP_PKT_TYPE_MAJOR_GET(type) == GTP_PKT_TYPE_MAJOR_DATA) {
            /* The data with the same TID shall not support 'need_ack = 0' and 'need_ack = 1'.
            * If 'need ack = 0 ', discard it, else check sequence and do reorder.
            * But, beetle is lack of memory, so this version do not support reorder and PC->Beetle fragment, .
            * TODO: fragment and reorder in future
            */
            GENERIC_TRANSMISSION_PRF_LOGD("[GTP] recv data tid %d, seq %d\n", tid, seq);

            if (s_generic_transmission_prf_env.rx_env.rx_need_ack_st[q_idx] == GENERIC_TRANSMISSION_PRF_NEED_ACK_ST_INVALID) {
                //first receive packet in this TID
                s_generic_transmission_prf_env.rx_env.rx_need_ack_st[q_idx] = need_ack;
            } else if (s_generic_transmission_prf_env.rx_env.rx_need_ack_st[q_idx] != need_ack) {
                GENERIC_TRANSMISSION_PRF_LOGE("[GTP] Error: RX QID %d use multiple need_ack value %d, first is %d!\n",
                                            q_idx, need_ack, s_generic_transmission_prf_env.rx_env.rx_need_ack_st[q_idx]);
                // use the first value to intead of current one
                need_ack = s_generic_transmission_prf_env.rx_env.rx_need_ack_st[q_idx];
            } else {
                //correct, do nothing
            }

            if (need_ack) {
                if (seq == expect_seq) {
                    //expect seq is corret expect seq, do callback to caller, expect seq increase later after ack tx success
                    do_cb = true;
                } else if (((expect_seq - seq) & 0xffff) < 0x8000) {
                    /* current seq is an older one, as beetle memory is not enough, treat all not expected seq as fail, notify remote device ok
                    * ack_status = GTP_ACK_ST_LT_EXPECT;
                    */
                } else {
                    //current seq is an newer one, as beetle memory is not enough, treat all not expected seq as fail, notify remote device retry
                    ack_status = GTP_ACK_ST_GT_EXPECT;
                }
            } else {
                //force update seq
                generic_transmission_prf_expect_rx_seq_update(q_idx, seq);
                do_cb = true;
            }
        } else if (GTP_PKT_TYPE_MAJOR_GET(type) == GTP_PKT_TYPE_MAJOR_CTRL) {
            /* Ctrl packet dont't support ack, although the packet 'need ack' is 1. Discard it.
            * As don't support ack, so the sequence should be increased, don't check sequence, too
            */
            if (GTP_PKT_TYPE_SUB_GET(type) == GTP_PKT_TYPE_CTRL_SUB_SINGLE_ACK) {
                uint16_t ack_seq = GTP_PROTO_PKT_CTRL_SINGLE_ACK_SEQ_GET(pkt);  //lint !e826 Suspicious pointer-to-pointer area too small
                uint16_t ack_status_l = GTP_PROTO_PKT_CTRL_SINGLE_ACK_STATUS_GET(pkt);  //lint !e826 Suspicious pointer-to-pointer area too small
                uint8_t ack_type = GTP_PROTO_PKT_CTRL_SINGLE_ACK_TYPE_GET(pkt); //lint !e826 Suspicious pointer-to-pointer area too small
                uint8_t ack_tid = GTP_PROTO_PKT_CTRL_SINGLE_ACK_TID_GET(pkt);   //lint !e826 Suspicious pointer-to-pointer area too small
                struct generic_transmission_prf_list_item *item;
                uint8_t found = 0;

                GENERIC_TRANSMISSION_PRF_LOGD("[GTP] recv ack seq %d\n", ack_seq);

                //re-get q_idx by ack content
                q_idx = GENERIC_TRANSMISSION_PRF_TID_TO_QIDX(ack_type, ack_tid);

                //search the tid and sequcence in sending_list, tid is treat as core id in this profile
                list_for_each_entry(item, &s_generic_transmission_prf_env.tx_env.tx_list_grp[q_idx].sending_list, node)  {
                    if (GTP_PROTO_PKT_HDR_SEQ_GET(item->pkt + GTP_PKT_PREAMBLE_LEN) == ack_seq) {
                        found = 1;
                        break;
                    }
                }

                if (found) {
                    //Remove from wait list
                    cpu_critical_enter();
                    list_del(&item->node);
                    s_generic_transmission_prf_env.tx_env.tx_list_grp[q_idx].sending_list_num--;
                    cpu_critical_exit();

                    if (ack_status_l == GTP_ACK_ST_OK) {
                        generic_transmission_prf_free_item(item);
                    } else {
                        //move add pkt into sending list
                        cpu_critical_enter();
                        list_add_tail(&item->node, &s_generic_transmission_prf_env.tx_env.tx_list_grp[q_idx].retry_list);
                        s_generic_transmission_prf_env.tx_env.tx_list_grp[q_idx].retry_list_num++;
                        cpu_critical_exit();
                    }

                    os_post_semaphore(s_generic_transmission_prf_env.tx_env.tx_notify_sem);
                }

                // if all the list empty, stop the timer
                if (generic_transmission_prf_is_all_sending_list_empty()) {
                    os_stop_timer(s_generic_transmission_prf_env.tx_env.ack_timer);
                }

                /* Warning: whatever need_ack value in SINGLE_ACK Packet, this type don't need reply ack.
                * To avoid remote device set invalid need_ack bit, force to set need_ack flag to 0 */
                need_ack = 0;
            } else {
                // unknown ctrl sub type, do nothing
            }

            generic_transmission_prf_expect_rx_seq_update(q_idx, seq);

        } else if (GTP_PKT_TYPE_MAJOR_GET(type) == GTP_PKT_TYPE_MAJOR_MGMT) {
            /* Managment type should force suport 'need_ack = 1' to be different from Control type.
            * Control type don't care packet lose, but Managment type should detect packet lose and retransimission.
            * TODO: implement in the future, such like MTU exchange.
            */
        } else {
            //unknown major type, do nothing
        }

        ret = RET_OK;
    }

    // HEC is already check before this function enter, so header is reliable, do replay ack if needed
    if (need_ack) {
        struct generic_transmission_prf_ctrl_tx_param param;

        if (seq != expect_seq) {
            //send ack with expect seq firstly
            param.io = io;
            param.u.single_ack.ack_seq = seq;
            param.u.single_ack.ack_status = ack_status;
            param.u.single_ack.ack_type = type;
            param.u.single_ack.ack_tid = tid;
            ret = generic_transmission_prf_ctrl_tx(GTP_PKT_TYPE_CTRL_SUB_SINGLE_ACK, &param);
            GENERIC_TRANSMISSION_PRF_LOGD("[GTP] 1.send ack tid %d, ack_seq %d, ack st %d, expect %d, ret %d\n",
                            tid, seq, ack_status, s_generic_transmission_prf_env.rx_env.rx_list_grp[q_idx].expect_rx_seq, ret);
            if (ret != RET_OK) {
                return ret;
            }

            if (ack_status == GTP_ACK_ST_GT_EXPECT) {
                //if current sequence is greater than expect, than force to make remote device retry expect seq packet
                GENERIC_TRANSMISSION_PRF_LOGD("[GTP] 2.send ack tid %d, ack_seq %d, ack st %d, ret %d\n", tid, seq, GTP_ACK_ST_FORCE_RETRY, ret);
                //send ack with current seq secondary
                param.u.single_ack.ack_seq = s_generic_transmission_prf_env.rx_env.rx_list_grp[q_idx].expect_rx_seq;
                param.u.single_ack.ack_status = GTP_ACK_ST_FORCE_RETRY;
                ret = generic_transmission_prf_ctrl_tx(GTP_PKT_TYPE_CTRL_SUB_SINGLE_ACK, &param);
                if (ret != RET_OK) {
                    return ret;
                }
            }
        } else {
            GENERIC_TRANSMISSION_PRF_LOGD("[GTP] 0.send ack tid %d, ack_seq %d, ack st %d, ret %d\n", tid, seq, ack_status, ret);
            param.io = io;
            param.u.single_ack.ack_seq = seq;
            param.u.single_ack.ack_status = ack_status;
            param.u.single_ack.ack_type = type;
            param.u.single_ack.ack_tid = tid;
            ret = generic_transmission_prf_ctrl_tx(GTP_PKT_TYPE_CTRL_SUB_SINGLE_ACK, &param);
            if (ret != RET_OK) {
                return ret;
            } else {
                generic_transmission_prf_expect_rx_seq_inc(q_idx);
            }
        }
    } else {
        //need no ack, do nothing
    }

    //if the packet is no problem, then call callback function
    if (do_cb && s_generic_transmission_prf_env.rx_env.rx_cb[tid]) {
        s_generic_transmission_prf_env.rx_env.rx_cb[tid](
            (generic_transmission_tid_t)tid,
            (generic_transmission_data_type_t)GTP_PKT_TYPE_SUB_GET(type),
            GTP_PROTO_PKT_DATA_START_GET(pkt), GTP_PROTO_PKT_DATA_LEN_GET(pkt),
            GENERIC_TRANSMISSION_DATA_RX_CB_ST_OK); //lint !e826 Suspicious pointer-to-pointer area too small
    }

    return ret;
}

inline static void generic_transmission_prf_rx_cache_read_idx_inc(uint8_t io, uint16_t inc_size)
{
    if (s_generic_transmission_prf_env.rx_env.rx_cache_grp[io].read_idx +
            inc_size >= GENERIC_TRANSMISSION_PRF_RX_CACHE_SIZE) {
        s_generic_transmission_prf_env.rx_env.rx_cache_grp[io].read_idx  = inc_size -
            (GENERIC_TRANSMISSION_PRF_RX_CACHE_SIZE - s_generic_transmission_prf_env.rx_env.rx_cache_grp[io].read_idx);
    } else {
        s_generic_transmission_prf_env.rx_env.rx_cache_grp[io].read_idx  += inc_size;
    }

    s_generic_transmission_prf_env.rx_env.rx_cache_grp[io].read_size += inc_size;
}

static int32_t generic_transmission_prf_rx_process(uint8_t io)
{
    uint16_t pkt_len;
    uint16_t pkt_size;
    int16_t preamble_pos;
    uint8_t calc_hec;
    uint8_t *buf;
    int32_t ret;
    uint32_t search_size;
    int16_t cached_size;

    //check rx_cache pointer
    if (s_generic_transmission_prf_env.rx_env.rx_cache_grp[io].rx_cache == NULL) {
        uint8_t *p = generic_transmission_io_recv_cache_get(io);
        if (p) {
            s_generic_transmission_prf_env.rx_env.rx_cache_grp[io].rx_cache = p + GENERIC_TRANSMISSION_PRF_RX_CACHE_PREFIX_RSVD_SIZE;
        } else {
            return -RET_NOT_READY;
        }
    }

    cached_size = (int16_t)(s_generic_transmission_prf_env.rx_env.rx_cache_grp[io].write_size
                            - s_generic_transmission_prf_env.rx_env.rx_cache_grp[io].read_size);

    assert(cached_size >= 0);

    //the packet is not completed
    if (cached_size < (int16_t)(GTP_PKT_PREAMBLE_SYNC_LEN + GTP_PKT_HDR_LEN + GTP_PKT_CRC_LEN)) {
        return -RET_NOT_READY;
    }

    buf = s_generic_transmission_prf_env.rx_env.rx_cache_grp[io].rx_cache + s_generic_transmission_prf_env.rx_env.rx_cache_grp[io].read_idx;

    //the read idx is too approach to the buf end, it may cause preamble cross boundary, use extra cache
    if (s_generic_transmission_prf_env.rx_env.rx_cache_grp[io].read_idx
            + GENERIC_TRANSMISSION_PRF_PKT_MAX_RX_SIZE
        > GENERIC_TRANSMISSION_PRF_RX_CACHE_SIZE) {   //lint !e587 Always true
        uint16_t end_len = GENERIC_TRANSMISSION_PRF_RX_CACHE_SIZE
            - s_generic_transmission_prf_env.rx_env.rx_cache_grp[io].read_idx;
        //copy to extra cache
        memcpy(s_generic_transmission_prf_env.rx_env.rx_cache_grp[io].rx_cache - end_len,
               s_generic_transmission_prf_env.rx_env.rx_cache_grp[io].rx_cache
                   + s_generic_transmission_prf_env.rx_env.rx_cache_grp[io].read_idx,
               end_len);
        buf = s_generic_transmission_prf_env.rx_env.rx_cache_grp[io].rx_cache - end_len;
    }

    //search preamble
    search_size = (uint32_t)MIN(cached_size, (signed)GENERIC_TRANSMISSION_PRF_PKT_MAX_RX_SIZE);
    preamble_pos = (int16_t)_kmp_search(buf, (uint32_t)search_size,
                               (const uint8_t *)GTP_PKT_PREAMBLE_SYNC_VAL, GTP_PKT_PREAMBLE_SYNC_LEN,
                               s_generic_transmission_prf_env.preamble_next);

    if (preamble_pos < 0) {
        // skip the length, but maintian SYNC LEN of the tail
        generic_transmission_prf_rx_cache_read_idx_inc(io, (uint16_t)(search_size - GTP_PKT_PREAMBLE_SYNC_LEN));
        return -RET_NOT_READY;
    }

    // set buf to preamble pos
    buf += preamble_pos;
    cached_size -= preamble_pos;
    generic_transmission_prf_rx_cache_read_idx_inc(io, (uint16_t)preamble_pos);

    if (cached_size < (int16_t)(GTP_PKT_PREAMBLE_SYNC_LEN + GTP_PKT_HDR_LEN + GTP_PKT_CRC_LEN)) {
        return -RET_NOT_READY;
    }

    // if the read idx is too approach to the buf end, it may cause head cross boundary
    if ((uint32_t)(buf + GTP_PKT_PREAMBLE_SYNC_LEN - s_generic_transmission_prf_env.rx_env.rx_cache_grp[io].rx_cache) +
            GTP_PKT_HDR_LEN > GENERIC_TRANSMISSION_PRF_RX_CACHE_SIZE) {
        uint16_t end_len = GENERIC_TRANSMISSION_PRF_RX_CACHE_SIZE - s_generic_transmission_prf_env.rx_env.rx_cache_grp[io].read_idx;
        memcpy(s_generic_transmission_prf_env.rx_env.rx_cache_grp[io].rx_cache - end_len ,
                s_generic_transmission_prf_env.rx_env.rx_cache_grp[io].rx_cache + s_generic_transmission_prf_env.rx_env.rx_cache_grp[io].read_idx,
                end_len);
        buf = s_generic_transmission_prf_env.rx_env.rx_cache_grp[io].rx_cache - end_len;
    }

    //find pkt header
    pkt_len = GTP_PROTO_PKT_HDR_LEN_GET(buf + GTP_PKT_PREAMBLE_SYNC_LEN);   //lint !e826 Suspicious pointer-to-pointer area too small
    pkt_size = pkt_len + GTP_PKT_PREAMBLE_SYNC_LEN;

    // check HEC
    calc_hec = getcrc8(buf + GTP_PKT_PREAMBLE_SYNC_LEN, GTP_PKT_HDR_LEN - GTP_PKT_HEC_LEN);
    if (calc_hec != GTP_PROTO_PKT_HDR_HEC_GET(buf + GTP_PKT_PREAMBLE_SYNC_LEN)) {   //lint !e826 Suspicious pointer-to-pointer area too small
        generic_transmission_prf_rx_cache_read_idx_inc(io, GTP_PKT_PREAMBLE_LEN);
        return -RET_CRC_FAIL;
    }

    //TODO:version

    //if remain len less than pkt len, the packet is still not compileted
    if (cached_size < pkt_size) {
        return -RET_NOT_READY;
    }

    assert(pkt_size <= GENERIC_TRANSMISSION_PRF_PKT_MAX_RX_SIZE);

    buf += GTP_PKT_PREAMBLE_SYNC_LEN;
    generic_transmission_prf_rx_cache_read_idx_inc(io, GTP_PKT_PREAMBLE_SYNC_LEN);

    // if the read idx is too approach to the buf end, it may cause payload cross boundary
    if ((uint32_t)(buf + GTP_PKT_HDR_LEN - s_generic_transmission_prf_env.rx_env.rx_cache_grp[io].rx_cache) +
            pkt_len > GENERIC_TRANSMISSION_PRF_RX_CACHE_SIZE) {
        uint16_t end_len = GENERIC_TRANSMISSION_PRF_RX_CACHE_SIZE - s_generic_transmission_prf_env.rx_env.rx_cache_grp[io].read_idx;
        memcpy(s_generic_transmission_prf_env.rx_env.rx_cache_grp[io].rx_cache - end_len ,
                s_generic_transmission_prf_env.rx_env.rx_cache_grp[io].rx_cache + s_generic_transmission_prf_env.rx_env.rx_cache_grp[io].read_idx,
                end_len);
        buf = s_generic_transmission_prf_env.rx_env.rx_cache_grp[io].rx_cache - end_len;
    }

    // handle the rx pkt
    ret = generic_transmission_prf_rx_proto_process(io, buf, pkt_len);
    if (ret != RET_OK) {
        generic_transmission_prf_rx_cache_read_idx_inc(io, GTP_PKT_HDR_LEN);
    } else {
        // finnally, set rx cache length to remain length and move remain content to the head
        generic_transmission_prf_rx_cache_read_idx_inc(io, pkt_len);
    }

    return ret;
}

static int32_t generic_transmission_prf_rx_handler(uint8_t io, const uint8_t *buf, uint32_t len, bool_t in_isr) IRAM_TEXT(generic_transmission_prf_rx_handler);
static int32_t generic_transmission_prf_rx_handler(uint8_t io, const uint8_t *buf, uint32_t len, bool_t in_isr)
{
    uint32_t cur_write_idx;
    int16_t cached_size;

    //check rx_cache pointer
    if (s_generic_transmission_prf_env.rx_env.rx_cache_grp[io].rx_cache == NULL) {
        uint8_t *p = generic_transmission_io_recv_cache_get(io);
        if (p) {
            s_generic_transmission_prf_env.rx_env.rx_cache_grp[io].rx_cache = p + GENERIC_TRANSMISSION_PRF_RX_CACHE_PREFIX_RSVD_SIZE;
        } else {
            return -RET_NOT_READY;
        }
    }

    cached_size = (int16_t)(s_generic_transmission_prf_env.rx_env.rx_cache_grp[io].write_size
        - s_generic_transmission_prf_env.rx_env.rx_cache_grp[io].read_size);

    assert(cached_size >= 0);

    if ((uint16_t)cached_size + len > GENERIC_TRANSMISSION_PRF_RX_CACHE_SIZE) {
        return 0;//assert(0);
    }

    cur_write_idx = s_generic_transmission_prf_env.rx_env.rx_cache_grp[io].write_idx;

    if (cur_write_idx + len >= GENERIC_TRANSMISSION_PRF_RX_CACHE_SIZE) {
        memcpy(s_generic_transmission_prf_env.rx_env.rx_cache_grp[io].rx_cache + cur_write_idx,
               buf,
               GENERIC_TRANSMISSION_PRF_RX_CACHE_SIZE - cur_write_idx);

        //in this memcpy, the copy size may be zero, just let memcpy call, it will copy nothing and quickly return.
        memcpy(s_generic_transmission_prf_env.rx_env.rx_cache_grp[io].rx_cache,
               buf + (GENERIC_TRANSMISSION_PRF_RX_CACHE_SIZE - cur_write_idx),
               len - (GENERIC_TRANSMISSION_PRF_RX_CACHE_SIZE - cur_write_idx));

        s_generic_transmission_prf_env.rx_env.rx_cache_grp[io].write_idx =
            (uint16_t)(len - (GENERIC_TRANSMISSION_PRF_RX_CACHE_SIZE - cur_write_idx));
    } else {
        memcpy(s_generic_transmission_prf_env.rx_env.rx_cache_grp[io].rx_cache
                   + s_generic_transmission_prf_env.rx_env.rx_cache_grp[io].write_idx,
               buf, len);

        s_generic_transmission_prf_env.rx_env.rx_cache_grp[io].write_idx = (uint16_t)(cur_write_idx + len);
    }

    s_generic_transmission_prf_env.rx_env.rx_cache_grp[io].write_size += (uint16_t)len;

    if (in_isr) {
        os_post_semaphore_from_isr(s_generic_transmission_prf_env.rx_env.rx_notify_sem);
    } else {
        os_post_semaphore(s_generic_transmission_prf_env.rx_env.rx_notify_sem);
    }

    return RET_OK;
}

static void generic_transmission_prf_check_pkt_timeout(void)
{
    struct generic_transmission_prf_list_item *item;
    struct list_head *saved;
    uint32_t current_time = iot_timer_get_time();

    for (uint32_t i = 0; i < GENERIC_TRANSMISSION_PRF_ALL_TXQ_NUM; i++) {
        cpu_critical_enter();
        list_for_each_entry_safe(item, &s_generic_transmission_prf_env.tx_env.tx_list_grp[i].sending_list, node, saved) {
            if (current_time - item->timestamp >= GENERIC_TRANSMISSION_MS_TO_US(s_generic_transmission_prf_env.tx_env.fc_env.dyn_timeout_ms)) {
                list_del(&item->node);
                s_generic_transmission_prf_env.tx_env.tx_list_grp[i].sending_list_num--;

                list_add_tail(&item->node, &s_generic_transmission_prf_env.tx_env.tx_list_grp[i].retry_list);
                s_generic_transmission_prf_env.tx_env.tx_list_grp[i].retry_list_num++;

                //update timeout, once one packet timeout, it indicate the traffic may be heavy
                if (s_generic_transmission_prf_env.tx_env.fc_env.dyn_timeout_ms == GENERIC_TRANSMISSION_PRF_MAX_ACK_TIMEOUT_MS) {
                    //the dyn_timeout is already max, decrease dyn mtu size
                    s_generic_transmission_prf_env.tx_env.fc_env.dyn_mtu >>= 1;
                    if (s_generic_transmission_prf_env.tx_env.fc_env.dyn_mtu < GENERIC_TRANSMISSION_PRF_PKT_MIN_TX_MTU_SIZE) {
                        s_generic_transmission_prf_env.tx_env.fc_env.dyn_mtu = GENERIC_TRANSMISSION_PRF_PKT_MIN_TX_MTU_SIZE;
                    }
                } else {
                    s_generic_transmission_prf_env.tx_env.fc_env.dyn_timeout_ms <<= 1;
                    if (s_generic_transmission_prf_env.tx_env.fc_env.dyn_timeout_ms > GENERIC_TRANSMISSION_PRF_MAX_ACK_TIMEOUT_MS) {
                        s_generic_transmission_prf_env.tx_env.fc_env.dyn_timeout_ms = GENERIC_TRANSMISSION_PRF_MAX_ACK_TIMEOUT_MS;
                    }
                }
                s_generic_transmission_prf_env.tx_env.fc_env.last_ajust_time = iot_timer_get_time();
            }
        }
        cpu_critical_exit();
    }

    //check the how much time no timeout occurs, recover the flow control
    if (iot_timer_get_time() - s_generic_transmission_prf_env.tx_env.fc_env.last_ajust_time >
            GENERIC_TRANSMISSION_MS_TO_US(s_generic_transmission_prf_env.tx_env.fc_env.dyn_timeout_ms)) {
        if (s_generic_transmission_prf_env.tx_env.fc_env.dyn_timeout_ms == GENERIC_TRANSMISSION_PRF_MAX_ACK_TIMEOUT_MS) {
            //just increase dync TX_MTU
            s_generic_transmission_prf_env.tx_env.fc_env.dyn_mtu <<= 1;
            if (s_generic_transmission_prf_env.tx_env.fc_env.dyn_mtu > GENERIC_TRANSMISSION_PRF_PKT_MAX_TX_MTU_SIZE) {
                s_generic_transmission_prf_env.tx_env.fc_env.dyn_mtu = GENERIC_TRANSMISSION_PRF_PKT_MAX_TX_MTU_SIZE;
            }
        } else {
            // decrease timeout
            s_generic_transmission_prf_env.tx_env.fc_env.dyn_timeout_ms >>= 1;
            if (s_generic_transmission_prf_env.tx_env.fc_env.dyn_timeout_ms < GENERIC_TRANSMISSION_PRF_MIN_ACK_TIMEOUT_MS) {
                s_generic_transmission_prf_env.tx_env.fc_env.dyn_timeout_ms = GENERIC_TRANSMISSION_PRF_MIN_ACK_TIMEOUT_MS;
            }
        }

        s_generic_transmission_prf_env.tx_env.fc_env.last_ajust_time = iot_timer_get_time();
    }
}

static void generic_transmission_prf_ack_timeout_handler(timer_id_t timer_id, void *arg)
{
    UNUSED(timer_id);
    UNUSED(arg);

    GENERIC_TRANSMISSION_PRF_LOGI("[GTP] TO HDL\n");
    generic_transmission_prf_check_pkt_timeout();

    os_post_semaphore(s_generic_transmission_prf_env.tx_env.tx_notify_sem);
}

static void generic_transmission_prf_tx_task(void *arg)
{
    struct generic_transmission_prf_list_item *item;
    struct list_head *saved;
    struct list_head *list_hdl;
    uint8_t list_idx = 0;
#if CONFIG_GENERIC_TRANSMISSION_PRF_IO_TX_FAIL_ABANDON
    int32_t ret;
#endif

    UNUSED(arg);

    GENERIC_TRANSMISSION_PRF_LOGI("[GTP] Profile TX Task Run\n");

    while (1) {     //lint !e716 task main loop
        os_pend_semaphore(s_generic_transmission_prf_env.tx_env.tx_notify_sem, 0xFFFFFFFF);

        GENERIC_TRANSMISSION_PRF_LOGD("[GTP] handle tx list\n");

        // check which packet is timeout, add it to retry list
        os_stop_timer(s_generic_transmission_prf_env.tx_env.ack_timer);
        generic_transmission_prf_check_pkt_timeout();

        // handle retry list firstly
        while (!generic_transmission_prf_is_all_retry_list_empty()) {
            list_idx = generic_transmission_prf_get_max_prio_retry_list_idx();
            list_hdl = &s_generic_transmission_prf_env.tx_env.tx_list_grp[list_idx].retry_list;

            list_for_each_entry_safe(item, list_hdl, node, saved) {
#if CONFIG_GENERIC_TRANSMISSION_PRF_IO_TX_FAIL_ABANDON
                ret = generic_transmission_io_send(item->io, item->pkt, item->pkt_size);
#else
                generic_transmission_io_send(item->io, item->pkt, item->pkt_size);
#endif
                //Remove from retry list
                cpu_critical_enter();
                list_del(&item->node);
                s_generic_transmission_prf_env.tx_env.tx_list_grp[list_idx].retry_list_num--;
                cpu_critical_exit();

                // update timestamp
                item->timestamp = iot_timer_get_time();

#if CONFIG_GENERIC_TRANSMISSION_PRF_IO_TX_FAIL_ABANDON
                if (ret == RET_OK)
#endif
                {
                    //add pkt into sending list
                    cpu_critical_enter();
                    list_add_tail(&item->node, &s_generic_transmission_prf_env.tx_env.tx_list_grp[list_idx].sending_list);
                    s_generic_transmission_prf_env.tx_env.tx_list_grp[list_idx].sending_list_num++;
                    cpu_critical_exit();
                }
#if CONFIG_GENERIC_TRANSMISSION_PRF_IO_TX_FAIL_ABANDON
                else {
                    generic_transmission_prf_free_item(item);
                }
#endif

                GENERIC_TRANSMISSION_PRF_LOGD("[GTP] retry %d, type %02x\n", GTP_PROTO_PKT_HDR_SEQ_GET(item->pkt + GTP_PKT_PREAMBLE_SYNC_LEN),
                                                                             GTP_PROTO_PKT_HDR_TYPE_GET(item->pkt + GTP_PKT_PREAMBLE_SYNC_LEN));

                if (!generic_transmission_prf_is_all_retry_list_under_low_watermark() &&
                        generic_transmission_prf_is_retry_list_under_low_watermark(list_idx)) {
                    break; //break out of list for each entry
                }
            }
        }

        // handle wait list
        while (!generic_transmission_prf_is_all_wait_list_empty()) {
            list_idx = generic_transmission_prf_get_max_prio_wait_list_idx();
            list_hdl = &s_generic_transmission_prf_env.tx_env.tx_list_grp[list_idx].wait_list;

            list_for_each_entry_safe(item, list_hdl, node, saved) {
#if CONFIG_GENERIC_TRANSMISSION_PRF_IO_TX_FAIL_ABANDON
                ret = generic_transmission_io_send(item->io, item->pkt, item->pkt_size);
#else
                generic_transmission_io_send(item->io, item->pkt, item->pkt_size);
#endif
                //remove from wait list
                cpu_critical_enter();
                list_del(&item->node);
                s_generic_transmission_prf_env.tx_env.tx_list_grp[list_idx].wait_list_num--;
                cpu_critical_exit();

                item->timestamp = iot_timer_get_time();


                GENERIC_TRANSMISSION_PRF_LOGD("[GTP] tx %d, type %02x\n", GTP_PROTO_PKT_HDR_SEQ_GET(item->pkt + GTP_PKT_PREAMBLE_SYNC_LEN),
                                                                          GTP_PROTO_PKT_HDR_TYPE_GET(item->pkt + GTP_PKT_PREAMBLE_SYNC_LEN));

                /* 1. check if the packet need ack.
                 * 2. if the io is not exist, altough need_ack == 1, force free the packet to avoid the packet always wait in the tx list
                 */
#if CONFIG_GENERIC_TRANSMISSION_PRF_IO_TX_FAIL_ABANDON
                if (ret == RET_OK && GTP_PROTO_PKT_HDR_FC_NEED_ACK_GET(item->pkt + GTP_PKT_PREAMBLE_LEN)) {
#else
                if (GTP_PROTO_PKT_HDR_FC_NEED_ACK_GET(item->pkt + GTP_PKT_PREAMBLE_LEN)) {
#endif
                    //add pkt into sending list, then wait ack
                    cpu_critical_enter();
                    list_add_tail(&item->node, &s_generic_transmission_prf_env.tx_env.tx_list_grp[list_idx].sending_list);
                    s_generic_transmission_prf_env.tx_env.tx_list_grp[list_idx].sending_list_num++;
                    cpu_critical_exit();
                } else {
                    generic_transmission_prf_free_item(item);
                }

                if (!generic_transmission_prf_is_all_wait_list_under_low_watermark() &&
                        generic_transmission_prf_is_wait_list_under_low_watermark(list_idx)) {
                    break; //break out of list for each entry
                }
            }
        }

        // start time
        if (!generic_transmission_prf_is_all_sending_list_empty()) {
            os_start_timer(s_generic_transmission_prf_env.tx_env.ack_timer, s_generic_transmission_prf_env.tx_env.fc_env.dyn_timeout_ms);
        }
    }
}

static void generic_transmission_prf_rx_task(void *arg)
{
    UNUSED(arg);

    GENERIC_TRANSMISSION_PRF_LOGI("[GTP] Profile RX Task Run\n");

    while (1) {     //lint !e716 task main loop
        os_pend_semaphore(s_generic_transmission_prf_env.rx_env.rx_notify_sem, 0xFFFFFFFF);

        // if ret value is -RET_NOT_READY it means the packet is not complete or rx cache is NULL, should wait IO RX
        for (uint8_t io = 0; io < GENERIC_TRANSMISSION_IO_NUM; io++) {
            while (generic_transmission_prf_rx_process(io) != -RET_NOT_READY) {
                //nothing
            }
        }
    }
}

static void generic_transmission_prf_calc_txq_order(void)
{
    uint8_t max_prio = 0;
    uint8_t max_prio_tid = 0;
    uint32_t assigned_mask = 0;

    //the max prio is always for ctrl packet
    s_generic_transmission_prf_env.tx_env.txq_order[0] = GENERIC_TRANSMISSION_PRF_TXQ_CTRL_IDX;
    s_generic_transmission_prf_env.tx_env.txq_order[1] = GENERIC_TRANSMISSION_PRF_TXQ_MGMT_IDX;

    for (uint8_t i = 2; i < GENERIC_TRANSMISSION_PRF_ALL_TXQ_NUM; i++) {
        for (uint8_t j = 0; j < GENERIC_TRANSMISSION_PRF_DATA_TXQ_NUM; j++) {
            if (assigned_mask & BIT(j)) {
                continue;
            }
            if (max_prio <= s_generic_transmission_prf_env.tx_env.tid_prio[j]) {
                max_prio = s_generic_transmission_prf_env.tx_env.tid_prio[j];
                max_prio_tid = j;
            }
        }
        assigned_mask |= BIT(max_prio_tid);
        s_generic_transmission_prf_env.tx_env.txq_order[i] = max_prio_tid;

        GENERIC_TRANSMISSION_PRF_LOGI("TID Order: %d\n", max_prio_tid);
    }
}

int32_t generic_transmission_prf_set_tid_priority(uint8_t tid, uint8_t prio)
{
    s_generic_transmission_prf_env.tx_env.tid_prio[tid] = prio;

    generic_transmission_prf_calc_txq_order();

    return RET_OK;
}

int32_t generic_transmission_prf_register_rx_callback(uint8_t tid, const generic_transmission_data_rx_cb_t cb)
{
    s_generic_transmission_prf_env.rx_env.rx_cb[tid] = cb;

    return RET_OK;
}

void generic_transmission_prf_init(void)
{
    generic_transmission_io_init();
    generic_transmission_io_register_recv_callback(generic_transmission_prf_rx_handler);

    // tx env
    for (uint32_t i = 0; i < GENERIC_TRANSMISSION_PRF_ALL_TXQ_NUM; i++) {
        list_init(&s_generic_transmission_prf_env.tx_env.tx_list_grp[i].wait_list);
        list_init(&s_generic_transmission_prf_env.tx_env.tx_list_grp[i].sending_list);
        list_init(&s_generic_transmission_prf_env.tx_env.tx_list_grp[i].retry_list);
        s_generic_transmission_prf_env.tx_env.tx_list_grp[i].wait_list_num = 0;
        s_generic_transmission_prf_env.tx_env.tx_list_grp[i].sending_list_num = 0;
        s_generic_transmission_prf_env.tx_env.tx_list_grp[i].retry_list_num = 0;
        s_generic_transmission_prf_env.tx_env.tx_list_grp[i].next_tx_seq = 0;
        s_generic_transmission_prf_env.tx_env.tx_need_ack_st[i] = GENERIC_TRANSMISSION_PRF_NEED_ACK_ST_INVALID;
    }

    // rx env
    for (uint32_t i = 0; i < GENERIC_TRANSMISSION_PRF_ALL_TXQ_NUM; i++) {
        s_generic_transmission_prf_env.rx_env.rx_list_grp[i].expect_rx_seq = 0;
        s_generic_transmission_prf_env.rx_env.rx_need_ack_st[i] = GENERIC_TRANSMISSION_PRF_NEED_ACK_ST_INVALID;
    }

    for (uint8_t io = 0; io < GENERIC_TRANSMISSION_IO_NUM; io++) {
        uint8_t *p = generic_transmission_io_recv_cache_get(io);
        if (p) {
            s_generic_transmission_prf_env.rx_env.rx_cache_grp[io].rx_cache = p + GENERIC_TRANSMISSION_PRF_RX_CACHE_PREFIX_RSVD_SIZE;
        } else {
            s_generic_transmission_prf_env.rx_env.rx_cache_grp[io].rx_cache = NULL;
        }
        s_generic_transmission_prf_env.rx_env.rx_cache_grp[io].write_idx = 0;
        s_generic_transmission_prf_env.rx_env.rx_cache_grp[io].write_idx = 0;
        s_generic_transmission_prf_env.rx_env.rx_cache_grp[io].write_size = 0;
        s_generic_transmission_prf_env.rx_env.rx_cache_grp[io].read_idx = 0;
        s_generic_transmission_prf_env.rx_env.rx_cache_grp[io].read_size = 0;
    }

    generic_transmission_prf_calc_txq_order();

    s_generic_transmission_prf_env.tx_env.fc_env.dyn_timeout_ms = GENERIC_TRANSMISSION_PRF_MIN_ACK_TIMEOUT_MS;
    s_generic_transmission_prf_env.tx_env.fc_env.dyn_mtu = GENERIC_TRANSMISSION_PRF_PKT_DFT_TX_MTU_SIZE;
    s_generic_transmission_prf_env.tx_env.fc_env.cont_miss_ack_num = 0;

    s_generic_transmission_prf_env.tx_env.tx_notify_sem = os_create_semaphore(IOT_GENERIC_TRANSMISSION_MID, 1, 0);
    s_generic_transmission_prf_env.tx_env.data_push_sem = os_create_semaphore(IOT_GENERIC_TRANSMISSION_MID, 1, 0);
    s_generic_transmission_prf_env.tx_env.ctrl_push_sem = os_create_semaphore(IOT_GENERIC_TRANSMISSION_MID, 1, 0);

    s_generic_transmission_prf_env.tx_env.ack_timer = os_create_timer(IOT_GENERIC_TRANSMISSION_MID, false, generic_transmission_prf_ack_timeout_handler, NULL);

    s_generic_transmission_prf_env.rx_env.rx_notify_sem = os_create_semaphore(IOT_GENERIC_TRANSMISSION_MID, 1, 0);

    s_generic_transmission_prf_env.preamble_next = _kmp_gen_next((const uint8_t *)GTP_PKT_PREAMBLE_SYNC_VAL, GTP_PKT_PREAMBLE_SYNC_LEN);

    s_generic_transmission_prf_env.tx_env.tx_task_hdl = os_create_task_ext(generic_transmission_prf_tx_task, NULL, CONFIG_GENERIC_TRANSMISSION_PRF_TX_TASK_PRIO, 384, "gt_prf_tx_task");

    s_generic_transmission_prf_env.rx_env.rx_task_hdl = os_create_task_ext(generic_transmission_prf_rx_task, NULL, CONFIG_GENERIC_TRANSMISSION_PRF_RX_TASK_PRIO, 384, "gt_prf_rx_task");
}

void generic_transmission_prf_deinit(void)
{
    struct generic_transmission_prf_list_item *item;
    struct list_head *saved;

    // stop rx for IO
    generic_transmission_io_register_recv_callback(NULL);

    // stop & deinit timer
    if (s_generic_transmission_prf_env.tx_env.ack_timer) {
        os_stop_timer(s_generic_transmission_prf_env.tx_env.ack_timer);
        os_delete_timer(s_generic_transmission_prf_env.tx_env.ack_timer);
        s_generic_transmission_prf_env.tx_env.ack_timer = 0;
    }

    // delete task
    if (s_generic_transmission_prf_env.tx_env.tx_task_hdl) {
        os_delete_task(s_generic_transmission_prf_env.tx_env.tx_task_hdl);
        s_generic_transmission_prf_env.tx_env.tx_task_hdl = NULL;
    }

    // delete task
    if (s_generic_transmission_prf_env.rx_env.rx_task_hdl) {
        os_delete_task(s_generic_transmission_prf_env.rx_env.rx_task_hdl);
        s_generic_transmission_prf_env.rx_env.rx_task_hdl = NULL;
    }

    //delete semaphores
    if (s_generic_transmission_prf_env.tx_env.tx_notify_sem) {
        os_delete_semaphore(s_generic_transmission_prf_env.tx_env.tx_notify_sem);
        s_generic_transmission_prf_env.tx_env.tx_notify_sem = NULL;
    }

    if (s_generic_transmission_prf_env.tx_env.data_push_sem) {
        os_delete_semaphore(s_generic_transmission_prf_env.tx_env.data_push_sem);
        s_generic_transmission_prf_env.tx_env.data_push_sem = NULL;
    }

    if (s_generic_transmission_prf_env.tx_env.ctrl_push_sem) {
        os_delete_semaphore(s_generic_transmission_prf_env.tx_env.ctrl_push_sem);
        s_generic_transmission_prf_env.tx_env.ctrl_push_sem = NULL;
    }

    //delete semaphores
    if (s_generic_transmission_prf_env.rx_env.rx_notify_sem) {
        os_delete_semaphore(s_generic_transmission_prf_env.rx_env.rx_notify_sem);
        s_generic_transmission_prf_env.rx_env.rx_notify_sem = NULL;
    }

    //clear tx list
    for (uint32_t i = 0; i < GENERIC_TRANSMISSION_PRF_ALL_TXQ_NUM; i++) {
        //wait list
        list_for_each_entry_safe(item, &s_generic_transmission_prf_env.tx_env.tx_list_grp[i].wait_list, node, saved) {
            list_del(&item->node);

            os_mem_free(item->pkt);
            os_mem_free(item);
        }
        //sending list
        list_for_each_entry_safe(item, &s_generic_transmission_prf_env.tx_env.tx_list_grp[i].sending_list, node, saved) {
            list_del(&item->node);

            os_mem_free(item->pkt);
            os_mem_free(item);
        }
        //retry list
        list_for_each_entry_safe(item, &s_generic_transmission_prf_env.tx_env.tx_list_grp[i].retry_list, node, saved) {
            list_del(&item->node);

            os_mem_free(item->pkt);
            os_mem_free(item);
        }
    }

    //free preamble next arrary of KMP
    os_mem_free(s_generic_transmission_prf_env.preamble_next);

    // clear s_generic_transmission_prf_env
    memset(&s_generic_transmission_prf_env, 0, sizeof(s_generic_transmission_prf_env));

    //deinit io
    generic_transmission_io_deinit();
}

int32_t generic_transmission_prf_tx_set_priority(uint8_t priority)
{
    if (s_generic_transmission_prf_env.tx_env.tx_task_hdl) {
        os_set_task_prio(s_generic_transmission_prf_env.tx_env.tx_task_hdl, priority);
    } else {
        return -1;
    }
    return 0;
}

int32_t generic_transmission_prf_tx_restore_priority(void)
{
    if (s_generic_transmission_prf_env.tx_env.tx_task_hdl) {
        os_set_task_prio(s_generic_transmission_prf_env.tx_env.tx_task_hdl, CONFIG_GENERIC_TRANSMISSION_PRF_TX_TASK_PRIO);
    } else {
        return -1;
    }
    return 0;
}
