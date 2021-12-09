/****************************************************************************

Copyright(c) 2019 by WuQi Technologies. ALL RIGHTS RESERVED.

This Information is proprietary to WuQi Technologies and MAY NOT
be copied by any method or incorporated into another program without
the express written consent of WuQi. This Information or any portion
thereof remains the property of WuQi. The Information contained herein
is believed to be accurate and WuQi assumes no responsibility or
liability for its use in any way and conveys no license or title under
any patent or copyright and makes no representation or warranty that this
Information is free from patent or copyright infringement.

****************************************************************************/

/* os shim includes */
#include "types.h"
#include "string.h"
#include "stdarg.h"
#include "riscv_cpu.h"
#include "exception.h"
#include "os_task.h"
#include "iot_coredump.h"
#include "iot_memory_config.h"
#include "iot_timer.h"
#include "os_mem.h"
#include "base64.h"
#ifdef LIB_DBGLOG_ENABLE
#include "dbglog.h"
#endif
#include "iot_debounce.h"
#define COREDUMP_OK 0

#define COREDUMP_MAX_TASK_STACK_SIZE        (64*1024)
#define COREDUMP_VERSION                    1
#define CONFIG_WQ_CORE_DUMP_MAX_TASKS_NUM  20

#define IOT_COREDUMP_MAX_LOG_SIZE       256
#define DATA_FLAG 0x00000001
#define BSS_FLAG  0x00000002
#define ROM_DATA_FLAG 0x00000004
#define ROM_BSS_FLAG 0x00000008
#define SHARE_MEMORY_FLAG 0x00000010
#define HEAP_MEMORY_FLAG  0x00000020

#if defined(BUILD_CORE_CORE1) && defined(BT_USE_DTOP_IRAM)
#define HEAP_MEMORY_FLAG2 0x00000040
#endif
#if defined(BUILD_CORE_CORE1)
#define EXCHANGE_MEMORY_FLAG 0x00000080
#define EXCHANGE_MEMORY_START 0x10610000
#define EXCHANGE_MEMORY_END   0x1061F518
#endif

#ifdef BUILD_PATCH
#define TBL_MEMORY_FLAG 0x00000100
#endif

extern uint32_t __stack_top;
extern uint32_t _data_start;
extern uint32_t _data_end;
extern uint32_t _bss_start;
extern uint32_t _bss_end;
#ifdef BUILD_PATCH
extern uint32_t _rom_data_start;
extern uint32_t _rom_data_end;
extern uint32_t _rom_bss_start;
extern uint32_t _rom_bss_end;
extern uint32_t _rom_tbl_start;
extern uint32_t _rom_tbl_end;
#endif

extern uint32_t _heap_start;
/*lint -esym(526, _heap_end) defined at link script */
extern uint32_t _heap_end;

typedef int (*iot_core_dump_write_prepare_t)(void *priv, uint32_t *data_len);
typedef int (*iot_core_dump_write_start_t)(void *priv);
typedef int (*iot_core_dump_write_end_t)(void *priv);
typedef int (*iot_core_dump_flash_write_data_t)(void *priv, void *data, uint32_t data_len);

/** core dump emitter control structure */
typedef struct _core_dump_write_config_t {
    // this function is called before core dump data writing
    // used for sanity checks
    iot_core_dump_write_prepare_t       prepare;
    // this function is called at the beginning of data writing
    iot_core_dump_write_start_t         start;
    // this function is called when all dump data are written
    iot_core_dump_write_end_t           end;
    // this function is called to write data chunk
    iot_core_dump_flash_write_data_t    write;
    // number of tasks with corrupted TCBs
    uint32_t                            bad_tasks_num;
    // pointer to data which are specific for particular core dump emitter
    void                               *priv;
} core_dump_write_config_t;

/** core dump data header */
typedef struct _core_dump_header_t {
    uint32_t data_len;  // data length
    uint32_t version;   // core dump struct version
    uint32_t tasks_num; // number of tasks
    uint32_t tcb_sz;    // size of TCB
} core_dump_header_t;

/** core dump task data header */
typedef struct _core_dump_task_header_t {
    void    *tcb_addr;    // TCB address
    uint32_t task_handle_id; // task handler
    uint32_t stack_start; // stack start address
    uint32_t stack_end;   // stack end address
} core_dump_task_header_t;

