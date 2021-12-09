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
#include "stdio.h"
#include "riscv_cpu.h"
#include "exception.h"
#include "encoding.h"

#include "iot_debug.h"
#include "iot_wdt.h"
#include "iot_coredump.h"
#include "iot_debounce.h"
#include "iot_timer.h"
#include "boot_reason.h"

#ifdef LIB_DBGLOG_ENABLE
#include "dbglog.h"
#endif
#ifdef LIB_GENERIC_TRANSMISSION_ENABLE
#include "iot_ipc.h"
#include "generic_transmission_api.h"
#endif
#ifdef LIB_DBGLOG_CACHE_ENABLE
#include "dbglog_cache.h"
#endif
#ifdef CHECK_ISR_FLASH
#include "iot_irq.h"
#include "iot_soc.h"
#endif
#ifdef CHECK_ISR_USAGE
#include "iot_irq.h"
#include "iot_soc.h"
#endif

#ifndef CONFIG_CRASH_RESET
#if defined(RELEASE)
#define CONFIG_CRASH_RESET 1
#elif defined(DEVELOPMENT)
#define CONFIG_CRASH_RESET 0
#else
#define CONFIG_CRASH_RESET 1
#endif
#endif

#define IOT_DEBUG_MAX_BACKTRACE_DEEP  10
#define IOT_DEBUG_MAX_LOG_SIZE        256
#define IOT_DEBUG_ASSERT_DUMP_FUN_MAX 5

enum {
    INSTRUCTION_ADDRESS_MISALIGNED,
    INSTRUCTION_ACCESS_FAULT,
    ILLEGAL_INSTRUCTION,
    BREAKPOINT,
    LOAD_ADDRESS_MISALIGNED,
    LOAD_ACCESS_FAULT,
    STORE_ADDRESS_MISALIGNED,
    STORE_ACCESS_FAULT,
    ENVIRONMENT_CALL_FROM_U_MODE,
    ENVIRONMENT_CALL_FROM_S_MODE,
    RESERVED0,
    ENVIRONMENT_CALL_FROM_M_MODE,
    INSTRUCTION_PAGE_FAULT,
    LOAD_PAGE_FAULT,
    RESERVED1,
    STORE_PAGE_FAULT,
    USER_DEFINED_MEMORY_CORRPUT,
    AVAILABLE_FOR_CUSTOM_USE0,
    RESERVED3,
    AVAILABLE_FOR_CUSTOM_USE1,
    RESERVED4,
};

extern uint32_t __stack_top;
extern uint32_t _data_start;
/*lint -esym(526, _start) defined at link script */
extern uint32_t _start;
/*lint -esym(526, _etext) defined at link script */
extern uint32_t _etext;

uint32_t _iram_text_start __attribute__((weak)) = 0;
uint32_t _iram_text_end __attribute__((weak)) = 0;

#ifdef BUILD_PATCH
extern uint32_t rom_text_start;
extern uint32_t rom_text_end;
#endif

#if defined(BUILD_OS_FREERTOS)
iot_core_dump_callback coredump_callback = NULL;
#endif

static iot_debug_assert_dump_fun_t iot_debug_assert_dump_fun[IOT_DEBUG_ASSERT_DUMP_FUN_MAX];

RET_TYPE iot_debug_reg_assert_dump_fun(iot_debug_assert_dump_fun_t fn)
{
    RET_TYPE ret = RET_NOMEM;
    for (uint8_t i = 0; i < IOT_DEBUG_ASSERT_DUMP_FUN_MAX; i++) {
        if (iot_debug_assert_dump_fun[i] == NULL) {
            iot_debug_assert_dump_fun[i] = fn;
            ret = RET_OK;
            break;
        }
    }
    return ret;
}

static void iot_debug_log_send(const uint8_t *buffer, uint8_t length)
{
#ifdef LIB_DBGLOG_ENABLE
    dbglog_crash_log_write(buffer, length);
#else
    UNUSED(buffer);
    UNUSED(length);
#endif
}

static void iot_debug_log_print(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
#ifdef LIB_DBGLOG_ENABLE
    char str[IOT_DEBUG_MAX_LOG_SIZE];
    int length = vsnprintf(str, IOT_DEBUG_MAX_LOG_SIZE, fmt, ap);   //lint !e530 ap initialized
    iot_debug_log_send((uint8_t *)str, (uint8_t)length);
#else
    UNUSED(iot_debug_log_send);
    vprintf(fmt, ap);
#endif
    va_end(ap);
}

