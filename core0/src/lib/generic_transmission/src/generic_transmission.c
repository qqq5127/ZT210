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
#include "riscv_cpu.h"
#include "lib_dbglog.h"
#include "critical_sec.h"

#include "os_task.h"
#include "os_lock.h"
#include "os_mem.h"

#include "iot_memory_config.h"
#include "iot_ipc.h"
#include "iot_rtc.h"

#include "generic_transmission_api.h"

#ifdef BUILD_CORE_CORE0
#include "generic_transmission_profile.h"
#include "generic_transmission_io_uart.h"
#include "generic_transmission_io_flash.h"
#endif

/******** Config Start ********/
#define CONFIG_GENERIC_TRANSMISSION_IOT_PRINTF         0
#define CONFIG_GENERIC_TRANSMISSION_BUF_DEBUG          0

/******** Config End ********/

#if (CONFIG_GENERIC_TRANSMISSION_BUF_DEBUG && CONFIG_GENERIC_TRANSMISSION_IOT_PRINTF)
#define GENERIC_TRANSMISSION_BUF_LOGD(fmt, arg...)   iot_printf(fmt, ##arg)
#define GENERIC_TRANSMISSION_BUF_LOGI(fmt, arg...)   iot_printf(fmt, ##arg)
#define GENERIC_TRANSMISSION_BUF_LOGE(fmt, arg...)   iot_printf(fmt, ##arg)
#else
#define GENERIC_TRANSMISSION_BUF_LOGD(fmt, arg...)
#define GENERIC_TRANSMISSION_BUF_LOGI(fmt, arg...)
#define GENERIC_TRANSMISSION_BUF_LOGE(fmt, arg...)
#endif

#define GENERIC_TRANSMISSION_BUF_MAGIC     (0x42535447)         //ascii of "GTSB"
#define GENERIC_TRANSMISSION_BUF_CORE_DTOP 0
#define GENERIC_TRANSMISSION_BUF_CORE_BT   1
#define GENERIC_TRANSMISSION_BUF_CORE_DSP  2
#define GENERIC_TRANSMISSION_BUF_CORE_NUM  3

#if defined(BUILD_CORE_CORE0)
#define GTB_CORE_SELF GENERIC_TRANSMISSION_BUF_CORE_DTOP
#elif defined(BUILD_CORE_CORE1)
#define GTB_CORE_SELF GENERIC_TRANSMISSION_BUF_CORE_BT
#else
#define GTB_CORE_SELF GENERIC_TRANSMISSION_BUF_CORE_DSP
#endif

#define GENERIC_TRANSMISSION_BUF_PREFIX_PANIC_SAVED_SIZE    12
#define GENERIC_TRANSMISSION_BUF_SUFFIX_PANIC_SAVED_SIZE    4
#define GENERIC_TRANSMISSION_BUF_PREFIX_EXTRA_SIZE          256
#define GENERIC_TRANSMISSION_BUF_SUFFIX_EXTRA_SIZE          0

#define GENERIC_TRANSMISSION_BUF_FIFO_START_OFFSET                               \
    (GENERIC_TRANSMISSION_CTRL_SIZE + GENERIC_TRANSMISSION_BUF_PREFIX_EXTRA_SIZE \
     + GENERIC_TRANSMISSION_BUF_PREFIX_PANIC_SAVED_SIZE)

/*lint -emacro(835, GENERIC_TRANSMISSION_BUF_ALLCTRL_SIZE) `+0` */
#define GENERIC_TRANSMISSION_BUF_ALLCTRL_SIZE                                    \
    (GENERIC_TRANSMISSION_CTRL_SIZE + GENERIC_TRANSMISSION_BUF_PREFIX_EXTRA_SIZE \
     + GENERIC_TRANSMISSION_BUF_SUFFIX_EXTRA_SIZE                                \
     + GENERIC_TRANSMISSION_BUF_PREFIX_PANIC_SAVED_SIZE                          \
     + GENERIC_TRANSMISSION_BUF_SUFFIX_PANIC_SAVED_SIZE)

struct generic_transmission_buf_ctrl_tag {
    uint32_t magic;                  //magic number to indicate the share memory is inited or not
    volatile uint32_t write_idx;     //32bits to avoid write idx and read idx in the same 1-word.
    volatile uint32_t write_size;    //total size since write
    volatile uint32_t read_idx;      //32bits to avoid write idx and read idx in the same 1-word.
    volatile uint32_t read_size;     //total size since read
    volatile uint8_t status;         // status of this share memory
};

struct generic_transmission_buf_env_tag {
    struct generic_transmission_buf_ctrl_tag *ctrl;
    uint8_t *fifo;
    uint32_t fifo_len;
};

#ifdef BUILD_CORE_CORE0
struct generic_transmission_consumer_env_tag {
    os_task_h task_hdl;
    os_sem_h notify_sem;
    uint32_t last_timestamp;
    bool_t panic_process_share_memory;
};
#endif

typedef struct {
    uint16_t len;
    uint16_t type:3;
    uint16_t tid:3;
    uint16_t io:3;
    uint16_t need_ack:1;
    uint16_t mode:1;
    uint16_t reserved:5;
    uint32_t timestamp;
} __attribute__((packed)) generic_transmission_buf_hdr_t;

static struct generic_transmission_buf_env_tag s_generic_transmission_buf_env[GENERIC_TRANSMISSION_BUF_CORE_NUM];

static generic_transmission_repack_cb_t generic_transmission_repack_cb[GENERIC_TRANSMISSION_DATA_TYPE_NUM] = {0};

#ifdef BUILD_CORE_CORE0
struct generic_transmission_consumer_env_tag s_generic_transmission_consumer_env;
#endif

inline static void generic_transmission_buf_fifo_set_status(uint8_t idx, uint8_t status)
{
    if (s_generic_transmission_buf_env[idx].ctrl) {
        s_generic_transmission_buf_env[idx].ctrl->status = status;
    }
}