/** core dump global data header */
typedef struct _core_dump_data_header_t {
    uint32_t segment_flag;       // segment_flag, data, bss, heap and so on
    uint32_t segment_id;         // segment_id
    uint32_t segment_start;      // segment start address
    uint32_t segment_end;        // segment end address
} core_dump_data_header_t;

static task_snapshot_t tasks[CONFIG_WQ_CORE_DUMP_MAX_TASKS_NUM];

static void iot_coredump_log_send(const uint8_t *buffer, uint8_t length)
{
#ifdef LIB_DBGLOG_ENABLE
    dbglog_crash_log_write(buffer, length);
#else
    UNUSED(buffer);
    UNUSED(length);
#endif
}

static void iot_coredump_log_print(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
#ifdef LIB_DBGLOG_ENABLE
    char str[IOT_COREDUMP_MAX_LOG_SIZE];
    int length = vsnprintf(str, IOT_COREDUMP_MAX_LOG_SIZE, fmt, ap);    //lint !e530 ap initialized
    iot_coredump_log_send((uint8_t *)str, (uint8_t)length);
#else
    UNUSED(iot_coredump_log_send);
    vprintf(fmt, ap);
#endif
    va_end(ap);
}

static inline bool iot_task_stack_start_is_sane(uint32_t sp)
{
#if defined(BUILD_CORE_CORE1) && defined(BT_USE_DTOP_IRAM)
    if ((sp > (uint32_t)&_data_start && sp < (uint32_t)&__stack_top) ||
            (sp > BT_IRAM2_START && sp < (BT_IRAM2_START + BT_IRAM2_LENGTH)) ||
            (sp > EXCHANGE_MEMORY_START && sp < EXCHANGE_MEMORY_END)) {
        return true;
    } else {
        return false;
    }
#else
    return !(sp < (uint32_t)&_data_start || sp > (uint32_t)&__stack_top);
#endif
}

static inline bool iot_tcb_addr_is_sane(uint32_t addr, uint32_t sz)
{
    if (addr == (uint32_t)&__stack_top - 0x1000) {
        // isr TCB addr
        return true;
    }
#if defined(BUILD_CORE_CORE1) && defined(BT_USE_DTOP_IRAM)
    if ((addr > (uint32_t)&_data_start && (addr + sz) < (uint32_t)&_heap_end) ||
            (addr > BT_IRAM2_START && (addr + sz) < (BT_IRAM2_START + BT_IRAM2_LENGTH)) ||
            (addr > EXCHANGE_MEMORY_START && (addr + sz) < EXCHANGE_MEMORY_END)) {
        return true;
    } else {
        return false;
    }
#else
    return !(addr < (uint32_t)&_data_start || (addr + sz) > (uint32_t)&_heap_end);
#endif
}

static inline bool iot_stack_ptr_is_sane(uint32_t sp)
{
#if defined(BUILD_CORE_CORE1) && defined(BT_USE_DTOP_IRAM)
    if ((sp > (uint32_t)&_data_start && sp < (uint32_t)&__stack_top && ((sp & 0x3) == 0)) ||
            (sp > BT_IRAM2_START && sp < (BT_IRAM2_START + BT_IRAM2_LENGTH) && ((sp & 0x3) == 0)) ||
            (sp > EXCHANGE_MEMORY_START && sp < EXCHANGE_MEMORY_END && ((sp & 0x3) == 0))) {
        return true;
    } else {
        return false;
    }
#else
    return !(sp < (uint32_t)&_data_start || sp > (uint32_t)&__stack_top || ((sp & 0x3) != 0));
#endif
}

