/****************************************************************************
 *
 * Copyright(c) 2020 by WuQi Technologies. ALL RIGHTS RESERVED.
 *
 * This Information is proprietary to WuQi Technologies and MAY NOT
 * be copied by any method or incorporated into another program without
 * the express written consent of WuQi. This Information or any portion
 * thereof remains the property of WuQi. The Information contained herein
 * is believed to be accurate and WuQi assumes no responsibility or
 * liability for its use in any way and conveys no license or title under
 * any patent or copyright and makes no representation or warranty that this
 * Information is free from patent or copyright infringement.
 *
 * ****************************************************************************/
/* os shim includes */

#include "types.h"
#include "string.h"
#include "encoding.h"
#include "riscv_cpu.h"
#include "vector.h"
#include "lib_dbglog.h"

#ifdef CHECK_ISR_USAGE
#include "iot_timer.h"
#include "iot_irq.h"
#include "iot_timer.h"
#define CPU_CRITICAL_REGION_TIME_LIMIT_US 800
#endif

extern const isr_handler clint_vector_table[ISR_INTERRUPT_MAX];
/*lint -esym(526, trap_entry) defined at entry.S */
extern void trap_entry(void);

static cpu_exception_handler exception_handler = NULL;

exception_info_t  exception_data;

uint32_t register_list[7];

/*lint -esym(843, trap_nest) could't be declared as const */
volatile uint32_t trap_nest = 0;

void trap_handler(uint32_t mcause, uint32_t mepc, uint32_t* sp) IRAM_TEXT(trap_handler);
void trap_handler(uint32_t mcause, uint32_t mepc, uint32_t* sp)
{
    exception_data.mcause = mcause;
    exception_data.mepc = mepc;
    exception_data.regs = sp;

    if (mcause & MCAUSE_INTR) {
        uint32_t id = mcause & MCAUSE_CAUSE;
        if (id < ISR_INTERRUPT_MAX) {
            clint_vector_table[id]();
        }
    } else {
        if (exception_handler != NULL) {
            exception_handler();
        }
    }
}

uint32_t cpu_get_mhartid(void) IRAM_TEXT(cpu_get_mhartid);
uint32_t cpu_get_mhartid(void)
{
    return read_csr(mhartid);
}

bool_t cpu_get_int_enable(void) IRAM_TEXT(cpu_get_int_enable);
bool_t cpu_get_int_enable(void)
{
    return read_csr(mstatus) & MSTATUS_MIE ? true : false;
}

void cpu_enable_irq(void) IRAM_TEXT(cpu_enable_irq);
void cpu_enable_irq(void)
{
    set_csr(mstatus, MSTATUS_MIE);
}

uint32_t cpu_disable_irq(void) IRAM_TEXT(cpu_disable_irq);
uint32_t cpu_disable_irq(void)
{
    uint32_t retval = clear_csr(mstatus, MSTATUS_MIE);
#ifdef CHECK_ISR_USAGE
    if (retval & MSTATUS_MIE) {
        iot_isr_usage.global_int_disable_time = iot_timer_get_time();
    }
#endif

    return retval;
}

uint32_t cpu_disable_irq_ignor_usage(void) IRAM_TEXT(cpu_disable_irq_ignor_usage);
uint32_t cpu_disable_irq_ignor_usage(void)
{
    return clear_csr(mstatus, MSTATUS_MIE);
}

void cpu_restore_irq(uint32_t mask) IRAM_TEXT(cpu_restore_irq);
void cpu_restore_irq(uint32_t mask)
{
#ifdef CHECK_ISR_USAGE
    uint32_t ra = read_reg(ra); //must the first line.
    if (mask & MSTATUS_MIE) {
        uint64_t restore_time = iot_timer_get_time();
        uint32_t duration_us = restore_time - iot_isr_usage.global_int_disable_time;
        iot_irq_usage_insert(duration_us , ra);

        //critical region time limit
        if(duration_us > CPU_CRITICAL_REGION_TIME_LIMIT_US) {
            DBGLOG_LIB_WARNING("[WARNING]func:%x in critical time:%d", ra, duration_us);
        }
    }
#endif

    write_csr(mstatus, mask);
}

void cpu_restore_irq_ignor_usage(uint32_t mask) IRAM_TEXT(cpu_restore_irq_ignor_usage);
void cpu_restore_irq_ignor_usage(uint32_t mask)
{
    write_csr(mstatus, mask);
}

void cpu_enable_software_irq(void) IRAM_TEXT(cpu_enable_software_irq);
void cpu_enable_software_irq(void)
{
    set_csr(mie, MIP_MSIP);
}

uint32_t cpu_disable_software_irq(void) IRAM_TEXT(cpu_disable_software_irq);
uint32_t cpu_disable_software_irq(void)
{
    return clear_csr(mie, MIP_MSIP);
}

void cpu_restore_software_irq(uint32_t mask) IRAM_TEXT(cpu_restore_software_irq);
void cpu_restore_software_irq(uint32_t mask)
{
    write_csr(mie, mask);
}

void cpu_enable_timer_irq(void) IRAM_TEXT(cpu_enable_timer_irq);
void cpu_enable_timer_irq(void)
{
    set_csr(mie, MIP_MTIP);
}

uint32_t cpu_disable_timer_irq(void) IRAM_TEXT(cpu_disable_timer_irq);
uint32_t cpu_disable_timer_irq(void)
{
    return clear_csr(mie, MIP_MTIP);
}

void cpu_restore_timer_irq(uint32_t mask) IRAM_TEXT(cpu_restore_timer_irq);
void cpu_restore_timer_irq(uint32_t mask)
{
    write_csr(mie, mask);
}

void cpu_enable_exirq(void) IRAM_TEXT(cpu_enable_exirq);
void cpu_enable_exirq(void)
{
    set_csr(mie, MIP_MEIP);
}

