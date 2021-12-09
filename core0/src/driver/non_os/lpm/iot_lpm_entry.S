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

#include "encoding.h"

#ifdef USE_IRAM_TEXT
.section      .iram_text.iot_lpm_warm_entry
#else
.section      .text.iot_lpm_warm_entry
#endif

.global iot_lpm_warm_entry

iot_lpm_warm_entry:
    la     gp, _gp
    csrw   mscratch, t0
    csrw   mepc,     t1
#ifndef __riscv_float_abi_soft
    li t0, MSTATUS_FS | MSTATUS_MPP
    csrw mstatus, t0
    fssr x0
#else
    li t0, MSTATUS_MPP
    csrw mstatus, t0
#endif
    csrr   t0, mhartid
    beqz   t0, dtop_reset
    lui    t0, 0x10800
    lw     t1, 64(t0) # 10800040
    lui    t0, 0x12345
    addi   t0, t0, 0x678
    beq    t0, t1, cpu_reset
    j      warm_reset
dtop_reset:
    lui    t0, 0x1200
    lw     t1, 288(t0) # 01200120
    lui    t0, 0x12345
    addi   t0, t0, 0x678
    beq    t0, t1, cpu_reset
    j      warm_reset
cpu_reset:
    lui    t0, 0x12345
    addi   t0, t0, 0x678
    lui    t1, 0x1a80
    sw     t0, 152(t1) # 0x1a80098
    csrr   t1, mepc
#if defined(BUILD_OS_FREERTOS)
    la     t0, freertos_risc_v_trap_handler
    csrw   mtvec, t0
    la     t0, iot_lpm_warm_entry
    csrw   mepc, t0
    li     t0, CAUSE_CUSTOM_FAULT
    csrw   mcause, t0
    csrr   t0, mscratch
    j   freertos_risc_v_trap_handler
    /* never return */
#endif
here:
    j      here

warm_reset:
    li t0, (MIP_MEIP | MIP_MTIP | MIP_MSIP)
    csrw mie, t0

    li  s0, 0
#if defined(BUILD_OS_FREERTOS)
    lw  sp, xISRStackTop                    /* Switch to ISR stack before function call. */
#elif defined(BUILD_OS_RTT)
    la  sp, __stack_start__                 /* Switch to ISR stack before function call. */
#else
    la  sp, __stack_top                     /* Switch to ISR stack before function call. */
#endif

#if defined(BUILD_OS_FREERTOS)
    la t0, freertos_risc_v_trap_handler
    csrw mtvec, t0

#if ((defined(BUILD_CORE_CORE1)) && (defined(LOW_POWER_ENABLE)))
    call apb_disable_bt_rom_lp_mode
#endif

    call os_sys_restore
    /* never return */
#endif

spin:
    j spin