static int iot_core_dump_segment(int segment_flag, const core_dump_write_config_t *write_cfg)
{
    int err = 0;
    union {
        core_dump_data_header_t data_hrd;
    } dump_data;

    dump_data.data_hrd.segment_flag  = (uint32_t)segment_flag;
    dump_data.data_hrd.segment_id   = 0;
    // add debounce reset here prevent for hardware reseting in coredump
    iot_debounce_reconfig_hard_reset_time_with_delay();
    if (segment_flag == DATA_FLAG) {
        dump_data.data_hrd.segment_start = (uint32_t)&_data_start;
        dump_data.data_hrd.segment_end    = (uint32_t)&_data_end;
        err |= write_cfg->write(write_cfg->priv, &dump_data, sizeof(core_dump_data_header_t));
        err |= write_cfg->write(write_cfg->priv, &_data_start, (uint32_t)&_data_end - (uint32_t)&_data_start);
    } else if (segment_flag == BSS_FLAG) {
        dump_data.data_hrd.segment_start = (uint32_t)&_bss_start;
        dump_data.data_hrd.segment_end    = (uint32_t)&_bss_end;
        err |= write_cfg->write(write_cfg->priv, &dump_data, sizeof(core_dump_data_header_t));
        err |= write_cfg->write(write_cfg->priv, &_bss_start, (uint32_t)&_bss_end - (uint32_t)&_bss_start);
    }
#ifdef BUILD_PATCH
    else if (segment_flag == ROM_DATA_FLAG) {
        dump_data.data_hrd.segment_start = (uint32_t)&_rom_data_start;
        dump_data.data_hrd.segment_end    = (uint32_t)&_rom_data_end;
        err |= write_cfg->write(write_cfg->priv, &dump_data, sizeof(core_dump_data_header_t));
        err |= write_cfg->write(write_cfg->priv, &_rom_data_start, (uint32_t)&_rom_data_end - (uint32_t)&_rom_data_start);
    } else if (segment_flag == ROM_BSS_FLAG) {
        dump_data.data_hrd.segment_start = (uint32_t)&_rom_bss_start;
        dump_data.data_hrd.segment_end    = (uint32_t)&_rom_bss_end;
        err |= write_cfg->write(write_cfg->priv, &dump_data, sizeof(core_dump_data_header_t));
        err |= write_cfg->write(write_cfg->priv, &_rom_bss_start, (uint32_t)&_rom_bss_end - (uint32_t)&_rom_bss_start);
    } else if (segment_flag == TBL_MEMORY_FLAG) {
        dump_data.data_hrd.segment_start = (uint32_t)&_rom_tbl_start;
        dump_data.data_hrd.segment_end    = (uint32_t)&_rom_tbl_end;
        err |= write_cfg->write(write_cfg->priv, &dump_data, sizeof(core_dump_data_header_t));
        err |= write_cfg->write(write_cfg->priv, &_rom_tbl_start, (uint32_t)&_rom_tbl_end - (uint32_t)&_rom_tbl_start);
    }
#endif
    else if (segment_flag == SHARE_MEMORY_FLAG) {
        dump_data.data_hrd.segment_start = SHARE_MEMORY_START;
        dump_data.data_hrd.segment_end    = SHARE_MEMORY_START + SHARE_MEMORY_LENGTH;
        err |= write_cfg->write(write_cfg->priv, &dump_data, sizeof(core_dump_data_header_t));
        err |= write_cfg->write(write_cfg->priv, (void *)SHARE_MEMORY_START, SHARE_MEMORY_LENGTH);
    } else if (segment_flag == HEAP_MEMORY_FLAG) {
        dump_data.data_hrd.segment_start = (uint32_t)&_heap_start;
        dump_data.data_hrd.segment_end   = (uint32_t)&_heap_end;
        err |= write_cfg->write(write_cfg->priv, &dump_data, sizeof(core_dump_data_header_t));
        err |= write_cfg->write(write_cfg->priv, &_heap_start, (uint32_t)&_heap_end - (uint32_t)&_heap_start);
    }
#if defined(BUILD_CORE_CORE1) && defined(BT_USE_DTOP_IRAM)
    else if (segment_flag == HEAP_MEMORY_FLAG2) {
        dump_data.data_hrd.segment_start = BT_IRAM2_START;
        dump_data.data_hrd.segment_end    = (BT_IRAM2_START + BT_IRAM2_LENGTH);
        err |= write_cfg->write(write_cfg->priv, &dump_data, sizeof(core_dump_data_header_t));
        err |= write_cfg->write(write_cfg->priv, (void *)BT_IRAM2_START, BT_IRAM2_LENGTH);
    }
#endif

#if defined(BUILD_CORE_CORE1)
    else if (segment_flag == EXCHANGE_MEMORY_FLAG) {
        dump_data.data_hrd.segment_start = EXCHANGE_MEMORY_START;
        dump_data.data_hrd.segment_end   = EXCHANGE_MEMORY_END;
        err |= write_cfg->write(write_cfg->priv, &dump_data, sizeof(core_dump_data_header_t));
        err |= write_cfg->write(write_cfg->priv, (void *)EXCHANGE_MEMORY_START, EXCHANGE_MEMORY_END - EXCHANGE_MEMORY_START);
    }
#endif
    if (err != COREDUMP_OK) {
        iot_coredump_log_print("Failed to write data segment (%d)!\n", err);
        return err;
    }
    return err;
}