inline static bool_t generic_transmission_buf_fifo_check_and_set_status(uint8_t idx, uint8_t check_st, uint8_t status)
{
    if (s_generic_transmission_buf_env[idx].ctrl &&
            s_generic_transmission_buf_env[idx].ctrl->status == check_st) {
        s_generic_transmission_buf_env[idx].ctrl->status = status;
        return true;
    }

    return false;
}

inline static void generic_transmission_buf_fifo_reset(uint8_t idx)
{
    if (s_generic_transmission_buf_env[idx].ctrl) {
        s_generic_transmission_buf_env[idx].ctrl->magic = GENERIC_TRANSMISSION_BUF_MAGIC;
        s_generic_transmission_buf_env[idx].ctrl->write_idx =  0;
        s_generic_transmission_buf_env[idx].ctrl->write_size = 0;
        s_generic_transmission_buf_env[idx].ctrl->read_size =  0;
        s_generic_transmission_buf_env[idx].ctrl->read_idx =   0;
    }
}

#ifdef BUILD_CORE_CORE0

static int32_t generic_transmission_consumer_data_tx(const uint8_t *data, uint32_t data_len,
                                                 const generic_transmission_prf_data_tx_param_t *param)
{
    int32_t ret;
    if (s_generic_transmission_buf_env[GTB_CORE_SELF].ctrl->status >= GTB_ST_IN_PANIC) {
        /* Use variable array in generic_transmission_prf_data_tx_panic to avoid dual core write share memory in panic status
         * Variable array object consumes ISR stack size .
         */
        ret = generic_transmission_prf_data_tx_panic(data, data_len, param);
    } else {
        ret = generic_transmission_prf_data_tx(data, data_len, param);
    }

    return ret;
}

static int32_t generic_transmission_consumer_tx_get_head(uint32_t core_id, generic_transmission_buf_hdr_t *buf_hdr) IRAM_TEXT(generic_transmission_consumer_tx_get_head);
static int32_t generic_transmission_consumer_tx_get_head(uint32_t core_id, generic_transmission_buf_hdr_t *buf_hdr)
{
    int32_t fifo_cached_size;
    uint32_t cur_read_idx;

    if (s_generic_transmission_buf_env[core_id].ctrl == NULL ||
            s_generic_transmission_buf_env[core_id].ctrl->magic != GENERIC_TRANSMISSION_BUF_MAGIC) {
        return -RET_NOT_READY;
    }

    fifo_cached_size = (int32_t)(s_generic_transmission_buf_env[core_id].ctrl->write_size  -
                           s_generic_transmission_buf_env[core_id].ctrl->read_size);

    if (fifo_cached_size < (int32_t)sizeof(generic_transmission_buf_hdr_t)) {
        return -RET_NOT_READY;
    }

    cur_read_idx = s_generic_transmission_buf_env[core_id].ctrl->read_idx;

    //seperate twice to read buf header
    if (cur_read_idx + sizeof(generic_transmission_buf_hdr_t) >= s_generic_transmission_buf_env[core_id].fifo_len) {
        memcpy(buf_hdr,
                s_generic_transmission_buf_env[core_id].fifo + cur_read_idx,
                s_generic_transmission_buf_env[core_id].fifo_len - cur_read_idx);
        //in this memcpy, the copy size may be zero, just let memcpy call, it will copy nothing and quickly return.
        memcpy(((uint8_t *)buf_hdr) + (s_generic_transmission_buf_env[core_id].fifo_len - cur_read_idx),
                s_generic_transmission_buf_env[core_id].fifo,
                sizeof(generic_transmission_buf_hdr_t) - (s_generic_transmission_buf_env[core_id].fifo_len - cur_read_idx));

        cur_read_idx = sizeof(generic_transmission_buf_hdr_t) - (s_generic_transmission_buf_env[core_id].fifo_len - cur_read_idx);
    } else {
        memcpy(buf_hdr, s_generic_transmission_buf_env[core_id].fifo + cur_read_idx, sizeof(generic_transmission_buf_hdr_t));
        cur_read_idx += sizeof(generic_transmission_buf_hdr_t);
    }

    return (int32_t)cur_read_idx;
}