static void iot_debug_reg_dump(trap_stack_registers_t *regs)
{
    iot_debug_log_print("[auto]core%d: exception registers dump\n", cpu_get_mhartid());
    iot_debug_log_print("ra = 0x%08x\n", regs->ra);
    iot_debug_log_print("sp = 0x%08x\n", (uint32_t)regs);

    for (uint8_t i = 0; i < 3; i++) {
        iot_debug_log_print("t%d = 0x%08x\n", i, regs->t0_2[i]);
    }

    iot_debug_log_print("fp = 0x%08x\n", regs->fp);
    iot_debug_log_print("s1 = 0x%08x\n", regs->s1);

    for (uint8_t i = 0; i < 8; i++) {
        iot_debug_log_print("a%d = 0x%08x\n", i, regs->a0_7[i]);
    }

    for (uint8_t i = 0; i < 10; i++) {
        iot_debug_log_print("s%d = 0x%08x\n", (i + 2), regs->s2_11[i]);
    }

    for (uint8_t i = 0; i < 4; i++) {
        iot_debug_log_print("t%d = 0x%08x\n", (i + 3), regs->t3_6[i]);
    }
}

static void iot_debug_illegal_instruct_dump(uint32_t epc)
{
    uint32_t *pc = (uint32_t *)(epc & 0xFFFFFFF0);

    uint32_t text_s = (uint32_t)&_start;
    uint32_t text_e = (uint32_t)&_etext;
    uint32_t ram_text_s = (uint32_t)&_iram_text_start;
    uint32_t ram_text_e = (uint32_t)&_iram_text_end;
#ifdef BUILD_PATCH
    uint32_t rom_s = rom_text_start;
    uint32_t rom_e = rom_text_end;
#endif

#ifdef BUILD_PATCH
    if (((uint32_t)pc >= text_s && (uint32_t)pc <= text_e)
        || ((uint32_t)pc >= ram_text_s && (uint32_t)pc <= ram_text_e)
        || ((uint32_t)pc >= rom_s && (uint32_t)pc <= rom_e)) {
#else
    if (((uint32_t)pc >= text_s && (uint32_t)pc <= text_e)
        || ((uint32_t)pc >= ram_text_s && (uint32_t)pc <= ram_text_e)) {
#endif
        /* 8 DWORD ahead dumpping */
        for (uint8_t i = 7; i > 0; i--) {
            iot_debug_log_print("0x%08x, value: 0x%08x\n", pc - i, *(pc - i));
        }

        /* 8 DWORD after dumping */
        for (uint8_t i = 0; i < 8; i++) {
            iot_debug_log_print("0x%08x, value: 0x%08x\n", pc + i, *(pc + i));
        }
    }
}

/**
 * Stack
    +->          .
    |   +-----------------+   |
    |   | return address  |   |
    |   |   previous fp ------+
    |   | saved registers |
    |   | local variables |
    |   |       ...       | <-+
    |   +-----------------+   |
    |   | return address  |   |
    +------ previous fp   |   |
        | saved registers |   |
        | local variables |   |
    +-> |       ...       |   |
    |   +-----------------+   |
    |   | return address  |   |
    |   |   previous fp ------+
    |   | saved registers |
    |   | local variables |
    |   |       ...       | <-+
    |   +-----------------+   |
    |   | return address  |   |
    +------ previous fp   |   |
        | saved registers |   |
        | local variables |   |
$fp --> |       ...       |   |
        +-----------------+   |
        | return address  |   |
        |   previous fp ------+
        | saved registers |
$sp --> | local variables |
        +-----------------+
 */
static int iot_debug_backtrace(uint32_t fp, uint32_t ra)
{
    uint32_t s0;
    volatile uint32_t deep_len = 0;
    uint32_t ret_fun = 0;
    uint32_t frame_start;

    s0 = fp;
    frame_start = s0;
    deep_len++;

    while (s0 <= frame_start && deep_len < IOT_DEBUG_MAX_BACKTRACE_DEEP) {
        s0 = frame_start;

        /* Validate frame pointer */
        if ((s0 >= (uint32_t)&__stack_top) || (s0 <= (uint32_t)&_data_start)) {
            break;
        }

        uint32_t *tmp = (uint32_t *)s0;
        uint32_t text_s = (uint32_t)&_start;
        uint32_t text_e = (uint32_t)&_etext;
        uint32_t ram_text_s = (uint32_t)&_iram_text_start;
        uint32_t ram_text_e = (uint32_t)&_iram_text_end;

        ret_fun = *(tmp - 1);
        if ((ret_fun > text_s) && (ret_fun < text_e)) {
            frame_start = *(tmp - 2);
        } else if (_iram_text_start && (ret_fun > ram_text_s) && (ret_fun < ram_text_e)) {
            frame_start = *(tmp - 2);
#ifdef BUILD_PATCH
        } else if ((ret_fun > rom_text_start) && (ret_fun < rom_text_end)) {
            frame_start = *(tmp - 2);
#endif
        } else {
            frame_start = *(tmp - 1);
            ret_fun = ra;
        }

        deep_len++;
        iot_debug_log_print("0x%08x:0x%08x ", ret_fun, frame_start);
    }

    return 0;
}

static void iot_debug_exception_info_dump(const exception_info_t *info)
{
    uint32_t mbadaddr = read_csr(mbadaddr);
    static bool_t first = false;

#if defined(BUILD_CORE_CORE1) && defined(LIB_GENERIC_TRANSMISSION_ENABLE)
    if (generic_transmission_get_status(0) < GTB_ST_IN_PANIC) {
        // if core1 is in panic done and core0 is working, trigger core0 in panic automatically
        iot_ipc_send_message(DTOP_CORE, IPC_TYPE_PANIC_REQ, NULL, 0, false);
        while(generic_transmission_get_status(0) < GTB_ST_IN_PANIC);
    }
#endif

    if (!first) {
        iot_wdt_set_feed_period(30);
#if defined(BUILD_CORE_CORE0)
        /* change pmm watchdog's feed period to 7.5 minutes, and make sure coredump successfully.*/
        iot_wdt_global_set_feed_period(30);
#endif
        first = true;
    }
    cpu_disable_irq();

#if !CONFIG_CRASH_RESET
    /* disable watchdog in debug version*/
    iot_debug_log_print("disable watch dog\n");
    iot_wdt_disable_all();
#if defined(BUILD_CORE_CORE0)
    /* enable hardware reset when watchdog reset disable */
    iot_debounce_enable_hard_reset(IOT_DEBOUNCE_CHARGER_RESET_TIME);
#endif
#endif

#ifdef LIB_DBGLOG_ENABLE
    if (generic_transmission_get_status(cpu_get_mhartid()) < GTB_ST_IN_PANIC){
        generic_transmission_panic_start();
    }
#ifdef BUILD_CORE_CORE0
    generic_transmission_consumer_tx_process_panic();
#endif
#endif /* LIB_DBGLOG_ENABLE */
    switch (info->mcause) {
        case INSTRUCTION_ADDRESS_MISALIGNED:
            iot_debug_log_print("[auto]mcause: 0x%02x[Instruction address misaligned]\n",
                                info->mcause);
            break;
        case INSTRUCTION_ACCESS_FAULT:
            iot_debug_log_print("[auto]mcause: 0x%02x[Instruction access fault]\n", info->mcause);
            break;
        case ILLEGAL_INSTRUCTION:
            iot_debug_log_print("[auto]mcause: 0x%02x[Illegal instruction]\n", info->mcause);
            iot_debug_illegal_instruct_dump(info->mepc);
            break;
        case BREAKPOINT:
            iot_debug_log_print("[auto]mcause: 0x%02x[Breakpoint]\n", info->mcause);
            break;
        case LOAD_ADDRESS_MISALIGNED:
            iot_debug_log_print("[auto]mcause: 0x%02x[Load address misaligned]\n", info->mcause);
            break;
        case LOAD_ACCESS_FAULT:
            iot_debug_log_print("[auto]mcause: 0x%02x[Load access fault]\n", info->mcause);
            break;
        case STORE_ADDRESS_MISALIGNED:
            iot_debug_log_print("[auto]mcause: 0x%02x[Store address misaligned]\n", info->mcause);
            break;
        case STORE_ACCESS_FAULT:
            iot_debug_log_print("[auto]mcause: 0x%02x[Store access fault]\n", info->mcause);
            break;
        case ENVIRONMENT_CALL_FROM_U_MODE:
            break;
        case ENVIRONMENT_CALL_FROM_S_MODE:
            break;
        case ENVIRONMENT_CALL_FROM_M_MODE:
            break;
        case INSTRUCTION_PAGE_FAULT:
            break;
        case LOAD_PAGE_FAULT:
            break;
        case STORE_PAGE_FAULT:
            break;
        case RESERVED0:
        case RESERVED1:
            iot_debug_log_print("[auto]mcause: 0x%02x[cpu reset]\n", info->mcause);
            break;
        case USER_DEFINED_MEMORY_CORRPUT:
            iot_debug_log_print("[auto]mcause: 0x%02x[memory corrpution detected]\n", info->mcause);
            break;
        case AVAILABLE_FOR_CUSTOM_USE0:
        case RESERVED3:
        case AVAILABLE_FOR_CUSTOM_USE1:
        case RESERVED4:
        default:
            iot_debug_log_print("[auto]mcause: 0x%02x\n", info->mcause);
            break;
    }

    // dump exception registers
    iot_debug_reg_dump((trap_stack_registers_t *)info->regs);
    iot_debug_backtrace(((trap_stack_registers_t *)info->regs)->fp,
                        ((trap_stack_registers_t *)info->regs)->ra);

    iot_debug_log_print("[auto]mepc: 0x%08x, mbadaddr: 0x%08x\n", info->mepc, mbadaddr);

#if defined(BUILD_OS_FREERTOS)
#if !defined(RELEASE)
    iot_debug_log_print("coredump_callback address: 0x%08x\n", coredump_callback);
    if (coredump_callback) {
        coredump_callback(info);
    }
#endif
#endif

#ifndef CONFIG_CRASH_RESET
    iot_debug_log_print("will do soft reset system\n");
#endif

#if !defined(RELEASE)
#ifdef LIB_DBGLOG_ENABLE
    generic_transmission_panic_end();
#ifdef BUILD_CORE_CORE1
    while (generic_transmission_get_status(0) != GTB_ST_PANIC_END);
#endif
#endif
#endif

#if CONFIG_CRASH_RESET
#ifdef BUILD_CORE_CORE1
    /* all core will do reset action at same time, we should let core0 do it first */
    iot_timer_delay_ms(100);
#endif
    uint8_t id = 0;
    bool_t wdt_timeout = iot_wdt_is_timeout(&id);
    if (wdt_timeout) {
        boot_reason_do_soft_reset(BOOT_REASON_SOFT_REASON_WDT_TIMEOUT, 0);
    } else {
        boot_reason_do_soft_reset(BOOT_REASON_SOFT_REASON_EXCEPTION, 0);
    }
#endif
}   //lint !e454 for assert path no need unlock irq

void iot_debug_assert(const char *file_name, unsigned long line) IRAM_TEXT(iot_debug_assert);
void iot_debug_assert(const char *file_name, unsigned long line)
{
#ifdef CHECK_ISR_FLASH
    iot_soc_cpu_access_enable(IOT_SOC_CPU_ACCESS_DTOP_FLASH, true);
#endif
    // Disable global interrupt
    cpu_disable_irq();
#ifdef LIB_DBGLOG_ENABLE
    if (generic_transmission_get_status(cpu_get_mhartid()) < GTB_ST_IN_PANIC) {
        generic_transmission_panic_start();
    }
#ifdef BUILD_CORE_CORE0
    generic_transmission_consumer_tx_process_panic();
#endif
#endif /* LIB_DBGLOG_ENABLE */
    iot_debug_log_print("[auto]%s:%d Asserted!\n", file_name, line);
    iot_debug_backtrace((uint32_t)__builtin_frame_address(0),
                        (uint32_t)__builtin_return_address(0));

    //print user assert dump function.
    for (uint8_t i = 0; i < IOT_DEBUG_ASSERT_DUMP_FUN_MAX; i++) {
        if (iot_debug_assert_dump_fun[i]) {
            iot_debug_assert_dump_fun[i](iot_debug_log_print);
        }
    }

    asm volatile("ebreak");
}   //lint !e454 for assert path no need unlock irq

void iot_debug_assert_dump(const char *file_name, unsigned long line, const unsigned long *dump_p,
                           unsigned long dump_size) IRAM_TEXT(iot_debug_assert_dump);
void iot_debug_assert_dump(const char *file_name, unsigned long line, const unsigned long *dump_p,
                           unsigned long dump_size)
{
#ifdef CHECK_ISR_FLASH
    iot_soc_cpu_access_enable(IOT_SOC_CPU_ACCESS_DTOP_FLASH, true);
#endif
    // Disable global interrupt
    cpu_disable_irq();
#ifdef LIB_DBGLOG_ENABLE
    if (generic_transmission_get_status(cpu_get_mhartid()) < GTB_ST_IN_PANIC) {
        generic_transmission_panic_start();
    }
#ifdef BUILD_CORE_CORE0
    generic_transmission_consumer_tx_process_panic();
#endif
#endif /* LIB_DBGLOG_ENABLE */
    for (uint32_t i = 0; i < dump_size; i++) {
        iot_debug_log_print("dump param[%d]: 0x%08x\n", i, *(dump_p + i));
    }

    iot_debug_assert(file_name, line);
}   //lint !e454 for assert path no need unlock irq

void iot_debug_init(void)
{
    assert_handler_register(iot_debug_assert);
    assert_dump_handler_register(iot_debug_assert_dump);

    exception_register_dump_callback(iot_debug_exception_info_dump);
    exception_default_register();
#if defined(BUILD_OS_FREERTOS)
    iot_core_dump_init();
#endif

#ifdef CHECK_ISR_USAGE
    iot_debug_reg_assert_dump_fun((iot_debug_assert_dump_fun_t)iot_irq_assert_dump);
#endif
#ifdef LIB_DBGLOG_CACHE_ENABLE
    iot_debug_reg_assert_dump_fun((iot_debug_assert_dump_fun_t)dbglog_cache_assert_dump);
#endif
}