static bool_t iot_core_dump_sp_in_irq(uint32_t *sp)
{
    /*lint -esym(526, xISRStackTop) defined at port.c */
    extern uint32_t xISRStackTop;
    if (((uint32_t)sp < (uint32_t)xISRStackTop) && ((uint32_t)sp > (uint32_t)xISRStackTop - 0x1000)) {
        return true;
    }
    return false;
}

static void iot_core_dump_write(const exception_info_t *info, core_dump_write_config_t *write_cfg)
{
    int cur_task_bad = 0;
    int crash_in_irq = 0;
    int err;
    uint32_t tcb_sz, tcb_sz_padded, task_num;
    uint32_t data_len, i, len;
    union {
        core_dump_header_t      hdr;
        core_dump_task_header_t task_hdr;
    } dump_data;

    if (iot_core_dump_sp_in_irq((uint32_t *)info->regs)) {
        // crashed in interruption
        tasks[0].tcb_ptr = (void *)((uint32_t)&__stack_top - 0x1000);
        tasks[0].task_id = 0;
        tasks[0].stack_start = (uint32_t *)(info->regs);
        tasks[0].stack_end = (uint32_t *)((uint32_t)&__stack_top);
        crash_in_irq = 1;
    }
    if(crash_in_irq) {
        task_num = os_task_snapshot_all(&tasks[1], CONFIG_WQ_CORE_DUMP_MAX_TASKS_NUM - 1, &tcb_sz);
        task_num += 1;
    } else {
        task_num = os_task_snapshot_all(tasks, CONFIG_WQ_CORE_DUMP_MAX_TASKS_NUM, &tcb_sz);
    }

    // take TCB padding into account, actual TCB size will be stored in header
    if (tcb_sz % sizeof(uint32_t)) {
        tcb_sz_padded = (tcb_sz / sizeof(uint32_t) + 1) * sizeof(uint32_t);
    } else {
        tcb_sz_padded = tcb_sz;
    }
    // header + tasknum*(tcb + stack start/end + tcb addr)
    data_len = sizeof(core_dump_header_t) + task_num * (tcb_sz_padded + sizeof(core_dump_task_header_t));
    for (i = 0; i < task_num; i++) {
        if (!iot_tcb_addr_is_sane((uint32_t)tasks[i].tcb_ptr, tcb_sz)) {
            iot_coredump_log_print("Bad TCB addr %x!\n", tasks[i].tcb_ptr);
            write_cfg->bad_tasks_num++;
            continue;
        }
        if (tasks[i].task_id == (uint32_t)(os_get_current_task_handle())) {
            // info->regs maybe not reliable, set correct stack top from TCB for current task
            iot_coredump_log_print("Current task RA/SP/A0/FP/EPC %x %x %x %x %x\n",
                                   ((trap_stack_registers_t *)info->regs)->ra, tasks[i].stack_start, \
                                   ((trap_stack_registers_t *)info->regs)->a0_7[0], ((trap_stack_registers_t *)info->regs)->fp, \
                                   ((trap_stack_registers_t *)info->regs)->mepc);
        }
        // iot_coredump_log_print("%08x ?= %08x",tasks[i].task_id, os_get_task_id(os_get_current_task_handle()));
        len = (uint32_t)tasks[i].stack_end - (uint32_t)tasks[i].stack_start;
        // check task's stack
        // 248 bytes = portCONTEXT_SIZE + portasmADDITIONAL_CONTEXT_SIZE * portWORD_SIZE
        if (iot_core_dump_sp_in_irq(tasks[i].stack_start)) {
            // handle interruption exception
            len = (len + sizeof(uint32_t) - 1) & ~(sizeof(uint32_t) - 1);
            data_len += len;
        } else if (!iot_stack_ptr_is_sane((uint32_t)tasks[i].stack_start) || !iot_task_stack_start_is_sane((uint32_t)tasks[i].stack_end)
                   || len > COREDUMP_MAX_TASK_STACK_SIZE || ((len - 248) & 0xf) != 0) {
            if (tasks[i].task_id == (uint32_t)(os_get_current_task_handle())) {
                cur_task_bad = 1;
            }
            iot_coredump_log_print("Corrupted TCB %x: stack len %lu, top %x, end %x! %d %d\n",
                                   tasks[i].tcb_ptr, len, tasks[i].stack_start, tasks[i].stack_end, \
                                   iot_stack_ptr_is_sane((uint32_t)tasks[i].stack_start), \
                                   iot_task_stack_start_is_sane((uint32_t)tasks[i].stack_end));
            tasks[i].tcb_ptr = 0; // make TCB addr invalid to skip it
            write_cfg->bad_tasks_num++;
        } else {
            // iot_coredump_log_print("Stack len = %lu (%x %x)\n", len, tasks[i].stack_start, tasks[i].stack_end);
            // take stack padding into account
            len = (len + sizeof(uint32_t) - 1) & ~(sizeof(uint32_t) - 1);
            data_len += len;
        }
    }
    data_len -= write_cfg->bad_tasks_num * (tcb_sz_padded + sizeof(core_dump_task_header_t));

    iot_coredump_log_print("Core dump len = %lu (%d %d)\n", data_len, task_num, write_cfg->bad_tasks_num);

    // prepare write
    if (write_cfg->prepare) {
        err = write_cfg->prepare(write_cfg->priv, &data_len);
        if (err != COREDUMP_OK) {
            iot_coredump_log_print("Failed to prepare core dump (%d)!\n", err);
            return;
        }
    }
    // write start
    if (write_cfg->start) {
        err = write_cfg->start(write_cfg->priv);
        if (err != COREDUMP_OK) {
            iot_coredump_log_print("Failed to start core dump (%d)!\n", err);
            return;
        }
    }
    // write header
    dump_data.hdr.data_len  = data_len;
    dump_data.hdr.version   = COREDUMP_VERSION;
    dump_data.hdr.tasks_num = task_num - write_cfg->bad_tasks_num;
    dump_data.hdr.tcb_sz    = tcb_sz;
    err = write_cfg->write(write_cfg->priv, &dump_data, sizeof(core_dump_header_t));
    if (err != COREDUMP_OK) {
        iot_coredump_log_print("Failed to write core dump header (%d)!\n", err);
        return;
    }
    // write tasks
    for (i = 0; i < task_num; i++) {
        if (!iot_tcb_addr_is_sane((uint32_t)tasks[i].tcb_ptr, tcb_sz)) {
            // iot_coredump_log_print("Skip TCB with bad addr %x!\n", tasks[i].tcb_ptr);
            continue;
        }
        // save TCB address, stack base and stack top addr
        dump_data.task_hdr.tcb_addr    = tasks[i].tcb_ptr;
        dump_data.task_hdr.task_handle_id    = (uint32_t)tasks[i].task_id;
        // for storing SP GP TP
        dump_data.task_hdr.stack_start = (uint32_t)tasks[i].stack_start - 3 * sizeof(int);
        dump_data.task_hdr.stack_end   = (uint32_t)tasks[i].stack_end;
        err = write_cfg->write(write_cfg->priv, &dump_data, sizeof(core_dump_task_header_t));
        if (err != COREDUMP_OK) {
            iot_coredump_log_print("Failed to write task header (%d)!\n", err);
            return;
        }
        // save TCB
        err = write_cfg->write(write_cfg->priv, tasks[i].tcb_ptr, tcb_sz);
        if (err != COREDUMP_OK) {
            iot_coredump_log_print("Failed to write TCB (%d)!\n", err);
            return;
        }
        // save task stack
        if (tasks[i].stack_start != 0 && tasks[i].stack_end != 0) {
            tasks[i].stack_start -= 3;
            memmove(tasks[i].stack_start, tasks[i].stack_start + 3, 34 * sizeof(int));
            tasks[i].stack_start[34] = (uint32_t)tasks[i].stack_start; //SP
            tasks[i].stack_start[35] = 0;//GP
            tasks[i].stack_start[36] = 0;//TP
            err = write_cfg->write(write_cfg->priv, tasks[i].stack_start, (uint32_t)tasks[i].stack_end - (uint32_t)tasks[i].stack_start);
            if (err != COREDUMP_OK) {
                iot_coredump_log_print("Failed to write task stack (%d)!\n", err);
                return;
            }
        } else {
            // iot_coredump_log_print("Skip corrupted task %x stack!\n", tasks[i].pxTCB);
        }
    }
    // write segments
#ifdef LIB_DBGLOG_ENABLE
    if (DBGLOG_DUMP_MINI != dbglog_get_dump_mode())
#endif
    {
        err |= iot_core_dump_segment(DATA_FLAG, write_cfg);
        err |= iot_core_dump_segment(BSS_FLAG, write_cfg);
        err |= iot_core_dump_segment(ROM_DATA_FLAG, write_cfg);
        err |= iot_core_dump_segment(ROM_BSS_FLAG, write_cfg);
        err |= iot_core_dump_segment(SHARE_MEMORY_FLAG, write_cfg);
        err |= iot_core_dump_segment(HEAP_MEMORY_FLAG, write_cfg);
#if defined(BUILD_CORE_CORE1) && defined(BT_USE_DTOP_IRAM)
        err |= iot_core_dump_segment(HEAP_MEMORY_FLAG2, write_cfg);
#endif
#if defined(BUILD_CORE_CORE1)
        err |= iot_core_dump_segment(EXCHANGE_MEMORY_FLAG, write_cfg);
#endif

#ifdef BUILD_PATCH
        err |= iot_core_dump_segment(TBL_MEMORY_FLAG, write_cfg);
#endif
    }
    // write end
    if (write_cfg->end) {
        err = write_cfg->end(write_cfg->priv);
        if (err != COREDUMP_OK) {
            iot_coredump_log_print("Failed to end core dump (%d)!\n", err);
            return;
        }
    }
    if (write_cfg->bad_tasks_num) {
        iot_coredump_log_print("Skipped %d tasks with bad TCB!\n", write_cfg->bad_tasks_num);
        if (cur_task_bad) {
            iot_coredump_log_print("Crashed task has skipped!\n");
        }
    }
}