static int32_t generic_transmission_consumer_tx_process_next(void) IRAM_TEXT(generic_transmission_consumer_tx_process_next);
static int32_t generic_transmission_consumer_tx_process_next(void)
{
    generic_transmission_prf_data_tx_param_t param;
    generic_transmission_buf_hdr_t cmp_buf_hdr;
    generic_transmission_buf_hdr_t earliest_buf_hdr;
    uint32_t earliest_delta = 0xFFFFFFFFUL;
    uint32_t earliest_timestamp = 0;
    uint32_t earliest_core = 0xFFFFFFFFUL;
    uint32_t cur_read_idx = 0;
    int32_t ret;

    //get earliest buf according to timestamp
    for (uint32_t core_id = 0; core_id < GENERIC_TRANSMISSION_BUF_CORE_NUM; core_id++) {
        ret = generic_transmission_consumer_tx_get_head(core_id, &cmp_buf_hdr);
        if (ret < 0) {
            continue;
        }

        GENERIC_TRANSMISSION_BUF_LOGD("[GTP] CHK: c %d, ts %u\n", core_id, cmp_buf_hdr.timestamp);

        if (cmp_buf_hdr.timestamp - s_generic_transmission_consumer_env.last_timestamp < earliest_delta) {
            earliest_delta = cmp_buf_hdr.timestamp - s_generic_transmission_consumer_env.last_timestamp;
            earliest_timestamp = cmp_buf_hdr.timestamp;
            earliest_core = core_id;
            cur_read_idx = (uint32_t)ret;
            memcpy(&earliest_buf_hdr, &cmp_buf_hdr, sizeof(generic_transmission_buf_hdr_t));
        }
    }

    if (earliest_core >= GENERIC_TRANSMISSION_BUF_CORE_NUM) {
        return -RET_NOT_READY;
    }

    GENERIC_TRANSMISSION_BUF_LOGD("[GTP] Select c %d, t %d, i %d, ts %u\n", earliest_core, earliest_buf_hdr.type, earliest_buf_hdr.tid, earliest_timestamp);

    //update last_timestamp
    s_generic_transmission_consumer_env.last_timestamp = earliest_timestamp;

    //set param
    param.io = earliest_buf_hdr.io;     //lint !e644 earliest_buf_hdr initialized by memcpy
    param.mode = earliest_buf_hdr.mode;
    param.type = earliest_buf_hdr.type;
    param.tid = earliest_buf_hdr.tid;
    param.need_ack = earliest_buf_hdr.need_ack == 1;
    //seperate twice to read buf data
    if (cur_read_idx + earliest_buf_hdr.len >= s_generic_transmission_buf_env[earliest_core].fifo_len) {
        if (earliest_buf_hdr.mode == GENERIC_TRANSMISSION_TX_MODE_LAZY) {
            //LAZY mode should make the tx data complete
            if (s_generic_transmission_buf_env[earliest_core].fifo_len - cur_read_idx <= GENERIC_TRANSMISSION_BUF_PREFIX_EXTRA_SIZE) {

                GENERIC_TRANSMISSION_BUF_LOGD("[GTP] extra copy: %d\n", s_generic_transmission_buf_env[earliest_core].fifo_len - cur_read_idx);
                //copy the first frag to extra area, so the buf is consequent
                memcpy(s_generic_transmission_buf_env[earliest_core].fifo - (s_generic_transmission_buf_env[earliest_core].fifo_len - cur_read_idx),
                        s_generic_transmission_buf_env[earliest_core].fifo + cur_read_idx,
                        s_generic_transmission_buf_env[earliest_core].fifo_len - cur_read_idx);

                generic_transmission_consumer_data_tx(s_generic_transmission_buf_env[earliest_core].fifo -
                                                          (s_generic_transmission_buf_env[earliest_core].fifo_len - cur_read_idx),
                                                      earliest_buf_hdr.len,
                                                      &param);
            } else if (s_generic_transmission_buf_env[GTB_CORE_SELF].ctrl->status < GTB_ST_IN_PANIC) {
                uint8_t *new_buf;
                GENERIC_TRANSMISSION_BUF_LOGD("[GTP] malloc copy: %d\n", earliest_buf_hdr.len);
                //if the extra area is not enough, then use malloc, this branch should be unlikely to execute
                new_buf = os_mem_malloc(IOT_GENERIC_TRANSMISSION_MID, earliest_buf_hdr.len);
                if (new_buf) {
                    //copy first frag
                    memcpy(new_buf,
                           s_generic_transmission_buf_env[earliest_core].fifo + cur_read_idx,
                           s_generic_transmission_buf_env[earliest_core].fifo_len - cur_read_idx);

                    //copy next frag
                    memcpy(new_buf + s_generic_transmission_buf_env[earliest_core].fifo_len - cur_read_idx,

                           s_generic_transmission_buf_env[earliest_core].fifo,
                           earliest_buf_hdr.len - (s_generic_transmission_buf_env[earliest_core].fifo_len - cur_read_idx));

                    generic_transmission_consumer_data_tx(new_buf,
                                                          earliest_buf_hdr.len,
                                                          &param);
                    //free new buf
                    os_mem_free(new_buf);
                } else {
                    //do nothing, discard the buf
                    GENERIC_TRANSMISSION_BUF_LOGE("[GTP] Consumer: missing, len %d\n", earliest_buf_hdr.len);
                }
            } else {
                //missing in panic
            }
        } else {
            //ASAP mode support separate the data to tx
            generic_transmission_consumer_data_tx(s_generic_transmission_buf_env[earliest_core].fifo + cur_read_idx,
                                                  s_generic_transmission_buf_env[earliest_core].fifo_len - cur_read_idx,
                                                  &param);

            s_generic_transmission_buf_env[earliest_core].ctrl->read_idx = 0;

            // this push length may be zero, just let the function call, it will do nothing and quickly return.
            generic_transmission_consumer_data_tx(s_generic_transmission_buf_env[earliest_core].fifo,
                                               earliest_buf_hdr.len - (s_generic_transmission_buf_env[earliest_core].fifo_len - cur_read_idx),
                                               &param);
        }

        cur_read_idx = earliest_buf_hdr.len - (s_generic_transmission_buf_env[earliest_core].fifo_len - cur_read_idx);
    } else {
        generic_transmission_consumer_data_tx(s_generic_transmission_buf_env[earliest_core].fifo + cur_read_idx,
                                           earliest_buf_hdr.len,
                                           &param);

        cur_read_idx += earliest_buf_hdr.len;
    }

    s_generic_transmission_buf_env[earliest_core].ctrl->read_idx = cur_read_idx;
    s_generic_transmission_buf_env[earliest_core].ctrl->read_size += (sizeof(generic_transmission_buf_hdr_t) + earliest_buf_hdr.len);

    GENERIC_TRANSMISSION_BUF_LOGD("[GTP] TX end, wr_idx %d, wr_size %d, rd_idx %d, rd_size %d, fifo_len %d\n",
                       s_generic_transmission_buf_env[earliest_core].ctrl->write_idx,
                       s_generic_transmission_buf_env[earliest_core].ctrl->write_size,
                       s_generic_transmission_buf_env[earliest_core].ctrl->read_idx,
                       s_generic_transmission_buf_env[earliest_core].ctrl->read_size,
                       s_generic_transmission_buf_env[earliest_core].fifo_len
                      );

    return RET_OK;
}

static void generic_transmission_consumer_tx_process(void)
{
    while (generic_transmission_consumer_tx_process_next() == RET_OK) {};
}

static bool_t generic_transmission_consumer_is_tx_process_panic_end(void)
{
    bool_t is_panic_end = true;

    // don't check status of DTOP core here prevent GTP from blocking
    for (int32_t core_idx = 1; core_idx < GENERIC_TRANSMISSION_BUF_CORE_NUM; core_idx++) {
        if (s_generic_transmission_buf_env[core_idx].ctrl &&
                s_generic_transmission_buf_env[core_idx].ctrl->status == GTB_ST_IN_PANIC) {
            is_panic_end = false;
            break;
        }
    }

    return is_panic_end;
}