uint32_t cpu_disable_exirq(void) IRAM_TEXT(cpu_disable_exirq);
uint32_t cpu_disable_exirq(void)
{
    return clear_csr(mie, MIP_MEIP);
}

void cpu_restore_exirq(uint32_t mask) IRAM_TEXT(cpu_restore_exirq);
void cpu_restore_exirq(uint32_t mask)
{
    write_csr(mie, mask);
}

uint64_t cpu_get_mcycle(void) IRAM_TEXT(cpu_restore_exirq);
uint64_t cpu_get_mcycle(void)
{
    uint64_t val;

#if __riscv_xlen == 32
    uint32_t hi, hi1, lo;

    do {
        hi = read_csr(mcycleh);
        lo = read_csr(mcycle);
        hi1 = read_csr(mcycleh);

        /* hi != hi1 means mcycle overflow during we get value,
         * so we must retry. */
    } while (hi != hi1);

    val = ((uint64_t)hi << 32) | lo;
#else
    val = read_csr(mcycle);
#endif

    return val;
}

/**
* Per ISA compressed instruction has last two bits of opcode set.
* The encoding '00' '01' '10' are used for compressed instruction.
* Only enconding '11' isn't regarded as compressed instruction (>16b).
*/
uint8_t cpu_get_instruction_length(uint32_t epc) IRAM_TEXT(cpu_get_instruction_length);
uint8_t cpu_get_instruction_length(uint32_t epc)
{
    return ((*(unsigned short *)epc & METAL_INSN_LENGTH_MASK)
            == METAL_INSN_NOT_COMPRESSED)
        ? 4
        : 2;
}

static void riscv_controller_interrupt_vector(TRAP_VECTOR_MODE mode, void *vec_table)
{
    uint32_t entry;
    uint32_t val;

    val = read_csr(mtvec);
    val &= ~(MTVEC_CLIC_VECTORED | MTVEC_CLIC_RESERVED);
    entry = (uint32_t)vec_table;

    switch (mode) {
        case SELECTIVE_NONVECTOR_MODE:
        case SELECTIVE_VECTOR_MODE:
            write_csr(0x307, entry);
            write_csr(mtvec, val | MTVEC_CLIC);
            break;
        case HARDWARE_VECTOR_MODE:
            write_csr(0x307, entry);
            write_csr(mtvec, val | MTVEC_CLIC_VECTORED);
            break;
        case VECTOR_MODE:
            write_csr(mtvec, entry | MTVEC_VECTORED);
            break;
        case DIRECT_MODE:
            write_csr(mtvec, entry & ~MTVEC_CLIC_VECTORED);
            break;
    }
}

void riscv_cpu_controller_interrupt_init(void)
{
    uint32_t val;

    /* Disable and clear all interrupt sources */
    asm volatile("csrc mie, %0" ::"r"(-1));
    asm volatile("csrc mip, %0" ::"r"(-1));

    /* Read the misa CSR to determine if the delegation registers exist */
    uint32_t misa = read_csr(misa);

    /* The delegation CSRs exist if user mode interrupts (N extension) or
        * supervisor mode (S extension) are supported */
    if ((misa & ISA_N_EXTENSIONS) || (misa & ISA_S_EXTENSIONS)) {
        /* Disable interrupt and exception delegation */
        asm volatile("csrc mideleg, %0" ::"r"(-1));
        asm volatile("csrc medeleg, %0" ::"r"(-1));
    }

    /* The satp CSR exists if supervisor mode (S extension) is supported */
    if (misa & ISA_S_EXTENSIONS) {
        /* Clear the entire CSR to make sure that satp.MODE = 0 */
        asm volatile("csrc satp, %0" ::"r"(-1));
    }

    riscv_controller_interrupt_vector(DIRECT_MODE, trap_entry);
    val = read_csr(misa);
    if (val & (ISA_D_EXTENSIONS | ISA_F_EXTENSIONS | ISA_Q_EXTENSIONS)) {
        /* Floating point architecture, so turn on FP register saving*/
        set_csr(mstatus, RISCV_MSTATUS_FS_INIT);
    }
}

void cpu_register_exception_handler(cpu_exception_handler handler)
{
    exception_handler = handler;
}

cpu_exception_handler cpu_get_exception_handler(void)
{
    return exception_handler;
}

exception_info_t *cpu_get_exception_data(void) IRAM_TEXT(cpu_get_exception_data);
exception_info_t *cpu_get_exception_data(void)
{
    return &exception_data;
}

bool_t in_irq(void) IRAM_TEXT(in_irq);
bool_t in_irq(void)
{
    return trap_nest != 0;
}

void save_cpu_context(void) IRAM_TEXT(save_cpu_context);
void save_cpu_context(void)
{
    register_list[0] = read_csr(mscratch);
    register_list[1] = read_csr(mtvec);
    register_list[2] = read_csr(pmpaddr0);
    register_list[3] = read_csr(pmpaddr1);
    register_list[4] = read_csr(pmpaddr2);
    register_list[5] = read_csr(pmpaddr3);
    register_list[6] = read_csr(pmpcfg0);
}

void restore_cpu_context(void) IRAM_TEXT(restore_cpu_context);
void restore_cpu_context(void)
{
    write_csr(mscratch, register_list[0]);
    write_csr(mtvec, register_list[1]);
    write_csr(pmpaddr0, register_list[2]);
    write_csr(pmpaddr1, register_list[3]);
    write_csr(pmpaddr2, register_list[4]);
    write_csr(pmpaddr3, register_list[5]);
    write_csr(pmpcfg0, register_list[6]);
}

uint32_t cpu_get_sp_register(void)
{
    return read_reg(sp);
}