static int iot_core_dump_uart_write_start(void *priv)
{
    UNUSED(priv);
    int err = COREDUMP_OK;
    iot_coredump_log_print("================= CORE DUMP START =================\r\n");
    return err;
}

static int iot_core_dump_uart_write_end(void *priv)
{
    UNUSED(priv);
    int err = COREDUMP_OK;
    iot_coredump_log_print("================= CORE DUMP END =================\r\n");
    return err;
}

static int iot_core_dump_uart_write_data(void *priv, void *data, uint32_t data_len)
{
    UNUSED(priv);
    int err = COREDUMP_OK;
    char buf[64 + 4], *addr = data;
    char *end = addr + data_len;

    while (addr < end) {
        size_t len = (uint32_t)(end - addr);
        if (len > 48) {
            len = 48;
        }
        /* Copy to stack to avoid alignment restrictions. */
        char *tmp = buf + (sizeof(buf) - len);
        memcpy(tmp, addr, len);
        b64_encode((const uint8_t *)tmp, len, (uint8_t *)buf);
        addr += len;
        iot_timer_delay_ms(2);
        iot_coredump_log_print("%s\r\n", buf);
    }

    return err;
}

//lint -sem(iot_task_dump_to_uart, thread_protected) dump only call in crash path
int32_t iot_task_dump_to_uart(const exception_info_t *info)
{
    core_dump_write_config_t wr_cfg;
    memset(&wr_cfg, 0, sizeof(wr_cfg));
    wr_cfg.prepare = NULL;
    wr_cfg.start = iot_core_dump_uart_write_start;
    wr_cfg.end = iot_core_dump_uart_write_end;
    wr_cfg.write = iot_core_dump_uart_write_data;
    wr_cfg.priv = NULL;
    iot_core_dump_write(info, &wr_cfg);
    iot_coredump_log_print("[auto]Core dump has written to uart.\r\n");
    return 0;
}

static void iot_debug_core_dump_register_callback(iot_core_dump_callback cb)
{
    extern iot_core_dump_callback coredump_callback;
    coredump_callback = cb;
}

void iot_core_dump_init(void)
{
    iot_debug_core_dump_register_callback(iot_task_dump_to_uart);
}