void generic_transmission_consumer_tx_process_panic(void) IRAM_TEXT(generic_transmission_consumer_tx_process_panic);
void generic_transmission_consumer_tx_process_panic(void)
{
    //flush the data already in tx list,
    //TODO: warning, there'a very low ratio the same packet tx twice
    generic_transmission_prf_tx_flush_panic();

    //handle share memory
    while (!generic_transmission_consumer_is_tx_process_panic_end()) {
        generic_transmission_consumer_tx_process_next();
    }
}

static void generic_transmission_consumer_task(void *arg)
{
    UNUSED(arg);

    GENERIC_TRANSMISSION_BUF_LOGI("[GTP] Consumer Task Run\n");

    while (1) {     //lint !e716 task main loop
        os_pend_semaphore(s_generic_transmission_consumer_env.notify_sem, 0xFFFFFFFF);
        generic_transmission_consumer_tx_process();
    }
}

static int32_t generic_transmission_consumer_ipc_handler(IPC_CORES src_cpu, const void *payload) IRAM_TEXT(generic_transmission_consumer_ipc_handler);
static int32_t generic_transmission_consumer_ipc_handler(IPC_CORES src_cpu, const void *payload)
{
    UNUSED(src_cpu);
    UNUSED(payload);

    os_post_semaphore_from_isr(s_generic_transmission_consumer_env.notify_sem);

    return RET_OK;
}

static int32_t generic_transmission_do_panic_handler(IPC_CORES src_cpu, const void *payload) IRAM_TEXT(generic_transmission_do_panic_handler);
static int32_t generic_transmission_do_panic_handler(IPC_CORES src_cpu, const void *payload)
{
    UNUSED(src_cpu);
    UNUSED(payload);
    assert(0);

    return RET_OK;      //lint !e527 Reachable when assert is empty
}

static void generic_transmission_consumer_init(void)
{
    GENERIC_TRANSMISSION_BUF_LOGI("[GTP] Init...\n");

    iot_ipc_register_handler(IPC_TYPE_GENERIC_TRANSMISSION, generic_transmission_consumer_ipc_handler);
    iot_ipc_register_handler(IPC_TYPE_PANIC_REQ, generic_transmission_do_panic_handler);

    s_generic_transmission_consumer_env.panic_process_share_memory = true;

    s_generic_transmission_consumer_env.last_timestamp = iot_rtc_get_global_time();

    s_generic_transmission_consumer_env.notify_sem = os_create_semaphore(IOT_GENERIC_TRANSMISSION_MID, 1, 0);

    s_generic_transmission_consumer_env.task_hdl =  os_create_task_ext(generic_transmission_consumer_task,
                                                            NULL,
                                                            CONFIG_GENERIC_TRANSMISSION_CONSUMER_TASK_PRIO,
                                                            512,
                                                            "generic_transmission_consumer");

    generic_transmission_prf_init();
}

static void generic_transmission_consumer_deinit(void)
{
    if (s_generic_transmission_consumer_env.task_hdl) {
        os_delete_task(s_generic_transmission_consumer_env.task_hdl);
        s_generic_transmission_consumer_env.task_hdl = NULL;
    }

    if (s_generic_transmission_consumer_env.notify_sem) {
        os_delete_semaphore(s_generic_transmission_consumer_env.notify_sem);
        s_generic_transmission_consumer_env.notify_sem = NULL;
    }

    s_generic_transmission_consumer_env.panic_process_share_memory = false;

    generic_transmission_prf_deinit();

    GENERIC_TRANSMISSION_BUF_LOGI("[GTP] Deinited\n");
}

#endif

static void generic_transmission_notify(bool_t critical)
{
    GENERIC_TRANSMISSION_BUF_LOGD("[GTP] Notify from core\n");
#ifdef BUILD_CORE_CORE0
    if (critical) {
        if(in_irq() || !in_scheduling()) {
            os_post_semaphore_from_isr(s_generic_transmission_consumer_env.notify_sem);
        } else if(!cpu_get_int_enable()) {
            os_post_semaphore_from_critical(s_generic_transmission_consumer_env.notify_sem);
        } else {
            assert(0);
        }
    } else {
        os_post_semaphore(s_generic_transmission_consumer_env.notify_sem);
    }
#else
    UNUSED(critical);
    while ((s_generic_transmission_buf_env[GENERIC_TRANSMISSION_BUF_CORE_DTOP].ctrl->status < GTB_ST_IN_PANIC) &&
           (iot_ipc_send_message(DTOP_CORE, IPC_TYPE_GENERIC_TRANSMISSION, NULL, 0, false) != RET_OK));
#endif
}

// API implement
//return pushed length, < 0 means error, =0 means pushed nothing
static int32_t generic_transmission_data_tx_implement(generic_transmission_tx_mode_t mode,
                                                  generic_transmission_data_type_t type,
                                                  generic_transmission_tid_t tid,
                                                  generic_transmission_io_t io,
                                                  const uint8_t *data, uint32_t data_len,
                                                  bool_t need_ack,
                                                  bool_t critical) IRAM_TEXT(generic_transmission_data_tx_implement);
