#ifndef _RISCV_CPU_H
#define _RISCV_CPU_H

#include "types.h"

#define ISA_A_EXTENSIONS     0x0001
#define ISA_C_EXTENSIONS     0x0004
#define ISA_D_EXTENSIONS     0x0008
#define ISA_E_EXTENSIONS     0x0010
#define ISA_F_EXTENSIONS     0x0020
#define ISA_G_EXTENSIONS     0x0040
#define ISA_I_EXTENSIONS     0x0100
#define ISA_M_EXTENSIONS     0x1000
#define ISA_N_EXTENSIONS     0x2000
#define ISA_Q_EXTENSIONS     0x10000
#define ISA_S_EXTENSIONS     0x40000
#define ISA_U_EXTENSIONS     0x100000
#define ISA_V_EXTENSIONS     0x200000
#define ISA_XL32_EXTENSIONS  0x40000000UL
#define ISA_XL64_EXTENSIONS  0x8000000000000000UL
#define ISA_XL128_EXTENSIONS 0xC000000000000000UL

#define MTVEC_DIRECT        0x00
#define MTVEC_VECTORED      0x01
#define MTVEC_CLIC          0x02
#define MTVEC_CLIC_VECTORED 0x03
#define MTVEC_CLIC_RESERVED 0x3C
#define MTVEC_MASK          0x3F
#if __riscv_xlen == 32
#define MCAUSE_INTR  0x80000000UL
#define MCAUSE_CAUSE 0x000003FFUL
#else
#define MCAUSE_INTR  0x8000000000000000UL
#define MCAUSE_CAUSE 0x00000000000003FFUL
#endif
#define RISCV_MCAUSE_MINHV     0x40000000UL
#define RISCV_MCAUSE_MPP       0x30000000UL
#define RISCV_MCAUSE_MPIE      0x08000000UL
#define RISCV_MCAUSE_MPIL      0x00FF0000UL
#define RISCV_MSTATUS_MIE      0x00000008UL
#define RISCV_MSTATUS_MPIE     0x00000080UL
#define RISCV_MSTATUS_MPP      0x00001800UL
#define RISCV_MSTATUS_FS_INIT  0x00002000UL
#define RISCV_MSTATUS_FS_CLEAN 0x00004000UL
#define RISCV_MSTATUS_FS_DIRTY 0x00006000UL
#define RISCV_MSTATUS_MPRV     0x00020000UL
#define RISCV_MSTATUS_MXR      0x00080000UL
#define RISCV_MINTSTATUS_MIL   0xFF000000UL
#define RISCV_MINTSTATUS_SIL   0x0000FF00UL
#define RISCV_MINTSTATUS_UIL   0x000000FFUL

#define METAL_INSN_LENGTH_MASK    0x3
#define METAL_INSN_NOT_COMPRESSED 0x3

#define cpu_enter_wfi()      \
    do {                     \
        asm volatile("wfi"); \
    } while (0)

#define CPU_GET_RET_ADDRESS()                               \
    ({                                                      \
        uint32_t _ra;                                       \
        __asm__ volatile ("mv %0, ra" : "=r"(_ra));         \
        _ra;                                                \
    })

/*!
 * @brief Possible mode of interrupts to operate
 */
typedef enum vector_mode_ {
    DIRECT_MODE,
    VECTOR_MODE,
    SELECTIVE_NONVECTOR_MODE,
    SELECTIVE_VECTOR_MODE,
    HARDWARE_VECTOR_MODE
} TRAP_VECTOR_MODE;

typedef struct exception_info {
    uint32_t mcause;
    uint32_t mepc;
    void *regs;
} exception_info_t;

typedef void (*cpu_exception_handler)(void);

uint32_t cpu_get_mhartid(void);
void cpu_enable_irq(void);
uint32_t cpu_disable_irq(void);
//ignor the time statistics
uint32_t cpu_disable_irq_ignor_usage(void);
void cpu_restore_irq(uint32_t mask);
//ignor the time statistics
void cpu_restore_irq_ignor_usage(uint32_t mask);
void cpu_enable_software_irq(void);
uint32_t cpu_disable_software_irq(void);
void cpu_restore_software_irq(uint32_t mask);
void cpu_enable_timer_irq(void);
uint32_t cpu_disable_timer_irq(void);
void cpu_restore_timer_irq(uint32_t mask);
void cpu_enable_exirq(void);
uint32_t cpu_disable_exirq(void);
void cpu_restore_exirq(uint32_t mask);
uint64_t cpu_get_mcycle(void);
uint8_t cpu_get_instruction_length(uint32_t epc);

void trap_handler(uint32_t mcause, uint32_t mepc, uint32_t *sp);
void cpu_register_exception_handler(cpu_exception_handler handler);
cpu_exception_handler cpu_get_exception_handler(void);
exception_info_t *cpu_get_exception_data(void);

void riscv_cpu_controller_interrupt_init(void);
void save_cpu_context(void);
void restore_cpu_context(void);

bool in_irq(void);
bool_t cpu_get_int_enable(void);
uint32_t cpu_get_sp_register(void);
#endif /* _RISCV_CPU_H */
