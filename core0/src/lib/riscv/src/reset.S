#include "encoding.h"
#include "bits.h"

#ifdef USE_IRAM_TEXT
.section      .iram_text.reset_entry
#else
.section      .text.reset_entry
#endif

.align 4
.global reset_entry
.extern trap_nest

reset_entry:
    csrw   mscratch, t0
#ifndef __riscv_float_abi_soft
    li t0, MSTATUS_FS | MSTATUS_MPP
    csrw mstatus, t0
    fssr x0
#else
    li t0, MSTATUS_MPP
    csrw mstatus, t0
#endif
    csrr   t0,  mscratch
    addi sp, sp, -(REGBYTES * 30)
    STORE x1, 1 * REGBYTES(sp)
    STORE x5, 2 * REGBYTES(sp)
    STORE x6, 3 * REGBYTES(sp)
    STORE x7, 4 * REGBYTES(sp)
    STORE x8, 5 * REGBYTES(sp)
    STORE x9, 6 * REGBYTES(sp)
    STORE x10, 7 * REGBYTES(sp)
    STORE x11, 8 * REGBYTES(sp)
    STORE x12, 9 * REGBYTES(sp)
    STORE x13, 10 * REGBYTES(sp)
    STORE x14, 11 * REGBYTES(sp)
    STORE x15, 12 * REGBYTES(sp)
    STORE x16, 13 * REGBYTES(sp)
    STORE x17, 14 * REGBYTES(sp)
    STORE x18, 15 * REGBYTES(sp)
    STORE x19, 16 * REGBYTES(sp)
    STORE x20, 17 * REGBYTES(sp)
    STORE x21, 18 * REGBYTES(sp)
    STORE x22, 19 * REGBYTES(sp)
    STORE x23, 20 * REGBYTES(sp)
    STORE x24, 21 * REGBYTES(sp)
    STORE x25, 22 * REGBYTES(sp)
    STORE x26, 23 * REGBYTES(sp)
    STORE x27, 24 * REGBYTES(sp)
    STORE x28, 25 * REGBYTES(sp)
    STORE x29, 26 * REGBYTES(sp)
    STORE x30, 27 * REGBYTES(sp)
    STORE x31, 28 * REGBYTES(sp)

    csrr t0, mstatus
    STORE t0, 29 * REGBYTES(sp)

    #ifndef __riscv_float_abi_soft
    addi sp, sp, -(REGBYTES * 32)
    fsw f0, 1 * REGBYTES(sp)
    fsw f1, 2 * REGBYTES(sp)
    fsw f2, 3 * REGBYTES(sp)
    fsw f3, 4 * REGBYTES(sp)
    fsw f4, 5 * REGBYTES(sp)
    fsw f5, 6 * REGBYTES(sp)
    fsw f6, 7 * REGBYTES(sp)
    fsw f7, 8 * REGBYTES(sp)
    fsw f8, 9 * REGBYTES(sp)
    fsw f9, 10 * REGBYTES(sp)
    fsw f10, 11 * REGBYTES(sp)
    fsw f11, 12 * REGBYTES(sp)
    fsw f12, 13 * REGBYTES(sp)
    fsw f13, 14 * REGBYTES(sp)
    fsw f14, 15 * REGBYTES(sp)
    fsw f15, 16 * REGBYTES(sp)
    fsw f16, 17 * REGBYTES(sp)
    fsw f17, 18 * REGBYTES(sp)
    fsw f18, 19 * REGBYTES(sp)
    fsw f19, 20 * REGBYTES(sp)
    fsw f20, 21 * REGBYTES(sp)
    fsw f21, 22 * REGBYTES(sp)
    fsw f22, 23 * REGBYTES(sp)
    fsw f23, 24 * REGBYTES(sp)
    fsw f24, 25 * REGBYTES(sp)
    fsw f25, 26 * REGBYTES(sp)
    fsw f26, 27 * REGBYTES(sp)
    fsw f27, 28 * REGBYTES(sp)
    fsw f28, 29 * REGBYTES(sp)
    fsw f29, 30 * REGBYTES(sp)
    fsw f30, 31 * REGBYTES(sp)
    fsw f31, 32 * REGBYTES(sp)
#endif

#ifndef __riscv_float_abi_soft
    addi    s0, sp, -REGBYTES*62
#else
    addi    s0, sp, -REGBYTES*30
#endif

    lw t0, trap_nest
    addi t0, t0, 1
    la t1, trap_nest
    sw t0, (t1)

    li     a0,  CAUSE_CUSTOM_FAULT
    la     a1,  reset_entry
    mv     a2, sp

    call    trap_handler

    lw t0, trap_nest
    addi t0, t0, -1
    la t1, trap_nest
    sw t0, (t1)

    LOAD  t0, 29 * REGBYTES(sp)
    csrw mstatus, t0

    LOAD x1, 1 * REGBYTES(sp)
    LOAD x5, 2 * REGBYTES(sp)       /* t0 */
    LOAD x6, 3 * REGBYTES(sp)       /* t1 */
    LOAD x7, 4 * REGBYTES(sp)       /* t2 */
    LOAD x8, 5 * REGBYTES(sp)       /* s0/fp */
    LOAD x9, 6 * REGBYTES(sp)       /* s1 */
    LOAD x10, 7 * REGBYTES(sp)      /* a0 */
    LOAD x11, 8 * REGBYTES(sp)      /* a1 */
    LOAD x12, 9 * REGBYTES(sp)      /* a2 */
    LOAD x13, 10 * REGBYTES(sp)     /* a3 */
    LOAD x14, 11 * REGBYTES(sp)     /* a4 */
    LOAD x15, 12 * REGBYTES(sp)     /* a5 */
    LOAD x16, 13 * REGBYTES(sp)     /* a6 */
    LOAD x17, 14 * REGBYTES(sp)     /* a7 */
    LOAD x18, 15 * REGBYTES(sp)     /* s2 */
    LOAD x19, 16 * REGBYTES(sp)     /* s3 */
    LOAD x20, 17 * REGBYTES(sp)     /* s4 */
    LOAD x21, 18 * REGBYTES(sp)     /* s5 */
    LOAD x22, 19 * REGBYTES(sp)     /* s6 */
    LOAD x23, 20 * REGBYTES(sp)     /* s7 */
    LOAD x24, 21 * REGBYTES(sp)     /* s8 */
    LOAD x25, 22 * REGBYTES(sp)     /* s9 */
    LOAD x26, 23 * REGBYTES(sp)     /* s10 */
    LOAD x27, 24 * REGBYTES(sp)     /* s11 */
    LOAD x28, 25 * REGBYTES(sp)     /* t3 */
    LOAD x29, 26 * REGBYTES(sp)     /* t4 */
    LOAD x30, 27 * REGBYTES(sp)     /* t5 */
    LOAD x31, 28 * REGBYTES(sp)     /* t6 */
    addi sp, sp, (REGBYTES * 30)

    mret