static int32_t generic_transmission_data_tx_implement(generic_transmission_tx_mode_t mode,
                                                  generic_transmission_data_type_t type,
                                                  generic_transmission_tid_t tid,
                                                  generic_transmission_io_t io,
                                                  const uint8_t *data, uint32_t data_len,
                                                  bool_t need_ack,
                                                  bool_t critical)
{
    int32_t fifo_cached_size;
    uint32_t write_len;
    uint32_t cur_write_idx;
    generic_transmission_buf_hdr_t buf_hdr;
    bool_t to_notify = false;

    GENERIC_TRANSMISSION_BUF_LOGD("[GTP] TX, wr_idx %d, wr_size %d, rd_idx %d, rd_size %d, fifo_len %d\n",
                       s_generic_transmission_buf_env[GTB_CORE_SELF].ctrl->write_idx,
                       s_generic_transmission_buf_env[GTB_CORE_SELF].ctrl->write_size,
                       s_generic_transmission_buf_env[GTB_CORE_SELF].ctrl->read_idx,
                       s_generic_transmission_buf_env[GTB_CORE_SELF].ctrl->read_size,
                       s_generic_transmission_buf_env[GTB_CORE_SELF].fifo_len
                      );

    /* 1.  os_try_acquire_mutex() declare comment indicate it cannot be called in ISR context,
     *     although the implementation of os_try_acquire_mutex seems safe in critical context.
     * 2. os_shim didn't implement semaphore take from ISR related functions. Besides semaphore
     *    take will cause task priority invert problem.
     * As the reasons above, use cpu_critical_enter/exit to implement the function be called both
     * in ISR context and task context.
     */
    cpu_critical_enter();

    fifo_cached_size = (int32_t)(s_generic_transmission_buf_env[GTB_CORE_SELF].ctrl->write_size  -
                       s_generic_transmission_buf_env[GTB_CORE_SELF].ctrl->read_size);

    assert(fifo_cached_size >= 0);

    if (s_generic_transmission_buf_env[GTB_CORE_SELF].fifo_len - (uint32_t)fifo_cached_size <= sizeof(generic_transmission_buf_hdr_t)) {
        cpu_critical_exit();
        return -RET_NOMEM;
    }

    if ((mode == GENERIC_TRANSMISSION_TX_MODE_LAZY) &&
            (s_generic_transmission_buf_env[GTB_CORE_SELF].fifo_len - (uint32_t)fifo_cached_size) < sizeof(generic_transmission_buf_hdr_t) + data_len) {
        cpu_critical_exit();
        return -RET_NOMEM;
    }

    if (generic_transmission_repack_cb[type]) {
        if (!(generic_transmission_repack_cb[type](data))) {
            cpu_critical_exit();
            return -RET_INVAL;
        }
    }

    write_len = ((s_generic_transmission_buf_env[GTB_CORE_SELF].fifo_len - (uint32_t)fifo_cached_size) < data_len + sizeof(generic_transmission_buf_hdr_t)) ?
                ((s_generic_transmission_buf_env[GTB_CORE_SELF].fifo_len - (uint32_t)fifo_cached_size) - sizeof(generic_transmission_buf_hdr_t)) :
                data_len;

    cur_write_idx = s_generic_transmission_buf_env[GTB_CORE_SELF].ctrl->write_idx;

    buf_hdr.len = (uint16_t)write_len;
    buf_hdr.type = type;
    buf_hdr.tid = tid;
    buf_hdr.io = io;
    buf_hdr.need_ack = need_ack ? 1 : 0;
    buf_hdr.mode = mode;
    buf_hdr.timestamp = iot_rtc_get_global_time();

    if (cur_write_idx + sizeof(generic_transmission_buf_hdr_t) >= s_generic_transmission_buf_env[GTB_CORE_SELF].fifo_len) {
        memcpy(s_generic_transmission_buf_env[GTB_CORE_SELF].fifo + cur_write_idx,
               &buf_hdr,
               s_generic_transmission_buf_env[GTB_CORE_SELF].fifo_len - cur_write_idx);

        //in this memcpy, the copy size may be zero, just let memcpy call, it will copy nothing and quickly return.
        memcpy(s_generic_transmission_buf_env[GTB_CORE_SELF].fifo,
               ((uint8_t *)&buf_hdr) + (s_generic_transmission_buf_env[GTB_CORE_SELF].fifo_len - cur_write_idx),
               sizeof(generic_transmission_buf_hdr_t) - (s_generic_transmission_buf_env[GTB_CORE_SELF].fifo_len - cur_write_idx));

        cur_write_idx = sizeof(generic_transmission_buf_hdr_t) - (s_generic_transmission_buf_env[GTB_CORE_SELF].fifo_len - cur_write_idx);
    } else {
        memcpy(s_generic_transmission_buf_env[GTB_CORE_SELF].fifo + cur_write_idx, &buf_hdr, sizeof(generic_transmission_buf_hdr_t));

        cur_write_idx += sizeof(generic_transmission_buf_hdr_t);;
    }

    if (cur_write_idx + write_len >=
            s_generic_transmission_buf_env[GTB_CORE_SELF].fifo_len) {

        memcpy(s_generic_transmission_buf_env[GTB_CORE_SELF].fifo + cur_write_idx,
               data,
               s_generic_transmission_buf_env[GTB_CORE_SELF].fifo_len - cur_write_idx);

        //in this memcpy, the copy size may be zero, just let memcpy call, it will copy nothing and quickly return.
        memcpy(s_generic_transmission_buf_env[GTB_CORE_SELF].fifo,
               data + (s_generic_transmission_buf_env[GTB_CORE_SELF].fifo_len - cur_write_idx),
               write_len - (s_generic_transmission_buf_env[GTB_CORE_SELF].fifo_len - cur_write_idx));

        s_generic_transmission_buf_env[GTB_CORE_SELF].ctrl->write_idx = write_len - (s_generic_transmission_buf_env[GTB_CORE_SELF].fifo_len - cur_write_idx);
    } else {

        memcpy(s_generic_transmission_buf_env[GTB_CORE_SELF].fifo + cur_write_idx,
               data,
               write_len);

        s_generic_transmission_buf_env[GTB_CORE_SELF].ctrl->write_idx = cur_write_idx + write_len;
    }

    s_generic_transmission_buf_env[GTB_CORE_SELF].ctrl->write_size += (sizeof(generic_transmission_buf_hdr_t) + write_len);

    /* update fifo cache size to decide do notify or not.
     * After write size update, then check old fifo cached size is safe.
     */
    if (s_generic_transmission_buf_env[GTB_CORE_SELF].ctrl->write_size ==
                (sizeof(generic_transmission_buf_hdr_t) + write_len) + s_generic_transmission_buf_env[GTB_CORE_SELF].ctrl->read_size) {
        to_notify = true;
    }

    cpu_critical_exit();

    if ((s_generic_transmission_buf_env[GENERIC_TRANSMISSION_BUF_CORE_DTOP].ctrl->status < GTB_ST_IN_PANIC) && to_notify) {
        generic_transmission_notify(critical);
    }

    return (int32_t)write_len;
}

int32_t generic_transmission_data_tx(generic_transmission_tx_mode_t mode,
                                 generic_transmission_data_type_t type,
                                 generic_transmission_tid_t tid,
                                 generic_transmission_io_t io,
                                 const uint8_t *data, uint32_t data_len,
                                 bool_t need_ack) IRAM_TEXT(generic_transmission_data_tx);
int32_t generic_transmission_data_tx(generic_transmission_tx_mode_t mode,
                                 generic_transmission_data_type_t type,
                                 generic_transmission_tid_t tid,
                                 generic_transmission_io_t io,
                                 const uint8_t *data, uint32_t data_len,
                                 bool_t need_ack)
{
    if (mode >= GENERIC_TRANSMISSION_TX_MODE_NUM) {
        return -RET_INVAL;
    }

    if (tid >= GENERIC_TRANSMISSION_TID_NUM) {
        return -RET_INVAL;
    }

    if (type >= GENERIC_TRANSMISSION_DATA_TYPE_NUM) {
        return -RET_INVAL;
    }

    if (io >= GENERIC_TRANSMISSION_IO_NUM) {
        return -RET_INVAL;
    }

    if (data == NULL) {
        return -RET_INVAL;
    }

    if (s_generic_transmission_buf_env[GTB_CORE_SELF].ctrl == NULL ||
            s_generic_transmission_buf_env[GTB_CORE_SELF].ctrl->magic != GENERIC_TRANSMISSION_BUF_MAGIC ||
            s_generic_transmission_buf_env[GTB_CORE_SELF].ctrl->status != GTB_ST_ENABLE) {
        return -RET_NOT_READY;
    }

    return generic_transmission_data_tx_implement(mode, type, tid, io, data, data_len, need_ack, false);
}

int32_t generic_transmission_data_tx_critical(generic_transmission_tx_mode_t mode,
                                 generic_transmission_data_type_t type,
                                 generic_transmission_tid_t tid,
                                 generic_transmission_io_t io,
                                 const uint8_t *data, uint32_t data_len,
                                 bool_t need_ack) IRAM_TEXT(generic_transmission_data_tx_critical);
int32_t generic_transmission_data_tx_critical(generic_transmission_tx_mode_t mode,
                                 generic_transmission_data_type_t type,
                                 generic_transmission_tid_t tid,
                                 generic_transmission_io_t io,
                                 const uint8_t *data, uint32_t data_len,
                                 bool_t need_ack)
{
    if (mode >= GENERIC_TRANSMISSION_TX_MODE_NUM) {
        return -RET_INVAL;
    }

    if (tid >= GENERIC_TRANSMISSION_TID_NUM) {
        return -RET_INVAL;
    }

    if (type >= GENERIC_TRANSMISSION_DATA_TYPE_NUM) {
        return -RET_INVAL;
    }

    if (io >= GENERIC_TRANSMISSION_IO_NUM) {
        return -RET_INVAL;
    }

    if (data == NULL) {
        return -RET_INVAL;
    }

    if (s_generic_transmission_buf_env[GTB_CORE_SELF].ctrl == NULL ||
            s_generic_transmission_buf_env[GTB_CORE_SELF].ctrl->magic != GENERIC_TRANSMISSION_BUF_MAGIC ||
            s_generic_transmission_buf_env[GTB_CORE_SELF].ctrl->status != GTB_ST_ENABLE) {
        return -RET_NOT_READY;
    }

    return generic_transmission_data_tx_implement(mode, type, tid, io, data, data_len, need_ack, true);
}

void generic_transmission_panic_start(void)
{
    generic_transmission_buf_fifo_set_status(GTB_CORE_SELF, GTB_ST_IN_PANIC);
#ifdef BUILD_CORE_CORE0
    generic_transmission_buf_fifo_check_and_set_status(GENERIC_TRANSMISSION_BUF_CORE_BT, GTB_ST_ENABLE, GTB_ST_DISABLE);
    generic_transmission_buf_fifo_check_and_set_status(GENERIC_TRANSMISSION_BUF_CORE_DSP, GTB_ST_ENABLE, GTB_ST_DISABLE);
#elif defined(BUILD_CORE_CORE1)
    generic_transmission_buf_fifo_check_and_set_status(GENERIC_TRANSMISSION_BUF_CORE_DTOP, GTB_ST_ENABLE, GTB_ST_DISABLE);
    generic_transmission_buf_fifo_check_and_set_status(GENERIC_TRANSMISSION_BUF_CORE_DSP, GTB_ST_ENABLE, GTB_ST_DISABLE);
#else
    generic_transmission_buf_fifo_check_and_set_status(GENERIC_TRANSMISSION_BUF_CORE_DTOP, GTB_ST_ENABLE, GTB_ST_DISABLE);
    generic_transmission_buf_fifo_check_and_set_status(GENERIC_TRANSMISSION_BUF_CORE_BT, GTB_ST_ENABLE, GTB_ST_DISABLE);
#endif
}

void generic_transmission_panic_end(void)
{
    generic_transmission_buf_fifo_check_and_set_status(GTB_CORE_SELF, GTB_ST_IN_PANIC, GTB_ST_PANIC_END);
#ifdef BUILD_CORE_CORE0
    generic_transmission_buf_fifo_check_and_set_status(GENERIC_TRANSMISSION_BUF_CORE_BT, GTB_ST_DISABLE, GTB_ST_ENABLE);
    generic_transmission_buf_fifo_check_and_set_status(GENERIC_TRANSMISSION_BUF_CORE_DSP, GTB_ST_DISABLE, GTB_ST_ENABLE);
#elif defined(BUILD_CORE_CORE1)
    generic_transmission_buf_fifo_check_and_set_status(GENERIC_TRANSMISSION_BUF_CORE_DTOP, GTB_ST_DISABLE, GTB_ST_ENABLE);
    generic_transmission_buf_fifo_check_and_set_status(GENERIC_TRANSMISSION_BUF_CORE_DSP, GTB_ST_DISABLE, GTB_ST_ENABLE);
#else
    generic_transmission_buf_fifo_check_and_set_status(GENERIC_TRANSMISSION_BUF_CORE_DTOP, GTB_ST_DISABLE, GTB_ST_ENABLE);
    generic_transmission_buf_fifo_check_and_set_status(GENERIC_TRANSMISSION_BUF_CORE_BT, GTB_ST_DISABLE, GTB_ST_ENABLE);
#endif
}

uint8_t generic_transmission_get_status(uint32_t core_id)
{
    if (s_generic_transmission_buf_env[core_id].ctrl) {
        return s_generic_transmission_buf_env[core_id].ctrl->status;
    } else {
        return GTB_ST_DISABLE;
    }
}

bool_t generic_transmission_in_panic(void) IRAM_TEXT(generic_transmission_in_panic);
bool_t generic_transmission_in_panic(void)
{
    if (s_generic_transmission_buf_env[GENERIC_TRANSMISSION_BUF_CORE_DTOP].ctrl && \
        s_generic_transmission_buf_env[GENERIC_TRANSMISSION_BUF_CORE_DTOP].ctrl->status >= GTB_ST_IN_PANIC) {
        return true;
    }

    if (s_generic_transmission_buf_env[GENERIC_TRANSMISSION_BUF_CORE_BT].ctrl && \
        s_generic_transmission_buf_env[GENERIC_TRANSMISSION_BUF_CORE_BT].ctrl->status >= GTB_ST_IN_PANIC) {
        return true;
    }

    if (s_generic_transmission_buf_env[GENERIC_TRANSMISSION_BUF_CORE_DSP].ctrl && \
        s_generic_transmission_buf_env[GENERIC_TRANSMISSION_BUF_CORE_DSP].ctrl->status >= GTB_ST_IN_PANIC) {
        return true;
    }
    return false;
}

//return pushed length, < 0 means error, =0 means pushed nothing
int32_t generic_transmission_data_tx_panic(generic_transmission_tx_mode_t mode,
                                       generic_transmission_data_type_t type,
                                       generic_transmission_tid_t tid,
                                       generic_transmission_io_t io,
                                       const uint8_t *data, uint32_t data_len)
{
    int32_t ret;
#ifdef BUILD_CORE_CORE0
    generic_transmission_prf_data_tx_param_t param;
#endif
    if (mode >= GENERIC_TRANSMISSION_TX_MODE_NUM) {
        return -RET_INVAL;
    }

    if (tid >= GENERIC_TRANSMISSION_TID_NUM) {
        return -RET_INVAL;
    }

    if (type >= GENERIC_TRANSMISSION_DATA_TYPE_NUM) {
        return -RET_INVAL;
    }

    if (io >= GENERIC_TRANSMISSION_IO_NUM) {
        return -RET_INVAL;
    }

    if (data == NULL) {
        return -RET_INVAL;
    }

    if (s_generic_transmission_buf_env[GTB_CORE_SELF].ctrl == NULL ||
            s_generic_transmission_buf_env[GTB_CORE_SELF].ctrl->magic != GENERIC_TRANSMISSION_BUF_MAGIC ||
            s_generic_transmission_buf_env[GTB_CORE_SELF].ctrl->status != GTB_ST_IN_PANIC) {
        return -RET_NOT_READY;
    }

#ifdef BUILD_CORE_CORE0

    if (s_generic_transmission_consumer_env.panic_process_share_memory) {
        generic_transmission_consumer_tx_process();
        s_generic_transmission_consumer_env.panic_process_share_memory = false;
    }

    /* then tx new panic log */
    param.io = io;
    param.mode = mode;
    param.type = type;
    param.tid = tid;
    param.need_ack = false; //force 0

    //use variable array in generic_transmission_prf_data_tx_panic, it consume ISR stack size
    ret = generic_transmission_prf_data_tx_panic(data, data_len, &param);
#elif defined(BUILD_CORE_CORE1)
    /* if other core in panic mode, the generic transmission producer-consumer model can still use */
    ret = generic_transmission_data_tx_implement(mode, type, tid, io, data, data_len, 0, true);
#else
    /* if other core in panic mode, the generic transmission producer-consumer model can still use */
    ret = generic_transmission_data_tx_implement(mode, type, tid, io, data, data_len, 0, true);
#endif
    return ret;
}

int32_t generic_transmission_set_tid_priority(generic_transmission_tid_t tid,
                                          generic_transmission_prio_t prio)
{
#ifdef BUILD_CORE_CORE0
    if (tid >= GENERIC_TRANSMISSION_TID_NUM) {
        return -RET_INVAL;
    }

    if (prio >= GENERIC_TRANSMISSION_PRIO_NUM) {
        return -RET_INVAL;
    }

    return generic_transmission_prf_set_tid_priority(tid, prio);
#else
    UNUSED(tid);
    UNUSED(prio);

    return -RET_NOSUPP;
#endif
}

int32_t generic_transmission_register_rx_callback(generic_transmission_tid_t tid,
                                              generic_transmission_data_rx_cb_t cb)
{
#ifdef BUILD_CORE_CORE0
    if (tid >= GENERIC_TRANSMISSION_TID_NUM) {
        return -RET_INVAL;
    }

    return generic_transmission_prf_register_rx_callback(tid, cb);
#else
    UNUSED(tid);
    UNUSED(cb);

    return -RET_NOSUPP;
#endif
}

int32_t generic_transmission_register_repack_callback(generic_transmission_data_type_t type,
                                              generic_transmission_repack_cb_t cb)
{
    if (type >= GENERIC_TRANSMISSION_DATA_TYPE_NUM) {
        return -RET_INVAL;
    }
    generic_transmission_repack_cb[type] = cb;
    return RET_OK;
}

static void generic_transmission_buf_fifo_init(void)
{
#if defined(BUILD_CORE_CORE0)
    memset((void*)GENERIC_TRANSMISSION_START, 0, GENERIC_TRANSMISSION_LENGTH);
#endif

#if (GENERIC_TRANSMISSION_DTOP_LENGTH > GENERIC_TRANSMISSION_CTRL_SIZE)
    s_generic_transmission_buf_env[GENERIC_TRANSMISSION_BUF_CORE_DTOP].ctrl =
        (struct generic_transmission_buf_ctrl_tag *)GENERIC_TRANSMISSION_DTOP_START;

    s_generic_transmission_buf_env[GENERIC_TRANSMISSION_BUF_CORE_DTOP].fifo =
        (uint8_t *)GENERIC_TRANSMISSION_DTOP_START + GENERIC_TRANSMISSION_BUF_FIFO_START_OFFSET;

    s_generic_transmission_buf_env[GENERIC_TRANSMISSION_BUF_CORE_DTOP].fifo_len =
        GENERIC_TRANSMISSION_DTOP_LENGTH - GENERIC_TRANSMISSION_BUF_ALLCTRL_SIZE;
#else
    s_generic_transmission_buf_env[GENERIC_TRANSMISSION_BUF_CORE_DTOP].ctrl = NULL;
#endif

#if (GENERIC_TRANSMISSION_BT_LENGTH > GENERIC_TRANSMISSION_CTRL_SIZE)
    s_generic_transmission_buf_env[GENERIC_TRANSMISSION_BUF_CORE_BT].ctrl =
        (struct generic_transmission_buf_ctrl_tag *)GENERIC_TRANSMISSION_BT_START;

    s_generic_transmission_buf_env[GENERIC_TRANSMISSION_BUF_CORE_BT].fifo =
        (uint8_t *)GENERIC_TRANSMISSION_BT_START + GENERIC_TRANSMISSION_BUF_FIFO_START_OFFSET;

    s_generic_transmission_buf_env[GENERIC_TRANSMISSION_BUF_CORE_BT].fifo_len =
        GENERIC_TRANSMISSION_BT_LENGTH - GENERIC_TRANSMISSION_BUF_ALLCTRL_SIZE;
#else
    s_generic_transmission_buf_env[GENERIC_TRANSMISSION_BUF_CORE_BT].ctrl = NULL;
#endif

#if (GENERIC_TRANSMISSION_DSP_LENGTH > GENERIC_TRANSMISSION_CTRL_SIZE)
    s_generic_transmission_buf_env[GENERIC_TRANSMISSION_BUF_CORE_DSP].ctrl =
        (struct generic_transmission_buf_ctrl_tag *)GENERIC_TRANSMISSION_DSP_START;

    s_generic_transmission_buf_env[GENERIC_TRANSMISSION_BUF_CORE_DSP].fifo =
        (uint8_t *)GENERIC_TRANSMISSION_DSP_START + GENERIC_TRANSMISSION_BUF_FIFO_START_OFFSET;

    s_generic_transmission_buf_env[GENERIC_TRANSMISSION_BUF_CORE_DSP].fifo_len =
        GENERIC_TRANSMISSION_DSP_LENGTH - GENERIC_TRANSMISSION_BUF_ALLCTRL_SIZE;
#else
    s_generic_transmission_buf_env[GENERIC_TRANSMISSION_BUF_CORE_DSP].ctrl = NULL;
#endif

#if defined(BUILD_CORE_CORE0)
    generic_transmission_buf_fifo_reset(GENERIC_TRANSMISSION_BUF_CORE_DTOP);
    generic_transmission_buf_fifo_set_status(GENERIC_TRANSMISSION_BUF_CORE_DTOP, GTB_ST_ENABLE);
#elif defined(BUILD_CORE_CORE1)
    generic_transmission_buf_fifo_reset(GENERIC_TRANSMISSION_BUF_CORE_BT);
    generic_transmission_buf_fifo_set_status(GENERIC_TRANSMISSION_BUF_CORE_BT, GTB_ST_ENABLE);
#else
    generic_transmission_buf_fifo_reset(GENERIC_TRANSMISSION_BUF_CORE_DSP);
    generic_transmission_buf_fifo_set_status(GENERIC_TRANSMISSION_BUF_CORE_DSP, GTB_ST_ENABLE);
#endif
}

int32_t generic_transmission_set_priority(uint8_t priority)
{
#ifdef BUILD_CORE_CORE0
    if (s_generic_transmission_consumer_env.task_hdl) {
        os_set_task_prio(s_generic_transmission_consumer_env.task_hdl, priority);
    } else {
        return -1;
    }
    return generic_transmission_prf_tx_set_priority(priority);
#else
    UNUSED(priority);
    return 0;
#endif
}

int32_t generic_transmission_restore_priority(void)
{
#ifdef BUILD_CORE_CORE0
    if (s_generic_transmission_consumer_env.task_hdl) {
        os_set_task_prio(s_generic_transmission_consumer_env.task_hdl, CONFIG_GENERIC_TRANSMISSION_CONSUMER_TASK_PRIO);
    } else {
        return -1;
    }
    return generic_transmission_prf_tx_restore_priority();
#else
    return 0;
#endif
}

static void generic_transmission_buf_fifo_deinit(void)
{
#if defined(BUILD_CORE_CORE0)
    generic_transmission_buf_fifo_set_status(GENERIC_TRANSMISSION_BUF_CORE_DTOP, GTB_ST_DISABLE);
#elif defined(BUILD_CORE_CORE1)
    generic_transmission_buf_fifo_set_status(GENERIC_TRANSMISSION_BUF_CORE_BT, GTB_ST_DISABLE);
#else
    generic_transmission_buf_fifo_set_status(GENERIC_TRANSMISSION_BUF_CORE_DSP, GTB_ST_DISABLE);
#endif
}

void generic_transmission_init(void)
{
    /* As IO abtract layer API is exposed, normally, generic_transmission_io_uart_method_register()
     * should be called out of generic_transmission_init().
     * But the UART IO method is the basic IO of generic transmission and for convenience, call it here.
     */
#ifdef BUILD_CORE_CORE0
    generic_transmission_io_uart_method_register();
    generic_transmission_io_flash_method_register();
#endif
    generic_transmission_buf_fifo_init();

#ifdef BUILD_CORE_CORE0
    generic_transmission_consumer_init();
#endif
}

void generic_transmission_deinit(void)
{
#ifdef BUILD_CORE_CORE0
    generic_transmission_consumer_deinit();
#endif

    generic_transmission_buf_fifo_deinit();
}
